#include <WBase/wdef.h>
#include "ValueNode.h"
#include <cstdio>

namespace white {

	const ValueNode&
		ValueNode::operator%=(const ValueNode& node)
	{
		auto& n((*this)[node.name]);

		n.Value = node.Value;
		return n;
	}
	const ValueNode&
		ValueNode::operator%=(const ValueNode&& node)
	{
		auto& n((*this)[node.name]);

		n.Value = std::move(node.Value);
		return n;
	}

	void
		ValueNode::SetContentIndirect(Container con, const ValueObject& vo) wnothrow
	{
		container.swap(con),
			Value = vo.MakeIndirect();
	}

	ValueNode::Container
		ValueNode::CreateRecursively(const Container& con, IValueHolder::Creation c)
	{
		Container res;

		for (auto& tm : con)
			res.emplace(CreateRecursively(tm.GetContainer(), c), tm.GetName(),
				tm.Value.Create(c));
		return res;
	}

	void
		ValueNode::SwapContent(ValueNode& node) wnothrow
	{
		SwapContainer(node),
			swap(Value, node.Value);
	}

	void
		ValueNode::ThrowIndexOutOfRange()
	{
		throw std::out_of_range("Index is out of range.");
	}

	void
		ValueNode::ThrowWrongNameFound()
	{
		throw std::out_of_range("Wrong name found.");
	}

	void
		swap(ValueNode& x, ValueNode& y) wnothrow
	{
		std::swap(x.name, y.name),
			x.SwapContent(y);
	}

	ValueNode&
		AccessNode(ValueNode::Container* p_con, const string& name)
	{
		if (const auto p = AccessNodePtr(p_con, name))
			return *p;
		ValueNode::ThrowWrongNameFound();
	}
	const ValueNode&
		AccessNode(const ValueNode::Container* p_con, const string& name)
	{
		if (const auto p = AccessNodePtr(p_con, name))
			return *p;
		ValueNode::ThrowWrongNameFound();
	}
	ValueNode&
		AccessNode(ValueNode& node, size_t n)
	{
		const auto p(AccessNodePtr(node, n));

		if (p)
			return *p;
		ValueNode::ThrowIndexOutOfRange();
	}
	const ValueNode&
		AccessNode(const ValueNode& node, size_t n)
	{
		const auto p(AccessNodePtr(node, n));

		if (p)
			return *p;
		ValueNode::ThrowIndexOutOfRange();
	}

	observer_ptr<ValueNode>
		AccessNodePtr(ValueNode::Container& con, const string& name) wnothrow
	{
		return make_observer(white::call_value_or<ValueNode*>(white::addrof<>(),
			con.find(name), {}, end(con)));
	}
	observer_ptr<const ValueNode>
		AccessNodePtr(const ValueNode::Container& con, const string& name) wnothrow
	{
		return make_observer(white::call_value_or<const ValueNode*>(
			white::addrof<>(), con.find(name), {}, end(con)));
	}
	observer_ptr<ValueNode>
		AccessNodePtr(ValueNode& node, size_t n)
	{
		auto& con(node.GetContainerRef());

		// XXX: Conversion to 'ptrdiff_t' might be implementation-defined.
		return n < con.size() ? make_observer(&*std::next(con.begin(), ptrdiff_t(n)))
			: nullptr;
	}
	observer_ptr<const ValueNode>
		AccessNodePtr(const ValueNode& node, size_t n)
	{
		auto& con(node.GetContainer());

		// XXX: Conversion to 'ptrdiff_t' might be implementation-defined.
		return n < con.size() ? make_observer(&*std::next(con.cbegin(), ptrdiff_t(n)))
			: nullptr;
	}

	ValueObject
		GetValueOf(observer_ptr<const ValueNode> p_node)
	{
		return white::call_value_or(std::mem_fn(&ValueNode::Value), p_node);
	}

	observer_ptr<const ValueObject>
		GetValuePtrOf(observer_ptr<const ValueNode> p_node)
	{
		return white::call_value_or(white::compose(make_observer<const ValueObject
		>, white::addrof<>(), std::mem_fn(&ValueNode::Value)), p_node);
	}

	void
		RemoveEmptyChildren(ValueNode::Container& con) wnothrow
	{
		erase_all_if(con, std::mem_fn(&ValueNode::operator!));
	}

	void
		RemoveHead(ValueNode::Container& con) wnothrowv
	{
		WAssert(!con.empty(), "Empty node container found.");
		con.erase(con.cbegin());
	}


	bool
		IsPrefixedIndex(string_view name, char prefix)
	{
		WAssertNonnull(name.data());
		if (name.length() > 1 && name.front() == prefix)
			try
		{
			const auto ss(name.substr(1));

			return MakeIndex(std::stoul(string(ss))) == ss;
		}
		CatchIgnore(std::invalid_argument&)
			return{};
	}

	string
		MakeIndex(size_t n)
	{
		char str[5]{};

		if (n < 10000)
			std::snprintf(str, 5, "%04u", unsigned(n));
		else
			throw std::invalid_argument("Argument is too large.");
		return str;
	}

	size_t
		GetLastIndexOf(const ValueNode::Container& con)
	{
		return con.empty() ? size_t(-1)
			: size_t(std::stoul(con.crbegin()->GetName()));
	}
}