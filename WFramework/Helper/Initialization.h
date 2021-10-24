#pragma once

#include "WFramework/Core/ValueNode.h"

namespace Test {

	/*!
	\brief ȡֵ���͸��ڵ㡣
	\pre ���ԣ��ѳ�ʼ����
	*/
	white::ValueNode&
		FetchRoot() wnothrow;

	/*!
	\brief ���� WSLV1 �����ļ���
	\param show_info �Ƿ��ڱ�׼�������ʾ��Ϣ��
	\pre ��Ӷ��ԣ�ָ������ǿա�
	\return ��ȡ�����á�
	\note Ԥ����Ϊ�������ļ���������ο� Documentation::YSLib ��
	*/
	WB_NONNULL(1, 2) white::ValueNode
		LoadWSLV1File(const char* disp, const char* path, white::ValueNode(*creator)(),
			bool show_info = {});

	/*!
	\brief ����Ĭ�����á�
	\return ��ȡ�����á�
	\sa LoadWSLA1File
	*/
	white::ValueNode LoadConfiguration(bool = {});
}
