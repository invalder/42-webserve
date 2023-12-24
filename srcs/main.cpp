#include <iostream>
#include "config_handler.hpp"

int	main (int argc, char **argv)
{
	if (argc <= 2)
	{
		std::string fileName = argv[1] ? argv[1] : "webserve.conf";

		// Construct configHandler with file name
		try {
			ConfigHandler configHandler = ConfigHandler( fileName );

			configHandler.testPrintAll();

			configHandler.bindAndSetSocketOptions();
			configHandler.execute();
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
		}
		// ConfigHandler configHandler = ConfigHandler( fileName );
 
		// configHandler.printData();
		// configHandler.printServerDirectives();

		// configHandler.printHTTPDirectives();

		// configHandler.bindAndSetSocketOptions();
		// configHandler.execute();
	} else {
		std::cerr << "Parameters are more than 2" << std::endl;

	}

	// TODO: handle SIGTERM, SIGINT, SIGQUIT
	// when signal is received, close all sockets and exit

	return 0;
}
