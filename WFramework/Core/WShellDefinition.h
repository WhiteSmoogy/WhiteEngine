/*!	\file WShellDefinition.h
\ingroup FrameWork/Core
\brief �궨�������������
*/

#ifndef WFrameWork_Core_WShellDefinition_h
#define WFrameWork_Core_WShellDefinition_h 1

#include <WFramework/Adaptor/WAdaptor.h>

namespace white {

	/*!	\defgroup reset Reset Pointers
	\brief ��ȫɾ��ָ�����õľ��ָ��Ķ���
	\post ָ�����õľ��ֵ���� nullptr ��
	*/
	//@{
	template<typename _type>
	inline bool
		reset(_type*& p) wnothrow
	{
		bool b(p);

		ldelete(p);
		p = {};
		return b;
	}
	//@}

	/*!
	\brief �Ƚϣ� shared_ptr ���ڽ�ָ�����͵���ȹ�ϵ��
	*/
	template<typename _type>
	inline bool
		operator==(const shared_ptr<_type>& sp, _type* p)
	{
		return sp.get() == p;
	}
	/*!
	\brief �Ƚϣ� weak_ptr ��ȹ�ϵ��
	\note ע��� shared_ptr �Ƚ��йܶ���ָ������岻ͬ��
	*/
	template<typename _type1, typename _type2>
	inline bool
		operator==(const weak_ptr<_type1>& x, const weak_ptr<_type2>& y)
	{
		return !x.owner_before(y) && !y.owner_before(x);
	}

	/*!
	\brief �Ƚϣ� shared_ptr ��ֵ���ڽ�ָ�����͵Ĳ��ȹ�ϵ��
	*/
	template<typename _type>
	inline bool
		operator!=(const shared_ptr<_type>& sp, _type* p)
	{
		return sp.get() != p;
	}
	/*!
	\brief �Ƚϣ� weak_ptr ���ȹ�ϵ��
	\note ע��� shared_ptr �Ƚ��йܶ���ָ������岻ͬ��
	*/
	template<typename _type1, typename _type2>
	inline bool
		operator!=(const weak_ptr<_type1>& x, const weak_ptr<_type2>& y)
	{
		return !(x == y);
	}

	namespace  alter{
		/*!
		\brief ��ռ����Ȩ������ָ�룺ʹ���߳�ģ�Ͷ�Ӧ�Ļ����������Լ� unique_ptr ��
		\sa white::threading::locked_ptr
		*/
		template<typename _type, class _tMutex = typename unlock_delete<>::mutex_type,
			class _tLock = typename unlock_delete<_tMutex>::lock_type>
			using locked_ptr = unique_ptr<_type, unlock_delete<_tMutex, _tLock>>;
	
	}

	namespace Shells
	{
		class Shell;
	} // namespace Shells;

	namespace Text
	{
		class String;
	} // namespace Text;


	class Application;

	using Shells::Shell;

	using Text::String;
}

#endif