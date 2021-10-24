/*!	\file LScheme.h
\par �޸�ʱ��:
2017-03-27 11:05 +0800
*/

#ifndef WScheme_WScheme_H
#define WScheme_WScheme_H 1

#include "WSchemeA.h" // for WSLATag, ValueNode, TermNode, LoggedEvent,
//	white::Access, white::equality_comparable, white::exclude_self_t,
//	white::AreEqualHeld, white::GHEvent, white::make_function_type_t,
//	white::make_parameter_tuple_t, white::make_expanded,
//	white::invoke_nonvoid, white::make_transform, std::accumulate,
//	white::bind1, std::placeholders::_2, white::examiners::equal_examiner;

namespace scheme {
	namespace v1 {
		/*!
		\brief WSLV1 Ԫ��ǩ��
		\note WSLV1 �� WSLA �ľ���ʵ�֡�
		*/
		struct WS_API WSLV1Tag : WSLATag
		{};


		//! \brief ֵ�Ǻţ��ڵ��е�ֵ��ռλ����
		enum class ValueToken
		{
			Null,
			/*!
			\brief δ����ֵ��
			*/
			Undefined,
			/*!
			\brief δָ��ֵ��
			*/
			Unspecified,
			GroupingAnchor,
			OrderedAnchor
		};

		/*!
		\brief ȡֵ�Ǻŵ��ַ�����ʾ��
		\return ��ʾ��Ӧ�Ǻ�ֵ���ַ�������֤����ȵ������Ӧ����ȵĽ����
		\throw std::invalid_argument �����ֵ���ǺϷ���ֵ�Ǻš�
		\relates ValueToken
		*/
		WS_API string
			to_string(ValueToken);

		//@{
		//! \brief ���� WSLV1 �ӽڵ㡣
		//@{
		/*!
		\note �������������˳��

		��һ����ָ���ı任�������ڶ�����ָ����������
		��ӳ��������ؽڵ�����Ϊ������ݵ�ǰ�������ӽڵ�������ǰ׺ $ �����Ա����ظ���
		*/
		WS_API void
			InsertChild(TermNode&&, TermNode::Container&);

		/*!
		\note ����˳��

		ֱ�Ӳ��� WSLV1 �ӽڵ㵽��������ĩβ��
		*/
		WS_API void
			InsertSequenceChild(TermNode&&, NodeSequence&);
		//@}

		/*!
		\brief �任 WSLV1 �ڵ� S ���ʽ�����﷨��Ϊ WSLV1 ����ṹ��
		\exception std::bad_function_call �����������Ϊ�ա�
		\return �任����½ڵ㣨���ӽڵ㣩��

		��һ����ָ��Դ�ڵ㣬�������ָ�����ֱ任����
		�������õĵڶ������Ĳ������޸Ĵ���Ľڵ����ʱ�任���޸�Դ�ڵ㡣
		�������£�
		��Դ�ڵ�ΪҶ�ڵ㣬ֱ�ӷ���ʹ�õ�����������ӳ��Ľڵ㡣
		��Դ�ڵ�ֻ��һ���ӽڵ㣬ֱ�ӷ�������ӽڵ�ı任�����
		����ʹ�õ��Ĳ����ӵ�һ���ӽڵ�ȡ��Ϊ�任������Ƶ��ַ�����
			�����Ʒǿ�����Ե�һ���ӽڵ㣬ֻ�任ʣ���ӽڵ㡣
				��ʣ��һ���ӽڵ㣨��Դ�ڵ��������ӽڵ㣩ʱ��ֱ�ӵݹ�任����ڵ㲢���ء�
				���任��Ľ�����Ʒǿգ�����Ϊ�����ֵ�����򣬽����Ϊ�����ڵ�һ��ֵ��
			�����½��ڵ��������������任ʣ��Ľڵ������������������������������Ľڵ㡣
				�ڶ�����ָ����ʱ��ӳ���������Ϊ����Ĭ��ʹ�õݹ� TransformNode ���á�
				���õ����������ӳ��Ľ����������
		*/
		WS_API ValueNode
			TransformNode(const TermNode&, NodeMapper = {}, NodeMapper = MapWSLALeafNode,
				NodeToString = ParseWSLANodeString, NodeInserter = InsertChild);

		/*!
		\brief �任 WSLV1 �ڵ� S ���ʽ�����﷨��Ϊ WSLV1 ��������ṹ��
		\exception std::bad_function_call �����������Ϊ�ա�
		\return �任����½ڵ㣨���ӽڵ㣩��
		\sa TransformNode

		�� TransformNode �任������ͬ��
		��������ӽڵ��� NodeSequence ����ʽ��Ϊ�任�ڵ��ֵ�������ӽڵ㣬�ɱ���˳��
		*/
		WS_API ValueNode
			TransformNodeSequence(const TermNode&, NodeMapper = {},
				NodeMapper = MapWSLALeafNode, NodeToString = ParseWSLANodeString,
				NodeSequenceInserter = InsertSequenceChild);
		//@}


		/*!
		\brief ���� WSLV1 ���뵥Ԫ��
		\throw LoggedEvent ���棺�����������е�ʵ��ת��ʧ�ܡ�
		*/
		//@{
		template<typename _type, typename... _tParams>
		ValueNode
			LoadNode(_type&& tree, _tParams&&... args)
		{
			TryRet(v1::TransformNode(std::forward<TermNode&&>(tree),
				wforward(args)...))
				CatchThrow(white::bad_any_cast& e, LoggedEvent(white::sfmt(
					"Bad WSLV1 tree found: cast failed from [%s] to [%s] .", e.from(),
					e.to()), white::Warning))
		}

		template<typename _type, typename... _tParams>
		ValueNode
			LoadNodeSequence(_type&& tree, _tParams&&... args)
		{
			TryRet(v1::TransformNodeSequence(std::forward<TermNode&&>(tree),
				wforward(args)...))
				CatchThrow(white::bad_any_cast& e, LoggedEvent(white::sfmt(
					"Bad WSLV1 tree found: cast failed from [%s] to [%s] .", e.from(),
					e.to()), white::Warning))
		}
		//@}



		/*!
		\brief WSLA1 ���ʽ�ڵ��Լ����������һ����ֵ���̹�Լ�ӱ��ʽ��
		\return ��Լ״̬��
		\sa ContextNode::RewriteGuarded
		\sa ReduceOnce

		�Ե�һ����Ϊ��� ReduceOnce Ϊ��Լ�������� ContextNode::RewriteGuarded ��
		*/
		WS_API ReductionStatus
			Reduce(TermNode&, ContextNode&);

		/*!
		\brief �ٴι�Լ��
		\sa ContextNode::SetupTail
		\sa ReduceOnce
		\sa RelayNext
		\return ReductionStatus::Retrying

		ȷ���ٴ� Reduce ���ò�����Ҫ���ع�Լ�Ľ����
		*/
		WS_API ReductionStatus
			ReduceAgain(TermNode&, ContextNode&);

		/*!
		\note ����ʹ�����������ĵ�����ʧЧ��
		\sa Reduce
		*/
		//@{
		/*!
		\note �����Թ淶�������Լ˳��δָ����
		\note ���������ع�ԼҪ��
		*/
		//@{
		/*!
		\brief �Է�Χ�ڵĵڶ��ʼ�����Լ��
		\throw InvalidSyntax ����Ϊ�ա�
		\sa ReduceChildren
		*/
		//@{
		WS_API void
			ReduceArguments(TNIter, TNIter, ContextNode&);
		inline PDefH(void, ReduceArguments, TermNode::Container& con, ContextNode& ctx)
			ImplRet(ReduceArguments(con.begin(), con.end(), ctx))
		inline PDefH(void, ReduceArguments, TermNode& term, ContextNode& ctx)
			ImplRet(ReduceArguments(term.GetContainerRef(), ctx))
		//@}

		//! \note ʧ����Ϊ�ع�Լ��
		//@{
		/*!
		\brief ��Լ�����ɹ�����Ч���� Reduce �������ֱ�������ع�Լ��
		\note ֧��β�����Ż�����ֱ��ʹ�� CheckedReduceWith �� Reduce ��
		\return ReductionStatus::Retrying ��
		\sa CheckedReduceWith
		\sa Reduce
		*/
		WS_API ReductionStatus
			ReduceChecked(TermNode&, ContextNode&);

		/*!
		\brief ��Լ�հ���
		\return ���ݹ�Լ��ʣ����ȷ���Ĺ�Լ�����
		\sa CheckNorm
		\sa ReduceChecked

		�����Լ���Լ���滻����һ����ָ���
		��Լ��������ɵ��Ĳ����ıհ�ָ������������ָ���Ƿ�ͨ��ת�ƹ����������ԭ�
		��Լ��ת�Ʊհ���Լ�Ľ���������Լ����õ�ֵ��Ŀ�걻ת�Ƶ���һ����ָ�����
		����������ֵ֮�䱻ת�Ƶ����˳��δָ����
		��Լ�հ�����Ϊ �� ��Լ��̬��������ֵ��β���á�
		*/
		WS_API ReductionStatus
			ReduceCheckedClosure(TermNode&, ContextNode&, bool, TermNode&);
		//@}

		/*!
		\brief ��Լ���
		*/
		//@{
		WS_API void
			ReduceChildren(TNIter, TNIter, ContextNode&);
		inline PDefH(void, ReduceChildren, TermNode::Container& con, ContextNode& ctx)
			ImplExpr(ReduceChildren(con.begin(), con.end(), ctx))
		inline PDefH(void, ReduceChildren, TermNode& term, ContextNode& ctx)
			ImplExpr(ReduceChildren(term.GetContainerRef(), ctx))
		//@}
		//@}



		/*!
		\brief �����Լ���
		\return ����������ʱΪ���һ������Ĺ�Լ״̬������Ϊ ReductionStatus::Clean ��
		*/
		//@{
			WS_API ReductionStatus
			ReduceChildrenOrdered(TNIter, TNIter, ContextNode&);
		inline PDefH(ReductionStatus, ReduceChildrenOrdered, TermNode::Container& con,
			ContextNode& ctx)
			ImplRet(ReduceChildrenOrdered(con.begin(), con.end(), ctx))
			inline PDefH(ReductionStatus, ReduceChildrenOrdered, TermNode& term,
				ContextNode& ctx)
			ImplRet(ReduceChildrenOrdered(term.GetContainerRef(), ctx))
			//@}

		/*!
		\brief ��Լ��һ�����
		\return ��Լ״̬��
		\sa Reduce
		\see https://en.wikipedia.org/wiki/Fexpr ��

		�����ϸ��Է�����
		��������ֵ֦�ڵ��һ���Ա����ȷ�����ƶ��ӱ��ʽ��ֵ�ĸ��Ӹ��Ӷȡ�
		*/
		WS_API ReductionStatus
		ReduceFirst(TermNode&, ContextNode&);

		/*!
		\brief WSLA1 ���ʽ�ڵ��Լ��������ֵ���̹�Լ�ӱ��ʽ��
		\return ��Լ״̬��
		\note �쳣��ȫΪ���ñ������쳣��ȫ��֤��
		\note ����ʹ�����������ĵ�����ʧЧ��
		\note Ĭ�ϲ���Ҫ�ع�Լ����ɱ���ֵ��ı䡣
		\note �ɱ���ֵ�������ʵ�ֵݹ���ֵ��
		\sa ContextNode::EvaluateLeaf
		\sa ContextNode::EvaluateList
		\sa ValueToken
		\sa ReduceAgain
		\todo ʵ�� ValueToken ��������

		��Ӧ��ͬ�Ľڵ�μ��ṹ���࣬һ�ε���������˳��ѡ�����·�֧֮һ�������Լ���
		��֦�ڵ���� ContextNode::EvaluateList ��ֵ��
		�Կսڵ��ֵΪ ValueToken ��Ҷ�ڵ㲻���в�����
		������Ҷ�ڵ���� ContextNode::EvaluateList ��ֵ��
		������������ CheckReducible �����ݽ���ж��Ƿ�����ع�Լ��
		�˴�Լ���ĵ����жԽڵ�ľ���ṹ����Ĭ��Ҳ���������� WSLA1 ʵ�� API ��
		�������Ӧ����ָ����ȷ��˳��
		���������������ڵ㲻�Ǳ��ʽ����ṹ������ AST ���� API ���� TransformNode ��
		��ǰʵ�ַ��صĹ�Լ״̬���� ReductionStatus::Clean �������ѭ��������
		����Ҫ��֤���쳣ʱ���ڹ�Լ�ɹ�����ֹ��ʹ�� ReduceChecked ���档
		*/
		WS_API ReductionStatus
			ReduceOnce(TermNode&, ContextNode&);

		/*!
		\brief ��Լ�������У�˳���Լ������Ϊ���һ������Ĺ�Լ�����
		\return ����������ʱΪ���һ������Ĺ�Լ״̬������Ϊ ReductionStatus::Clean ��
		\sa ReduceChildrenOrdered
		*/
		WS_API ReductionStatus
			ReduceOrdered(TermNode&, ContextNode&);

		/*!
		\brief �Ƴ��������ָ��������������Լ��
		*/
		WS_API ReductionStatus
		ReduceTail(TermNode&, ContextNode&, TNIter);
		//@}

		/*!
		\brief ���ø�����Ƚڵ㣺���ù�Լʱ��ʾ��Ⱥ������ĵ���Ϣ��
		\note ��Ҫ���ڵ��ԡ�
		\sa InvokeGuard
		*/
		WS_API void
		SetupTraceDepth(ContextNode& ctx, const string& name = wimpl("$__depth"));

		/*!
		\note ValueObject �����ֱ�ָ���滻��ӵ�ǰ׺�ͱ��滻�ķָ�����ֵ��
		*/
		//@{
		/*!
		\brief �任�ָ�����׺���ʽΪǰ׺���ʽ��
		\sa AsIndexNode

		�Ƴ�������ֵ��ָ���ָ���ָ��������� AsIndexNode ���ָ��ǰ׺ֵ��Ϊ���
		����ӵ���������ֻ��һ��������б���������ֱ�Ӽ���ת���������Ϊ���
		���һ������ָ������ֵ�����ơ�
		*/
		//@{

		//! \note �ǵݹ�任��
		//@{
		WS_API TermNode
			TransformForSeparator(const TermNode&, const ValueObject&, const ValueObject&,
				const TokenValue& = {});
		WS_API TermNode
			TransformForSeparator(TermNode&, const ValueObject&, const ValueObject&,
				const TokenValue& = {});
		//@}

		//! \note �ݹ�任��
		//@{
		WS_API TermNode
			TransformForSeparatorRecursive(const TermNode&, const ValueObject&,
				const ValueObject&, const TokenValue& = {});

		WS_API TermNode
			TransformForSeparatorRecursive(TermNode&, const ValueObject&,
				const ValueObject&, const TokenValue& = {});
		//@}
		//@}

		/*!
		\brief �������е�ָ���ָ��������ҵ����滻��Ϊȥ���ָ���������滻ǰ׺����ʽ��
		\return �Ƿ��ҵ����滻���
		\sa EvaluationPasses
		\sa TransformForSeparator
		*/
		WS_API ReductionStatus
			ReplaceSeparatedChildren(TermNode&, const ValueObject&, const ValueObject&);
		//@}


		//@{
		/*!
		\brief ��װ�����Ĵ�������
		\note ���Ա���װ�������Ĵ��������ܴ��ڵķ���ֵ������ӦĬ�Ϸ��ع�Լ�����
		*/
		template<typename _func>
		struct WrappedContextHandler
			: private white::equality_comparable<WrappedContextHandler<_func>>
		{
			_func Handler;

			//@{
			template<typename _tParam, wimpl(typename
				= white::exclude_self_t<WrappedContextHandler, _tParam>)>
				WrappedContextHandler(_tParam&& arg)
				: Handler(wforward(arg))
			{}
			template<typename _tParam1, typename _tParam2, typename... _tParams>
			WrappedContextHandler(_tParam1&& arg1, _tParam2&& arg2, _tParams&&... args)
				: Handler(wforward(arg1), wforward(arg2), wforward(args)...)
			{}

			DefDeCopyMoveCtorAssignment(WrappedContextHandler)
				//@}

				template<typename... _tParams>
			ReductionStatus
				operator()(_tParams&&... args) const
			{
				Handler(wforward(args)...);
				return ReductionStatus::Clean;
			}

			/*!
			\brief �Ƚ������Ĵ�������ȡ�
			\note ʹ�� white::AreEqualHeld ��
			*/
			friend PDefHOp(bool, == , const WrappedContextHandler& x,
				const WrappedContextHandler& y)
				ImplRet(white::AreEqualHeld(x.Handler, y.Handler))
		};

		template<class _tDst, typename _func>
		inline _tDst
			WrapContextHandler(_func&& h, white::false_)
		{
			return WrappedContextHandler<white::GHEvent<white::make_function_type_t<
				void, white::make_parameter_tuple_t<typename _tDst::BaseType>>>>(
					wforward(h));
		}
		template<class, typename _func>
		inline _func
			WrapContextHandler(_func&& h, white::true_)
		{
			return wforward(h);
		}
		template<class _tDst, typename _func>
		inline _tDst
			WrapContextHandler(_func&& h)
		{
			using BaseType = typename _tDst::BaseType;

			// XXX: It is a hack to adjust the convertible result for the expanded
			//	caller here. It should have been implemented in %GHEvent, however types
			//	those cannot convert to expanded caller cannot be SFINAE'd out,
			//	otherwise it would cause G++ 5.4 crash with internal compiler error:
			//	"error reporting routines re-entered".
			return v1::WrapContextHandler<_tDst>(wforward(h), white::or_<
				std::is_constructible<BaseType, _func&&>,
				std::is_constructible<BaseType, white::expanded_caller<
				typename _tDst::FuncType, white::decay_t<_func>>>>());
		}
		//@}

		/*!
		\brief ��ʽ�����Ĵ�������
		*/
		class WS_API FormContextHandler
		{
		public:
			ContextHandler Handler;
			/*!
			\brief �������̣���֤����װ�Ĵ������ĵ��÷���ǰ��������
			*/
			std::function<bool(const TermNode&)> Check{IsBranch};

			template<typename _func,
				wimpl(typename = white::exclude_self_t<FormContextHandler, _func>)>
				FormContextHandler(_func&& f)
				: Handler(v1::WrapContextHandler<ContextHandler>(wforward(f)))
			{}
			template<typename _func, typename _fCheck>
			FormContextHandler(_func&& f, _fCheck c)
				: Handler(v1::WrapContextHandler<ContextHandler>(wforward(f))), Check(c)
			{}

			DefDeCopyMoveCtorAssignment(FormContextHandler)

			/*!
			\brief �Ƚ������Ĵ�������ȡ�
			\note ���Լ�����̵ĵȼ��ԡ�
			*/
			friend PDefHOp(bool, == , const FormContextHandler& x,
					const FormContextHandler& y)
				ImplRet(x.Handler == y.Handler)

			/*!
			\brief ����һ����ʽ��
			\exception WSLException �쳣������
			\throw LoggedEvent ���棺���Ͳ�ƥ�䣬
			�� Handler �׳��� white::bad_any_cast ת����
			\throw LoggedEvent ������ Handler �׳��� white::bad_any_cast ���
			std::exception ת����
			\throw std::invalid_argument ����δͨ����

			���鲻���ڻ��ڼ��ͨ���󣬶Խڵ���� Hanlder �������׳��쳣��
			*/
			ReductionStatus
				operator()(TermNode&, ContextNode&) const;
		};


		/*!
		\brief �ϸ������Ĵ�������
		*/
		class WS_API StrictContextHandler
		{
		public:
			FormContextHandler Handler;

			template<typename _func,
				wimpl(typename = white::exclude_self_t<StrictContextHandler, _func>)>
				StrictContextHandler(_func&& f)
				: Handler(wforward(f))
			{}
			template<typename _func, typename _fCheck>
			StrictContextHandler(_func&& f, _fCheck c)
				: Handler(wforward(f), c)
			{}

			DefDeCopyMoveCtorAssignment(StrictContextHandler)

			friend PDefHOp(bool, == , const StrictContextHandler& x,
					const StrictContextHandler& y)
				ImplRet(x.Handler == y.Handler)
			/*!
			\brief ��������
			\throw ListReductionFailure �б��������һ�
			\sa ReduceArguments

			��ÿһ��������ֵ��Ȼ�����������Կɵ��õ������ Hanlder �������׳��쳣��
			*/
			ReductionStatus
				operator()(TermNode&, ContextNode&) const;
		};


		//@{
		template<typename... _tParams>
		inline void
			RegisterForm(ContextNode& node, const string& name,
				_tParams&&... args)
		{
			scheme::RegisterContextHandler(node, name,
				FormContextHandler(wforward(args)...));
		}

		//! \brief ת�������Ĵ�������
		template<typename... _tParams>
		inline ContextHandler
			ToContextHandler(_tParams&&... args)
		{
			return StrictContextHandler(wforward(args)...);
		}

		/*!
		\brief ע���ϸ������Ĵ�������
		\note ʹ�� ADL ToContextHandler ��
		*/
		template<typename... _tParams>
		inline void
			RegisterStrict(ContextNode& node, const string& name, _tParams&&... args)
		{
			scheme::RegisterContextHandler(node, name,
				ToContextHandler(wforward(args)...));
		}
		//@}

		/*!
		\brief ע��ָ���ת���任�ʹ������̡�
		\sa WSL::RegisterContextHandler
		\sa ReduceChildren
		\sa ReduceOrdered
		\sa ReplaceSeparatedChildren

		�任������׺��ʽ�ķָ����Ǻŵı��ʽΪָ�����Ƶ�ǰ׺���ʽ��ȥ���ָ�����
		���һ������ָ���Ƿ�����ѡ���﷨��ʽΪ ReduceOrdered �� ReduceChildren ֮һ��
		ǰ׺���Ʋ���Ҫ�ǼǺ�֧�ֵı�ʶ����
		*/
		WS_API void
			RegisterSequenceContextTransformer(EvaluationPasses&,const ValueObject&,
				bool = {});

		/*!
		\brief ȡ��Ĳ����������������� 1 ��
		\pre ��Ӷ��ԣ�����ָ��������֦�ڵ㡣
		\return ��Ĳ���������
		*/
		inline PDefH(size_t, FetchArgumentN, const TermNode& term) wnothrowv
			ImplRet(AssertBranch(term), term.size() - 1)

		//! \note ��һ����ָ���������� Value ָ�������ֵ��
		//@{
		//! \sa LiftDelayed
		//@{
		/*!
		\brief ��ֵ�Խڵ����ݽṹ��ӱ�ʾ���

		�� TermNode �������ֵ�����ɹ����� LiftTermRef �滻ֵ������Ҫ���ع�Լ��
		������ʶԹ�Լ����ת�ƵĿ���δ��ֵ�Ĳ������Ǳ�Ҫ�ġ�
		*/
		WS_API ReductionStatus
		EvaluateDelayed(TermNode&);
		/*!
		\brief ��ֵָ�����ӳ���ֵ�
		\return ReductionStatus::Retrying ��

		����ָ�����ӳ���ֵ���Լ��
		*/
		WS_API ReductionStatus
			EvaluateDelayed(TermNode&, DelayedTerm&);
		//@}


		/*!
		\exception BadIdentifier ��ʶ��δ������
		\note ��һ����ָ���������� Value ָ�������ֵ��
		\note Ĭ����Ϊ��Լ�ɹ��Ա�֤ǿ�淶�����ʡ�
		*/
		//@{
		//! \pre ���ԣ���������������ָ��ǿա�
		//@{
		/*!
		\brief ��ֵ��ʶ����
		\note ����֤��ʶ���Ƿ�Ϊ����������������������ʱ������Ҫ�ع�Լ��
		\sa EvaluateDelayed
		\sa LiftTermRef
		\sa LiteralHandler
		\sa ResolveName

		���ν���������ֵ������
		���� ResolveName ����ָ�����Ʋ���ֵ����ʧ���׳�δ�����쳣��
		���� LiftTermRef �� TermNode::SetContentIndirect �滻���б���б�ڵ��ֵ��
		�� LiteralHandler ���������������������ɹ����ò������������������Ĵ�������
		��δ���أ����ݽڵ��ʾ��ֵ��һ������
			�Ա�ʾ�� TokenValue ֵ��Ҷ�ڵ㣬���� EvaluateDelayed ��ֵ��
			�Ա�ʾ TokenValue ֵ��Ҷ�ڵ㣬���� ReductionStatus::Retrying �ع�Լ��
			��֦�ڵ���Ϊ�б����� ReductionStatus::Retained �����һ����ֵ��
		*/
		WS_API ReductionStatus
			EvaluateIdentifier(TermNode&, const ContextNode&, string_view);

		/*!
		\brief ��ֵҶ�ڵ�Ǻš�
		\sa CategorizeLiteral
		\sa DeliteralizeUnchecked
		\sa EvaluateIdentifier
		\sa InvokeLiteral

		����ǿ��ַ�����ʾ�Ľڵ�Ǻš�
		���ν���������ֵ������
		�Դ�����������ȥ���������߽�ָ������һ����ֵ��
		��������������ȥ���������߽�ָ�����Ϊ�ַ���ֵ��
		��������������ͨ�������������鴦��
		�����ֵ���������ı�ʶ����
		*/
		WS_API ReductionStatus
			EvaluateLeafToken(TermNode&, ContextNode&, string_view);
		//@}

		/*!
		\brief ��Լ�ϲ�������ĵ�һ���������Ϊ���������к���Ӧ�ã����淶����
		\return ��Լ״̬��
		\throw ListReductionFailure ��Լʧ�ܣ�֦�ڵ�ĵ�һ�������ʾ�����Ĵ�������
		\sa ContextHandler
		\sa Reduce

		��֦�ڵ㳢���Ե�һ������� Value ���ݳ�ԱΪ�����Ĵ����������ã��ҵ���Լ��ֹʱ�淶����
		������Ϊ��Լ�ɹ���û���������á�
		������ ContextHandler ���ã�����ǰ��ת�ƴ�������֤�����ڣ�
		�����������ڲ��Ƴ����޸�֮ǰռ�õĵ�һ������������е� Value ���ݳ�Ա����
		*/
		WS_API ReductionStatus
			ReduceCombined(TermNode&, ContextNode&);

		/*!
		\brief ��Լ��ȡ���Ƶ�Ҷ�ڵ�Ǻš�
		\sa EvaluateLeafToken
		\sa TermToNode
		*/
		WS_API ReductionStatus
			ReduceLeafToken(TermNode&, ContextNode&);
		//@}

		/*!
		\brief �������ƣ����������Ʋ��������ơ�
		\pre ���ԣ��ڶ�����������ָ��ǿա�
		\exception NPLException ���ʹ����ض���������ʧ�ܡ�
		\sa Environment::ResolveName
		*/
		WS_API  Environment::NameResolution
			ResolveName(const ContextNode&, string_view);

		/*!
		\brief ����������
		\return ȡ������Ȩ�Ļ���ָ�뼰�Ƿ��������Ȩ��
		\note ֻ֧������ֵ���� \c shared_ptr<Environment> �� \c weak_ptr<Environment> ��
		*/
		//@{
		WS_API pair<shared_ptr<Environment>, bool>
			ResolveEnvironment(ValueObject&);
		inline PDefH(pair<shared_ptr<Environment> WPP_Comma bool>, ResolveEnvironment,
			TermNode& term)
			ImplRet(ResolveEnvironment(ReferenceTerm(term).Value))
			//@}

		/*!
		\brief ����Ĭ�Ͻ��ͣ�����ʹ�õĹ�������顣
		\note ��ǿ�쳣��ȫ������������ǰ����״̬������ʧ��ʱ�ع���
		\sa EvaluateContextFirst
		\sa ReduceFirst
		\sa ReduceLeafToken
		*/
		WS_API void
			SetupDefaultInterpretation(ContextNode&, EvaluationPasses);

		/*!
		\brief WSLV1 �﷨��ʽ��Ӧ�Ĺ���ʵ�֡�
		*/
		namespace Forms
		{
			/*!
			\brief �ж��ַ���ֵ�Ƿ�ɹ��ɷ��š�
			�ο��ķ���

			symbol? <object>
			*/
			WS_API bool
				IsSymbol(const string&) wnothrow;

			//@{
			/*!
			\brief ��������ָ���ַ���ֵ�ļǺ�ֵ��
			\note �����ֵ�Ƿ���Ϸ���Ҫ��
			*/
			WS_API TokenValue
				StringToSymbol(const string&);

			//! \brief ȡ���Ŷ�Ӧ�������ַ�����
			WS_API const string&
				SymbolToString(const TokenValue&) wnothrow;
			//@}


			/*!
			\pre ���ԣ����������Ӧ֦�ڵ㡣
			\sa AssertBranch
			*/
			//@{
			/*!
			\note ������ֵ����������;��һ�㲻��Ҫ����Ϊ�û�����ֱ��ʹ�á�

			��ʹ�� RegisterForm ע�������Ĵ��������ο��ķ���
			$retain|$retainN <expression>
			*/
			//@{
			//! \brief �����������ֵ��
			inline PDefH(ReductionStatus, Retain, const TermNode& term) wnothrowv
				ImplRet(AssertBranch(term), ReductionStatus::Retained)


			/*!
			\brief ���������ȷ������ָ�������������������ֵ��
			\return ��Ĳ���������
			\throw ArityMismatch ��Ĳ������������ڵڶ�������
			\sa FetchArgumentN
			*/
			WS_API size_t
			RetainN(const TermNode&, size_t = 1);
			//@}

			//! \throw ParameterMismatch ƥ��ʧ�ܡ�
			//@{
			/*!
			\pre ���ԣ��ַ�������������ָ��ǿա�
			*/
			//@{
			//! \brief ���Ǻ�ֵ�Ƿ���ƥ�������ķ��š�
			template<typename _func>
			auto
				CheckSymbol(string_view n, _func f) -> decltype(f())
			{
				if (IsWSLASymbol(n))
					return f();
				throw ParameterMismatch(white::sfmt(
					"Invalid token '%s' found for symbol parameter.", n.data()));
			}

			//! \brief ���Ǻ�ֵ�Ƿ���ƥ�������Ĳ������š�
			template<typename _func>
			auto
				CheckParameterLeafToken(string_view n, _func f) -> decltype(f())
			{
				if (n != "#ignore")
					CheckSymbol(n, f);
			}
			//@}

			/*!
			\note ������ǿ�쳣��ȫ��֤��ƥ��ʧ��ʱ�������İ�״̬δָ����
			\sa LiftToSelf

			�ݹ���������Ͳ����������нṹ��ƥ�䡣
			��ƥ��ʧ�ܣ����׳��쳣��
			*/
			//@{
			/*!
			\brief ʹ�ò������ṹ��ƥ�䲢�󶨲�����
			\throw ArityMismatch ������ƥ��ʧ�ܡ�
			\note ��һ����ָ���������ľ����󶨵Ļ�����
			\sa MatchParameter
			\sa TermReference

			��ʽ�����Ͳ�����Ϊ��ָ���ı��ʽ����
			�ڶ�����ָ����ʽ��������������ָ����������
			������������ǰ���Բ��������� LiftToSelf ��������������ʽ������
			����ƥ����㷨�ݹ�������ʽ�����������Ҫ��μ� MatchParameter ��
			��ƥ��ɹ����ڵ�һ����ָ���Ļ����ڰ�δ�����Ե�ƥ����
			�Խ�β��������ƥ��ǰ׺Ϊ . �ķ���ΪĿ�갴���¹�����Ի�󶨣�
			����Ϊ . ʱ����Ӧ�������Ľ�β���б����ԣ�
			���򣬰����Ŀ��Ϊ�Ƴ�ǰ׺ . �ͺ�����ѡǰ׺ & ��ķ��š�
			���б���İ�ʹ�����¹���
			��ƥ��ɹ����ڵ�һ����ָ���Ļ����ڰ�δ�����Ե�ƥ��ķ��б��
			ƥ��Ҫ�����£�
			������ #ignore ������Բ�������Ӧ���
			�����ֵ�Ƿ��ţ���������Ķ�Ӧ����ӦΪ���б��
			�����󶨵�Ŀ���� & �����԰����ô��ݵķ�ʽ�󶨣������԰�ֵ���ݵķ�ʽ�󶨡�
			�����ô��ݰ�ֱ��ת�Ƹ�������ݡ�
			*/
			WS_API void
				BindParameter(ContextNode&, const TermNode&, TermNode&);
			
			/*!
			\brief ƥ�������
			\exception std::bad_function �쳣����������ָ���Ĵ�����Ϊ�ա�

			����ƥ����㷨�ݹ�������ʽ�����������
			��ƥ��ɹ������ò���ָ����ƥ�䴦������
			������Ϊ�����б��β�Ľ�β���д�������ֵ���������ֱ�ƥ���� . ��ʼ����ͷ��б��
			��β���д�����������ַ���������ʾ��󶨵ı�ʾ��β���е��б��ʶ����
			ƥ��Ҫ�����£�
			�����Ƿǿ��б���������Ķ�Ӧ����ӦΪ����ȷ�����������б�
			����������Ϊ . ��ʼ�ķ��ţ���ƥ��������н�β���������������Ϊ��β���У�
			��������һһƥ������������
			�����ǿ��б���������Ķ�Ӧ����ӦΪ���б�
			����ƥ����б��
			*/
			WS_API void
				MatchParameter(const TermNode&, TermNode&,
					std::function<void(TNIter, TNIter, const TokenValue&)>,
					std::function<void(const TokenValue&, TermNode&&)>);
			//@}
			//@}
			//@}

			/*!
			\brief ���ʽڵ㲢����һԪ������
			\sa white::EmplaceCallResult
			ȷ�������һ��ʵ�ʲ�����չ�����ò���ָ���ĺ�����
			�������õĺ����������ͷ� void ������ֵ��Ϊ���ֵ�����졣
			���� white::EmplaceCallResult �� ValueObject ������ֵ����ͬ��
			�����Ժ��������͵�ֵ���Ƶķ�ʽ����װ���ڵ�һ�������й��� ValueObject ����
			*/
			//@{
			template<typename _func, typename... _tParams>
			void
				CallUnary(_func&& f, TermNode& term, _tParams&&... args)
			{
				RetainN(term);
				white::EmplaceCallResult(term.Value, white::invoke_nonvoid(
					white::make_expanded<void(TermNode&, _tParams&&...)>(wforward(f)),
					Deref(std::next(term.begin())), wforward(args)...));
			}

			template<typename _type, typename _func, typename... _tParams>
			void
				CallUnaryAs(_func&& f, TermNode& term, _tParams&&... args)
			{
				Forms::CallUnary([&](TermNode& node) {
					// XXX: Blocked. 'wforward' cause G++ 5.3 crash: internal compiler
					//	error: Segmentation fault.
					return white::make_expanded<void(_type&, _tParams&&...)>(wforward(f))(
						white::Access<_type>(node), std::forward<_tParams&&>(args)...);
				}, term);
			}
			//@}

			/*!
			\brief ���ʽڵ㲢���ö�Ԫ������
			*/
			//@{
			template<typename _func, typename... _tParams>
			void
				CallBinary(_func&& f, TermNode& term, _tParams&&... args)
			{
				RetainN(term, 2);

				auto i(term.begin());
				auto& x(Deref(++i));

				white::EmplaceCallResult(term.Value, white::invoke_nonvoid(
					white::make_expanded<void(TermNode&, TermNode&, _tParams&&...)>(
						wforward(f)), x, Deref(++i), wforward(args)...));
			}

			template<typename _type, typename _func, typename... _tParams>
			void
				CallBinaryAs(_func&& f, TermNode& term, _tParams&&... args)
			{
				RetainN(term, 2);

				auto i(term.begin());
				auto& x(white::Access<_type>(Deref(++i)));

				white::EmplaceCallResult(term.Value, white::invoke_nonvoid(
					white::make_expanded<void(_type&, _type&, _tParams&&...)>(wforward(f)),
					x, white::Access<_type>(Deref(++i)), wforward(args)...));
			}
			//@}

			/*!
			\brief ���ʽڵ㲢��ָ���ĳ�ʼֵΪ����������ö�Ԫ������
			\note Ϊ֧�� std::bind �ƶ����ͣ������Ϻ��������β�ͬ����֧��ʡ�Բ�����
			*/
			template<typename _type, typename _func, typename... _tParams>
			void
				CallBinaryFold(_func f, _type val, TermNode& term, _tParams&&... args)
			{
				const auto n(FetchArgumentN(term));
				auto i(term.begin());
				const auto j(white::make_transform(++i, [](TNIter it) {
					return white::Access<_type>(Deref(it));
				}));

				white::EmplaceCallResult(term.Value, std::accumulate(j, std::next(j,
					typename std::iterator_traits<decltype(j)>::difference_type(n)), val,
					white::bind1(f, std::placeholders::_2, wforward(args)...)));
			}

			/*!
			\brief ���溯��չ�����õĺ�������
			\todo ʹ�� C++1y lambda ���ʽ���档
			
			������Ϊ�����Ĵ������ĳ��������ѡ�����ĺ�������
			Ϊ�ʺ���Ϊ�����Ĵ�������֧�ֵĲ����б�����ʵ�ʴ������ƣ�
			�����б��Ժ�Ԫ����ͬ�����ı���� TermNode& ���͵Ĳ���
			*/
			//@{
			//@{
			//! \sa Forms::CallUnary
			template<typename _func>
			struct UnaryExpansion : private white::equality_comparable<UnaryExpansion<_func>>
			{
				_func Function;

				UnaryExpansion(_func f)
					: Function(f)
				{}

				friend PDefHOp(bool, == , const UnaryExpansion& x, const UnaryExpansion& y)
					ImplRet(white::examiners::equal_examiner::are_equal(x.Function,
						y.Function))

				template<typename... _tParams>
				inline void
					operator()(_tParams&&... args)
				{
					Forms::CallUnary(Function, wforward(args)...);
				}
			};


			//! \sa Forms::CallUnaryAs
			template<typename _type, typename _func>
			struct UnaryAsExpansion : private white::equality_comparable<UnaryAsExpansion<_type, _func>>
			{
				_func Function;

				UnaryAsExpansion(_func f)
					: Function(f)
				{}

				friend PDefHOp(bool, == , const UnaryAsExpansion& x,
					const UnaryAsExpansion& y)
					ImplRet(white::examiners::equal_examiner::are_equal(x.Function,
						y.Function))

				template<typename... _tParams>
				inline void
					operator()( _tParams&&... args)
				{
					Forms::CallUnaryAs<_type>(Function, wforward(args)...);
				}
			};
			//@}


			//@{
			//! \sa Forms::CallBinary
			template<typename _func>
			struct BinaryExpansion
				: private white::equality_comparable<BinaryExpansion<_func>>
			{
				_func Function;

				BinaryExpansion(_func f)
					: Function(f)
				{}

				/*!
				\brief �Ƚϴ�������ȡ�
				*/
				friend PDefHOp(bool, == , const BinaryExpansion& x, const BinaryExpansion& y)
					ImplRet(white::examiners::equal_examiner::are_equal(x.Function,
						y.Function))

				template<typename... _tParams>
				inline void
					operator()(_tParams&&... args) const
				{
					Forms::CallBinary(Function, wforward(args)...);
				}
			};


			//! \sa Forms::CallBinaryAs
			template<typename _type, typename _func>
			struct BinaryAsExpansion
				: private white::equality_comparable<BinaryAsExpansion<_type, _func>>
			{
				_func Function;

				BinaryAsExpansion(_func f)
					: Function(f)
				{}

				/*!
				\brief �Ƚϴ�������ȡ�
				*/
				friend PDefHOp(bool, == , const BinaryAsExpansion& x,
					const BinaryAsExpansion& y)
					ImplRet(white::examiners::equal_examiner::are_equal(x.Function,
						y.Function))

					template<typename... _tParams>
				inline void
					operator()(_tParams&&... args) const
				{
					Forms::CallBinaryAs<_type>(Function, wforward(args)...);
				}
			};
			//@}
			//@}

			/*!
			\brief ע��һԪ�ϸ���ֵ�����Ĵ�������
			*/
			//@{
			template<typename _func>
			void
				RegisterStrictUnary(ContextNode& node, const string& name, _func f)
			{
				RegisterStrict(node, name, UnaryExpansion<_func>(f));
			}
			template<typename _type, typename _func>
			void
				RegisterStrictUnary(ContextNode& node, const string& name, _func f)
			{
				RegisterStrict(node, name, UnaryAsExpansion<_type, _func>(f));
			}
			//@}

			/*!
			\brief ע���Ԫ�ϸ���ֵ�����Ĵ�������
			*/
			//@{
			template<typename _func>
			void
				RegisterStrictBinary(ContextNode& node, const string& name, _func f)
			{
				RegisterStrict(node, name, BinaryExpansion<_func>(f));
			}
			template<typename _type, typename _func>
			void
				RegisterStrictBinary(ContextNode& node, const string& name, _func f)
			{
				RegisterStrict(node, name, BinaryAsExpansion<_type, _func>(f));
			}
			//@}


			//! \pre ��Ӷ��ԣ���һ����ָ��������֦�ڵ㡣
			//@{
			/*
			\note ʵ��������ʽ��
			\throw InvalidSyntax �﷨����
			*/
			//@{
			/*!
			\brief ���塣
			\exception ParameterMismatch ��ƥ��ʧ�ܡ�
			\throw InvalidSyntax �󶨵�ԴΪ�ա�
			\sa BindParameter
			\sa Vau

			ʵ���޸Ļ�����������ʽ��
			ʹ�� <definiend> ָ����Ŀ�꣬�� Vau �� <formals> ��ʽ��ͬ��
			ʣ����ʽ <expressions> ָ���󶨵�Դ��
			����δָ��ֵ��
			�޶������������ʹ�� RegisterForm ע�������Ĵ�������
			*/
			//@{
			/*!
			\note ����ʣ����ʽ��һ����ֵ��

			ʣ����ʽ��Ϊ��ֵ�����ֱ�Ӱ󶨵� <definiend> ��
			�ο������ķ���
			$deflazy! <definiend> <expressions>
			*/
			WS_API void
				DefineLazy(TermNode&, ContextNode&);

			/*!
			\note ��֧�ֵݹ�󶨡�

			ʣ����ʽ��Ϊһ�����ʽ������ֵ��󶨵� <definiend> ��
			�ο������ķ���
			$def! <definiend> <expressions>
			*/
			WS_API void
				DefineWithNoRecursion(TermNode&, ContextNode&);

			/*!
			\note ֧��ֱ�ӵݹ�ͻ���ݹ�󶨡�
			\sa InvalidReference

			�������ܵݹ�󶨵����ƣ�ʣ����ʽ��Ϊһ�����ʽ������ֵ��󶨵� <definiend> ��
			ѭ�������Դ���������ƿ����׳� InvalidReference �쳣��
			$defrec! <definiend> <expressions>
			*/
			WS_API void
				DefineWithRecursion(TermNode&, ContextNode&);
			//@}

			/*!
			\brief �Ƴ����ư󶨡�
			\exception BadIdentifier ��ǿ��ʱ�Ƴ������ڵ����ơ�
			\throw InvalidSyntax ��ʶ�����Ƿ��š�
			\sa IsNPLASymbol
			\sa RemoveIdentifier

			�Ƴ����ƺ͹�����ֵ�������Ƿ��Ƴ���
			������������ʾ�Ƿ�ǿ�ơ�����ǿ�ƣ��Ƴ������ڵ������׳��쳣��
			�ο������ķ���
			$undef! <symbol>
			$undef-checked! <symbol>
			*/
			WS_API void
				Undefine(TermNode&, ContextNode&, bool);


			/*
			\pre ��Ӷ��ԣ���һ����ָ��������֦�ڵ㡣
			\note ʵ��������ʽ��
			\throw InvalidSyntax �﷨����
			*/
			//@{
			/*!
			\brief �����жϣ�������ֵ������ȡ���ʽ��
			\sa ReduceChecked

			��ֵ��һ������Ϊ��������������ʱȡ�ڶ�������򵱵�������ʱȡ�������
			������ʽ�ο��ķ���
			$if <test> <consequent> <alternate>
			$if <test> <consequent>
			*/
			WS_API ReductionStatus
				If(TermNode&, ContextNode&);


			/*!
			\exception ParameterMismatch �쳣�������� BindParameter �׳���
			\sa EvaluateIdentifier
			\sa ExtractParameters
			\sa MatchParameter
			\warning ���رհ��������ñ���������Ŀ�������������δ������Ϊ��
			\todo �Ż���������
			
			ʹ�� ExtractParameters �������б�����Ͱ󶨱�����
			Ȼ�����ýڵ��ֵΪ��ʾ �� ����������Ĵ�������
			��ʹ�� RegisterForm ע�������Ĵ�������
			�� Scheme �Ȳ�ͬ���������������λ�õ���ʽ��ת�ƣ�����Ӧ��ʱ�����и����á�
			�����ò����������еİ󶨡���������������еİ������������Ե������ڹ���
			*/
			//@{
			/*!
			\brief �� ������ֵΪһ������ǰ�����ĵ��ϸ���ֵ�ĺ�����
			
			����ľ�̬�����ɵ�ǰ��̬������ʽȷ����
			����������������Ȩ��
			*/
			//@{
			/*!
			
			��ֵ���ݷ���ֵ���������Ա��ⷵ����������ڴ氲ȫ���⡣
			�ο������ķ���
			$lambda <formals> <body>
			*/
			WS_API void
				Lambda(TermNode&, ContextNode&);

			/*!

			�ڷ���ʱ����������������á�
			�ο������ķ���
			$lambda& <formals> <body>
			*/
			WS_API void
				LambdaRef(TermNode&, ContextNode&);
			//@}


			/*!
			\note ��̬�����������Ĳ���������Ϊһ�� ystdex::ref<ContextNode> ����
			\note ��ʼ���� <eformal> ��ʾ��̬�����������Ĳ�����ӦΪһ�����Ż� #ignore ��
			\note ����Ĵ������� operator() ֧�ֱ��浱ǰ������
			\throw InvalidSyntax <eformal> ������Ҫ��
			\sa ReduceCheckedClosure
			�������л�����������ݳ�Ա���Ǳ����ƶ�����ת�ƣ�
			�Ա�����ֵ�����м���������Щ��Ա����δ������Ϊ��
			*/
			//@{
			/*!
			\brief vau ������ֵΪһ������ǰ�����ĵķ��ϸ���ֵ�ĺ�����

			����ľ�̬�����ɵ�ǰ��̬������ʽȷ����
			����������������Ȩ��
			*/
			//@{
			/*!
			��ֵ���ݷ���ֵ���������Ա��ⷵ����������ڴ氲ȫ���⡣
			�ο������ķ���
			$vau <formals> <eformal> <body>
			*/
			WS_API void
				Vau(TermNode&, ContextNode&);

			/*!

			�ڷ���ʱ����������������á�
			�ο������ķ���
			$vau& <formals> <eformal> <body>
			*/
			WS_API void
				VauRef(TermNode&, ContextNode&);
			//@}

			/*!
			\brief �������� vau ������ֵΪһ������ǰ�����ĵķ��ϸ���ֵ�ĺ�����
			\sa ResolveEnvironment

			����ľ�̬�����ɻ������� <env> ��ֵ��ָ����
			���ݻ�������������Ϊ \c shared_ptr<Environment> �� \c weak_ptr<Environment>
			�����Ƿ�������Ȩ��
			*/
			//@{
			/*!
			��ֵ���ݷ���ֵ���������Ա��ⷵ����������ڴ氲ȫ���⡣
			�ο������ķ���
			$vaue <env> <formals> <eformal> <body>
			*/
			WS_API void
				VauWithEnvironment(TermNode&, ContextNode&);

			/*!

			�ڷ���ʱ����������������á�
			�ο������ķ���
			$vaue& <env> <formals> <eformal> <body>
			*/
			WS_API void
				VauWithEnvironmentRef(TermNode&, ContextNode&);
			//@}
			//@}
			//@}

			/*!
			\brief �������������Լ���Ƴ���һ���˳���Լ������Ϊ���һ������Ĺ�Լ�����
			\return �����ԼʱΪ���һ������Ĺ�Լ״̬������Ϊ ReductionStatus::Clean ��
			\note ��ֱ��ʵ��˳����ֵ���ڶ��������У�������Ϊ�գ�����δָ��ֵ��
			\sa ReduceOrdered
			\sa RemoveHead

			�ο������ķ���
			$sequence <object>...
			*/
			WS_API ReductionStatus
				Sequence(TermNode&, ContextNode&);

			/*!
			\sa ReduceChecked
			*/
			//@{
			/*!
			\brief �߼��롣

			���ϸ���ֵ���ɸ����������ֵ������߼��룺
			����һ�����û����������ʱ������ true ����������������ֵ���
			������ȫ��ֵΪ true ʱ�������һ�������ֵ�����򷵻� false ��
			������ʽ�ο��ķ���
			$and <test1>...
			*/
			WS_API ReductionStatus
				And(TermNode&, ContextNode&);

			/*!
			\brief �߼���

			���ϸ���ֵ���ɸ����������ֵ������߼���
			����һ�����û����������ʱ������ false ����������������ֵ���
			������ȫ��ֵΪ false ʱ���� false�����򷵻ص�һ������ false �������ֵ��
			������ʽ�ο��ķ���
			$or <test1>...
			*/
			WS_API ReductionStatus
				Or(TermNode&, ContextNode&);
			//@}

			/*!
			\brief ���� UTF-8 �ַ�����ϵͳ��������� int ���͵Ľ�������ֵ�С�
			\sa usystem
			*/
			WS_API void
				CallSystem(TermNode&);


			/*!
			\brief �������������������Ե�һ��������Ϊ�������ڶ��������������µ��б�
			\return ReductionStatus::Retained ��
			\throw InvalidSyntax �ڶ������������б�
			\note NPLA �� cons �ԣ�����Ҫ�󴴽��������б�
			*/
			//@{
			/*!
			\sa LiftSubtermsToSelfSafe
			��ֵ���ݷ���ֵ���������Ա��ⷵ����������ڴ氲ȫ���⡣
			�ο������ķ���
			cons <object> <list>
			*/
			WS_API ReductionStatus
				Cons(TermNode&);

			/*!
			�ڷ���ʱ����������������á�
			�ο������ķ���
			cons& <object> <list>
			*/
			WS_API ReductionStatus
				ConsRef(TermNode&);

			/*!
			\brief �Ƚ����������ʾ��ֵ������ͬ�Ķ���
			\sa white::HoldSame
			�ο������ķ���
			eq? <object1> <object2>
			*/
			WS_API void
				Equal(TermNode&);

			/*!
			\brief �Ƚ����������ֵ��ȡ�
			\sa white::ValueObject

			�ο������ķ���
			eql? <object1> <object2>
			*/
			WS_API void
				EqualLeaf(TermNode&);

			//@{
			/*!
			\brief �Ƚ����������ֵ������ͬ�Ķ���
			\sa white::HoldSame
			�ο������ķ���
			eqr? <object1> <object2>
			*/
			WS_API void
				EqualReference(TermNode&);

			/*!
			\brief �Ƚ����������ʾ��ֵ��ȡ�
			\sa white::ValueObject
			�ο������ķ���
			eqv? <object1> <object2>
			*/
			WS_API void
				EqualValue(TermNode&);
			//@}

			/*!
			\brief ��ָ���ָ���Ļ�����ֵ��
			\note ֧�ֱ��浱ǰ������
			\sa ReduceCheckedClosure
			\sa ResolveEnvironment

			�Ա��ʽ <expression> �ͻ��� <environment> Ϊָ���Ĳ���������ֵ��
			������ ContextNode �����ñ�ʾ��
			*/
			//@{
			/*!
			��ֵ���ݷ���ֵ���������Ա��ⷵ����������ڴ氲ȫ���⡣

			�ο������ķ���
			eval <expression> <environment>
			*/
			WS_API ReductionStatus
				Eval(TermNode&, ContextNode&);

			/*!

			�ڷ���ʱ����������������á�
			�ο������ķ���
			eval& <expression> <environment>
			*/
			WS_API ReductionStatus
				EvalRef(TermNode&, ContextNode&);
			//@}
			//@}

			/*!
			\brief �����Բ���ָ���Ļ����б���Ϊ���������»�����
			\exception NPLException �쳣�������� Environment �Ĺ��캯���׳���
			\sa Environment::CheckParent
			\sa EnvironmentList
			\todo ʹ��ר�õ��쳣���͡�

			ȡ��ָ���Ĳ�����ʼ���´����ĸ�������
			�ο������ķ���
			make-environment <environment>...
			*/
			WS_API void
				MakeEnvironment(TermNode&);

			/*!
			\brief ȡ��ǰ���������á�
			ȡ�õ�����ֵ����Ϊ weak_ptr<Environment> ��
			�ο������ķ���
			() get-current-environment
			*/
			WS_API void
				GetCurrentEnvironment(TermNode&, ContextNode&);

			/*!
			\brief ��ֵ��ʶ���õ�ָ�Ƶ�ʵ�塣
			\sa EvaluateIdentifier
			�ڶ���������ʵ�ֺ�������һ�� string ���͵Ĳ��������ֵΪָ����ʵ�塣
			�����Ʋ���ʧ��ʱ�����ص�ֵΪ ValueToken::Null ��
			�ο������ķ���
			value-of <object>
			*/
			WS_API ReductionStatus
				ValueOf(TermNode&, const ContextNode&);
			//@}


			//@{
			/*!
			\brief ��װ�ϲ���ΪӦ���ӡ�
			�ο������ķ���
			wrap <combiner>
			*/
			WS_API ContextHandler
				Wrap(const ContextHandler&);

			//! \exception NPLException ���Ͳ�����Ҫ��
			//@{
			/*!
			\brief ��װ������ΪӦ���ӡ�
			�ο������ķ���
			wrap1 <operative>
			*/
			WS_API ContextHandler
				WrapOnce(const ContextHandler&);

			/*!
			\brief ���װӦ����Ϊ�ϲ��ӡ�
			�ο������ķ���
			unwrap <applicative>
			*/
			WS_API ContextHandler
				Unwrap(const ContextHandler&);
			//@}
			//@}

		} // namespace Forms;
	} // namspace v1;
} // namespace scheme;

#endif