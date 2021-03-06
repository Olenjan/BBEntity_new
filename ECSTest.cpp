// ECSTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <string>
#include "CollectorManager.h"
#include "Entity.h"
#include "BaseComponent.h"

struct sData { float data; };
class CComponent_1 : public CBaseComponent
{
public:
	sData data;
	CComponent_1() = default;
	CComponent_1(float var)
	{
		data.data = var;
	}
	const inline static std::string name() { return "First"; }
};

class CComponent_2 : public CBaseComponent
{
public:
	sData data;
	CComponent_2() = default;
	CComponent_2(float var)
	{
		data.data = var;
	}
	const inline static std::string name() { return "Second"; }
};

class CComponent_3 : public CBaseComponent
{
public:
	sData data;
	CComponent_3() = default;
	CComponent_3(float var)
	{
		data.data = var;
	}
	const inline static std::string name() { return "Third"; }
};
class CComponent_4 : public CBaseComponent
{
public:
	CComponent_4() = default;
	CComponent_4(float var)
	{
		data.data = var;
	}
	sData data;
	const inline static std::string name() { return "Fourth"; }
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
		int EntityCount = 20000;
		auto start = getCurrentTimeMS();
		std::cout << "Creating components" << std::endl;
		std::vector<CEntity> entities;
		for (int i = 0; i < EntityCount; i++)
		{
			CEntity e(mgr);
			e.addComponent<CComponent_1>(0.1);
			//if (i % 2 == 0)
			e.addComponent<CComponent_2>(0.2);

			if (i % 2 == 0)
				e.addComponent<CComponent_3>(0.3);

			e.addComponent<CComponent_4>(0.4);

			entities.push_back(e);
		}
		std::cout << getCurrentTimeMS() - start << "ms created " << EntityCount << " Entities" << std::endl;
	}

	
	{
		auto start = getCurrentTimeMS();
		int i = 0;
		std::cout << "Calculating single thread:" << std::endl;
		//mgr->for_each<CComponent_1, CComponent_2, CComponent_3, CComponent_4>([&](CComponent_1& c1, CComponent_2& c2, CComponent_3& c3, CComponent_4& c4) {
		//	i++;
		//});
		
		mgr->for_each<CComponent_1, CComponent_2, CComponent_3, CComponent_4>([&](CComponent_1& c1, CComponent_2& c2, CComponent_3& c3, CComponent_4& c4) {
			i++;
		});

		std::cout << getCurrentTimeMS() - start << "ms looped " << i <<"(2)"<< std::endl;
	}

	/*
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
	*/
	/*
	for (const auto& c : mgr->getCollectors())
	{
		std::cout << c.second->componentName() <<std::endl;
	}
	*/

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
