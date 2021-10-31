#pragma once

#include "Archive.h"

namespace WhiteEngine
{
	/**
	 * Base class for serializing arbitrary data in memory.
	 */
	class MemoryArchive : public Archive
	{
	protected:
		/** Marked as protected to avoid instantiating this class directly */
		MemoryArchive()
			: Archive(), Offset(0)
		{
		}

		int64				Offset;
	};
}