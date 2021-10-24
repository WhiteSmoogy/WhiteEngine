#pragma once

#include <WBase/cassert.h>
#include <WBase/cstdio.h>
#include "Platform.h"
#include "WDescriptions.h"

namespace platform_ex {
#if WF_Multithread == 1
	/*!
	\brief ��־���Ժ�����
	\note Ĭ�϶��� white::wassert �������
	\warning �������Ҽ���������Ϊδ���塣Ӧֻ���ڵ�����;��
	\todo ������ Win32 �Ͻ�����Ϣ��

	�� Win32 ������ʹ��ͼ�ν��棨��Ϣ����ʾ����ʧ�ܡ�ע�ⲻ��������Ϣѭ���ڲ���
	������Բ�������������ֹ���򡣵�ѡ����ֹʱ�����ȷ��� \c SIGABRT �źš�
	���Դ˹��̵����д��󣬰������б��׳����쳣���������쳣�����������Ϊ��
	����������־��¼������� white::wassert �����յ��� std::terminate ��ֹ����
	*/
	WB_API void
		LogAssert(const char*, const char*, int, const char*) wnothrow;

#	if LB_Use_WAssert
#		undef WAssert
#		define WAssert(_expr, _msg) \
	((_expr) ? void(0) \
		: platform_ex::LogAssert(#_expr, __FILE__, __LINE__, _msg))
#	endif
#endif

#if WFL_Win32

	/*!
	\brief �����ַ�������������
	\pre ��Ӷ��ԣ��ַ��������ǿա�
	\note ��ǰֱ�ӵ��� ::OutputDebugStringA ��
	*/
	WB_API WB_NONNULL(3) void
		SendDebugString(platform::Descriptions::RecordLevel, const char*)
		wnothrowv;

#endif
}

namespace platform
{
	/*!
	\brief ���Բ����طǿղ�����
	\pre ���ԣ������ǿա�
	*/
	template<typename _type>
	inline _type&&
		Nonnull(_type&& p) wnothrowv
	{
		WAssertNonnull(p);
		return wforward(p);
	}

	/*!
	\brief ���Բ����ؿɽ����õĵ�������
	\pre ���ԣ���������ȷ�����ɽ����á�
	*/
	template<typename _type>
	inline _type&&
		FwdIter(_type&& i) wnothrowv
	{
		using white::is_undereferenceable;

		WAssert(!is_undereferenceable(i), "Invalid iterator found.");
		return wforward(i);
	}

	/*!
	\brief ���Բ������÷ǿ�ָ�롣
	\pre ʹ�� ADL ָ���� FwdIter ���ñ��ʽ��ֵ�ȼ��ڵ��� platform::FwdIter ��
	\pre ��Ӷ��ԣ�ָ��ǿա�
	*/
	template<typename _type>
	wconstfn auto
		Deref(_type&& p) -> decltype(*p)
	{
		return *FwdIter(wforward(p));
	}

} // namespace platform;