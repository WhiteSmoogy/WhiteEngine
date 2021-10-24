/*! \file Core\AssetResourceScheduler.h
\ingroup Engine
\brief �ṩ�ʲ�(Asset)�Ĺ�������Դ��ʵ����(Resource)��
*/

#ifndef WE_Core_AssetResourceScheduler_h
#define WE_Core_AssetResourceScheduler_h 1

#include <WBase/cache.hpp>
#include "../Asset/Loader.hpp"
#include "../Core/Coroutine/Task.h"
#include <WBase/sutility.h>
#include <shared_mutex>

namespace platform {

	/*! \brief ��Ҫְ���Ǹ���ַ�����TaskScheduler(TODO),Ҳ�����ӳ��ʲ�����������
	*/
	class AssetResourceScheduler :white::nonmovable, white::noncopyable {
	public:
		template<typename Loading, typename... _tParams>
		std::shared_ptr<typename Loading::AssetType> SyncLoad(_tParams&&... args) {
			std::shared_ptr<asset::IAssetLoading> key{ new Loading(wforward(args)...) };

			auto itr = asset_loaded_caches.find(key);
			if (itr != asset_loaded_caches.end()) {
				//TODO Refresh Ticks
				return std::static_pointer_cast<typename Loading::AssetType>(itr->second.loaded_asset);
			}

			std::shared_ptr<Loading> loading = std::static_pointer_cast<Loading>(key);
			std::shared_ptr<typename Loading::AssetType> ret{};
			auto coroutine = loading->Coroutine();
			for (auto yiled : coroutine)
				ret = yiled;

			AssetLoadedDesc desc;
			desc.loaded_asset = std::static_pointer_cast<void>(ret);
			desc.loaded_tick = 0;
			desc.delay_tick = 0;
			asset_loaded_caches.emplace(key, desc);
			return ret;
		}

		template<typename Loading, typename... _tParams>
		white::coroutine::Task<std::shared_ptr<typename Loading::AssetType>> AsyncLoad(_tParams&&... args)
		{
			std::shared_ptr<asset::IAssetLoading> key{ new Loading(wforward(args)...) };

			{
				std::shared_lock read_lock{ caches_mutex };
				auto itr = asset_loaded_caches.find(key);
				if (itr != asset_loaded_caches.end()) {
					co_return std::static_pointer_cast<typename Loading::AssetType>(itr->second.loaded_asset);
				}
			}

			std::shared_ptr<Loading> loading = std::static_pointer_cast<Loading>(key);
			std::shared_ptr<typename Loading::AssetType> ret{};

			ret = co_await loading->GetAwaiter();

			{
				std::unique_lock write_lock{ caches_mutex };
				AssetLoadedDesc desc;
				desc.loaded_asset = std::static_pointer_cast<void>(ret);
				desc.loaded_tick = 0;
				desc.delay_tick = 0;
				asset_loaded_caches.emplace(key, desc);
			}

			co_return ret;
		}

		template<typename _type, typename... _tParams>
		std::shared_ptr<_type> SyncSpawnResource(_tParams&&... args);

		static AssetResourceScheduler& Instance();

		//��\brief ��ȡĳ����Դ��·��,�޷�ȡ��ʱ�׳��쳣
		const asset::path& FindAssetPath(const void* pAsset);

	private:
		AssetResourceScheduler();
		~AssetResourceScheduler();

	private:
		struct AssetLoadedDesc {
			//@{
			/*!\breif loaded_tick ��delay_tick����Loaded���֮��ˢ��
			   \warning SpawnResource ����ˢ�����״̬
			*/
			white::uint32 loaded_tick;
			white::uint8 delay_tick;
			//@}
			std::shared_ptr<void> loaded_asset;
		};

		struct IAssetLoadingHash {
			std::size_t operator()(const std::shared_ptr<asset::IAssetLoading>& iasset) const wnoexcept {
				return iasset->Hash();
			}
		};

		struct IAssetLoadingEqual {
			bool operator()(const std::shared_ptr<asset::IAssetLoading>& lhs, const std::shared_ptr<asset::IAssetLoading>& rhs) const wnoexcept {
				return lhs->Hash() == rhs->Hash();
			}
		};

		white::used_list_cache<std::shared_ptr<asset::IAssetLoading>, AssetLoadedDesc, IAssetLoadingEqual, IAssetLoadingHash> asset_loaded_caches{ 8192 };

		std::shared_mutex caches_mutex;
	};
}

#endif