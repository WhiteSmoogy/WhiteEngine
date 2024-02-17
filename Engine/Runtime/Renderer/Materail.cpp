#include "Materail.h"
#include "Runtime/WSLBuilder.h"
#include "Runtime/AssetResourceScheduler.h"
#include "Asset/MaterialX.h"
#include "Asset/TextureX.h"

using namespace platform;
using namespace scheme;
using namespace v1;
using namespace platform::Render::Shader;
using platform::Render::EAccessHint;

namespace fs = std::filesystem;

platform::Material::Material(const asset::MaterailAsset & asset, const std::string & name)
	:identity_name(asset::path(AssetResourceScheduler::Instance().FindAssetPath(&asset)).replace_extension().string() + "-" + name)
{
	for (auto& bind_value : asset.GetBindValues()) {
		if WB_UNLIKELY(bind_value.second.GetContent().type() == white::type_id<MaterialEvaluator::InstanceDelayedTerm>()){
			auto ret = GetInstanceEvaluator().Reduce(bind_value.second.Access<MaterialEvaluator::InstanceDelayedTerm>());

			MaterialEvaluator::CheckReductionStatus(ret.second);
			bind_values.emplace_back(bind_value.first,
				ret.first.Value.GetContent()
			);
		}
		else if WB_UNLIKELY(bind_value.second.GetContent().type() == white::type_id<MaterialEvaluator::RenderDelayedTerm>()) {
			delay_values.emplace_back(bind_value.first,
				scheme::TermNode(bind_value.second.Access<MaterialEvaluator::RenderDelayedTerm>()));
		}
		else{
			bind_values.emplace_back(bind_value.first,Render::Effect::Parameter::any_cast(bind_value.second.GetContent()));
		}
	}

	bind_effects = platform::X::LoadEffect(asset.GetEffectName());

	for (auto& bind_value : bind_values) {
		if (bind_effects->GetParameter(bind_value.first).GetType() < SPT_textureCUBEArray) {
			if WB_LIKELY(bind_value.second.type() == white::type_id<std::string>()) {
				auto pTexture = X::LoadTexture(std::any_cast<std::string>(bind_value.second), EAccessHint::GPURead | EAccessHint::Immutable);

				bind_value.second =Render::TextureSubresource(pTexture, 0, pTexture->GetArraySize(),0, pTexture->GetNumMipMaps());
			}
		}
	}
}

void platform::Material::UpdateParams(const Renderable* pRenderable) const{
	UpdateParams(pRenderable, bind_effects.get());
}

void platform::Material::UpdateParams(const Renderable* pRenderable, Render::Effect::Effect* target_effect) const
{
	if (target_effect == nullptr)
		return;

	for (auto& bind_value : bind_values) {
		target_effect->TrySetParametr(bind_value.first, bind_value.second);
	}
	for (auto& delay_value : delay_values) {
		auto ret = GetInstanceEvaluator().Reduce(delay_value.second);
		if (ret.second == ReductionStatus::Clean)
			target_effect->TrySetParametr(delay_value.first, Render::Effect::Parameter::any_cast(ret.first.Value.GetContent()));
		else
			WF_TraceRaw(Descriptions::Warning, "Material::UpdateParams(pRenderable=%p) ��ֵ%s ��Լʧ��,�������ø�ֵ", pRenderable, target_effect->GetParameter(delay_value.first).Name.c_str());
	}
}



MaterialEvaluator & platform::Material::GetInstanceEvaluator()
{
	static MaterialEvaluator instance{};

	struct InitBlock final
	{
		InitBlock(MaterialEvaluator& evaluator) {
			auto& context = evaluator.context;
			auto & root(context.Root);

			//TODO add global variable
		}
	};

	static white::call_once_init<InitBlock, white::once_flag> init{ instance };

	return instance;
}

template<>
std::shared_ptr<Material> platform::AssetResourceScheduler::SyncSpawnResource<Material, const X::path&, const std::string&>(const X::path& path, const std::string & name) {
	auto pAsset = X::LoadMaterialAsset(path);
	if (!pAsset)
		return {};
	auto pMaterial = std::make_shared<Material>(*pAsset, name);
	return pMaterial;
}

template std::shared_ptr<Material> platform::AssetResourceScheduler::SyncSpawnResource<Material, const X::path&, const std::string&>(const X::path& path, const std::string & name);




MaterialEvaluator::MaterialEvaluator()
	:WSLEvaluator(MaterialEvalFunctions)
{
}

ReductionStatus LazyOnInstance(TermNode& term, ContextNode&) {
	term.Remove(term.begin());

	ValueObject x(MaterialEvaluator::InstanceDelayedTerm(std::move(term)));

	term.Value = std::move(x);

	return ReductionStatus::Clean;
}

ReductionStatus LazyOnRender(TermNode& term, ContextNode&) {
	term.Remove(term.begin());

	ValueObject x(MaterialEvaluator::RenderDelayedTerm(std::move(term)));

	term.Value = std::move(x);

	return ReductionStatus::Clean;
}


void MaterialEvaluator::MaterialEvalFunctions(REPLContext& context) {
	using namespace scheme::v1::Forms;
	auto & root(context.Root);

	RegisterForm(root, "lazy-oninstance", LazyOnInstance);
	RegisterForm(root, "lazy-onrender", LazyOnRender);
}

void MaterialEvaluator::CheckReductionStatus(ReductionStatus status)
{
	if (status != ReductionStatus::Clean)
		throw white::GeneralEvent(white::sfmt("Bad Reduct State: %s", status == ReductionStatus::Retained ? "Retained" : "Retrying"));
}

void MaterialEvaluator::RegisterMathDotLssFile() {
	lsl::math::RegisterMathDotLssFile(context);
}

void MaterialEvaluator::LoadFile(const path & filepath)
{
	auto hash_value = fs::hash_value(filepath);
	if (loaded_fileshash_set.find(hash_value) != loaded_fileshash_set.end())
		return;
	try {
		if (filepath == "math.wss") {
			RegisterMathDotLssFile();
		}
		else {
			std::ifstream fin(filepath);
			LoadFrom(fin);
		}
		loaded_fileshash_set.emplace(hash_value);
	}
	catch (std::invalid_argument& e) {
		WF_TraceRaw(Descriptions::Err, "���� (env %s) �����쳣:%s", filepath.c_str(), e.what());
	}
}

void MaterialEvaluator::Define(string_view id,ValueObject && vo, bool forced)
{
	auto & root(context.Root);
	auto& root_env(root.GetRecordRef());
	root_env.Define(id,std::move(vo), forced);
}

