#include "config_handler.hpp"

// Helper functions to trim whitespace
static inline std::string& ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), not1(std::ptr_fun<int, int>(isspace))));
	return s;
}

static inline std::string& rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), not1(std::ptr_fun<int, int>(isspace))).base(), s.end());
	return s;
}

static inline std::string& trim(std::string& s) {
	return ltrim(rtrim(s));
}

void parseHttpDirectives(std::string line, HTTPConfig& httpConfig)
{
	// Trim any leading and trailing whitespace from the line
	trim(line);

	// Check if the line is a valid directive (must contain at least one space and end with a semicolon)
	size_t semicolonPos = line.rfind(";");
	if (semicolonPos == std::string::npos || semicolonPos == 0) {
		// Not a valid directive line
		return;
	}

	// Extract the directive (excluding the semicolon)
	std::string directive = line.substr(0, semicolonPos);
	trim(directive);

	// Find the space between the key and the value
	size_t spacePos = directive.find(" ");
	if (spacePos == std::string::npos) {
		// No space found, so it's not a key-value pair
		return;
	}

	// Extract the key and value
	std::string key = directive.substr(0, spacePos);
	std::string value = directive.substr(spacePos + 1);

	// Trim any excess whitespace from the key and value
	trim(key);
	trim(value);

	// Store the key-value pair in the directives map
	if (!key.empty() && !value.empty()) {
		std::cerr << "Adding directive " << key << " with value " << value << std::endl;
		httpConfig.directives[key] = value;
	}
}

// Helper function to parse server directives
void parseServerDirectives(std::string line, Server *currentServer) {
	// Trim any leading and trailing whitespace from the line
	trim(line);
	std::cerr << "Trimmed line1: " << line << std::endl;
	// Check if the line is a valid directive (must contain at least one space and end with a semicolon)
	size_t semicolonPos = line.rfind(";");
	if (semicolonPos == std::string::npos || semicolonPos == 0) {
		// Not a valid directive line
		std::cerr << "Not Valid semicolonPos: " << std::endl;
		return;
	}

	// Extract the directive (excluding the semicolon)
	std::string directive = line.substr(0, semicolonPos);
	trim(directive);
	std::cerr << "Trimmed directive: " << directive << std::endl;

	// Find the space between the key and the value
	size_t spacePos = directive.find(": ");
	if (spacePos == std::string::npos) {
		// No space found, so it's not a key-value pair
		std::cerr << "Not Valid spacePos: " << std::endl;
		return;
	}

	// Extract the key and value
	std::string key = directive.substr(0, spacePos);
	std::string value = directive.substr(spacePos + 1);

	// Trim any excess whitespace from the key and value
	trim(key);
	trim(value);

	// Store the key-value pair in the directives map
	if (!key.empty() && !value.empty()) {
		std::cerr << "Adding directive " << key << " with value " << value << std::endl;
		currentServer->directives[key] = value;
	}
}

// default constructor
ConfigHandler::ConfigHandler()
{
}

/**
 * @brief string constructor
 * 	Steps:
 * 	- check if file extension is .conf
 * 	- if not, add .conf to the end of file name
 * 	- read file and save data to _data
 *
 * @param file file name with extension
 */
ConfigHandler::ConfigHandler( std::string fileName )
{
	// get infile stream from file name
	// std::ifstream fileIn = this->_getFileStream( fileName );
	// std::ifstream fileIn(fileName.c_str(), std::ifstream::in);

	// read file and construct to config data map
	// this->_initializedConfigDataMap( fileIn );
	this->_httpConfig = this->_parseHTTPConfig(fileName);
}

// destructor
ConfigHandler::~ConfigHandler()
{
}

// copy constructor
ConfigHandler::ConfigHandler(const ConfigHandler &other)
{
	*this = other;
}

// assignment operator
ConfigHandler &ConfigHandler::operator=(const ConfigHandler &other)
{
	if (this != &other)
	{
		this->_data = other._data;
	}
	return *this;
}

/**
 * @brief Initialize config data map from data read from file stream
 * 	Steps:
 * 	- read file stream line by line
 * 	- skip empty line and comment line
 *  - when find server block, read server block
 *  - store server block data to _configMap with key of server port and value of server config
 */
void ConfigHandler::_initializedConfigDataMap(std::ifstream &file) {
	// read file stream line by line
	std::string line;
	// bool insideHttp = false;
	// bool insideServer = false;
	// Server currentServer;

	while (std::getline(file, line))
	{

	}
}

HTTPConfig ConfigHandler::_parseHTTPConfig(const std::string& filename)
{
	// std::cerr << "Parsing " << filename << std::endl;
	std::ifstream file(filename.c_str());
	std::string line;
	bool insideHttp = false;
	bool insideServer = false;
	// class HTTPConfig httpConfig;
	Server *currentServer;

	while (getline(file, line)) {


		std::string::size_type startPos = line.find_first_not_of(" \t");
		// skip empty line
		if (startPos == std::string::npos) {
			continue;
		}

		// Skip if line starts with '#'
		if (line[startPos] == '#') {
			// std::cerr << "Found comment" << std::endl;
			continue;
		}

		if (line.find("http {") != std::string::npos) {
			// std::cerr << "Found http block" << std::endl;
			insideHttp = true;
			continue;
		}

		if (insideHttp && line.find("server {") != std::string::npos) {
			// std::cerr << "Found server block" << std::endl;
			insideServer = true;
			currentServer = new Server();
			continue;
		}

		if (insideServer && line.find("}") != std::string::npos) {
			// std::cerr << "Found closing bracket" << std::endl;
			insideServer = false;
			_httpConfig.servers.push_back(currentServer);
			continue;
		}

		if (insideHttp && !insideServer) {
			// Parse general http directives, for simplicity, let's say each directive is on its own line.
			// size_t spacePos = line.find(" ");
			// if (spacePos != std::string::npos) {
			// 	std::string key = line.substr(0, spacePos);
			// 	std::string value = line.substr(spacePos+1, line.find(";") - spacePos - 1);
			// 	_httpConfig.directives[key] = value;
			// }
			// std::cerr << "Parsing http directive" << std::endl;
			// std::cerr << line << std::endl;
			parseHttpDirectives(line, _httpConfig);
			continue;
		}

		if (insideServer) {
			// Parse server directives similarly. For simplicity, we won't handle location blocks here.
			std::cerr << "Parsing server directive" << std::endl;
			std::cerr << line << std::endl;

			parseServerDirectives(line, currentServer);
			// printServerDirectives();
			continue;
		}

		if (line.find("}") != std::string::npos) {
			insideHttp = false;
		}
	}

	return _httpConfig;
}

void ConfigHandler::printServerDirectives() const {
	// Loop over each server in the configuration
	for (std::vector<Server *>::const_iterator serverIt = _httpConfig.servers.begin(); serverIt != _httpConfig.servers.end(); ++serverIt) {
		// std::cout << "Server Name: " << serverIt->server_names << std::endl;
		std::cerr << "-------------------------------------------" << std::endl;
		// Now loop over the directives for this server
		for (std::map<std::string, std::string>::const_iterator directiveIt = (*serverIt)->directives.begin(); directiveIt != (*serverIt)->directives.end(); ++directiveIt) {
			std::cout << "Server Directive: " << directiveIt->first << " Value: " << directiveIt->second << std::endl;
		}
		std::cout << std::endl; // Add a newline for readability between servers
	}

}

void ConfigHandler::printHTTPDirectives() const {
	// Loop over each directive in the configuration
	for (std::map<std::string, std::string>::const_iterator directiveIt = _httpConfig.directives.begin(); directiveIt != _httpConfig.directives.end(); ++directiveIt) {
		std::cout << "HTTP Directive: " << directiveIt->first << " Value: " << directiveIt->second << std::endl;
	}
}

void ConfigHandler::bindAndSetSocketOptions() const {
	// Loop over each server in the configuration
	for (std::vector<Server *>::const_iterator serverIt = _httpConfig.servers.begin(); serverIt != _httpConfig.servers.end(); ++serverIt) {
		// std::cout << "Server Name: " << serverIt->server_names << std::endl;
		std::cerr << "-------------------------------------------" << std::endl;
		// Now loop over the directives for this server
		for (std::map<std::string, std::string>::const_iterator directiveIt = (*serverIt)->directives.begin(); directiveIt != (*serverIt)->directives.end(); ++directiveIt) {
			std::cout << "Server Directive: " << directiveIt->first << " Value: " << directiveIt->second << std::endl;
		}
		(*serverIt)->listener = socket(AF_INET, SOCK_STREAM, 0);
		if ((*serverIt)->listener < 0) {
			std::cerr << "Error creating socket" << std::endl;
			continue;
		}
		(*serverIt)->addr.sin_family = AF_INET;
		(*serverIt)->addr.sin_port = htons(atoi((*serverIt)->directives["listen"].c_str()));
		(*serverIt)->addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind((*serverIt)->listener, (struct sockaddr *)&(*serverIt)->addr, sizeof((*serverIt)->addr)) < 0) {
			std::cerr << "Error binding socket" << std::endl;
			continue;
		}
		if (listen((*serverIt)->listener, 1024) < 0) {
			std::cerr << "Error listening on socket" << std::endl;
			continue;
		}
		std::cout << std::endl; // Add a newline for readability between servers
	}
}

void ConfigHandler::execute() const {
	std::cout << "Executing config" << std::endl;
	while (true) {
		// Loop over each server in the configuration
		for (std::vector<Server *>::const_iterator serverIt = _httpConfig.servers.begin(); serverIt != _httpConfig.servers.end(); ++serverIt) {
			// std::cout << "Server Name: " << serverIt->server_names << std::endl;
			std::cerr << "-------------------------------------------" << std::endl;
			// Now loop over the directives for this server
			// for (std::map<std::string, std::string>::const_iterator directiveIt = (*serverIt)->directives.begin(); directiveIt != (*serverIt)->directives.end(); ++directiveIt) {
			// 	std::cout << "Server Directive: " << directiveIt->first << " Value: " << directiveIt->second << std::endl;
			// }
			// std::cout << std::endl; // Add a newline for readability between servers
		}
	}
}

// FileOpenException
const char* ConfigHandler::FileOpenException::what() const throw()
{
	return "Error: Fail to open file";
}

// IncorrectExtensionException
const char* ConfigHandler::IncorrectExtensionException::what() const throw()
{
	return "Error: Incorrect file extension";
}

Server::Server()
{
}

Server::~Server()
{
	// clear listener
	close(listener);
}
