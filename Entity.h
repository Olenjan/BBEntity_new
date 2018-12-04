#pragma once

#include "CollectorManager.h"
#include <memory>


//todo: better entity counter
int globalEntityCtr = 0;
class CEntity
{
private:
	int m_EntityID = -1;
	std::shared_ptr<CComponentCollectorManager> mgr;
public:
	CEntity(const std::shared_ptr<CComponentCollectorManager>& mgr)
		: mgr(mgr)
	{
		m_EntityID = globalEntityCtr;
		globalEntityCtr++;
		//Generate Entity ID
	}

	int id()
	{
		return m_EntityID;
	}

	/**/
	template<typename T>
	T* getComponent() const
	{
		auto component = mgr->getComponent<T>(m_EntityID);
		if (component->valid)
		{
			return component;
		}
		return nullptr;
	}

	template<typename T, typename... TArgs>
	void addComponent(TArgs... args) const
	{
		if (!mgr->hasCollector<T>())
		{
			mgr->newCollector<T>();
		}

		mgr->newComponent<T>(m_EntityID, args...);
	}
};