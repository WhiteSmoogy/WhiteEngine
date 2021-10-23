/*! \file Core\Resource.h
\ingroup Engine
\brief 公共资源描述。
*/
#ifndef WE_Core_Resource_H
#define WE_Core_Resource_H 1

#include <string>
#include <WBase/sutility.h>
#include <WBase/wmacro.h>

namespace platform {

	class Resource:white::noncopyable {
	public:
		virtual ~Resource();

		virtual  const std::string& GetName() const wnothrow = 0;
	};
}

#endif