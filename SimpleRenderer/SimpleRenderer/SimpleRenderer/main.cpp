#include <iostream>

#include "Application.h"

int main(/*int argc, char* argv[]*/)
{
	Application renderingApp;
	if (!renderingApp.Initialize())
	{
		std::cout << "Initialization failed." << std::endl;
		return -1;
	}
	std::cout << "Initialization success." << std::endl;

	return renderingApp.Run();
}