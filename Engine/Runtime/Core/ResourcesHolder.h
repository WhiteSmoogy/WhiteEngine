/*! \file Core\ResourcesHolder.h
\ingroup Engine
\brief 可以关联资产实例化后的资源(Resource),也提供对资源状态的查询。
*/

#ifndef WE_Core_ResourcesHolder_h
#define WE_Core_ResourcesHolder_h 1

#include <WBase/sutility.h>
#include <any>
#include <WBase/tuple.hpp>
#include "Resource.h"

#include <memory_resource>

namespace platform {
	class IResourcesHolder :white::nonmovable, white::noncopyable {
	public:
		virtual std::shared_ptr<void> FindResource(const std::any& key) = 0;

		template<typename _type,typename ... _tParams>
		std::shared_ptr<void> FindResource(const std::shared_ptr<_type> &asset, _tParams&&... args) {
			std::weak_ptr<void> base = asset;
			std::any key = std::make_tuple(base,std::forward<_tParams>(args)...);
			return FindResource(key);
		}

		template<typename _type, typename ... _tParams>
		static std::any MakeKey(const std::shared_ptr<_type> &asset, _tParams&&... args) {
			std::weak_ptr<void> base = asset;
			return { std::make_tuple(base, wforward(args)...) };
		}
	protected:
		virtual ~IResourcesHolder();
		IResourcesHolder();

		static std::pmr::synchronized_pool_resource pool_resource;
	};

	template<typename _type>
	class  ResourcesHolder :public IResourcesHolder {
		static_assert(white::is_convertible<_type&, Resource&>());

		template<typename _tAsset, typename ... _tParams>
		std::shared_ptr<_type> FindResource(const std::shared_ptr<_tAsset> &asset, _tParams&&... args) {
			return std::static_pointer_cast<_type>(IResourcesHolder::FindResource(asset, wforward(args)...));
		}
	};
}

#endif