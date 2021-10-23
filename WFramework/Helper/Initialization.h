#pragma once

#include "WFramework/Core/ValueNode.h"

namespace Test {

	/*!
	\brief 取值类型根节点。
	\pre 断言：已初始化。
	*/
	white::ValueNode&
		FetchRoot() wnothrow;

	/*!
	\brief 载入 WSLV1 配置文件。
	\param show_info 是否在标准输出中显示信息。
	\pre 间接断言：指针参数非空。
	\return 读取的配置。
	\note 预设行为、配置文件和配置项参考 Documentation::YSLib 。
	*/
	WB_NONNULL(1, 2) white::ValueNode
		LoadWSLV1File(const char* disp, const char* path, white::ValueNode(*creator)(),
			bool show_info = {});

	/*!
	\brief 载入默认配置。
	\return 读取的配置。
	\sa LoadWSLA1File
	*/
	white::ValueNode LoadConfiguration(bool = {});
}
