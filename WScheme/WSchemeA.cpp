/*
\par ÐÞ¸ÄÊ±¼ä:
2017-03-14 00:06 +0800
*/

#include "WSchemeA.h"
#include "SContext.h"

using namespace white;

namespace scheme
{

	ValueNode
		MapWSLALeafNode(const TermNode& term)
	{
		return AsNode(string(),
			string(Deliteralize(ParseWSLANodeString(MapToValueNode(term)))));
	}

	ValueNode
		TransformToSyntaxNode(const ValueNode& node, const string& name)
	{
		ValueNode::Container con{ AsIndexNode(size_t(), node.GetName()) };
		const auto nested_call([&](const ValueNode& nd) {
			con.emplace(TransformToSyntaxNode(nd, MakeIndex(con)));
		});

		if (node.empty())
		{
			if (const auto p = AccessPtr<NodeSequence>(node))
				for (auto& nd : *p)
					nested_call(nd);
			else
				con.emplace(NoContainer, MakeIndex(1),
					Literalize(ParseWSLANodeString(node)));
		}
		else
			for (auto& nd : node)
				nested_call(nd);
		return{ std::move(con), name };
	}

	string
		EscapeNodeLiteral(const ValueNode& node)
	{
		return EscapeLiteral(Access<string>(node));
	}

	string
		LiteralizeEscapeNodeLiteral(const ValueNode& node)
	{
		return Literalize(EscapeNodeLiteral(node));
	}

	string
		ParseWSLANodeString(const ValueNode& node)
	{
		return white::value_or(AccessPtr<string>(node));
	}


	string
		DefaultGenerateIndent(size_t n)
	{
		return string(n, '\t');
	}

	void
		PrintIndent(std::ostream& os, IndentGenerator igen, size_t n)
	{
		if WB_LIKELY(n != 0)
			white::write(os, igen(n));
	}

	void
		PrintNode(std::ostream& os, const ValueNode& node, NodeToString node_to_str,
			IndentGenerator igen, size_t depth)
	{
		PrintIndent(os, igen, depth);
		os << EscapeLiteral(node.GetName()) << ' ';
		if (PrintNodeString(os, node, node_to_str))
			return;
		os << '\n';
		if (node)
			TraverseNodeChildAndPrint(os, node, [&] {
				PrintIndent(os, igen, depth);
			}, [&](const ValueNode& nd) {
				return PrintNodeString(os, nd, node_to_str);
			}, [&](const ValueNode& nd) {
				return PrintNode(os, nd, node_to_str, igen, depth + 1);
			});
	}

	bool
		PrintNodeString(std::ostream& os, const ValueNode& node,
			NodeToString node_to_str)
	{
		TryRet(os << node_to_str(node) << '\n', true)
			CatchIgnore(white::bad_any_cast&)
			return{};
	}

	namespace sxml
	{

		string
			ConvertAttributeNodeString(const TermNode& term)
		{
			switch (term.size())
			{
			default:
				TraceDe(Warning, "Invalid term with more than 2 children found.");
			case 2:
			{
				auto i(term.begin());
				const auto& n(Access<string>(Deref(i)));

				return n + '=' + Access<string>(Deref(++i));
			}
			case 1:
				return Access<string>(Deref(term.begin()));
			case 0:
				break;
			}
			throw LoggedEvent("Invalid term with less than 1 children found.", Warning);
		}

		string
			ConvertDocumentNode(const TermNode& term, IndentGenerator igen, size_t depth,
				ParseOption opt)
		{
			if (term)
			{
				string res(ConvertStringNode(term));

				if (res.empty())
				{
					if (opt == ParseOption::String)
						throw LoggedEvent("Invalid non-string term found.");

					const auto& con(term.GetContainer());

					if (!con.empty())
						try
					{
						auto i(con.cbegin());
						const auto& str(Access<string>(Deref(i)));

						++i;
						if (str == "@")
						{
							for (; i != con.cend(); ++i)
								res += ' ' + ConvertAttributeNodeString(Deref(i));
							return res;
						}
						if (opt == ParseOption::Attribute)
							throw LoggedEvent("Invalid non-attribute term found.");
						if (str == "*PI*")
						{
							res = "<?";
							for (; i != con.cend(); ++i)
								res += string(Deliteralize(ConvertDocumentNode(
									Deref(i), igen, depth, ParseOption::String)))
								+ ' ';
							if (!res.empty())
								res.pop_back();
							return res + "?>";
						}
						if (str == "*ENTITY*" || str == "*NAMESPACES*")
						{
							if (opt == ParseOption::Strict)
								throw white::unimplemented();
						}
						else if (str == "*COMMENT*")
							;
						else if (!str.empty())
						{
							const bool is_content(str != "*TOP*");
							string head('<' + str);
							bool nl{};

							if WB_UNLIKELY(!is_content && depth > 0)
								TraceDe(Warning, "Invalid *TOP* found.");
							if (i != con.end())
							{
								if (!Deref(i).empty()
									&& (i->begin())->Value == string("@"))
								{
									head += string(
										Deliteralize(ConvertDocumentNode(*i, igen,
											depth, ParseOption::Attribute)));
									if (++i == con.cend())
										return head + " />";
								}
								head += '>';
							}
							else
								return head + " />";
							for (; i != con.cend(); ++i)
							{
								nl = Deref(i).Value.type()
									!= white::type_id<string>();
								if (nl)
									res += '\n' + igen(depth + size_t(is_content));
								else
									res += ' ';
								res += string(Deliteralize(ConvertDocumentNode(*i,
									igen, depth + size_t(is_content))));
							}
							if (res.size() > 1 && res.front() == ' ')
								res.erase(0, 1);
							if (!res.empty() && res.back() == ' ')
								res.pop_back();
							if (is_content)
							{
								if (nl)
									res += '\n' + igen(depth);
								return head + res + white::quote(str, "</", '>');
							}
						}
						else
							throw LoggedEvent("Empty item found.", Warning);
					}
					CatchExpr(white::bad_any_cast& e, TraceDe(Warning,
						"Conversion failed: <%s> to <%s>.", e.from(), e.to()))
				}
				return res;
			}
			throw LoggedEvent("Empty term found.", Warning);
		}

		string
			ConvertStringNode(const TermNode& term)
		{
			return white::call_value_or(EscapeXML, AccessPtr<string>(term));
		}

		void
			PrintSyntaxNode(std::ostream& os, const TermNode& term, IndentGenerator igen,
				size_t depth)
		{
			if (IsBranch(term))
				white::write(os,
					ConvertDocumentNode(Deref(term.begin()), igen, depth), 1);
			os << std::flush;
		}


		ValueNode
			MakeXMLDecl(const string& name, const string& ver, const string& enc,
				const string& sd)
		{
			auto decl("version=\"" + ver + '"');

			if (!enc.empty())
				decl += " encoding=\"" + enc + '"';
			if (!sd.empty())
				decl += " standalone=\"" + sd + '"';
			return AsNodeLiteral(name, { { MakeIndex(0), "*PI*" },{ MakeIndex(1), "xml" },
			{ MakeIndex(2), decl + ' ' } });
		}

		ValueNode
			MakeXMLDoc(const string& name, const string& ver, const string& enc,
				const string& sd)
		{
			auto doc(MakeTop(name));

			doc.Add(MakeXMLDecl(MakeIndex(1), ver, enc, sd));
			return doc;
		}

	} // namespace sxml;


	namespace
	{

		string
			InitBadIdentifierExceptionString(string&& id, size_t n)
		{
			return (n != 0 ? (n == 1 ? "Bad identifier: '" : "Duplicate identifier: '")
				: "Unknown identifier: '") + std::move(id) + "'.";
		}

		WB_NORETURN void
			ThrowInvalidEnvironment()
		{
			throw std::invalid_argument("Invalid environment record pointer found.");
		}

	} // unnamed namespace;


	ImplDeDtor(WSLException)


		ImplDeDtor(ListReductionFailure)


		ImplDeDtor(InvalidSyntax)

		ImplDeDtor(ParameterMismatch)

		ArityMismatch::ArityMismatch(size_t e, size_t r, RecordLevel lv)
		: WSLException(white::sfmt("Arity mismatch: expected %zu, received %zu.",
			e, r), lv),
		expected(e), received(r)
	{}
	ImplDeDtor(ArityMismatch)

		BadIdentifier::BadIdentifier(const char* id, size_t n, RecordLevel lv)
		: WSLException(InitBadIdentifierExceptionString(id, n), lv),
		p_identifier(make_shared<string>(id))
	{}
	BadIdentifier::BadIdentifier(string_view id, size_t n, RecordLevel lv)
		: WSLException(InitBadIdentifierExceptionString(string(id), n), lv),
		p_identifier(make_shared<string>(id))
	{}
	ImplDeDtor(BadIdentifier)

	ImplDeDtor(InvalidReference)

		LexemeCategory
		CategorizeBasicLexeme(string_view id) wnothrowv
	{
		WAssertNonnull(id.data() && !id.empty());

		const auto c(CheckLiteral(id));

		if (c == '\'')
			return LexemeCategory::Code;
		if (c != char())
			return LexemeCategory::Data;
		return LexemeCategory::Symbol;
	}

	LexemeCategory
		CategorizeLexeme(string_view id) wnothrowv
	{
		const auto res(CategorizeBasicLexeme(id));

		return res == LexemeCategory::Symbol && IsWSLAExtendedLiteral(id)
			? LexemeCategory::Extended : res;
	}

	bool
		IsWSLAExtendedLiteral(string_view id) wnothrowv
	{
		WAssertNonnull(id.data() && !id.empty());

		const char f(id.front());

		return (id.size() > 1 && IsWSLAExtendedLiteralNonDigitPrefix(f)
			&& id.find_first_not_of("+-") != string_view::npos)
			|| std::isdigit(f);
	}



	observer_ptr<const TokenValue>
		TermToNamePtr(const TermNode& term)
	{
		return AccessPtr<TokenValue>(term);
	}

	string
		TermToString(const TermNode& term)
	{
		if (const auto p = TermToNamePtr(term))
			return *p;
		return sfmt("#<unknown{%zu}:%s>", term.size(), term.Value.type().name());
	}

	void
		TokenizeTerm(TermNode& term)
	{
		for (auto& child : term)
			TokenizeTerm(child);
		if (const auto p = AccessPtr<string>(term))
			term.Value.emplace<TokenValue>(std::move(*p));
	}


	TermNode&
		ReferenceTerm(TermNode& term)
	{
		return white::call_value_or(
			[&](const TermReference& term_ref) wnothrow->TermNode&{
				return term_ref.get();
			}, AccessPtr<const TermReference>(term), term);
	}
	const TermNode&
		ReferenceTerm(const TermNode& term)
	{
		return white::call_value_or(
			[&](const TermReference& term_ref) wnothrow -> const TermNode&{
				return term_ref.get();
			}, AccessPtr<TermReference>(term), term);
	}

	bool
		CheckReducible(ReductionStatus status)
	{
		if (status == ReductionStatus::Clean
			|| status == ReductionStatus::Retained)
			return {};
		if WB_UNLIKELY(status != ReductionStatus::Retrying)
			TraceDe(Warning, "Unexpected status found");
		return true;
	}

	void
		LiftTermOrRef(TermNode& term, TermNode& tm)
	{
		if (const auto p = AccessPtr<const TermReference>(tm))
			LiftTermRef(term, p->get());
		else
			LiftTerm(term, tm);
	}

	void
		LiftTermRefToSelf(TermNode& term)
	{
		if (const auto p = AccessPtr<const TermReference>(term))
			LiftTermRef(term, p->get());
	}

	void
		LiftToReference(TermNode& term, TermNode& tm)
	{
		if (tm)
		{
			if (const auto p = AccessPtr<const TermReference>(tm))
				LiftTermRef(term, p->get());
			else if (!tm.Value.OwnsUnique())
				LiftTerm(term, tm);
			else
				throw InvalidReference(
					"Value of a temporary shall not be referenced.");
		}
		else
			white::throw_invalid_construction();
	}

	void
		LiftToSelf(TermNode& term)
	{
		// NOTE: The order is significant.
		LiftTermRefToSelf(term);
		for (auto& child : term)
			LiftToSelf(child);
	}

	void
		LiftToSelfSafe(TermNode& term)
	{
		// TODO: Detect lifetime escape to perform copy elision?
		// NOTE: To keep lifetime of objects referenced by references introduced in
		//	%EvaluateIdentifier sane, %ValueObject::MakeMoveCopy is not enough
		//	because it will not copy objects referenced in holders of
		//	%YSLib::RefHolder instances). On the other hand, the references captured
		//	by vau handlers (which requries recursive copy of vau handler members if
		//	forced) are not blessed here to avoid leaking abstraction of detailed
		//	implementation of vau handlers; it can be checked by the vau handler
		//	itself, if necessary.
		LiftToSelf(term);
		LiftTermIndirection(term, term);
	}

	void
		LiftToOther(TermNode& term, TermNode& tm)
	{
		LiftTermRefToSelf(tm);
		LiftTerm(term, tm);
	}


	ReductionStatus
		ReduceBranchToList(TermNode& term) wnothrowv
	{
		AssertBranch(term);
		RemoveHead(term);
		return ReductionStatus::Retained;
	}

	ReductionStatus
		ReduceBranchToListValue(TermNode& term) wnothrowv
	{
		RemoveHead(term);
		LiftSubtermsToSelfSafe(term);
		return ReductionStatus::Retained;
	}

	ReductionStatus
		ReduceHeadEmptyList(TermNode& term) wnothrow
	{
		if (term.size() > 1 && IsEmpty(Deref(term.begin())))
			RemoveHead(term);
		return ReductionStatus::Clean;
	}

	ReductionStatus
		ReduceToList(TermNode& term) wnothrow
	{
		return IsBranch(term) ? (void(RemoveHead(term)), ReductionStatus::Retained)
			: ReductionStatus::Clean;
	}

	ReductionStatus
		ReduceToListValue(TermNode& term) wnothrow
	{
		return IsBranch(term) ? ReduceBranchToListValue(term)
			: ReductionStatus::Clean;
	}

	namespace
	{
		bool
			IsReserved(string_view id) wnothrowv
		{
			WAssertNonnull(id.data());
			return white::begins_with(id, "__");
		}

		observer_ptr<Environment>
			RedirectToShared(string_view id, const shared_ptr<Environment>& p_shared)
		{
			if (p_shared)
				return make_observer(get_raw(p_shared));
			// TODO: Use concrete semantic failure exception.
			throw WSLException(white::sfmt("Invalid reference found for%s name '%s',"
				" probably due to invalid context access by dangling reference.",
				IsReserved(id) ? " reserved" : "", id.data()));
		}

		observer_ptr<const Environment>
			RedirectParent(const ValueObject& parent, string_view id)
		{
			const auto& tp(parent.type());

			if (tp == white::type_id<EnvironmentList>())
				for (const auto& vo : parent.GetObject<EnvironmentList>())
				{
					auto p(RedirectParent(vo, id));

					if (p)
						return p;
				}
			if (tp == white::type_id<observer_ptr<const Environment>>())
				return parent.GetObject<observer_ptr<const Environment>>();
			if (tp == white::type_id<EnvironmentReference>())
				return RedirectToShared(id,
					parent.GetObject<EnvironmentReference>().Lock());
			if (tp == white::type_id<shared_ptr<Environment>>())
				return RedirectToShared(id,
					parent.GetObject<shared_ptr<Environment>>());
			return {};
		}

	} // unnamed namespace;

	void
		Environment::CheckParent(const ValueObject& vo)
	{
		const auto& tp(vo.type());

		if (tp == white::type_id<EnvironmentList>())
		{
			for (const auto& env : vo.GetObject<EnvironmentList>())
				CheckParent(env);
		}
		else if WB_UNLIKELY(tp != white::type_id<observer_ptr<const Environment>>()
			&& tp != white::type_id<EnvironmentReference>()
			&& tp != white::type_id<shared_ptr<Environment>>())
			ThrowForInvalidType(tp);
	}

	bool
		Environment::Deduplicate(BindingMap& dst, const BindingMap& src)
	{
		for (const auto& binding : src)
			// XXX: Non-trivially destructible objects is treated same.
			// NOTE: Redirection is not needed here.
			dst.Remove(binding.GetName());
		// NOTE: If the resulted parent environment is empty, it is safe to be
		//	removed.
		return dst.empty();
	}

	observer_ptr<const Environment>
		Environment::DefaultRedirect(const Environment& env, string_view id)
	{
		return RedirectParent(env.Parent, id);
	}

	Environment::NameResolution
		Environment::DefaultResolve(const Environment& e, string_view id)
	{
		WAssertNonnull(id.data());

		observer_ptr<ValueNode> p;
		auto env_ref(white::ref<const Environment>(e));

		white::retry_on_cond(
			[&](observer_ptr<const Environment> p_env) wnothrowv -> bool{
				if (p_env)
				{
					env_ref = white::ref(Deref(p_env));
					return true;
				}
		return {};
			}, [&, id]() -> observer_ptr<const Environment> {
				auto& env(env_ref.get());

				p = env.LookupName(id);
				return p ? nullptr : env.Redirect(id);
			});
		return { p, env_ref };
	}

	void
		Environment::Define(string_view id, ValueObject&& vo, bool forced)
	{
		WAssertNonnull(id.data());
		if (forced)
			// XXX: Self overwriting is possible.
			swap(Bindings[id].Value, vo);
		else if (!Bindings.AddValue(id, std::move(vo)))
			throw BadIdentifier(id, 2);
	}

	observer_ptr<ValueNode>
		Environment::LookupName(string_view id) const
	{
		WAssertNonnull(id.data());
		return AccessNodePtr(Bindings, id);
	}

	void
		Environment::Redefine(string_view id, ValueObject&& vo, bool forced)
	{
		if (const auto p = LookupName(id))
			swap(p->Value, vo);
		else if (!forced)
			throw BadIdentifier(id, 0);
	}

	bool
		Environment::Remove(string_view id, bool forced)
	{
		WAssertNonnull(id.data());
		if (Bindings.Remove(id))
			return true;
		if (forced)
			return {};
		throw BadIdentifier(id, 0);
	}

	void
		Environment::ThrowForInvalidType(const white::type_info& tp)
	{
		throw WSLException(white::sfmt("Invalid environment type '%s' found.",
			tp.name()));
	}


	EnvironmentReference::EnvironmentReference(const shared_ptr<Environment>& p_env)
		// TODO: Blocked. Use C++1z %weak_from_this and throw-expression?
		: EnvironmentReference(p_env, p_env ? p_env->Anchor() : nullptr)
	{}


	ContextNode::ContextNode(const ContextNode& ctx,
		shared_ptr<Environment>&& p_rec)
		: p_record([&] {
		if (p_rec)
			return std::move(p_rec);
		ThrowInvalidEnvironment();
	}()),
		EvaluateLeaf(ctx.EvaluateLeaf), EvaluateList(ctx.EvaluateList),
		EvaluateLiteral(ctx.EvaluateLiteral), Trace(ctx.Trace)
	{}
	ContextNode::ContextNode(ContextNode&& ctx) wnothrow
		: ContextNode()
	{
		swap(ctx, *this);
	}

	ReductionStatus
		ContextNode::ApplyTail()
	{
		// TODO: Avoid stack overflow when current action is called.
		WAssert(bool(Current), "No tail action found.");
		return LastStatus = Switch()();
	}

	void
		ContextNode::Pop() wnothrow
	{
		WAssert(!Delimited.empty(), "No continuation is delimited.");
		SetupTail(std::move(Delimited.front()));
		Delimited.pop_front();
	}

	void
		ContextNode::Push(const Reducer& reducer)
	{
		WAssert(Current, "No continuation can be captured.");
		Delimited.push_front(reducer);
		std::swap(Current, Delimited.front());
	}
	void
		ContextNode::Push(Reducer&& reducer)
	{
		WAssert(Current, "No continuation can be captured.");
		Delimited.push_front(std::move(reducer));
		std::swap(Current, Delimited.front());
	}

	ReductionStatus
		ContextNode::Rewrite(Reducer reduce)
	{
		SetupTail(reduce);
		// NOTE: Rewriting loop until no actions remain.
		return white::retry_on_cond(std::bind(&ContextNode::Transit, this), [&] {
			return ApplyTail();
		});
	}

	ReductionStatus
		ContextNode::RewriteGuarded(TermNode& term, Reducer reduce)
	{
		const auto gd(Guard(term, *this));

		return Rewrite(reduce);
	}

	shared_ptr<Environment>
		ContextNode::SwitchEnvironment(shared_ptr<Environment> p_env)
	{
		if (p_env)
			return SwitchEnvironmentUnchecked(std::move(p_env));
		ThrowInvalidEnvironment();
	}

	shared_ptr<Environment>
		ContextNode::SwitchEnvironmentUnchecked(shared_ptr<Environment> p_env) wnothrowv
	{
		WAssertNonnull(p_env);
		return white::exchange(p_record, p_env);
	}

	bool
		ContextNode::Transit() wnothrow
	{
		if (!Current)
		{
			if (!Delimited.empty())
				Pop();
			else
				return {};
		}
		return true;
	}


	Reducer
		CombineActions(ContextNode& ctx, Reducer&& cur, Reducer&& next)
	{
		// NOTE: Lambda is not used to avoid unspecified destruction order of
		//	captured component and better performance (compared to the case of
		//	%pair used to keep the order).
		struct Action final
		{
			lref<ContextNode> Context;
			// NOTE: The destruction order of captured component is significant.
			//@{
			mutable Reducer Next;
			Reducer Current;

			Action(ContextNode& ctx, Reducer& cur, Reducer& next)
				: Context(ctx), Next(std::move(next)), Current(std::move(cur))
			{}
			//@}
			// XXX: Copy is not intended used directly, but for well-formness.
			DefDeCopyMoveCtor(Action)

				DefDeMoveAssignment(Action)

				ReductionStatus
				operator()() const
			{
				RelaySwitched(Context, std::move(Next));
				return Current();
			}
		};

		return cur ? Action(ctx, cur, next) : std::move(next);
	}

	ReductionStatus
		RelayNext(ContextNode& ctx, Reducer&& cur, Reducer&& next)
	{
		ctx.SetupTail(CombineActions(ctx, std::move(cur), std::move(next)));
		return ReductionStatus::Retrying;
	}
	
} // namespace scheme;
