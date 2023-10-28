#include <iostream>
#include "config_handler.hpp"

int	main (int argc, char **argv)
{
	if (argc <= 2)
	{
		std::string fileName = argv[1] ? argv[1] : "default.conf";

		// Construct configHandler with file name
		ConfigHandler configHandler = ConfigHandler( fileName );
	} else {
		std::cerr << "Parameters are more than 2" << std::endl;
	}

	return 0;
}
