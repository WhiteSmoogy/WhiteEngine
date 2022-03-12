/*! \file Engine\Render\Effect\Effect.hpp
\ingroup Engine\Render\Effect
\brief Effect��������
*/
#ifndef WE_RENDER_EFFECT_h
#define WE_RENDER_EFFECT_h 1

#include <WBase/winttype.hpp>
#include <WBase/observer_ptr.hpp>
#include <WBase/any.h>
#include <any>
#include <WBase/examiner.hpp>
#include <WBase/pointer.hpp>
#include <WFramework/WCLib/Debug.h>

#include "../PipleState.h"
#include "Runtime/Core/ResourcesHolder.h"
#include "../ShaderCore.h"
#include "../IShaderCompose.h"

#include <tuple>
#include <vector>
#include <memory>
#include <optional>

namespace asset {
	class EffectAsset;
}

namespace platform {
	class Material;
}

namespace platform::Render {
	struct  TextureSubresource
	{
		std::shared_ptr<Texture> tex;

		uint32_t first_array_index;

		uint32_t num_items;

		uint32_t first_level;

		uint32_t num_levels;


		TextureSubresource()
		{
		}


		TextureSubresource(std::shared_ptr<Texture> const& t, uint32_t fai, uint32_t ni, uint32_t fl, uint32_t nl)
			: tex(t), first_array_index(fai), num_items(ni), first_level(fl), num_levels(nl)
		{
		}
	};

	class ConstantBuffer;
}


namespace platform::Render::Effect {
	struct NameKey : public white::noncopyable{
		NameKey(const std::string& name)
			:NameKey(name,std::hash<std::string>()(name))
		{}

		NameKey(const std::string_view& name)
		:Name(name),Hash(std::hash<std::string_view>()(name)){
		}

		NameKey(const std::string& name,size_t hash)
			:Name(name),Hash(hash)
		{}

	

		const std::string Name;
		const size_t Hash;
	};

	//TODO ����һ���Լ���Parameter
	class ConstantBuffer :public NameKey {
	public:
		using NameKey::NameKey;

		void Dirty(bool val)
		{
			dirty = val;
		}

		platform::Render::ConstantBuffer* GetGraphicsBuffer() const wnothrow;

		friend class Effect;

		template<typename UpdateFunc>
		void Update(UpdateFunc&& update)
		{
			if (dirty) {
				update(cpu_buffer.data(), cpu_buffer.size());
				dirty = false;
			}
		}
	protected:

		template <typename T>
		T const & VariableInBuff(uint32_t offset) const
		{
			union Raw2T
			{
				uint8_t const * raw;
				T const * t;
			} r2t;
			r2t.raw = &cpu_buffer[offset];
			return *r2t.t;
		}
		template <typename T>
		T& VariableInBuff(uint32_t offset)
		{
			union Raw2T
			{
				uint8_t* raw;
				T* t;
			} r2t;
			r2t.raw = &cpu_buffer[offset];
			return *r2t.t;
		}

		friend class Variable;
	private:
		std::unique_ptr<platform::Render::ConstantBuffer> gpu_buffer;
		std::vector<stdex::byte> cpu_buffer;
		bool dirty;
	};

	class Variable {
	public:
		//T must be value semantic!
		template<typename T>
		Variable& operator=(T const & val) {
			if (bind.target) {
				T& bindvalue = bind.target->template VariableInBuff<T>(bind.offset);
				if (!white::examiners::equal_examiner::are_equal(bindvalue, val)) {
					bindvalue = val;
					bind.target->Dirty(true);
				}
			}
			else {
				value = val;
			}
			return *this;
		}

		template<typename T>
		T Get() const {
			if (bind.target)
				return bind.target->template VariableInBuff<T>(bind.offset);
			else
				return std::any_cast<T>(value);
		}

#if ENGINE_TOOL
		void Bind(std::shared_ptr<ConstantBuffer> target, uint32 offset, uint32 stride)
		{
			//save value?
			bind.target = target;
			bind.offset = offset;
			bind.stride = stride;
		}
#endif

		friend class Effect;
		friend class Parameter;
	private:
		Variable& operator=(void*  const & pointer) {
			WAssert(bind.target, "Memory Assign Must Has CBufferBind");
			std::memcpy(&bind.target->cpu_buffer[bind.offset],pointer,bind.stride);
			bind.target->Dirty(true);
			return *this;
		}

		template<typename T>
		T& Ref() {
			if (bind.target)
				return bind.target->template VariableInBuff<T>(bind.offset);
			else
				return Deref(*std::any_cast<T*>(&value));
		}

		template<typename T>
		T* Pointer() {
			if (bind.target)
				return &bind.target->template VariableInBuff<T>(bind.offset);
			else
				return std::any_cast<T>(&value);
		}
	private:
		std::any value;
		struct CBufferBind {
			std::shared_ptr<ConstantBuffer> target;
			uint32 offset =0;
			uint32 stride =0;
		} bind;
	};

	class Parameter :public NameKey {
	public:
		Parameter(const std::string_view& name, ShaderParamType type_)
			:NameKey(name),type(type_)
		{}

		Parameter(const std::string& name, size_t hash, ShaderParamType type_)
			:NameKey(name,hash), type(type_)
		{}

		DefGetter(const wnothrow, ShaderParamType,Type,type)

		template<typename T>
		void Value(T& value) {
			value = var.Get<T>();
		}

		template<typename T>
		T* TryGetValue() {
			return var.Pointer<T>();
		}

		template<typename T>
		Parameter& operator=(T const & val) {
			var = val;
			return *this;
		}

		Parameter& operator=(const white::any& val) =delete;

		static std::any any_cast(const white::any& value);

		friend class Effect;
		friend class Material;
	private:
		Parameter& operator=(const std::any& val);
	private:
		Variable var;

		ShaderParamType type;
	};

	class Pass {
	public:
		Pass() = default;
		Pass(Pass&&) = default;
		ShaderCompose& GetShader(const Effect&) const;
		const PipleState& GetState() const;

		friend class Effect;
	private:
		std::unique_ptr<PipleState> state;
		white::uint8 bind_index;
	};

	class Technique : public NameKey {
	public:
		Technique() = default;
		Technique(Technique&&) = default;

		using NameKey::NameKey;
		
		const Pass& GetPass(white::uint8 index) const;
		Pass& GetPass(white::uint8 index);

		std::size_t NumPasses() const {
			return passes.size();
		}

		bool HasTessellation() const{
			return false;
		}

		friend class Effect;
	private:
		std::vector<Pass> passes;
	};

	class Effect :public NameKey,public Resource {
	public:
		Effect(const std::string& name);
		virtual ~Effect();

		const std::string& GetName()  const wnothrow override;

		void Bind(white::uint8 index);
		ShaderCompose& GetShader(white::uint8 index) const ;

		const Technique& GetTechnique(const std::string& name) const;
		const Technique& GetTechnique(size_t hash) const;
		const Technique& GetTechniqueByIndex(size_t index) const;

		Parameter& GetParameter(const std::string_view& name);
		Parameter& GetParameter(size_t hash);

		bool TrySetParametr(size_t hash, const std::any& val);


		ConstantBuffer& GetConstantBuffer(size_t index);
		size_t ConstantBufferIndex(const std::string& name);
		size_t ConstantBufferIndex(size_t hash);
	protected:
		Technique& GetTechnique(const std::string& name);

		template<typename T>
		T& GetParameterRef(const std::string& name){
			return GetParameter(name).var.Ref<T>();
		}
	private:
		void LoadAsset(white::observer_ptr<asset::EffectAsset> pEffectAsset);
	private:
		std::vector<Technique> techniques;
		std::unordered_map<size_t,Parameter> parameters;
		std::vector<std::shared_ptr<ConstantBuffer>> constantbuffs;
		std::vector<std::unique_ptr<ShaderCompose>> shaders;
	};

	class EffectsHolder :ResourcesHolder<Effect> {
	public:
		EffectsHolder();
		~EffectsHolder();
		
		std::shared_ptr<void> FindResource(const std::any& key) override;
		std::shared_ptr<Effect> FindResource(const std::string& name);
		std::shared_ptr<Effect> FindResource(const std::shared_ptr<asset::EffectAsset>& asset);

		void Connect(const std::shared_ptr<asset::EffectAsset>& asset, const std::shared_ptr<Effect>& effect);

		static EffectsHolder& Instance();
	private:
		std::pmr::vector<std::pair<std::weak_ptr<asset::EffectAsset>,std::shared_ptr<Effect>>> loaded_effects;

		std::shared_mutex CS;
	};
}

namespace WhiteEngine::RenderEffect {
	using namespace platform::Render::Effect;
	struct EffectItem {
		std::shared_ptr<Effect> effect = nullptr;
		Technique* technique = nullptr;
	};
}

#endif