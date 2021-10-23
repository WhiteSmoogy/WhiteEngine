/*! \file Engine\WIN32\Handle.h
\ingroup Engine
\brief Win32 HANDLE 接口。
*/
#ifndef WE_WIN32_HANDLE_H
#define WE_WIN32_HANDLE_H 1

#include <WBase/wdef.h>
#include <WBase/type_traits.hpp>
#include <WBase/memory.hpp>

namespace platform_ex {
	namespace MCF {

		// 这个代码部分是 MCF 的一部分。
		// 有关具体授权说明，请参阅 MCFLicense.txt。
		template<class CloserT>
		class UniqueHandle {
		public:
			using Handle = std::decay_t<decltype(CloserT()())>;
			using Closer = CloserT;

			static_assert(std::is_scalar<Handle>::value, "Handle must be a scalar type.");
			wnoexcept_assert("Handle closer must not throw.",Closer()(std::declval<Handle>()));

		private:
			Handle x_hObject;

		private:
			void X_Dispose() wnoexcept {
				const auto hObject = x_hObject;
				if (hObject) {
					Closer()(hObject);
				}
#ifndef NDEBUG
				std::memset(&x_hObject, 0xEF, sizeof(x_hObject));
#endif
			}

		public:
			wconstexpr UniqueHandle() wnoexcept
				: x_hObject(Closer()())
			{
			}
			explicit wconstexpr UniqueHandle(Handle rhs) wnoexcept
				: x_hObject(rhs)
			{
			}
			UniqueHandle(UniqueHandle &&rhs) wnoexcept
				: x_hObject(rhs.Release())
			{
			}
			UniqueHandle &operator=(UniqueHandle &&rhs) wnoexcept {
				return Reset(std::move(rhs));
			}
			~UniqueHandle() {
				X_Dispose();
			}

			UniqueHandle(const UniqueHandle &) = delete;
			UniqueHandle &operator=(const UniqueHandle &) = delete;

		public:
			wconstexpr bool IsNull() const wnoexcept {
				return x_hObject == Closer()();
			}
			wconstexpr Handle Get() const wnoexcept {
				return x_hObject;
			}
			Handle Release() wnoexcept {
				return std::exchange(x_hObject, Closer()());
			}

			UniqueHandle &Reset(Handle rhs = Closer()()) wnoexcept {
				UniqueHandle(rhs).Swap(*this);
				return *this;
			}
			UniqueHandle &Reset(UniqueHandle &&rhs) wnoexcept {
				UniqueHandle(std::move(rhs)).Swap(*this);
				return *this;
			}

			void Swap(UniqueHandle &rhs) wnoexcept {
				using std::swap;
				swap(x_hObject, rhs.x_hObject);
			}

		public:
			explicit wconstexpr operator bool() const wnoexcept {
				return !IsNull();
			}
			explicit wconstexpr operator Handle() const wnoexcept {
				return Get();
			}

			template<class OtherCloserT>
			wconstexpr bool operator==(const UniqueHandle<OtherCloserT> &rhs) const wnoexcept {
				return x_hObject == rhs.x_hObject;
			}
			wconstexpr bool operator==(Handle rhs) const wnoexcept {
				return x_hObject == rhs;
			}
			friend wconstexpr bool operator==(Handle lhs, const UniqueHandle &rhs) wnoexcept {
				return lhs == rhs.x_hObject;
			}

			template<class OtherCloserT>
			wconstexpr bool operator!=(const UniqueHandle<OtherCloserT> &rhs) const wnoexcept {
				return x_hObject != rhs.x_hObject;
			}
			wconstexpr bool operator!=(Handle rhs) const wnoexcept {
				return x_hObject != rhs;
			}
			friend wconstexpr bool operator!=(Handle lhs, const UniqueHandle &rhs) wnoexcept {
				return lhs != rhs.x_hObject;
			}

			template<class OtherCloserT>
			wconstexpr bool operator<(const UniqueHandle<OtherCloserT> &rhs) const wnoexcept {
				return x_hObject < rhs.x_hObject;
			}
			wconstexpr bool operator<(Handle rhs) const wnoexcept {
				return x_hObject < rhs;
			}
			friend wconstexpr bool operator<(Handle lhs, const UniqueHandle &rhs) wnoexcept {
				return lhs < rhs.x_hObject;
			}

			template<class OtherCloserT>
			wconstexpr bool operator>(const UniqueHandle<OtherCloserT> &rhs) const wnoexcept {
				return x_hObject > rhs.x_hObject;
			}
			wconstexpr bool operator>(Handle rhs) const wnoexcept {
				return x_hObject > rhs;
			}
			friend wconstexpr bool operator>(Handle lhs, const UniqueHandle &rhs) wnoexcept {
				return lhs > rhs.x_hObject;
			}

			template<class OtherCloserT>
			wconstexpr bool operator<=(const UniqueHandle<OtherCloserT> &rhs) const wnoexcept {
				return x_hObject <= rhs.x_hObject;
			}
			wconstexpr bool operator<=(Handle rhs) const wnoexcept {
				return x_hObject <= rhs;
			}
			friend wconstexpr bool operator<=(Handle lhs, const UniqueHandle &rhs) wnoexcept {
				return lhs <= rhs.x_hObject;
			}

			template<class OtherCloserT>
			wconstexpr bool operator>=(const UniqueHandle<OtherCloserT> &rhs) const wnoexcept {
				return x_hObject >= rhs.x_hObject;
			}
			wconstexpr bool operator>=(Handle rhs) const wnoexcept {
				return x_hObject >= rhs;
			}
			friend wconstexpr bool operator>=(Handle lhs, const UniqueHandle &rhs) wnoexcept {
				return lhs >= rhs.x_hObject;
			}

			friend void swap(UniqueHandle &lhs, UniqueHandle &rhs) wnoexcept {
				lhs.Swap(rhs);
			}
		};
	}
}

#endif
