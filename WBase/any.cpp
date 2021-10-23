/*! \file any.cpp
\ingroup WBase
\brief 动态泛型类型。
*/
#include  "any.h"
#include "cassert.h"

namespace white
{
	namespace any_ops
	{

		holder::~holder() = default;

	} // namespace any_ops;


	bad_any_cast::~bad_any_cast() = default;

	const char*
		bad_any_cast::from() const wnothrow
	{
		return
			from_type() == type_id<void>() ? "unknown" : from_type().name();
	}

	const char*
		bad_any_cast::to() const wnothrow
	{
		return to_type() == type_id<void>() ? "unknown" : to_type().name();
	}

	const char*
		bad_any_cast::what() const wnothrow
	{
		return "Failed conversion: any_cast.";
	}


	namespace details
	{

		any_ops::any_storage&
			any_base::call(any_ops::any_storage& t, any_ops::op_code op) const
		{
			wconstraint(manager);

			manager(t, storage, op);
			return t;
		}

		void
			any_base::clear() wnothrowv
		{
			wconstraint(manager);

			manager(storage, storage, any_ops::destroy);
			manager = {};
		}

		void
			any_base::copy(const any_base& a)
		{
			wconstraint(manager);

			manager(storage, a.storage, any_ops::clone);
		}

		void
			any_base::destroy() wnothrowv
		{
			wconstraint(manager);

			manager(storage, storage, any_ops::destroy);
		}

		void*
			any_base::get() const wnothrowv
		{
			return unchecked_access<void*>(any_ops::get_ptr);
		}

		any_ops::holder*
			any_base::get_holder() const
		{
			return unchecked_access<any_ops::holder*>(any_ops::get_holder_ptr);
		}

		void
			any_base::swap(any_base& a) wnothrow
		{
			std::swap(storage, a.storage),
				std::swap(manager, a.manager);
		}

	} // namespace details;


	any::any(const any& a)
		: any_base(a)
	{
		if (manager)
			copy(a);
	}
	any::~any()
	{
		if (manager)
			destroy();
	}

	void
		any::reset() wnothrow
	{
		if (manager)
			clear();
	}
}