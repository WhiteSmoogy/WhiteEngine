/*!	\file FReference.h
\ingroup Framework
\brief 指针和引用访问操作模块。
*/

#ifndef FrameWork_FReference_h
#define FrameWork_FReference_h 1

#include<WBase/memory.hpp>
#include<WBase/smart_ptr.hpp>
#include<WBase/ref.hpp>
#include<WBase/pointer.hpp>
#include<utility>


namespace platform
{
	inline namespace references
	{
		using wimpl(std)::bad_weak_ptr;
		using wimpl(std)::const_pointer_cast;
		using wimpl(std)::default_delete;
		using wimpl(std)::dynamic_pointer_cast;
		using wimpl(std)::enable_shared_from_this;
		using wimpl(std)::get_deleter;
		using wimpl(std)::make_shared;
		using std::make_shared;
		using wimpl(white)::make_observer;
		using wimpl(std)::make_unique;
		using wimpl(white)::make_unique_default_init;
		using white::get_raw;
		using wimpl(std)::owner_less;
		using wimpl(white)::observer_ptr;
		using white::reset;

		using white::share_copy;
		using white::share_forward;
		using white::share_move;

		using white::share_raw;
		using wimpl(std)::shared_ptr;
		using wimpl(std)::static_pointer_cast;
		using white::unique_raw;
		using wimpl(std)::unique_ptr;
		/*!
		\ingroup metafunctions
		\brief 取删除器对应的 unique_ptr 实例。
		\sa std::defer_element
		\sa std::unique_ptr_pointer

		元素类型通过 unique_ptr_pointer 按指定的删除器推断，若失败则为第二参数。
		*/
		template<class _tDeleter, typename _tDefault = void>
		using unique_ptr_from = unique_ptr<white::defer_element<
			white::unique_ptr_pointer<void, _tDeleter>, _tDefault>, _tDeleter>;
		using wimpl(std)::weak_ptr;

		using white::lref;

		//@{
		using white::nptr;
		using white::tidy_ptr;
		using white::pointer_iterator;
		//@}

	} // inline namespace references;

} // namespace platform;

namespace platform_ex
{

	using namespace platform::references;

} // namespace platform_ex;

#endif