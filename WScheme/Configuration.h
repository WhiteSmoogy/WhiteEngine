#ifndef WScheme_Configuration_H
#define WScheme_Configuration_H 1

#include "WScheme.h"

namespace scheme {
	/*!
	\brief ���ã�ʹ�� S ���ʽ�洢�ⲿ״̬��
	\warning ����������
	*/
	class WS_API Configuration
	{
	private:
		ValueNode root;

	public:
		DefDeCtor(Configuration)
			//@{
			Configuration(const ValueNode& node)
			: root(node)
		{}
		Configuration(ValueNode&& node)
			: root(std::move(node))
		{}
		//@}
		//@{
		template<typename _tParam,
			wimpl(typename = white::exclude_self_t<Configuration, _tParam>)>
			explicit
			Configuration(_tParam&& arg)
			: root(white::NoContainer, wforward(arg))
		{}
		template<typename _tParam1, typename _tParam2, typename... _tParams>
		explicit
			Configuration(_tParam1&& arg1, _tParam2&& arg2, _tParams&&... args)
			: root(white::NoContainer, wforward(arg1), wforward(arg2),
				wforward(args)...)
		{}
		//@}
		DefDeCopyMoveCtorAssignment(Configuration)

			DefGetter(const wnothrow, const ValueNode&, Root, root)

			/*!
			\brief ���ļ��������á�
			\throw GeneralEvent �ļ���Ч��
			\throw LoggedEvent �ļ����ݶ�ȡʧ�ܡ�
			*/
			WS_API friend std::istream&
			operator >> (std::istream&, Configuration&);

		DefGetter(const wnothrow, const ValueNode&, Node, root)
			/*!
			\brief ȡ���ýڵ����ֵ���á�
			*/
			DefGetter(wnothrow, ValueNode&&, NodeRRef, std::move(root))
	};

	/*!
	\brief ����������������
	\relates Configuration
	*/
	WS_API std::ostream&
		operator<<(std::ostream&, const Configuration&);
}

#endif