#pragma once

#include <string>

class CBaseComponent
{
public:
	int ctr = -1;
	bool valid = false;
	const inline static std::string name() { return "Base"; }
};
