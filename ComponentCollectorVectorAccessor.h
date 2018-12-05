#pragma once

#include <vector>
#include <deque>
#include "ComponentCollector.h"

//! Accessing map<type, collector> is slow, preprocess collectors to vector in order of types
class CComponentCollectorVectorAccessor
{
private:
	std::vector<std::shared_ptr<CBaseComponentCollector>> m_ComponentCollectors;
	int m_CurrentAccessedCollector = 0;
public:

	void addCollector(const std::shared_ptr<CBaseComponentCollector>& collector)
	{
		m_ComponentCollectors.push_back(collector);
	}

	void reset()
	{
		m_CurrentAccessedCollector = 0;
	}

	template <typename T>
	bool isValid(int entityID)
	{
		if (m_CurrentAccessedCollector > m_ComponentCollectors.size() - 1)
		{
			return false;
		}
		auto& ref = m_ComponentCollectors.at(m_CurrentAccessedCollector)->getComponent(entityID);
		m_CurrentAccessedCollector++;

		return ref.valid;
	}

	template <typename T>
	CBaseComponent& getRaw(int id)
	{
		auto& ref = m_ComponentCollectors.at(m_CurrentAccessedCollector)->getComponent(id);
		m_CurrentAccessedCollector++;
		return ref;
	}

	template <typename T>
	T& get(int id)
	{
		auto& ref = m_ComponentCollectors.at(m_CurrentAccessedCollector)->getComponent(id);
		m_CurrentAccessedCollector++;

		auto& castRef = static_cast<T&>(ref);
		return castRef;
	}

	int collectorCount()
	{
		return m_ComponentCollectors.size();
	}
};