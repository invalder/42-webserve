#ifndef CONFIG_HANDLER_HPP
#define CONFIG_HANDLER_HPP

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <exception>

class Location {
	public:
		std::string path;
		std::map<std::string, std::string> directives;

		// Location();
		// Location(const Location &);
		// Location &operator=(const Location &);
		// ~Location();
};

class Server {
	public:
		int listen_port;
		std::vector<std::string> server_names;
		std::map<std::string, std::string> directives;
		std::vector<Location> locations;

		// Server();
		// Server(const Server &);
		// Server &operator=(const Server &);
		// ~Server();
};

class HTTPConfig {
	public:
		std::map<std::string, std::string> directives;
		std::vector<Server *> servers;

		// HTTPConfig();
		// HTTPConfig(const HTTPConfig &);
		// HTTPConfig &operator=(const HTTPConfig &);
		// ~HTTPConfig();
};

class ConfigHandler
{
	private:
		std::string _data;

		/**
		 * @brief map to save config data
		 * 	- key : server port
		 * 	- value : server config
		 */
		std::map<std::string, std::string> _configMap;

		void			_initializedConfigDataMap( std::ifstream & );
		// std::ifstream	_getFileStream( std::string );
		HTTPConfig		_httpConfig;

		HTTPConfig 	_parseHTTPConfig(const std::string& filename);

	public:
		ConfigHandler();
		ConfigHandler( std::string );
		~ConfigHandler();
		ConfigHandler(const ConfigHandler &);
		ConfigHandler &operator=(const ConfigHandler &);

		// void	printData();

		void	printHTTPConfig() const;
		void	printHTTPDirectives() const;

		void	printServerConfig() const;
		void	printServerDirectives() const;

	// exceptions
	// file open exception
	class FileOpenException : public std::exception
	{
		public:
			virtual const char* what() const throw();
	};

	// incorrect extension exception
	class IncorrectExtensionException : public std::exception
	{
		public:
			virtual const char* what() const throw();
	};
};

#endif
