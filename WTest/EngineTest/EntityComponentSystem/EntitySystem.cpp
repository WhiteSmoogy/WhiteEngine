#include "EntitySystem.h"
#include <WBase/typeindex.h>
#include <WFramework/Adaptor/WAdaptor.h>

using namespace ecs;

namespace ecs {
	namespace details {
		using namespace white;

		template<typename _type1 = white::uint16, typename _type2 = white::uint16>
		struct SaltHandle {
			_type1 salt;
			_type2 index;

			wconstexpr SaltHandle(_type1 salt_, _type2 index_) wnoexcept
				:salt(salt_), index(index_)
			{}

			wconstexpr SaltHandle() wnoexcept
				: salt(0), index(0)
			{}


			wconstexpr bool operator==(const SaltHandle<_type1, _type2>& rhs) const wnoexcept
			{
				return salt == rhs.salt && index == rhs.index;
			}

			wconstexpr bool operator!=(const SaltHandle<_type1, _type2>& rhs) const wnoexcept
			{
				return !(*this == rhs);
			}

			// conversion to bool
			// e.g. if(id){ ..nil.. } else { ..valid or not valid.. }
			wconstexpr operator bool() const wnoexcept
			{
				return salt != 0 && index != 0;
			}
		};

		wconstexpr auto salt_default_size = 64 * 1024u;

		template<typename _type1 = white::uint16, typename _type2 = white::uint16, decltype(salt_default_size) size = salt_default_size - 3>
		class SaltHandleArray {
		public:
			using salt_type = _type1;
			using index_type = _type2;

			wconstexpr static index_type end_index = std::numeric_limits<_type2>::max();
			wconstexpr static index_type invalid_index = end_index - 1;

			static_assert(std::is_unsigned_v<_type2>);
			static_assert(size != std::numeric_limits<_type2>::max());
			static_assert(size != invalid_index);


			SaltHandleArray() {
				index_type i;
				for (i = size - 1; i > 1; --i)
				{
					buffer[i].salt = 0;
					buffer[i].next_index = i - 1;
				}
				buffer[1].salt = 0;
				buffer[1].next_index = end_index;     // end marker
				free_index = size - 1;

				// 0 is not used because it's nil
				buffer[0].salt = ~0;
				buffer[0].next_index = invalid_index;
			}

			SaltHandle<salt_type, index_type> RentDynamic() wnoexcept {
				if (free_index == end_index)
					return SaltHandle<salt_type, index_type>();

				SaltHandle<salt_type, index_type> ret{ buffer[free_index].salt,free_index };

				auto& element = buffer[free_index];

				free_index = element.next_index;

				element.next_index = invalid_index;

				return ret;
			}

			void Return(const SaltHandle<salt_type, index_type>& handle) wnoexcept {
				auto index = handle.index;
				WAssert(IsUsed(index), " Index was not used, Insert() wasn't called or Remove() called twice");

				auto& salt = buffer[index].salt;
				WAssert(handle.salt == salt, "Handle Is't Valid");
				++salt;

				buffer[index].next_index = free_index;
				free_index = index;
			}


			bool IsValid(const SaltHandle<salt_type, index_type>& handle) const wnoexcept
			{
				if (!handle)
					return false;

				if (handle.index > size)
					return false;

				return buffer[handle.index].salt == handle.salt;
			}

			bool IsUsed(index_type index) const wnoexcept {
				return buffer[index].next_index == invalid_index;
			}

		private:
			void Remove(index_type index) {
				if (free_index == index)
					free_index = buffer[index].next_index;
				else {
					auto old = free_index;
					auto it = buffer[old].next_index;

					for (;;) {
						auto next = buffer[it].next_index;

						if (it == index)
						{
							buffer[old].next_index = next;
							break;
						}

						old = it;
						it = next;
					}
				}
				buffer[index].next_index = invalid_index;
			}


			struct Element {
				salt_type salt;
				index_type next_index;
			};

			Element buffer[size];

			index_type free_index;

		};
	}

	details::SaltHandleArray<> SaltHandleArray;
	white::unordered_map<EntityId, std::unique_ptr<Entity>> EntityMap;
	white::unordered_multimap<white::type_index, std::unique_ptr<System>> SystemMap;

	white::uint16              IdToIndex(const EntityId id) { return id & 0xffff; }
	details::SaltHandle<>    IdToHandle(const EntityId id) { return details::SaltHandle<>(id >> 16, id & 0xffff); }
	EntityId            HandleToId(const details::SaltHandle<> id) { return (((white::uint32)id.salt) << 16) | ((white::uint32)id.index); }
}


white::uint32 ecs::EntitySystem::Update(const UpdateParams & params)
{
	for (auto& pSystem : SystemMap)
	{
		pSystem.second->Update(params);
	}

	return {};
}

white::observer_ptr<Entity> ecs::EntitySystem::GetEntity(EntityId id) {
	if (id == InvalidEntityId)
		return {};
	WAssert(EntityMap.count(id) != 0, "EntitySystem Don't Exist this Entity");

	return white::make_observer(EntityMap.at(id).get());
}

white::observer_ptr<Entity> ecs::EntitySystem::Add(const white::type_info& type_info, EntityId id, std::unique_ptr<Entity> pEntity)
{
	WAssert(SaltHandleArray.IsUsed(id), "SaltHandleArray Things go awry");
	WAssert(EntityMap.count(id) == 0, "EntitySystem Things go awry");
	return white::make_observer(EntityMap.emplace(id, pEntity.release()).first->second.get());
}

EntityId EntitySystem::GenerateEntityId() const wnothrow {
	auto ret = InvalidEntityId;

	ret = HandleToId(SaltHandleArray.RentDynamic());

	return ret;
}

void EntitySystem::RemoveEntity(EntityId id) wnothrow {
	if (id == InvalidEntityId)
		return;
	WAssert(EntityMap.count(id) == 1, "Remove Same Entity Many Times");
	EntityMap.erase(id);
	SaltHandleArray.Return(IdToHandle(id));
}



void ecs::EntitySystem::PostMessage(const white::Message & message)
{
	OnGotMessage(message);
}

void ecs::EntitySystem::OnGotMessage(const white::Message & message)
{
	for (auto& pSystem : SystemMap)
	{
		pSystem.second->OnGotMessage(message);
	}
}

EntitySystem & ecs::EntitySystem::Instance()
{
	static EntitySystem Instance;
	return Instance;
}

white::observer_ptr<System> ecs::EntitySystem::Add(const white::type_info& type_info, std::unique_ptr<System> pSystem)
{
	return white::make_observer(SystemMap.emplace(white::type_index(type_info), pSystem.release())->second.get());
}
