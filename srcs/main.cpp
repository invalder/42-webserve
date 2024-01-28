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

			configHandler.bindAndSetSocketOptions();
			configHandler.run();

			return (EXIT_SUCCESS);
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
		}
	} else {
		std::cerr << "Parameters are more than 2" << std::endl;
		return (EXIT_FAILURE);
	}

	return 0;
}
