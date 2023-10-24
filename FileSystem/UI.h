#pragma once
#include "Controller.h"
#include <string>

class UI
{
	Controller controller;

	bool execute(const std::string& line); // false when the user asked to exit
	void help() const;

public:
	void run();
};
