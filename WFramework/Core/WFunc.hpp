/*!	\file WFunc.hpp
\ingroup WFrameWork/Core
\brief �������úͷº�����װ��
*/

#ifndef WFrameWork_Core_WFunc_Hpp
#define WFrameWork_Core_WFunc_Hpp 1

#include "WFramework/Core/WShellDefinition.h"

namespace white
{
	/*!
	\brief �ݹ���������ġ�
	\note ���ڹ��������
	*/
	template<typename _tKey, typename _tValue>
	class GRecursiveCallContext
	{
	public:
		using KeyType = _tKey;
		using ValueType = _tValue;
		using CallerType = std::function<ValueType(const GRecursiveCallContext&)>;

	private:
		std::function<CallerType(const KeyType&)> build;

	public:
		template<typename _func, wimpl(typename
			= exclude_self_t<GRecursiveCallContext, _func>)>
			GRecursiveCallContext(_func f)
			: build(f)
		{}
		DefDeCopyMoveCtorAssignment(GRecursiveCallContext)

			PDefHOp(ValueType, (), const KeyType& key) const
			ImplRet(build(key)(*this))
	};


	/*!
	\brief ע�ᴦ��������ģ�壺�����������һ����������ָ�����͹��������̡�
	\note ���صļ������������͵�������Ҫ����һ�¡�
	\warning ����������
	*/
	template<class _tDerived, typename _tKey, typename _fHandler>
	struct GHandlerRegisterBase
	{
	private:
		unordered_map<_tKey, _fHandler> registered_map{};

	public:
		PDefH(bool, Contains, const _tKey& key) const
			ImplRet(registered_map.count(key) != 0)

			DeclSEntry(template<_type, _fHandler> _fHandler GetRegister() const)

			template<typename... _tParams>
		auto
			Call(const _tKey& key, _tParams&&... args)
			-> invoke_result_t<_fHandler&,_tParams...>
		{
			if (const auto f = registered_map[key])
			{
				// TODO: Do right trace.
				//	TraceDe(Notice, "Found registered handler: %s.",
				//		to_string(key).c_str());

				return f(wforward(args)...);
			}
			return invoke_result_t<_fHandler&,_tParams...>();
		}

		template<class _type>
		void
			Register(const _tKey& key)
		{
			registered_map.emplace(key, static_cast<_tDerived*>(this)
				->template GetRegister<_type, _fHandler>(key));
		}
		template<typename _tIn, class _type, class _tTuple>
		void
			Register(_tIn first, _tIn last)
		{
			WAssert(first != last && std::distance(first, last)
				== decltype(std::distance(first, last))(
					std::tuple_size<_tTuple>::value + 1), "Wrong range found.");
			Register<_type>(*first);
			++first;
			WAssert((first == last) == (std::tuple_size<_tTuple>::value == 0),
				"Wrong number of parameters found.");
			//	static_if(std::tuple_size<_tTuple>::value != 0)
			//		RegisterTail<_tIn, std::tuple_element<0, _tTuple>,
			//			typename tuple_split<_tTuple>::tail>(first, last);
			RegisterTail<_tIn>(identity<_tTuple>(), first, last);
		}
		template<class _type, class... _types>
		void
			Register(std::initializer_list<string> il)
		{
			WAssert(il.size() == sizeof...(_types)+1,
				"Wrong size of initializer list found.");
			Register<std::initializer_list<string>::const_iterator, _type,
				tuple<_types...>>(il.begin(), il.end());
		}

	private:
		template<typename _tIn>
		void
			RegisterTail(identity<tuple<>>, _tIn first, _tIn last)
		{
			WAssert(first == last, "Wrong size of initializer list found.");
			wunused(first),
				wunused(last);
		}
		template<typename _tIn, class _type, class... _types>
		void
			RegisterTail(identity<tuple<_type, _types...>>, _tIn first,
				_tIn last)
		{
			Register<_tIn, _type, tuple<_types...>>(first, last);
		}
	};

} // namespace white;


#endif