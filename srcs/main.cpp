#include <iostream>
#include "config_handler.hpp"

int	main (int argc, char **argv)
{
	if (argc <= 2)
	{
		std::string fileName = argv[1] ? argv[1] : "webserve.conf";

		// Construct configHandler with file name
		ConfigHandler configHandler = ConfigHandler( fileName );
		// configHandler.printData();
		configHandler.printServerDirectives();

		configHandler.printHTTPDirectives();
	} else {
		std::cerr << "Parameters are more than 2" << std::endl;
	}

	return 0;
}
