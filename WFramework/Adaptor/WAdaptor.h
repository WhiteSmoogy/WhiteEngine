/*!	\file WAdaptor.h
\ingroup FrameWork
\brief �ⲿ�������
*/

#ifndef FrameWork_WAdaptor_h
#define FrameWork_WAdaptor_h 1

#include<WBase/algorithm.hpp>
#include<WBase/functional.hpp>
#include<WFramework/WCLib/Debug.h>
#include<WFramework/WCLib/Mutex.h>
#include<WFramework/WCLib/FReference.h>
#include<WFramework/WCLib/FFileIO.h>// for <array>, <deque>, <forward_list>, <istream>,
//	<ostream>, <queue>, <set>, <stack>, <unordered_map>, <unordered_set>,
//	<vector>, forward_as_tuple, '*string_view', get,
//	ignore, make_pair, make_tuple, pair, tie, tuple, tuple_cat, size, uopen,
//	'uf*', 'up*', etc;
#include <WFramework/WCLib/FileSystem.h>
#include <WFramework/WCLib/MemoryMapping.h>
#include <WFramework/WCLib/Logger.h>

namespace white {
	using platform::deque;
	using platform::list;
	using platform::vector;

	using platform::map;
	using platform::unordered_map;
	using platform::multimap;
	using platform::multiset;
	using platform::set;

	using platform::unordered_map;
	using platform::unordered_multimap;
	using platform::unordered_multiset;
	using platform::unordered_set;

	using platform::stack;
	using platform::priority_queue;
	using platform::queue;

	using platform::forward_as_tuple;
	using platform::get;
	using platform::ignore;
	using platform::make_pair;
	using platform::make_tuple;
	using platform::pair;
	using platform::tie;
	using platform::tuple;
	using platform::tuple_cat;

	using platform::basic_string;
	using platform::string;
	using platform::wstring;
	using platform::sfmt;
	using platform::vsfmt;

	using platform::to_string;
	using platform::to_wstring;

	using platform::basic_string_view;
	using platform::string_view;
	using platform::u16string_view;
	using platform::u8string_view;
	using platform::wstring_view;

	using platform::bad_weak_ptr;
	using platform::const_pointer_cast;
	using platform::dynamic_pointer_cast;
	using platform::enable_shared_from_this;
	using platform::get_deleter;

	using platform::make_observer;
	using platform::make_shared;
	using platform::make_unique;
	using platform::make_unique_default_init;
	using platform::get_raw;
	using platform::observer_ptr;
	using platform::owner_less;
	using platform::reset;

	using platform::share_copy;
	using platform::share_forward;
	using platform::share_move;

	using platform::share_raw;
	using platform::shared_ptr;
	using platform::static_pointer_cast;
	using platform::unique_raw;
	using platform::unique_ptr;
	using platform::unique_ptr_from;
	using platform::weak_ptr;

	using platform::lref;

	//@{
	using platform::nptr;
	using platform::tidy_ptr;
	using platform::pointer_iterator;
	//@}

	/*!
	\brief ����ɾ������ʹ���߳�ģ�Ͷ�Ӧ�Ļ�����������
	*/
	using platform::Threading::unlock_delete;


	/*!
	\brief ����������
	*/
	using namespace platform::Concurrency;
	/*!
	\brief ƽ̨����������
	*/
	using namespace platform::Descriptions;

	/*!
	\brief ����ʵ�����̡�
	*/
	//@{
	using platform::SystemOption;
	using platform::FetchLimit;
	using platform::usystem;
	//@}

	/*!
	\brief ��־��
	*/
	//@{
	using platform::Echo;
	using platform::Logger;
	using platform::FetchCommonLogger;
	//@}

	/*!
	\brief ���Բ�����
	\pre ����Բ�����ʹ�� ADL ����ͬ����������Ҫ�����յı��ʽ����͵�������������ȼۡ�
	*/
	using platform::Nonnull;

	using platform::FwdIter;
	using platform::Deref;

	/*!
	\brief �ļ��������̡�
	*/
	//@{
	using platform::uaccess;
	using platform::uopen;
	using platform::ufopen;
	using platform::ufexists;
	using platform::uchdir;
	using platform::umkdir;
	using platform::urmdir;
	using platform::uunlink;
	using platform::uremove;



	using platform::basic_filebuf;
	using platform::filebuf;
	using platform::wfilebuf;
	using platform::basic_ifstream;
	using platform::basic_ofstream;
	using platform::basic_fstream;
	using platform::ifstream;
	using platform::ofstream;
	using platform::fstream;
	using platform::wifstream;
	using platform::wofstream;
	using platform::wfstream;
	//@}

	using platform::FetchCurrentWorkingDirectory;
	using platform::MappedFile;

	//ϵͳ��������
	using platform::terminate;

	namespace IO
	{
		/*!
		\brief �ļ����ʺ��ļ�ϵͳ���̡�
		*/
		//@{
		using platform::MakePathString;
		using platform::mode_t;
		using platform::FileDescriptor;
		using platform::UniqueFile;
		using platform::DefaultPMode;
		using platform::omode_conv;
		using platform::omode_convb;
		using platform::GetFileAccessTimeOf;
		using platform::GetFileModificationTimeOf;
		using platform::GetFileModificationAndAccessTimeOf;
		using platform::FetchNumberOfLinks;
		using platform::EnsureUniqueFile;
		//@{
		using platform::HaveSameContents;
		using platform::IsNodeShared;

		using platform::NodeCategory;

		using platform::IsDirectory;

		using platform::CreateHardLink;
		using platform::CreateSymbolicLink;
		//@}
		using platform::ReadLink;
		using platform::IterateLink;
		//@{
		using platform::DirectorySession;
		using platform::HDirectory;
		//@}
		using platform::FetchSeparator;
		using platform::IsSeparator;
		using platform::IsAbsolute;
		using platform::FetchRootNameLength;
		using platform::TrimTrailingSeperator;
		//@}
		using NativePathView = basic_string_view<HDirectory::NativeChar>;

	} // namespace IO;
}
#endif