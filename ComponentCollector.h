#pragma once

#include <vector>

#include "BaseComponent.h"

class CBaseComponentCollector
{
private:
public:
	const std::type_info* componentType;
	virtual void dummyComponent(int slotID) = 0;
	virtual void newComponent(int slotID, CBaseComponent& component) = 0;
	virtual std::string componentName() const = 0;
	virtual CBaseComponent& getComponent(int entityID) = 0;
	virtual bool hasComponent(int entityID) = 0;
};

//Resize invalidated pointers to components
// When resizing, intermediary container required that is not invalidated
template<typename T>
class CComponentCollector : public CBaseComponentCollector
{
private:
	int m_ComponentSizeStep = 50;
	//Validity check for each component is required.
	std::vector<T> m_Components;
public:

	CComponentCollector()
	{
		m_Components.reserve(m_ComponentSizeStep);
	}

	virtual void dummyComponent(int slotID)
	{
		while (m_Components.capacity() <= slotID)
		{
			m_Components.reserve(m_Components.size() + m_ComponentSizeStep);
		}
		auto insertPosition = m_Components.begin() + slotID;
		auto it = m_Components.insert(insertPosition, T());
	}
	//input component must be released after new pointer is returned
	//new pointer is not same as old, data is copied
	virtual void newComponent(int slotID, CBaseComponent& component)
	{
		//resize by step on overflow
		while (m_Components.capacity() <= slotID)
		{
			m_Components.resize(m_Components.size() + m_ComponentSizeStep);
		}

		auto insertPosition = m_Components.begin() + slotID;
		auto it = m_Components.insert(insertPosition, std::move(static_cast<T&>(component)));
		it->valid = true;
		it->ctr = slotID;
	}

	virtual std::string componentName() const
	{
		return T::name();
	}

	CBaseComponent& getComponent(int entityID)
	{
		auto& ret = m_Components.at(entityID);
		return static_cast<CBaseComponent&>(ret);
	}

	virtual bool hasComponent(int entityID)
	{
		if (m_Components.size() <= entityID)
		{
			return false;
		}
		return true;
		//return getComponent(entityID).valid;
	}
};