#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>

class Location {
	public:
		std::string path;
		std::map<std::string, std::string> directives;

		// ... Other member functions if necessary ...
};

class Server {
	public:
		int listen_port;
		std::vector<std::string> server_names;
		std::map<std::string, std::string> directives;
		std::vector<Location> locations;

		// ... Other member functions if necessary ...
};
class ConfigParser
{
	private:
		std::string	m_fileName;
		std::string	m_data;


		ConfigParser(ConfigParser const &);

		ConfigParser	&operator=(ConfigParser const &);
	public:
		std::vector<Server> servers;
		std::map<std::string, std::string> directives;

		ConfigParser();
		ConfigParser(std::string);
		~ConfigParser();

		void	setFileName(std::string const &);
};



#endif
