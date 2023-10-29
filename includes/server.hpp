#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <map>

class Server
{
	private:
		unsigned int	_listen;
		std::string		_serverName;
		std::string		_root;
		std::map <std::string, std::string>	_location;

		void	_getServerName( std::string serverBlock );
		void	_getListen( std::string serverBlock );
		void	_getRoot( std::string serverBlock );
		void	_getLocation( std::string serverBlock );


	public:
		Server( std::string serverBlock );

};

#endif