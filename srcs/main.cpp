#include <iostream>
#include <config_parser.hpp>

int	main (int argc, char **argv)
{
	if (argc > 2)
	{
		std::cerr << "Parameters are more than 2" << std::endl;
	}
	// with specific conf file
	else if (argc == 2)
	{
		ConfigParser startingData(argv[1]);
	}
	// without conf file, use default
	else if (argc == 1)
	{
		ConfigParser startingData("default.conf");
	}


	return 0;
}

