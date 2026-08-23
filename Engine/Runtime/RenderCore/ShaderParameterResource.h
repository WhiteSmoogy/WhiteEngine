#pragma once

#include <WBase/winttype.hpp>

#include <utility>

namespace platform::Render::HLSLTraits
{
	/**
	 * Type-erased storage shared by shader parameters which may reference either
	 * an RHI object or its render-graph wrapper. Keeping the tag next to the
	 * pointer lets graph setup ignore ordinary RHI bindings safely.
	 */
	class UnionPointerBase
	{
	public:
		bool IsRHI() const noexcept { return Index == 0; }
		bool IsRenderGraph() const noexcept { return Index == 1; }
		void* GetValue() const noexcept { return Value; }

	protected:
		UnionPointerBase() noexcept = default;
		UnionPointerBase(void* InValue, white::uint32 InIndex) noexcept
			: Value(InValue), Index(InIndex)
		{}

		void* Value = nullptr;
		white::uint32 Index = 0;
	};

	template<typename RHIType, typename RenderGraphType>
	class UnionPointer final : public UnionPointerBase
	{
	public:
		UnionPointer() noexcept = default;

		UnionPointer(RHIType* InValue) noexcept
			: UnionPointerBase(InValue, 0)
		{}

		UnionPointer(RenderGraphType* InValue) noexcept
			: UnionPointerBase(InValue, 1)
		{}

		UnionPointer& operator=(RHIType* InValue) noexcept
		{
			Value = InValue;
			Index = 0;
			return *this;
		}

		UnionPointer& operator=(RenderGraphType* InValue) noexcept
		{
			Value = InValue;
			Index = 1;
			return *this;
		}

		template<typename Visitor>
		decltype(auto) visit(Visitor&& Visit)
		{
			if (IsRenderGraph())
			{
				return std::forward<Visitor>(Visit)(static_cast<RenderGraphType*>(Value));
			}
			return std::forward<Visitor>(Visit)(static_cast<RHIType*>(Value));
		}

		template<typename Visitor>
		decltype(auto) visit(Visitor&& Visit) const
		{
			if (IsRenderGraph())
			{
				return std::forward<Visitor>(Visit)(static_cast<RenderGraphType*>(Value));
			}
			return std::forward<Visitor>(Visit)(static_cast<RHIType*>(Value));
		}
	};
}
