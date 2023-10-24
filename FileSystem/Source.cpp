#include "UI.h"
#include "Superblock.h"
#include "Controller.h"
#include <iostream>

int main()
{
	try
	{
		UI userInterface;
		userInterface.run();
	}
	catch (std::exception e)
	{
		std::cout << e.what();
	}

	return 0;
}