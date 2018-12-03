// ECSTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Singleton.h"
#include "pch.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <future>
#include <any>


class CComponent
{
public:
	int ctr = -1;
	bool valid = false;
	const inline static std::string name() { return "Base"; }
};

struct sData1 { float data[16]; };
class CComponent_1 : public CComponent
{
public:
	sData1 data;

	const inline static std::string name() { return "First"; }
};

struct sData2{ float data[16]; };
class CComponent_2 : public CComponent
{
public:
	sData2 data;
	const inline static std::string name() { return "Second"; }
};


class CBaseComponentCollector
{
private:
public:
	virtual void dummyComponent(int slotID) = 0;
	virtual void newComponent(int slotID, CComponent* component) = 0;
	virtual std::string componentName() const = 0;
	virtual CComponent* getComponent(int entityID) const = 0;
};

//Resize invalidated pointers to components
// When resizing, intermediary container required that is not invalidated
template<typename T>
class CComponentCollector : public CBaseComponentCollector
{
private:
	int m_ComponentSizeStep = 50;
public:
	//Validity check for each component is required.
	std::vector<T> m_Components;

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
	virtual void newComponent(int slotID, CComponent* component)
	{
		//resize by step on overflow
		while (m_Components.capacity() <= slotID)
		{
			m_Components.reserve(m_Components.size() + m_ComponentSizeStep);
		}

		T* castComponent = static_cast<T*>(component);
		auto insertPosition = m_Components.begin() + slotID;
 		auto it = m_Components.insert(insertPosition, *castComponent);
		it->valid = true;
		it->ctr = slotID;
		//return &(*it);
	}
		/*
 		T* castComponent = static_cast<T*>(component);
		m_Components[slotID] = *castComponent;
		T& ref = m_Components[slotID];
		ref.ctr = slotID;
		ref.valid = true;
		return &ref;
	}
	*/

	virtual std::string componentName() const
	{
		return T::name();
	}

	virtual CComponent* getComponent(int entityID) const
	{
		if (m_Components.size() - 1 >= entityID)
		{
			return (CComponent*)(&m_Components.at(entityID));
		}
		return nullptr;		
	}
};


class CComponentCollectorManager
{
private:
	int m_EntityCount = 0;
	std::map< const std::type_info*, std::shared_ptr<CBaseComponentCollector>> m_ComponentCollectors;
public:

	template<typename T>
	void newComponent(int slotID)
	{
		const std::type_info* typeInfo = &typeid(T);
		auto foundCollector = m_ComponentCollectors.find(typeInfo);
		if (foundCollector != m_ComponentCollectors.end())
		{
			auto& collector = foundCollector->second;
			CComponent* tmpComponent = new T();//temporarily create new raw pointer for interface
			collector->newComponent(slotID, tmpComponent);
			delete tmpComponent;
			m_EntityCount = std::max(m_EntityCount, slotID+1);

			//Create dummy components for all other collectors,
			for (auto coll : m_ComponentCollectors)
			{
				auto collFound = coll.second->getComponent(slotID);
				if (!collFound)
					coll.second->dummyComponent(slotID);
			}
		}
		return;
	}

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

	template<typename T>
	T* getComponent(int entityID)
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

	template<typename T>
	std::shared_ptr<CComponentCollector<T>> newCollector()
	{
		const std::type_info* typeInfo = &typeid(T);
		std::shared_ptr<CComponentCollector<T>> collector = std::make_shared<CComponentCollector<T>>();
		m_ComponentCollectors[typeInfo] = collector;
		std::cout << "" << T::name() << " collector added" << std::endl;

		return collector;
	}

	template < typename... T >
	void for_each(std::function<void(T*...)> call)
	{
		int invalid = 0;
		for (int i = 0; i < m_EntityCount; i++)
		{
			bool valid = true;//Check if all components are valid
			for (auto i: { m_ComponentCollectors[&typeid(T)]->getComponent(i)... })
			{
				if (!i->valid)
				{
					invalid++;
					valid = false;
					continue;
				}
			}

			if (valid)
			{
				call(static_cast<T*>(m_ComponentCollectors[&typeid(T)]->getComponent(i))...);
			}
		}
		//call(getComponent<T>()...);
	}

	template < typename... T >
	void for_each_threaded(int threadCount, const std::function<void(T*...)>& call)
	{
		int opPerThread = m_EntityCount / threadCount;
		std::vector<std::future<void>> futures;
		for (int i = 0; i < m_EntityCount; i += opPerThread)
		{
			auto func = [&](int startOp) {
				for (int op = startOp; op < startOp + opPerThread; op++)
				{
					bool valid = true;//Check if all components are valid
					for (auto i : { m_ComponentCollectors[&typeid(T)]->getComponent(op)... })
					{
						if (!i->valid)
						{
							valid = false;
							continue;
						}
					}
					if (valid)
					{
						//double call, is bad
						call(static_cast<T*>(m_ComponentCollectors[&typeid(T)]->getComponent(i))...);
					}
				}
			};
			futures.push_back(std::async(func, i));
		}
		for (const auto& f : futures)
			f.wait();
		//call(getComponent<T>()...);
	}

	const std::map< const std::type_info*, std::shared_ptr<CBaseComponentCollector>>& getCollectors()
	{
		return m_ComponentCollectors;
	}
};

int globalEntityCtr = 0;
class CEntity
{
private:
	int m_EntityID = -1;
	std::shared_ptr<CComponentCollectorManager> mgr;
public:
	CEntity(const std::shared_ptr<CComponentCollectorManager>& mgr)
		:	mgr(mgr)
	{
		m_EntityID = globalEntityCtr;
		globalEntityCtr++;
		//Generate Entity ID
	}

	int id()
	{
		return m_EntityID;
	}	

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

	template<typename T>
	void addComponent() const
	{
		if (!mgr->hasCollector<T>())
		{
			mgr->newCollector<T>();
		}
		mgr->newComponent<T>(m_EntityID);
	}
};
#include <chrono>
static inline uint64_t getCurrentTimeMS()
{
	auto now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}
int main()
{
	auto mgr = std::make_shared<CComponentCollectorManager>();

	std::cout << sizeof(CComponent_1) << std::endl;

	{
		int EntityCount = 40000;
		auto start = getCurrentTimeMS();
		std::cout << "Creating components" << std::endl;
		std::vector<CEntity> entities;
		for (int i = 0; i < EntityCount; i++)
		{
			CEntity e(mgr);
			e.addComponent<CComponent_1>();
			if (i % 2 == 0)
				e.addComponent<CComponent_2>();

			entities.push_back(e);
		}
		std::cout << getCurrentTimeMS() - start << "ms created " << EntityCount << " Entities" << std::endl;
	}

	{
		auto start = getCurrentTimeMS();
		int i = 0;
		std::mutex m;
		std::cout << "Calculating multithreaded - 4 threads:" << std::endl;
		mgr->for_each_threaded<CComponent_1, CComponent_2>(4, [&](CComponent_1* c1, CComponent_2* c2) {
			std::lock_guard<std::mutex> lock(m);
			i++;
		});
		std::cout << getCurrentTimeMS() - start << "ms looped " << i << std::endl;
	}
	{
		auto start = getCurrentTimeMS();
		int i = 0;
		std::mutex m;
		std::cout << "Calculating single thread:" << std::endl;
		mgr->for_each<CComponent_1, CComponent_2>([&](CComponent_1* c1, CComponent_2* c2) {
			i++;
		});
		std::cout << getCurrentTimeMS() - start << "ms looped " << i << std::endl;
	}

	for (const auto& c : mgr->getCollectors())
	{
		std::cout << c.second->componentName() <<std::endl;
	}

    std::cout << "Hello World!\n"; 
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
