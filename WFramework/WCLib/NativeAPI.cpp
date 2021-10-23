#include "NativeAPI.h"
#include "FFileIO.h"

#if WFL_Win32 || WCL_API_POSIXFileSystem
namespace platform_ex
{

	WF_API WB_NONNULL(2) void
		cstat(struct ::stat& st, const char* path, bool follow_link, const char* sig)
	{
		const int res(estat(st, path, follow_link));

		if (res < 0)
#if !(WFL_DS || WFL_Win32)
			WCL_Raise_SysE(, "::stat", sig);
#else
		{
			if (follow_link)
				WCL_Raise_SysE(, "::stat", sig);
			else
				WCL_Raise_SysE(, "::lstat", sig);
		}
#endif
	}
	void
		cstat(struct ::stat& st, int fd, const char* sig)
	{
		const int res(::fstat(fd, &st));

		if (res < 0)
			WCL_Raise_SysE(, "::stat", sig);
	}


	WB_NONNULL(2) int
		estat(struct ::stat& st, const char* path, bool follow_link) wnothrowv
	{
		using platform::Nonnull;

#	if WCL_DS || WFL_Win32
		wunused(follow_link);
		return ::stat(Nonnull(path), &st);
#	else
		return (follow_link ? ::stat : ::lstat)(Nonnull(path), &st);
#	endif
	}

} // namespace platform_ex;
#endif