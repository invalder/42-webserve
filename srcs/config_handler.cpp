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
ConfigHandler::ConfigHandler( std::string fileName )
{
	// get infile stream from file name
	// std::ifstream fileIn = this->_getFileStream( fileName );
	std::ifstream fileIn(fileName.c_str(), std::ifstream::in);

	// read file and construct to config data map
	this->_initializedConfigDataMap( fileIn );
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

// std::ifstream ConfigHandler::_getFileStream( std::string fileName )
// {
// 	// get extension of file from the last dot
// 	std::string ext = fileName.substr(fileName.find_last_of(".") + 1);

// 	// check if extension is .conf
// 	if (ext.compare("conf"))
// 	{
// 		// if not, rasie exception
// 		throw IncorrectExtensionException();
// 	}

// 	// open file
// 	std::ifstream fileIn(fileName.c_str(), std::ifstream::in);
// 	if (!fileIn.is_open())
// 	{
// 		throw FileOpenException();
// 	}

// 	return fileIn;
// }

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
		// if (line.empty() or line[0] == '#')
		// 	continue;
		// if (line.find("http {") != std::string::npos)
		// {
		// 	insideHttp = true;
		// 	continue;
		// }

		// if (insideHttp && line.find("server {") != std::string::npos)
		// {
		// 	insideServer = true;
		// 	currentServer = Server();
		// 	continue;
		// }

		// if (insideServer && line.find("}") != std::string::npos)
		// {
		// 	insideServer = false;
		// 	_parseHTTPConfig.servers.push_back(currentServer);
		// 	continue;
		// }
		// // skip empty line or comment line
		// if (line.empty() or line[0] == '#')
		// 	continue;

		// // when find server block, read server block
		// if (line.find("server") != std::string::npos)
		// {
		// 	// loop to run until find closing curly bracket of server block

		// 	// if find port, get port number

		// 	// when find closing curly bracket of server block, store server block data to _configMap with key of server port and value of server config

		// }

	}
}

HTTPConfig ConfigHandler::_parseHTTPConfig(const std::string& filename)
{
	std::ifstream file(filename.c_str());
	std::string line;
	bool insideHttp = false;
	bool insideServer = false;
	Server currentServer;
	class HTTPConfig httpConfig;

	while (getline(file, line)) {
		if (line.find("http {") != std::string::npos) {
			insideHttp = true;
			continue;
		}

		if (insideHttp && line.find("server {") != std::string::npos) {
			insideServer = true;
			currentServer = Server();
			continue;
		}

		if (insideServer && line.find("}") != std::string::npos) {
			insideServer = false;
			httpConfig.servers.push_back(currentServer);
			continue;
		}

		if (insideHttp && !insideServer) {
			// Parse general http directives, for simplicity, let's say each directive is on its own line.
			size_t spacePos = line.find(" ");
			if (spacePos != std::string::npos) {
				std::string key = line.substr(0, spacePos);
				std::string value = line.substr(spacePos+1, line.find(";") - spacePos - 1);
				httpConfig.directives[key] = value;
			}
			continue;
		}

		if (insideServer) {
			// Parse server directives similarly. For simplicity, we won't handle location blocks here.
			size_t spacePos = line.find(" ");
			if (spacePos != std::string::npos) {
				std::string key = line.substr(0, spacePos);
				std::string value = line.substr(spacePos+1, line.find(";") - spacePos - 1);
				currentServer.directives[key] = value;
			}
			continue;
		}

		if (line.find("}") != std::string::npos) {
			insideHttp = false;
		}
	}

	return httpConfig;
}

void ConfigHandler::printData()
{
	std::cout << _data << std::endl;
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
