#ifndef CONFIG_HANDLER_HPP
#define CONFIG_HANDLER_HPP

// for easy read test
#include <iomanip>

#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <sys/wait.h>

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
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>

#include "webserve.hpp"
#include "color.hpp"

extern char **environ;

#define SVMSG "[SV-MSG] "

typedef struct s_HttpRequset
{
	std::string method;
	std::string path;
	std::string httpVersion;
	std::map<std::string, std::string> headers;
	std::string body;
	std::string requestPath;
	std::string argPath;
} t_HttpRequest;

class Location {
	public:
		std::string path;
		std::map<std::string, std::string>	directives;

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

		std::string _cwd;

		mutable std::vector<int> _boundPorts;

		/**
		 * @brief map to save config data
		 * 	- key : server port
		 * 	- value : server config
		 */
		std::map<std::string, std::string>	_globalConfig;
		std::map<std::string, std::string> _configMap;
		std::map<std::string, std::string> _locationConfigMap;
		std::map<std::string, std::string> _cgiConfigMap;

		void			_initializedConfigDataMap( std::ifstream & );
		// std::ifstream	_getFileStream( std::string );
		HTTPConfig		_httpConfig;
		HTTPConfig 	_parseHTTPConfig(const std::string& filename);
		// bool		matchPort(t_HttpRequest request, Server *matchedServer);

	public:
		ConfigHandler();
		ConfigHandler( std::string );
		~ConfigHandler();
		ConfigHandler(const ConfigHandler &);
		ConfigHandler &operator=(const ConfigHandler &);

		// void	printData();

		// void	printHTTPConfig() const;
		void	printHTTPDirectives() const;

		// void	printServerConfig() const;
		void	printServerDirectives() const;

		void	bindAndSetSocketOptions() const;

		void	run() const;
		int		execute(Location const *mLoc, std::string &response, t_HttpRequest request) const;

		// ===== ForTest Remove Bofore Push =====
		void	testPrintAll() const;

		bool	checkFileExist( std::string ) const;
		bool	checkImageFile( std::string ) const;

		static void	signalHandler( int );
		void	closePorts() const;

		int		checkLocation(std::string &response, t_HttpRequest request, Server *matchedServer) const;
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


t_HttpRequest	parseHttpRequest(std::string requestString);

// Utility

std::string		getCgiFileName(std::string method);
std::string		createHtmlResponse(int statusCode, const std::string &htmlContent);
const Location	*matchRequestToLocation(std::string requestPath, Server *server);
Server			*matchRequestToServer(const t_HttpRequest &request, const std::vector<Server *> &servers);
std::string		readHtmlFile(const std::string &filePath);
std::string		getHttpStatusString(int statusCode);
std::string		formatSize(size_t size);
void			timeoutHandler(int signum);
char * const	*createCgiEnvp( const std::map<std::string, std::string> &cgiEnv );
std::string		createFileResponse(int statusCode, const std::string &filePath);


#endif
