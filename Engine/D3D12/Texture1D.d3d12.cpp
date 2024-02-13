#include "Texture.h"
#include "View.h"
#include "Convert.h"
#include "ShaderCompose.h"
#include "PipleState.h"

#include <WBase/algorithm.hpp>

using namespace platform_ex::Windows::D3D12;
using BTexture = platform::Render::Texture1D;

Texture1D::Texture1D(uint16 width_, uint8 numMipMaps, uint8 array_size_, EFormat format_, uint32 access_hint, platform::Render::SampleDesc sample_info)
	:BTexture(numMipMaps, array_size_, format_, access_hint, sample_info),
	Texture(GetDefaultNodeDevice(), format_),
	width(width_)
{
	if (0 == mipmap_size) {
		mipmap_size = 1;
		auto w = width;
		while (w > 1) {
			++mipmap_size;
			w /= 2;
		}
	}
}

void Texture1D::BuildMipSubLevels()
{
	if (IsDepthFormat(format) || IsCompressedFormat(format))
	{
		for (uint8 index = 0; index <GetArraySize(); ++index)
		{
			for (uint8 level = 1; level < GetNumMipMaps(); ++level)
			{
				Resize(*this, { index, level, 0,  GetWidth(level) },
				{ index, static_cast<uint8>(level - 1), 0,GetWidth(level - 1) }, true);
			}
		}
	}
	else
		DoHWBuildMipSubLevels(array_size, mipmap_size, width);
}

void Texture1D::HWResourceCreate(ResourceCreateInfo& CreateInfo)
{
	Texture::DoCreateHWResource(D3D12_RESOURCE_DIMENSION_TEXTURE1D,
		width, 1, 1, array_size,
		CreateInfo);

	if (CreateInfo.clear_value) {
		this->clear_value = *CreateInfo.clear_value;
	}
}

void Texture1D::HWResourceDelete()
{
	return Texture::DeleteHWResource();
}

bool Texture1D::HWResourceReady() const
{
	return Texture::ReadyHWResource();
}

uint16 Texture1D::GetWidth(uint8 level) const
{
	WAssert(level < mipmap_size, "level out of range");
	return std::max(1, width >> level);
}

void Texture1D::Map(TextureMapAccess tma, void *& data,const Box1D& box)
{
	auto subres = CalcSubresource(box.level, box.array_index, 0, mipmap_size, array_size);

	uint32 row_pitch;
	uint32 slice_pitch;
	DoMap(format,subres, tma, box.x_offset, 0, 0, 1, 1, data, row_pitch, slice_pitch);
}

void Texture1D::UnMap(const Sub1D& box)
{
	auto subres = CalcSubresource(box.level, box.array_index, 0, mipmap_size, array_size);
	DoUnmap(subres);
}

D3D12_SHADER_RESOURCE_VIEW_DESC Texture1D::CreateSRVDesc(uint8 first_array_index, uint8 num_items, uint8 first_level, uint8 num_levels) const
{
	WAssert(GetAccessMode() & EA_GPURead, "Access mode must have EA_GPURead flag");
	D3D12_SHADER_RESOURCE_VIEW_DESC desc;
	switch (format) {
	case EF_D16:
		desc.Format = DXGI_FORMAT_R16_UNORM;
		break;
	case EF_D24S8:
		desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;
	case EF_D32F:
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		break;
	default:
		desc.Format = dxgi_format;
	}

	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (GetArraySize() > 1)
	{
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		desc.Texture1DArray.MostDetailedMip = first_level;
		desc.Texture1DArray.MipLevels = num_levels;
		desc.Texture1DArray.ArraySize = num_items;
		desc.Texture1DArray.FirstArraySlice = first_array_index;
		desc.Texture1DArray.ResourceMinLODClamp = 0;
	}
	else
	{
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		desc.Texture1D.MostDetailedMip = first_level;
		desc.Texture1D.MipLevels = num_levels;
		desc.Texture1D.ResourceMinLODClamp = 0;
	}

	return desc;
}

ShaderResourceView* Texture1D::RetriveShaderResourceView()
{
	if (!default_srv)
	{
		auto srv = new ShaderResourceView(Location.GetParentDevice());
		srv->CreateView(CreateSRVDesc(0, GetArraySize(), 0, GetNumMipMaps()), this, ShaderResourceView::EFlags::None);
		default_srv.reset(srv);
	}
	return default_srv.get();
}

D3D12_UNORDERED_ACCESS_VIEW_DESC Texture1D::CreateUAVDesc(uint8 first_array_index, uint8 num_items, uint8 level) const
{
	WAssert(GetAccessMode() & EA_GPUUnordered, "Access mode must have EA_GPUUnordered flag");

	D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};

	desc.Format = dxgi_format;

	if (GetArraySize() > 1) {
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		desc.Texture1DArray.MipSlice = level;
		desc.Texture1DArray.ArraySize = num_items;
		desc.Texture1DArray.FirstArraySlice = first_array_index;
	}
	else {
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		desc.Texture1D.MipSlice = level;
	}

	return desc;
}

D3D12_RENDER_TARGET_VIEW_DESC platform_ex::Windows::D3D12::Texture1D::CreateRTVDesc(uint8 first_array_index, uint8 num_items, uint8 level) const
{
	WAssert(GetAccessMode() & EA_GPUWrite, "Access mode must have EA_GPUWrite flag");

	D3D12_RENDER_TARGET_VIEW_DESC desc{};

	desc.Format = Convert(format);
	if (GetArraySize() > 1) {
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		desc.Texture1DArray.MipSlice = level;
		desc.Texture1DArray.ArraySize = array_size;
		desc.Texture1DArray.FirstArraySlice = first_array_index;
	}
	else {
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		desc.Texture1D.MipSlice = level;
	}

	return desc;
}
