#include "config_handler.hpp"


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
ConfigHandler::ConfigHandler(std::string fileName)
{
	// get current working directory

	char cwd[1024];  // You may need to adjust the buffer size accordingly
	getcwd(cwd, sizeof(cwd));

	this->_cwd = cwd;

	std::cout << "CWD: " << this->_cwd << std::endl;
	// get infile stream from file name

	// read file and construct to config data map
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


// ========================================================================================================


// Helper functions to trim whitespace
static inline std::string &ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), not1(std::ptr_fun<int, int>(isspace))));
	return s;
}

static inline std::string &rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), not1(std::ptr_fun<int, int>(isspace))).base(), s.end());
	return s;
}

static inline std::string &trim(std::string &s)
{
	return ltrim(rtrim(s));
}

static inline int splitKeyValue(std::string line, std::string &key, std::string &value)
{
	size_t semicolonPos = line.rfind(";");
	if (semicolonPos == std::string::npos || semicolonPos == 0)
	{
		// Not a valid directive line
		// std::cerr << "Not Valid semicolonPos: " << std::endl;
		return 1;
	}

	line = line.substr(0, semicolonPos);
	trim(line);

	// find tab first, if no tab --> find space
	size_t spacePos = line.find("\t");

	if (spacePos == std::string::npos)
	{
		spacePos = line.find(" ");
		if (spacePos == std::string::npos)
		{
			std::cerr << DEBUG_MSG << "Not Valid spacePos: " << std::endl;
			return 1;
		}
	}
	key = line.substr(0, spacePos);
	value = line.substr(spacePos + 1);

	size_t colonPos = key.find(":");
	if (colonPos != std::string::npos)
	{
		key = key.substr(0, colonPos);
	}

	trim(key);
	trim(value);

	return 0;
}

void parseDefaultErrorPages(std::ifstream &file, HTTPConfig &httpConfig, std::string cwd)
{
	std::string line;

	while (getline(file, line))
	{
		if (line.find("}") != std::string::npos)
		{
			break;
		}

		std::string key, value;
		if (splitKeyValue(line, key, value))
		{
			continue;
		}

		if (!key.empty() && !value.empty())
		{
			httpConfig.defaultErrorPages[key] = cwd + value;
		}
	}
	return;
}

static inline void parseAddMap(std::ifstream &file, std::map<std::string, std::string> &toAdd, std::string cwd)
{
	std::string line;

	while (getline(file, line))
	{
		if (line.find("}") != std::string::npos)
		{
			break;
		}

		std::string key, value;
		if (splitKeyValue(line, key, value))
		{
			continue;
		}

		if (!key.empty() && !value.empty())
		{
			if (key == "save_path") {
				value = cwd + value;
			}
			toAdd[key] = value;
		}
		std::cout << DEBUG_MSG << "QWERTY " << "KEY: " << key << " VALUE: " << value << std::endl;
	}
}

void parseLocationConfig(std::ifstream &file, Server *currentServer, std::string line, std::string cwd)
{
	trim(line);
	// get location path from first line
	line = line.substr(0, line.length() - 1);
	line = line.substr(9, line.length() - 10);

	Location toAddLoc;
	toAddLoc.path = line;

	// loop until }
	while (getline(file, line))
	{
		if (line.find("}") != std::string::npos)
			break;

		if (line.find("cgi") != std::string::npos && line.find("{") != std::string::npos)
		{
			// add cgi to current location
			parseAddMap(file, toAddLoc.cgi, cwd);
		}
		else if (line.find("upload") != std::string::npos && line.find("{") != std::string::npos)
		{
			// add upload to current location
			parseAddMap(file, toAddLoc.upload, cwd);
		}
		else
		{
			// add directive
			std::string key, value;
			if (splitKeyValue(line, key, value))
			{
				continue;
			}

			if (!key.empty() && !value.empty())
			{
				// remove "[", "]", and "," from value
				value.erase(std::remove(value.begin(), value.end(), '['), value.end());
				value.erase(std::remove(value.begin(), value.end(), ']'), value.end());
				value.erase(std::remove(value.begin(), value.end(), ','), value.end());

				if (key == "root") {
					value = cwd + value;
				}
				toAddLoc.directives[key] = value;
			}
		}
	}

	currentServer->locations.push_back(toAddLoc);
}

static void parseGlobalConfig(std::string line, std::map<std::string, std::string> &toAdd)
{
	std::string key, value;

	trim(line);
	if (splitKeyValue(line, key, value))
	{
		return;
	}

	toAdd[key] = value;
}

void parseHttpDirectives(std::string line, HTTPConfig &httpConfig)
{
	// Trim any leading and trailing whitespace from the line
	trim(line);

	// Check if the line is a valid directive (must contain at least one space and end with a semicolon)
	size_t semicolonPos = line.rfind(";");
	if (semicolonPos == std::string::npos || semicolonPos == 0)
	{
		// Not a valid directive line
		return;
	}

	// Extract the directive (excluding the semicolon)
	std::string directive = line.substr(0, semicolonPos);
	trim(directive);

	// Find the space between the key and the value
	size_t spacePos = directive.find("\t");
	if (spacePos == std::string::npos)
	{
		spacePos = directive.find(" ");
		if (spacePos == std::string::npos)
		{
			// No space found, so it's not a key-value pair
			return;
		}
	}

	// Extract the key and value
	std::string key = directive.substr(0, spacePos);
	std::string value = directive.substr(spacePos + 1);

	// Trim any excess whitespace from the key and value
	trim(key);
	trim(value);

	size_t colonPos = line.rfind(":");
	if (colonPos != std::string::npos)
	{
		key = key.substr(0, colonPos);
	}

	// Store the key-value pair in the directives map
	if (!key.empty() && !value.empty())
	{
		// std::cerr << "Adding directive " << key << " with value " << value << std::endl;
		httpConfig.directives[key] = value;
	}
}

// Helper function to parse server directives
void parseServerDirectives(std::string line, Server *currentServer, std::string cwd)
{
	// Trim any leading and trailing whitespace from the line
	trim(line);
	// std::cerr << "Trimmed line1: " << line << std::endl;
	// Check if the line is a valid directive (must contain at least one space and end with a semicolon)
	size_t semicolonPos = line.rfind(";");
	if (semicolonPos == std::string::npos || semicolonPos == 0)
	{
		// Not a valid directive line
		std::cerr << DEBUG_MSG << "Not Valid semicolonPos: " << std::endl;
		return;
	}

	// Extract the directive (excluding the semicolon)
	std::string directive = line.substr(0, semicolonPos);
	trim(directive);
	// std::cerr << "Trimmed directive: " << directive << std::endl;

	// Find the space between the key and the value
	size_t spacePos = directive.find(": ");
	if (spacePos == std::string::npos)
	{
		// No space found, so it's not a key-value pair
		std::cerr << DEBUG_MSG << "Not Valid spacePos: " << std::endl;
		return;
	}

	// Extract the key and value
	std::string key = directive.substr(0, spacePos);
	std::string value = directive.substr(spacePos + 1);

	// Trim any excess whitespace from the key and value
	trim(key);
	trim(value);

	// Store the key-value pair in the directives map
	if (!key.empty() && !value.empty())
	{
		if (key == "root") {
			value = cwd + value;
		}
		std::cerr << DEBUG_MSG << "Adding directive " << key << " with value " << value << std::endl;
		currentServer->directives[key] = value;
	}
}


t_HttpRequest parseHttpRequest(std::string requestString)
{
	t_HttpRequest request;
	std::istringstream requestStream(requestString);
	std::string line;
	bool headerSection = true;

	// Parse the request line
	if (std::getline(requestStream, line) && !line.empty())
	{
		std::istringstream lineStream(line);
		lineStream >> request.method >> request.path >> request.httpVersion;
	}

	// Parse headers and body
	while (std::getline(requestStream, line))
	{
		// Handle different line endings
		std::string::size_type end = line.find("\r");
		if (end != std::string::npos)
		{
			line.erase(end);
		}

		// Check for the end of the header section
		if (line.empty() && headerSection)
		{
			headerSection = false;
			continue;
		}

		if (headerSection)
		{
			std::string::size_type pos = line.find(":");
			if (pos != std::string::npos)
			{
				std::string headerName = line.substr(0, pos);
				std::string headerValue = line.substr(pos + 2); // Skip the ": "
				request.headers[headerName] = headerValue;
			}
		}
		else
		{
			// Add to body
			request.body += line;
			if (!requestStream.eof())
			{
				request.body += "\n"; // Maintain line breaks in body
			}
		}

		std::cout << BMAG << "BODY: " << request.body << RESET << std::endl;
	}

	return request;
}

/**
 * @brief Get file stream from file name
 * 	Steps:
 * 	- check if file extension is .conf
 * 	- if not, add .conf to the end of file name
 * 	- return file stream
 *
 * @param fileName file name with extension
 * @return HTTPConfig
 */
HTTPConfig ConfigHandler::_parseHTTPConfig(const std::string &filename)
{
	// std::cerr << "Parsing " << filename << std::endl;

	// Check if the file extension is .conf
	std::string extension = filename.substr(filename.find_last_of(".") + 1);
	if (extension != "conf")
	{
		std::cerr << DEBUG_MSG << "Incorrect file extension" << std::endl;
		throw IncorrectExtensionException();
	}

	std::ifstream file(filename.c_str());
	std::string line;
	bool insideHttp = false;
	bool insideServer = false;
	// class HTTPConfig httpConfig;
	Server *currentServer;

	while (getline(file, line))
	{

		std::string::size_type startPos = line.find_first_not_of(" \t");
		// skip empty line
		if (startPos == std::string::npos)
		{
			continue;
		}

		// Skip if line starts with '#'
		if (line[startPos] == '#')
		{
			// std::cerr << "Found comment" << std::endl;
			continue;
		}

		if (line.find("http {") != std::string::npos)
		{
			// std::cerr << "Found http block" << std::endl;
			insideHttp = true;
			continue;
		}

		if (insideHttp && line.find("server {") != std::string::npos)
		{
			// std::cerr << "Found server block" << std::endl;
			insideServer = true;
			currentServer = new Server();
			continue;
		}

		if (insideServer && line.find("}") != std::string::npos)
		{
			// std::cerr << "Found closing bracket" << std::endl;
			insideServer = false;
			_httpConfig.servers.push_back(currentServer);
			continue;
		}

		if (insideHttp && !insideServer)
		{
			// cx if default error pages
			if (line.find("default_error_pages {") != std::string::npos)
			{
				parseDefaultErrorPages(file, _httpConfig, this->_cwd);
			}
			// Parse http directives
			parseHttpDirectives(line, _httpConfig);
			continue;
		}

		if (insideServer)
		{
			// Parse server directives similarly. For simplicity, we won't handle location blocks here.
			// std::cerr << "Parsing server directive" << line << std::endl;
			if (currentServer && line.find("location ") != std::string::npos && line.find(" {") != std::string::npos)
			{
				parseLocationConfig(file, currentServer, line, this->_cwd);
			}
			parseServerDirectives(line, currentServer, this->_cwd);
			// printServerDirectives();
			continue;
		}

		if (!insideServer && !insideHttp)
		{
			parseGlobalConfig(line, _globalConfig);
		}

		if (line.find("}") != std::string::npos)
		{
			insideHttp = false;
		}
	}

	return _httpConfig;
}

void ConfigHandler::printServerDirectives() const
{
	// Loop over each server in the configuration
	for (std::vector<Server *>::const_iterator serverIt = _httpConfig.servers.begin(); serverIt != _httpConfig.servers.end(); ++serverIt)
	{
		// std::cerr << "Server Name: " << serverIt->server_names << std::endl;
		std::cerr << "-------------------------------------------" << std::endl;
		// Now loop over the directives for this server
		for (std::map<std::string, std::string>::const_iterator directiveIt = (*serverIt)->directives.begin(); directiveIt != (*serverIt)->directives.end(); ++directiveIt)
		{
			std::cerr << "Server Directive: " << directiveIt->first << " Value: " << directiveIt->second << std::endl;
		}
		std::cerr << std::endl; // Add a newline for readability between servers
	}
}

void ConfigHandler::printHTTPDirectives() const
{
	// Loop over each directive in the configuration
	for (std::map<std::string, std::string>::const_iterator directiveIt = _httpConfig.directives.begin(); directiveIt != _httpConfig.directives.end(); ++directiveIt)
	{
		std::cerr << DEBUG_MSG << "HTTP Directive: " << directiveIt->first << " Value: " << directiveIt->second << std::endl;
	}
}

/**
 * @brief check if file exist in path htdocs
 *
 * @param path
 * @return true if file exist
 * @return false if file not exist
 */
bool ConfigHandler::checkFileExist(std::string path) const
{
	std::ifstream file(path.c_str());
	std::cerr << DEBUG_MSG << "Check file exist: " << path << std::endl;
	std::cerr << DEBUG_MSG << "Check file exist: " << file.good() << std::endl;
	if (file.good())
	{
		return true;
	}
	else
	{
		return false;
	}
}


// std::string constructImageResponse( int statusCode, std::string path ) const {
// }

void ConfigHandler::closePorts() const
{
	for (std::vector<int>::iterator it = _boundPorts.begin(); it != _boundPorts.end(); ++it)
	{
		close(*it);
	}
}

/**
 * @brief Function to handle signal by closing all sockets and exit
 *
 * @param signal signal number
 */

// FileOpenException
const char *ConfigHandler::FileOpenException::what() const throw()
{
	return "Error: Fail to open file";
}

// IncorrectExtensionException
const char *ConfigHandler::IncorrectExtensionException::what() const throw()
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
