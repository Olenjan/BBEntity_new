#pragma once

#include "ComponentCollector.h"
#include "ComponentCollectorVectorAccessor.h"

#include <unordered_map>
#include <typeinfo>
#include <memory>
#include <functional>
#include <algorithm>
#include <iterator>
#include <map>

inline static std::vector<int> getEntitiesByTypes(const std::vector<std::pair<int, std::vector<const std::type_info*>>>& registry, const std::vector<const ::type_info*>& types)
{
	std::vector<int> result;
	for (const auto& r: registry)
	{
		if (r.second == types)
		{
			result.push_back(r.first);
		}
	}
	return result;
}
/*
inline static std::vector<const std::type_info*> getTypesByEntity(const std::vector<std::pair<int, std::vector<const std::type_info*>>>& registry, int entity)
{
	for (const auto& r: registry)
	{
		if (r.first == entity)
			return r.second;
	}
	return std::vector<const std::type_info*>();
}
*/

class CComponentCollectorManager
{
private:
	int m_EntityCount = 0;
	//std::unordered_map< const std::type_info*, std::shared_ptr<CBaseComponentCollector>> m_ComponentCollectors;
	std::vector<std::shared_ptr<CBaseComponentCollector>> m_ComponentCollectors;
	//std::vector<std::pair<int, std::vector<const std::type_info*>>> m_ComponentRegistry;
 	std::map<int, std::vector<const std::type_info*>> m_EntityComponentMap;
	std::map<std::vector<const std::type_info*>, std::vector<int>> m_ComponentEntityMap;
public:

	template<typename T, typename... Targs>
	void newComponent(int slotID, Targs... args)
	{
		const std::type_info* typeInfo = &typeid(T);
		/*
		auto foundCollector = m_ComponentCollectors.find(typeInfo);
		if (foundCollector != m_ComponentCollectors.end())
		{
			
			auto& collector = foundCollector->second;
		*/
		auto foundCollector = findCollector<T>();
		if(foundCollector)
		{
			auto& collector = foundCollector;
			T tmpComponent(std::forward<Targs>(args)...);
			collector->newComponent(slotID, tmpComponent);
			m_EntityCount = std::max(m_EntityCount, slotID + 1);
			
			//Create dummy components for all other collectors,
			for (auto coll : m_ComponentCollectors)
			{
				if (coll == collector)
					continue;
				if (!coll->hasComponent(slotID))
					coll->dummyComponent(slotID);

			}
			//unoptimized
			/*
			bool found = false;
			for (auto& r : m_ComponentRegistry)
			{
				//auto foundEntityRegistry = std::find(std::begin(r.first), std::end(r.first), slotID);
				//remove from old and create new when types dont match
				if (r.first == slotID)
				{
					r.second.push_back(typeInfo);
					found = true;
					break;
				}
			}
			if (!found)
			{
				m_ComponentRegistry.push_back(std::make_pair(slotID, std::vector<const std::type_info*>{ typeInfo }));
			}
			*/



			
			//Remove from old
			{
				auto foundCE = m_ComponentEntityMap.find(m_EntityComponentMap[slotID]);
				if (foundCE != m_ComponentEntityMap.end())
				{
					auto foundE = std::find(std::begin(foundCE->second), std::end(foundCE->second), slotID);
					
					if (foundE != std::end(foundCE->second))
					{
						m_ComponentEntityMap[m_EntityComponentMap[slotID]].erase(foundE);
					}
					
				}
			}
			//Add to new
			m_EntityComponentMap[slotID].push_back(typeInfo);
			auto foundCE = m_ComponentEntityMap.find(m_EntityComponentMap[slotID]);
			if (foundCE == m_ComponentEntityMap.end())
			{
				m_ComponentEntityMap[m_EntityComponentMap[slotID]] = {slotID};
			}
			else
			{
				m_ComponentEntityMap[m_EntityComponentMap[slotID]].push_back(slotID);
			}
			int i = 0;
			
		}
		return;
	}
	template<typename T>
	std::shared_ptr<CBaseComponentCollector> findCollector()
	{
		const std::type_info* typeInfo = &typeid(T);
		for (auto& c: m_ComponentCollectors)
		{
			if (c->componentType == typeInfo)
				return c;
		}
		return nullptr;
	}

	//slow
	template<typename T>
	bool hasCollector()
	{
		//const std::type_info* typeInfo = &typeid(T);
		auto found = findCollector<T>();
		if (found)
			return true;
		/*
		auto foundCollector = m_ComponentCollectors.find(typeInfo);
		if (foundCollector != m_ComponentCollectors.end())
		{
			return true;
		}
		*/
		return false;
	}

	//slow
	template<typename T>
	T* getComponent(int entityID) const
	{
		auto found = findCollector<T>();
		if(found)
		{
			return static_cast<T*>(found->getComponent(entityID));
		}
		/*
		auto foundCollector = m_ComponentCollectors.find(typeInfo);
		if (foundCollector != m_ComponentCollectors.end())
		{
			auto& collector = foundCollector->second;
			return static_cast<T*>(collector->getComponent(entityID));
		}
		*/
		return nullptr;
	}

	//slow
	template<typename T>
	std::shared_ptr<CComponentCollector<T>> newCollector()
	{
		const std::type_info* typeInfo = &typeid(T);
		std::shared_ptr<CComponentCollector<T>> collector = std::make_shared<CComponentCollector<T>>();
		collector->componentType = typeInfo;
		m_ComponentCollectors.push_back(collector);//[typeInfo] = collector;
		std::cout << T::name() <<"("<<&typeid(T) << ") collector added" << std::endl;

		return collector;
	}


	//fast 
	template < typename... T >
	void for_each(std::function<void(T& ...)> call)
	{

		CComponentCollectorVectorAccessor accessor;

		//reversing iterators here seems to do the trick, otherwise call parameters are reversed

		//Creates component collectors that previously did not exist
		auto initl = { findCollector<T>()... };//m_ComponentCollectors.find(&typeid(T))... }; //[&typeid(T)]... };
		for (auto c = std::rbegin(initl); c != std::rend(initl); ++c)
		{
			if(*c)
				accessor.addCollector(*c);
		}

		//Tiny optimization, end when not enough collectors found
		if (accessor.collectorCount() != initl.size())
		{
			return;
		}

		std::vector<const std::type_info*> componentMask{ &typeid(T)... };

		
		//const auto& entities = getEntitiesByTypes(m_ComponentRegistry, componentMask);
		const auto& entities = m_ComponentEntityMap[componentMask];
		
		for (const auto& e: entities)
		{
			//someshow parameters are reversed ...
			call(accessor.get<T>(e)...);

			accessor.reset();
		}
		
		//Need better and faster mechanism for getting entities that have all components.
		//Checking all entities if they have components is not effective
		/*
		for (int i = 0; i < m_EntityCount; ++i)
		{
			bool valid = true;
					
			for (const auto& c : { accessor.isValid<T>(i)... })
			{
				if (!c)
				{
					valid = false;
					continue;
				}
			}
			accessor.reset();
			

			if (valid)
			{
				//someshow parameters are reversed ...
				call(accessor.get<T>(i)...);
			}
			accessor.reset();
		}
		*/
	}

	/*
	const std::unordered_map< const std::type_info*, std::shared_ptr<CBaseComponentCollector>>& getCollectors()
	{
		return m_ComponentCollectors;
	}
	*/
};