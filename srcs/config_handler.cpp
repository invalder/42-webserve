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
	std::ifstream fileIn = this->_getFileStream( fileName );

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

std::ifstream ConfigHandler::_getFileStream( std::string fileName )
{
	// get extension of file from the last dot
	std::string ext = fileName.substr(fileName.find_last_of(".") + 1);

	// check if extension is .conf
	if (ext.compare("conf"))
	{
		// if not, rasie exception
		throw IncorrectExtensionException();
	}

	// open file
	std::ifstream fileIn(fileName.c_str(), std::ifstream::in);
	if (!fileIn.is_open())
	{
		throw FileOpenException();
	}

	return fileIn;
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
	while (std::getline(file, line))
	{
		// skip empty line or comment line
		if (line.empty() or line[0] == '#')
			continue;

		// when find server block, read server block
		if (line.find("server") != std::string::npos)
		{
			// loop to run until find closing curly bracket of server block

			// if find port, get port number

			// when find closing curly bracket of server block, store server block data to _configMap with key of server port and value of server config

		}
	}
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