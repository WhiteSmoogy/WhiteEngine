﻿/*!	\file Registry.h
\ingroup WCLib
\ingroup Win32
\brief 注册表。
*/

#ifndef Framework_MinGW32_INC_Registry_h_
#define Framework_MinGW32_INC_Registry_h_ 1

#include <WFramework/Win32/WCLib/Mingw32.h>
//	pair;

namespace platform_ex
{
	namespace Windows
	{

		/*!
		\brief 注册表键。
		\warning 不检查外部进程交互：并发访问可能导致错误的结果。
		\todo 增加和实现创建和删除值等功能接口。
		*/
		class WF_API RegistryKey
		{
		private:
			::HKEY h_key;

		public:
			/*!
			\brief 构造：使用本机键、名称、选项和访问权限。
			\pre 间接断言：字符串参数非空。
			*/
			WB_NONNULL(2)
				RegistryKey(::HKEY h_parent, const wchar_t* name, unsigned long ul_opt = 0,
					::REGSAM access = KEY_READ)
			{
				WCL_RaiseZ_Win32E(::RegOpenKeyExW(h_parent,
					platform::Nonnull(name), ul_opt, access, &h_key), "RegOpenKeyEx");
			}
			RegistryKey(RegistryKey&& key)
				: h_key(key.h_key)
			{
				key.h_key = {};
			}
			~RegistryKey()
			{
				::RegCloseKey(h_key);
			}

			DefDeMoveAssignment(RegistryKey)

				DefGetter(const wnothrow, ::HKEY, Key, h_key)
				/*!
				\brief 取指定名称和类型的值的存储表示。
				\return 成功得到的值的类型和内容。
				\note 类型为 \c REG_NONE 时表示允许任意类型的值。
				*/
				//@{
				/*!
				\brief 间接断言：第一参数非空。
				*/
				pair<unsigned long, vector<byte>>
				GetRawValue(const wchar_t*, unsigned long = REG_NONE) const;
			/*!
			\brief 间接断言：第一参数的数据指针非空。
			*/
			PDefH(pair<unsigned long WPP_Comma vector<byte>>, GetRawValue,
				wstring_view name, unsigned long type = REG_NONE) const
				ImplRet(GetRawValue(name.data(), type))
				//@}
				size_t
				GetSubKeyCount() const;
			vector<wstring>
				GetSubKeyNames() const;
			size_t
				GetValueCount() const;
			vector<wstring>
				GetValueNames() const;

			//! \throw Win32Exception 刷新失败。
			void
				Flush();
		};

		/*!
		\brief 取注册表字符串值。
		\pre 间接断言：字符串参数非空。
		\return 若成功则为指定的值，否则为空串。
		\note 字符串内容保证不以空字符结尾。
		\relates RegistryKey
		*/
		//@{
		WF_API WB_NONNULL(2) wstring
			FetchRegistryString(const RegistryKey&, const wchar_t*);
		inline WB_NONNULL(2, 3) PDefH(wstring, FetchRegistryString,
			::HKEY h_parent, const wchar_t* key_name, const wchar_t* name)
			ImplRet(FetchRegistryString(RegistryKey(h_parent, key_name), name))
			//@}

	} //namespace Windows;

} // namespace platform_ex;

#endif

