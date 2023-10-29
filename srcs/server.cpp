#include "../includes/server.hpp"

// constructor
Server::Server( std::string serverBlock )
{
	// get server name
	this->_getServerName( serverBlock );

	// get listen port
	this->_getListen( serverBlock );

	// get root
	this->_getRoot( serverBlock );

	// get location
	this->_getLocation( serverBlock );
}

// private methods
void Server::_getServerName( std::string serverBlock )
{
	// get server name
	std::string serverName = serverBlock.substr( serverBlock.find("server_name") + 12 );
	serverName = serverName.substr( 0, serverName.find(";") );

	// return server name
	this->_serverName = serverName;
}

void Server::_getListen( std::string serverBlock )
{
	// get listen port
	std::string listen = serverBlock.substr( serverBlock.find("listen") + 7 );
	listen = listen.substr( 0, listen.find(";") );

	// return listen port
	this->_listen = std::stoi( listen );
}

void Server::_getRoot( std::string serverBlock )
{
	// get root
	std::string root = serverBlock.substr( serverBlock.find("root") + 5 );
	root = root.substr( 0, root.find(";") );

	// return root
	this->_root = root;
}

void Server::_getLocation( std::string serverBlock )
{
	// find location block
	std::string locationBlock = serverBlock.substr( serverBlock.find("location") + 9 );

	// separate location block by newline
	std::vector<std::string> locationBlockVector;
	std::string delimiter = "\n";
	size_t pos = 0;
	std::string token;

	while ((pos = locationBlock.find(delimiter)) != std::string::npos) {
		token = locationBlock.substr(0, pos);
		locationBlockVector.push_back(token);
		locationBlock.erase(0, pos + delimiter.length());
	}

	// loop through location block vector
	for (std::vector<std::string>::iterator it = locationBlockVector.begin(); it != locationBlockVector.end(); ++it)
	{
		// get location path
		std::string locationPath = *it;
		locationPath = locationPath.substr( 0, locationPath.find("{") );

		// get location root
		std::string locationRoot = *it;
		locationRoot = locationRoot.substr( locationRoot.find("root") + 5 );
		locationRoot = locationRoot.substr( 0, locationRoot.find(";") );

		// store location path and location root to location map
		std::cout << "Path " << locationPath << std::endl;
		std::cout << "Location " << locationRoot << std::endl << std::endl;
		this->_location[locationPath] = locationRoot;
	}
}