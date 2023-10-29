#include <iostream>
#include "config_handler.hpp"
#include "server.hpp"

std::string server = "server { # php/fastcgi\n    listen       80; # using\n    server_name  domain1.com www.domain1.com; # using\n    access_log   logs/domain1.access.log  main;\n    root         html; # using\n    location ~ \\.php$ {\n      fastcgi_pass   127.0.0.1:1025;\n    }\n  }";

int	main (int argc, char **argv)
{
	(void) argc;
	(void) argv;
	std::cout << server << std::endl << std::endl;
	Server s = Server(server);
	// if (argc <= 2)
	// {
	// 	std::string fileName = argv[1] ? argv[1] : "default.conf";

	// 	// Construct configHandler with file name
	// 	ConfigHandler configHandler = ConfigHandler( fileName );
	// } else {
	// 	std::cerr << "Parameters are more than 2" << std::endl;
	// }

	return 0;
}
