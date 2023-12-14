#ifndef CONFIG_HANDLER_HPP
#define CONFIG_HANDLER_HPP

// for easy read test
#include <iomanip>

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <exception>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct s_HttpRequset
{
	std::string method;
	std::string path;
	std::string httpVersion;
	std::map<std::string, std::string> headers;
	std::string body;
} t_HttpRequest;

class Location {
	public:
		std::string path;
		std::map<std::string, std::string> directives;

		std::map<std::string, std::string>	cgi;
		std::map<std::string, std::string>	upload;

		// Location();
		// Location(const Location &);
		// Location &operator=(const Location &);
		// ~Location();
};

class Server {
	public:
		int listener;
		sockaddr_in addr;

		int listen_port;
		// std::vector<std::string> server_names;
		std::map<std::string, std::string> directives;
		std::vector<Location> locations;

		Server();
		// Server(const Server &);
		// Server &operator=(const Server &);
		~Server();
};

class HTTPConfig {
	public:
		std::map<std::string, std::string> directives;
		std::map<std::string, std::string> defaultErrorPages;
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

		std::map<std::string, std::string> _locationConfigMap;

		std::map<std::string, std::string> _cgiConfigMap;

		void			_initializedConfigDataMap( std::ifstream & );
		// std::ifstream	_getFileStream( std::string );
		HTTPConfig		_httpConfig;

		HTTPConfig 	_parseHTTPConfig(const std::string& filename);

		// function to parse cgi block
		void	_parseCGIConfig(std::ifstream &file);

		// function to parse server block
		void	_parseLocationConfig(std::ifstream &file);

		// function to parse upload block
		void	_parseUploadConfig(std::ifstream &file);

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

		void	bindAndSetSocketOptions() const;

		void	execute() const;

		// ===== ForTest Remove Bofore Push =====
		void	testPrintAll() const;

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
