#include <iostream>

int	main (int argc, char **argv)
{
	if (argc > 2)
	{
		std::cerr << "Parameters are more than 2" << std::endl;
	}
	// with specific conf file
	else if (argc == 2)
	{
		/* code */
	}
	// without conf file, use default
	else if (argc == 1)
	{
		/* code */
	}


	return 0;
}

