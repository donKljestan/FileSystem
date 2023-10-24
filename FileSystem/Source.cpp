#include "UI.h"
#include <iostream>
#include <stdexcept>

int main()
{
	try
	{
		UI userInterface;
		userInterface.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
