#pragma once

#include "ComponentCollector.h"
#include "ComponentCollectorVectorAccessor.h"

#include <unordered_map>
#include <typeinfo>
#include <memory>
#include <functional>
#include <algorithm>

class CComponentCollectorManager
{
private:
	int m_EntityCount = 0;
	std::unordered_map< const std::type_info*, std::shared_ptr<CBaseComponentCollector>> m_ComponentCollectors;
public:

	template<typename T, typename... Targs>
	void newComponent(int slotID, Targs... args)
	{
		const std::type_info* typeInfo = &typeid(T);
		auto foundCollector = m_ComponentCollectors.find(typeInfo);
		if (foundCollector != m_ComponentCollectors.end())
		{
			auto& collector = foundCollector->second;
			T tmpComponent(std::forward<Targs>(args)...);
			collector->newComponent(slotID, tmpComponent);
			m_EntityCount = std::max(m_EntityCount, slotID + 1);

			//Create dummy components for all other collectors,
			for (auto coll : m_ComponentCollectors)
			{
				if (coll.second == collector)
					continue;
				if (!coll.second->hasComponent(slotID))
					coll.second->dummyComponent(slotID);

			}
		}
		return;
	}

	//slow
	template<typename T>
	bool hasCollector()
	{
		const std::type_info* typeInfo = &typeid(T);
		auto foundCollector = m_ComponentCollectors.find(typeInfo);
		if (foundCollector != m_ComponentCollectors.end())
		{
			return true;
		}
		return false;
	}

	//slow
	template<typename T>
	T* getComponent(int entityID) const
	{
		const std::type_info* typeInfo = &typeid(T);
		auto foundCollector = m_ComponentCollectors.find(typeInfo);
		if (foundCollector != m_ComponentCollectors.end())
		{
			auto& collector = foundCollector->second;
			return static_cast<T*>(collector->getComponent(entityID));
		}
		return nullptr;
	}

	//slow
	template<typename T>
	std::shared_ptr<CComponentCollector<T>> newCollector()
	{
		const std::type_info* typeInfo = &typeid(T);
		std::shared_ptr<CComponentCollector<T>> collector = std::make_shared<CComponentCollector<T>>();
		m_ComponentCollectors[typeInfo] = collector;
		std::cout << T::name() <<"("<<&typeid(T) << ") collector added" << std::endl;

		return collector;
	}


	//fast 
	template < typename... T >
	void for_each(std::function<void(T& ...)> call)
	{

		CComponentCollectorVectorAccessor accessor;

		//reversing iterators here seems to do the trick, otherwise call parameters are reversed
		auto initl = { m_ComponentCollectors[&typeid(T)]... };
		for (auto c = std::rbegin(initl); c != std::rend(initl); ++c)
		{
			accessor.addCollector(*c);
		}

		//translate map to vector, and access vector by integer key
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
	}

	const std::unordered_map< const std::type_info*, std::shared_ptr<CBaseComponentCollector>>& getCollectors()
	{
		return m_ComponentCollectors;
	}
};