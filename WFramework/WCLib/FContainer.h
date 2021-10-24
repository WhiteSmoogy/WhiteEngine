/*!	\file FContainer.h
\ingroup Framework
\brief ����������������������
*/

#ifndef FrameWork_Container_h
#define FrameWork_Container_h 1

#include <WFramework/WCLib/FCommon.h>
#include <WBase/string.hpp>
#include <string_view>
#include <WBase/array.hpp>
#include <WBase/cformat.h>
#include <deque>
#include <forward_list>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>

namespace platform
{
	inline namespace containers
	{
		using std::forward_as_tuple;
		using std::get;
		using std::ignore;
		using std::make_pair;
		using std::make_tuple;
		using std::pair;
		using std::tie;
		using std::tuple;
		using std::tuple_cat;

		using std::begin;
		using std::end;

		using std::array;
		using std::deque;
		using std::forward_list;
		using std::list;
		using std::vector;

		using std::map;
		using std::multimap;
		using std::multiset;
		using std::set;

		using std::unordered_map;
		using std::unordered_multimap;
		using std::unordered_multiset;
		using std::unordered_set;

		using std::stack;
		using std::priority_queue;
		using std::queue;

		/*!
		\brief ���� ISO C++03 std::basic_string ������֤���� ISO C++11 ��ʵ�֡�
		\note �ٶ�Ĭ�Ϲ��첻�׳��쳣��
		*/
		template<typename _tChar, typename _tTraits = std::char_traits<_tChar>,
			class _tAlloc = std::allocator<_tChar>>
			using basic_string = std::basic_string<_tChar, _tTraits, _tAlloc>;
		// using versa_string = __gnu_cxx::__versa_string<_tChar>;

		using string = basic_string<char>;
		using u16string = basic_string<char16_t>;
		using u32string = basic_string<char32_t>;
		//@{
		using wstring = basic_string<wchar_t>;

		//@{

		using std::basic_string_view;
		using std::string_view;
		using std::u8string_view;
		using std::u16string_view;
		using std::wstring_view;
		//@}
		//@{
		//using white::basic_tstring_view;
		//using white::tstring_view;
		//using white::u16tstring_view;
		//using white::wtstring_view;
		//@}

		using white::sfmt;
		using white::vsfmt;
		//@}

		using white::size;
		using std::to_string;
		using white::to_string;
		using std::to_wstring;

	} // inline namespace containers;

} // namespace platform;

namespace platform_ex
{
	using namespace platform::containers;
} // namespace platform_ex;



#endif
