#include "config_handler.hpp"

extern char **environ;

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

	size_t colonPos = line.rfind(":");
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
void ConfigHandler::_initializedConfigDataMap(std::ifstream &file)
{
	// read file stream line by line
	std::string line;
	// bool insideHttp = false;
	// bool insideServer = false;
	// Server currentServer;

	while (std::getline(file, line))
	{
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
	}

	return request;
}

// Function to match the request to a server
Server *matchRequestToServer(const t_HttpRequest &request, const std::vector<Server *> &servers)
{
	std::string requestedHost;

	// Find the "Host" header in a const-safe manner
	std::map<std::string, std::string>::const_iterator itHost = request.headers.find("Host");
	std::cerr << "\033[1;31m" << DEBUG_MSG << "Request headers: " << itHost->first << ": " << itHost->second << "\033[0m" << std::endl;
	if (itHost != request.headers.end())
	{
		requestedHost = itHost->second;
	}
	else
	{
		// Handle case where "Host" header is not found
		// Check if server has
		return 0; // Or handle it according to your application logic
	}

	// Optionally, you can parse the port from the requested host if it's in the format "host:port"
	size_t colonPos = requestedHost.find(":");
	if (colonPos != std::string::npos)
	{
		requestedHost = requestedHost.substr(0, colonPos);
	}

	for (std::vector<Server *>::const_iterator it = servers.begin(); it != servers.end(); ++it)
	{
		std::map<std::string, std::string>::const_iterator itDirective = (*it)->directives.find("server_name");
		if (itDirective != (*it)->directives.end())
		{
			// Split the server names by spaces and match each one
			std::istringstream iss(itDirective->second);
			std::string serverName;
			while (iss >> serverName)
			{
				if (serverName == requestedHost)
				{
					// Found a matching server
					return *it;
				}
			}
		}
	}
	return 0; // No matching server found
}

const Location *matchRequestToLocation(const t_HttpRequest &request, Server *server)
{
	// Check for null server pointer
	if (server == NULL)
	{
		std::cerr << DEBUG_MSG << "Server is null" << std::endl;
		return NULL;
	}

	const std::string &requestedPath = request.path;
	std::cerr << DEBUG_MSG << "Requested path: " << requestedPath << std::endl;

	// Iterate through locations to find a match
	for (std::vector<Location>::const_iterator it = server->locations.begin(); it != server->locations.end(); ++it)
	{
		const Location &location = *it;

		// Check if the requested path starts with the location path
		if (requestedPath == location.path)
		{
			// Found a matching location
			// Return the address of the location in the vector
			// std::cerr << "requestedPath: " << requestedPath << std::endl;
			// std::cerr << "Found matching location: " << location.path << std::endl;
			return &(*it);
		}
	}

	// No matching location found
	// std::cerr << "No matching location found" << std::endl;
	return 0;
}

std::string readHtmlFile(const std::string &filePath)
{

	std::cerr << DEBUG_MSG << "Read HTML file path: " << filePath << std::endl;

	std::ifstream htmlFile(filePath.c_str());
	std::ostringstream buffer;
	buffer << htmlFile.rdbuf();

	// std::cerr << DEBUG_MSG << "Read HTML file: " << buffer.str() << std::endl;

	return buffer.str();
}

std::string getHttpStatusString(int statusCode)
{
	switch (statusCode)
	{
	case 200:
		return "OK";
	case 201:
		return "Created";
	case 202:
		return "Accepted";
	case 204:
		return "No Content";
	case 400:
		return "Bad Request";
	case 401:
		return "Unauthorized";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	case 405:
		return "Method Not Allowed";
	case 500:
		return "Internal Server Error";
	case 501:
		return "Not Implemented";
	case 502:
		return "Bad Gateway";
	case 503:
		return "Service Unavailable";
	// Add more cases for other status codes
	default:
		return "Unknown Status";
	}
}

std::string createHtmlResponse(int statusCode, const std::string &htmlContent)
{
	std::ostringstream response;
	response << "HTTP/1.1 " << statusCode << " " << getHttpStatusString(statusCode) << "\r\n"
			 << "Content-Type: text/html\r\n"
			 << "Content-Length: " << htmlContent.length() << "\r\n\r\n"
			 << htmlContent;
	return response.str();
}

std::string getFileExtension(const std::string& filePath) {
	std::string::size_type dotPos = filePath.find_last_of('.');
	if(dotPos != std::string::npos && dotPos != filePath.length() - 1) {
		return filePath.substr(dotPos + 1);
	} else {
		return "";
	}
}

std::string detectMimeType(const std::string& extension) {
	std::map<std::string, std::string> mimeTypes;
	mimeTypes["html"] = "text/html";
	mimeTypes["txt"] = "text/plain";
	mimeTypes["jpg"] = "image/jpeg";
	mimeTypes["jpeg"] = "image/jpeg";
	mimeTypes["png"] = "image/png";
	mimeTypes["pdf"] = "application/pdf";
	// Add more mappings as needed

	std::map<std::string, std::string>::const_iterator it = mimeTypes.find(extension);
	if (it != mimeTypes.end()) {
		return it->second;
	} else {
		return "application/octet-stream"; // default or unknown types
	}
}

std::string createFileResponse(int statusCode, const std::string &filePath)
{
	std::string extension = getFileExtension(filePath);
	std::string mimeType = detectMimeType(extension);

	std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
	if(!file.is_open()) {
		return "Error: Unable to open file";
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(static_cast<size_t>(size));
	if(file.read(&buffer[0], size))
	{
		std::ostringstream response;
		response << "HTTP/1.1 " << statusCode << " " << getHttpStatusString(statusCode) << "\r\n"
			<< "Content-Type: " << mimeType << "\r\n"
			<< "Content-Length: " << size << "\r\n\r\n"
			<< std::string(buffer.begin(), buffer.end());
		return response.str();
	}
	else
	{
		return "Error: Unable to read file content";
	}
}

std::string formatSize(size_t size) {
	std::ostringstream stream; // Declaring an ostringstream object
	stream << size << " bytes"; // Using the insertion operator to format the string
	return stream.str(); // Returning the formatted string
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

void ConfigHandler::bindAndSetSocketOptions() const
{
	// Loop over each server in the configuration
	for (std::vector<Server *>::const_iterator serverIt = _httpConfig.servers.begin(); serverIt != _httpConfig.servers.end(); ++serverIt)
	{
		// std::cerr << "Server Name: " << serverIt->server_names << std::endl;
		std::cerr << "-------------------------------------------" << std::endl;
		// Now loop over the directives for this server
		for (std::map<std::string, std::string>::const_iterator directiveIt = (*serverIt)->directives.begin(); directiveIt != (*serverIt)->directives.end(); ++directiveIt)
		{
			std::cerr << DEBUG_MSG << "Server Directive: " << directiveIt->first << " Value: " << directiveIt->second << std::endl;
		}
		(*serverIt)->listener = socket(AF_INET, SOCK_STREAM, 0);
		if ((*serverIt)->listener < 0)
		{
			std::cerr << DEBUG_MSG << "Error creating socket" << std::endl;
			continue;
		}

		// store listen port to _sockets
		int port = atoi((*serverIt)->directives["listen"].c_str());
		_boundPorts.push_back(port);

		// Set socket to non-blocking
		int flags = fcntl((*serverIt)->listener, F_GETFL, 0);
		if (flags < 0)
		{
			std::cerr << DEBUG_MSG << "Error getting socket flags" << std::endl;
			continue;
		}

		// Set socket options
		struct timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		if (setsockopt((*serverIt)->listener, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			std::cerr << DEBUG_MSG << "Error setting socket options" << std::endl;
			continue;
		}

		// Set socket to reuse address
		// NOTE: This need to be consider when we have multiple server that listen on same port
		int optval = 1;
		if (setsockopt((*serverIt)->listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
		{
			std::cerr << DEBUG_MSG << "Error setting socket options" << std::endl;
			continue;
		}

		// Bind socket
		(*serverIt)->addr.sin_family = AF_INET;
		(*serverIt)->addr.sin_port = htons(atoi((*serverIt)->directives["listen"].c_str()));
		(*serverIt)->addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind((*serverIt)->listener, (struct sockaddr *)&(*serverIt)->addr, sizeof((*serverIt)->addr)) < 0)
		{
			std::cerr << DEBUG_MSG << "Error binding socket" << std::endl;
			continue;
		}
		// Listen on socket
		if (listen((*serverIt)->listener, 1024) < 0)
		{
			std::cerr << DEBUG_MSG << "Error listening on socket" << std::endl;
			continue;
		}
		std::cerr << std::endl; // Add a newline for readability between servers
		std::cerr << DEBUG_MSG << "Listening on port " << (*serverIt)->directives["listen"] << std::endl;
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
	if (file.good())
	{
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @brief check if file is image file
 *
 * @param path
 * @return true if file is image file
 * @return false if file is not image file
 */
bool ConfigHandler::checkImageFile(std::string path) const
{
	// get file extension
	std::string extension = path.substr(path.find_last_of(".") + 1);
	if (extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "gif")
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

	static ConfigHandler& instance() {
		static ConfigHandler configHandler;
		return configHandler;
	}

/**
 * @brief Function to handle signal by closing all sockets and exit
 *
 * @param signal signal number
 */
void ConfigHandler::signalHandler(int signal)
{
	if (signal == SIGINT)
		std::cerr << DEBUG_MSG << "Received SIGINT" << std::endl;
	else if (signal == SIGTERM)
		std::cerr << DEBUG_MSG << "Received SIGTERM" << std::endl;
	else if (signal == SIGQUIT)
		std::cerr << DEBUG_MSG << "Received SIGQUIT" << std::endl;
	else
		std::cerr << DEBUG_MSG << "Received signal " << signal << std::endl;

	// close all sockets
	instance().closePorts();

	exit(0);
}

void ConfigHandler::execute() const
{
	// handle signal
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGQUIT, signalHandler);

	std::cerr << DEBUG_MSG << "Executing config" << std::endl;

	fd_set readfds;
	std::vector<int> activeSockets;
	std::vector<int>::iterator it;

	while (true)
	{
		FD_ZERO(&readfds);

		// Add listening sockets to the set
		for (std::vector<Server *>::const_iterator serverIt = _httpConfig.servers.begin(); serverIt != _httpConfig.servers.end(); ++serverIt)
		{
			FD_SET((*serverIt)->listener, &readfds);
		}

		// Add active sockets to the set
		for (it = activeSockets.begin(); it != activeSockets.end(); ++it)
		{
			FD_SET(*it, &readfds);
		}

		// Wait for an activity on one of the sockets
		int activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
		if (activity < 0)
		{
			std::cerr << DEBUG_MSG << "Error selecting" << std::endl;
			continue;
		}

		// Accept new connections
		for (std::vector<Server *>::const_iterator serverIt = _httpConfig.servers.begin(); serverIt != _httpConfig.servers.end(); ++serverIt)
		{
			if (FD_ISSET((*serverIt)->listener, &readfds))
			{
				int newSocket = accept((*serverIt)->listener, NULL, NULL);
				if (newSocket < 0)
				{
					std::cerr << DEBUG_MSG << "Error accepting connection" << std::endl;
					continue;
				}

				// Set the new socket to non-blocking mode
				int flags = fcntl(newSocket, F_GETFL, 0);
				fcntl(newSocket, F_SETFL, flags | O_NONBLOCK);

				// Add new socket to active sockets (assumed to be in blocking mode)
				activeSockets.push_back(newSocket);
			}
		}

		// Read data from active sockets
		for (it = activeSockets.begin(); it != activeSockets.end();)
		{
			if (FD_ISSET(*it, &readfds))
			{
				char buffer[4096];
				ssize_t bytesReceived = recv(*it, buffer, sizeof(buffer), 0);

				std::cout << "Buffer: " << buffer << std::endl;

				if (bytesReceived > 0)
				{
					// Process the data
					std::string response = "";
					std::string requestString(buffer, bytesReceived);
					// check host and path
					t_HttpRequest request = parseHttpRequest(requestString);
					Server *matchedServer = matchRequestToServer(request, _httpConfig.servers);

					// Matched Server
					if (matchedServer)
					{
						// std::cerr << "\033[1;31m" << "Matched server: " << matchedServer << "\033[0m" << std::endl;

						// If Server Matched, Check Location ...
						const Location *matchedLocation = matchRequestToLocation(request, matchedServer);
						if (matchedLocation)
						{
							std::cerr << DEBUG_MSG << "Matched location" << std::endl;

							// If Location Matched, Check Method ...
							// Check if method is allowed
							std::map<std::string, std::string>::const_iterator allowedMethod = matchedLocation->directives.find("methods");

							if (allowedMethod != matchedLocation->directives.end())
							{
								// check if request method is allowed
								std::cerr << DEBUG_MSG << "Method: " << request.method << std::endl;

								bool methodAllowed = false;

								// print all Allowed Methods
								std::istringstream iss(allowedMethod->second);
								std::string method;
								while (iss >> method)
								{
									// std::cerr << "Allowed method: " << method << std::endl;
									if (method == request.method)
									{
										methodAllowed = true;
										break;
									}
								}

								if (!methodAllowed)
								{
									std::cerr << DEBUG_MSG << "Method not allowed" << std::endl;
									response = createHtmlResponse(405, "Method not allowed");
								}
								else
								{

									// check if file exist
									std::map<std::string, std::string>::const_iterator rootDirective = matchedLocation->directives.find("root");
									std::map<std::string, std::string>::const_iterator defaultFile = matchedLocation->directives.find("default_file");

									if (rootDirective == matchedLocation->directives.end())
									{
										rootDirective = matchedServer->directives.find("root");
									}

									std::string filePath = rootDirective->second + request.path;
									std::cerr << DEBUG_MSG << "TEST TEST File path: " << filePath << std::endl;

									// check if directory or file
									struct stat s;
									if (stat(filePath.c_str(), &s) == 0)
									{
										if (s.st_mode & S_IFDIR)
										{
											// it's a directory

											// check if default file exist
											if (defaultFile != matchedLocation->directives.end())
											{
												filePath = rootDirective->second + "/" + defaultFile->second;
												std::cerr << "\033[1;31m" << DEBUG_MSG << "Default file path: " << filePath << "\033[0m" << std::endl;

												if (checkFileExist(filePath))
												{
													std::cerr<< "\033[1;31m" << DEBUG_MSG << "Default file exist" << "\033[0m" << std::endl;

													response = createHtmlResponse(200, readHtmlFile(filePath));
												}
												else
												{
													std::cerr << "\033[1;31m" << DEBUG_MSG << "Default file not exist" << "\033[0m" << std::endl;

													response = createHtmlResponse(404, "File not found");
												}
											}
											else
											{
												// check if auto index is on
												std::map<std::string, std::string>::const_iterator autoIndexDirective = matchedLocation->directives.find("autoindex");

												if (autoIndexDirective != matchedLocation->directives.end())
												{
													// auto index is on
													std::cerr << "\033[1;31m" << DEBUG_MSG << "Auto index is on" << "\033[0m" << std::endl;

													DIR *dir;
													struct dirent *ent;
													struct stat fileInfo;
													std::string fullPath;
													char timeBuff[20];
													std::string responseTemp = "<html>\n<head>\n<title>Directory Listing</title>\n</head>\n<body>\n<h1>Index Of ";
													responseTemp += request.path;
													responseTemp += "</h1>\n<table>\n<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n";

													dir = opendir(filePath.c_str());
													if (dir != NULL) {
														while ((ent = readdir(dir)) != NULL) {
															fullPath = filePath + "/" + ent->d_name;

															if(stat(fullPath.c_str(), &fileInfo) == 0) {
																std::strftime(timeBuff, 20, "%Y-%m-%d %H:%M:%S", std::localtime(&fileInfo.st_mtime));

																responseTemp += "<tr><td><a href=\"";
																responseTemp += request.path + "/" + ent->d_name;
																responseTemp += "\">";
																responseTemp += ent->d_name;
																responseTemp += "</a></td><td>";
																responseTemp += timeBuff;
																responseTemp += "</td><td>";
																responseTemp += formatSize(fileInfo.st_size);
																responseTemp += "</td></tr>\n";
															}
														}
														closedir(dir);
														responseTemp += "</table>\n</body>\n</html>";
														response = createHtmlResponse(200, responseTemp);
													} else {
														std::cerr << "Error opening directory: " << filePath << std::endl;
														response = createHtmlResponse(404, "Not Found");
													}
												}
												else
												{
													// auto index is off
													std::cerr << "\033[1;31m" << DEBUG_MSG << "Auto index is off" << "\033[0m" << std::endl;

													response = createHtmlResponse(404, "File not found");
												}
											}
										}
										else if (s.st_mode & S_IFREG)
										{
											// it's a file
											if (checkFileExist(filePath))
											{
												std::cerr << "\033[1;31m" << DEBUG_MSG << "File exist 2" << "\033[0m" << std::endl;

												response = createHtmlResponse(200, readHtmlFile(filePath));
											}
											else
											{
												std::cerr << "\033[1;31m" << DEBUG_MSG << "File not exist" << "\033[0m" << std::endl;

												response = createHtmlResponse(404, "File not found");
											}
										}
										else
										{
											// something else
										}
									}
									else
									{
										// error
										std::cerr << "\033[1;31m" << DEBUG_MSG << "File not exist" << "\033[0m" << std::endl;

										response = createHtmlResponse(404, "File not found");
									}

								}
							}
						}
						else
						{
							std::cerr << DEBUG_MSG << "No matching location found" << std::endl;
							std::cerr << DEBUG_MSG << "Checking CGI" << std::endl;
							if (request.path.find("/cgi-bin/") != std::string::npos && (request.path.find(".py") != std::string::npos || request.path.find(".sh") != std::string::npos)){
								std::cerr << DEBUG_MSG << "CGI found" << std::endl;
								// Get the root directory from server configuration

								std::map<std::string, std::string>::const_iterator rootDirective = matchedServer->directives.find("root");

								if (rootDirective != matchedServer->directives.end())
								{
									std::string filePath = rootDirective->second + request.path;

									std::cerr << DEBUG_MSG << "File path: " << filePath << std::endl;

									// Check if the CGI script exists
									if (checkFileExist(filePath))
									{
										std::cerr << DEBUG_MSG << "File exists, executing CGI" << std::endl;

										// Set up pipes for communication
										int pipeCgi[2];
										pipe(pipeCgi);

										pid_t pid = fork();
										if (pid == 0)
										{
											// Child process

											// Close the read end of the pipe, it's not needed
											close(pipeCgi[0]);

											// Redirect stdout to the write end of the pipe
											dup2(pipeCgi[1], STDOUT_FILENO);

											// Construct the argument array for execve
											char *args[] = {const_cast<char *>(filePath.c_str()), NULL};

											// Execute the CGI script
											execve(args[0], args, environ);

											// Exit the child process when done
											exit(0);
										}
										else if (pid > 0)
										{
											// Parent process

											// Close the write end of the pipe, it's not needed
											close(pipeCgi[1]);

											// Read from the pipe
											char buffer[4096];
											std::string output;
											ssize_t bytesRead;

											while ((bytesRead = read(pipeCgi[0], buffer, sizeof(buffer) - 1)) > 0)
											{
												buffer[bytesRead] = '\0'; // Null-terminate the buffer
												output += buffer;
											}

											std::cerr << DEBUG_MSG << "Output: " << output << std::endl;

											// Close the read end of the pipe
											close(pipeCgi[0]);

											// Wait for the child process to finish
											waitpid(pid, NULL, 0);

											// Create the response with the output of the CGI script
											response = createHtmlResponse(200, output);
										}
										else
										{
											std::cerr << DEBUG_MSG << "Failed to fork for CGI processing" << std::endl;
											response = createHtmlResponse(500, "Internal Server Error");
										}
									}
									else
									{
										std::cerr << DEBUG_MSG << "File does not exist" << std::endl;
										response = createHtmlResponse(404, "File not found");
									}

								}
							}
							else if (request.path.find("/upload/") != std::string::npos)
							{
								std::cerr << DEBUG_MSG << "Upload found" << std::endl;
								std::cerr << DEBUG_MSG << "Request body: " << request.body << std::endl;

								// std::string boundary = contentType.substr(contentType.find("boundary=") + 9);

								// Get the root directory from server configuration
								response = createHtmlResponse(200, "Recieved");
							}
							else
							{
								std::cerr << DEBUG_MSG << "CGI not found" << std::endl;
								std::cerr << DEBUG_MSG << "Checking if File Existed" << std::endl;

								std::map<std::string, std::string>::const_iterator rootDirective = matchedServer->directives.find("root");
								std::string filePath = rootDirective->second + request.path;
								std::cerr << DEBUG_MSG << "File path: " << filePath << std::endl;

								struct stat s;
								if (stat(filePath.c_str(), &s) == 0)
								{
									if (s.st_mode & S_IFDIR)
									{
										// if directory return 404
										response = createHtmlResponse(404, "File not found");
									}
									else
									{
										if (checkFileExist(filePath))
										{
											std::cerr << "\033[1;31m" << DEBUG_MSG << "File exist 3" << "\033[0m" << std::endl;

											// response = createHtmlResponse(200, readHtmlFile(filePath));
											response = createFileResponse(200, filePath);
										}
										else
										{
											std::cerr << "\033[1;31m" << DEBUG_MSG << "File not exist" << "\033[0m" << std::endl;

											response = createHtmlResponse(404, "File not found");
										}
									}
								}
								else
								{
									// error
									std::cerr << "\033[1;31m" << DEBUG_MSG << "File not exist" << "\033[0m" << std::endl;

									response = createHtmlResponse(404, "File not found");
								}

							}

								// // check if cgi
								// // check if file exist (.py and .sh)
								// std::cerr << DEBUG_MSG << "Checking CGI" << std::endl;
								// if (request.path.find(".py") != std::string::npos || request.path.find(".sh") != std::string::npos)
								// {
								// 	std::cerr << DEBUG_MSG << "CGI found" << std::endl;

								// 	// Get the root directory from server configuration
								// 	std::map<std::string, std::string>::const_iterator rootDirective = matchedServer->directives.find("root");
								// 	if (rootDirective != matchedServer->directives.end()) {
								// 	std::string filePath = rootDirective->second + request.path;

								// 	std::cerr << DEBUG_MSG << "File path: " << filePath << std::endl;

								// 	// Check if the CGI script exists
								// 	if (checkFileExist(filePath)) {
								// 		std::cerr << DEBUG_MSG << "File exists, executing CGI" << std::endl;

								// 		// Set up pipes for communication
								// 		int pipeCgi[2];
								// 		pipe(pipeCgi);

								// 		pid_t pid = fork();
								// 		if (pid == 0) {
								// 			// Child process

								// 			// Close the read end of the pipe, it's not needed
								// 			close(pipeCgi[0]);

								// 			// Redirect stdout to the write end of the pipe
								// 			dup2(pipeCgi[1], STDOUT_FILENO);

								// 			// Construct the argument array for execve
								// 			char *args[] = { const_cast<char *>(filePath.c_str()), NULL };

								// 			// Execute the CGI script
								// 			execve(args[0], args, environ);

								// 			// Exit the child process when done
								// 			exit(0);
								// 		} else if (pid > 0) {
								// 			// Parent process

								// 			// Close the write end of the pipe, it's not needed
								// 			close(pipeCgi[1]);

								// 			// Read from the pipe
								// 			char buffer[4096];
								// 			std::string output;
								// 			ssize_t bytesRead;

								// 			while ((bytesRead = read(pipeCgi[0], buffer, sizeof(buffer) - 1)) > 0) {
								// 				buffer[bytesRead] = '\0';  // Null-terminate the buffer
								// 				output += buffer;
								// 			}

								// 			// Close the read end of the pipe
								// 			close(pipeCgi[0]);

								// 			// Wait for the child process to finish
								// 			waitpid(pid, NULL, 0);

								// 			// Create the response with the output of the CGI script
								// 			response = createHtmlResponse(200, output);
								// 		} else {
								// 			std::cerr << DEBUG_MSG << "Failed to fork for CGI processing" << std::endl;
								// 			response = createHtmlResponse(500, "Internal Server Error");
								// 		}
								// 	} else {
								// 		std::cerr << DEBUG_MSG << "File does not exist" << std::endl;
								// 		response = createHtmlResponse(404, "File not found");
								// 	}
								// }

								// else
								// {
								// 	std::cerr << DEBUG_MSG << "Checking If File Existed" << std::endl;
								// 	// check if file exist
								// 	std::map<std::string, std::string>::const_iterator rootDirective = matchedServer->directives.find("root");
								// 	std::string filePath = rootDirective->second + request.path;
								// 	std::cerr << DEBUG_MSG << "File path: " << filePath << std::endl;

								// 	if (checkFileExist(filePath))
								// 	{
								// 		std::cerr << "\033[1;31m" << DEBUG_MSG << "File exist 3" << "\033[0m" << std::endl;

								// 		response = createHtmlResponse(200, readHtmlFile(filePath));
								// 	}
								// 	else
								// 	{
								// 		std::cerr << "\033[1;31m" << DEBUG_MSG << "File not exist" << "\033[0m" << std::endl;

								// 		response = createHtmlResponse(404, "File not found");
								// 	}
								// }

								// response = createHtmlResponse(404, "File not found");
								// // check if cgi
								// // std::map<std::string, std::string>::const_iterator cgiDirective = matchedServer->directives.find("cgi");

								// // if (cgiDirective != matchedServer->directives.end())
								// // {
								// // 	std::cerr << "CGI found" << std::endl;

								// // }
								// }

						}
					}
					else
					{
						std::cerr << DEBUG_MSG << "No matching server found" << std::endl;

						// if (request.path.find(".png") != std::string::npos)
						// {
						// 	response = createHtmlResponse(200, readHtmlFile(this->_cwd + "/htdocs/error/404_error_page.png"));
						// }
						// else
						// {
						// 	response = createHtmlResponse(404, readHtmlFile(this->_cwd + "/htdocs/error/404.html"));
						// }
					}
					send(*it, response.c_str(), response.length(), 0);
				}
				else if (bytesReceived == 0)
				{
					// Client closed connection
					close(*it);
					it = activeSockets.erase(it); // Erase returns the next iterator
					continue;
				}
				else
				{
					// Connection closed or an error occurred
					std::cerr << DEBUG_MSG << "Error receiving data" << std::endl;
					close(*it);
					it = activeSockets.erase(it); // Erase returns the next iterator
					continue;
				}
			}
			++it;
		}
	}
}

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

// ============================================================
// ================= For Test Print All config ================
// ============================================================

void ConfigHandler::testPrintAll() const
{
	std::map<std::string, std::string>::const_iterator cur;
	std::map<std::string, std::string>::const_iterator end;

	std::cerr << "============== Global Config ===============" << std::endl;
	cur = _globalConfig.begin();
	end = _globalConfig.end();
	while (cur != end)
	{
		std::cerr << std::setw(10) << "Global =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
		cur++;
	}

	std::cerr << "============== HTTP config ==============" << std::endl;
	cur = _httpConfig.directives.begin();
	end = _httpConfig.directives.end();
	while (cur != end)
	{
		std::cerr << std::setw(10) << "HTTP =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
		cur++;
	}
	std::cerr << "=============== default err pages ===============" << std::endl;
	cur = _httpConfig.defaultErrorPages.begin();
	end = _httpConfig.defaultErrorPages.end();
	while (cur != end)
	{
		std::cerr << std::setw(10) << "DEPs =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
		cur++;
	}

	std::vector<Server *>::const_iterator curServ = _httpConfig.servers.begin();

	while (curServ != _httpConfig.servers.end())
	{
		std::cerr << "============== Server config ==============" << std::endl;
		std::cerr << std::setw(10) << "SERVER =" << std::setw(25) << "listener"
				  << " | " << (*curServ)->listener << std::endl;
		// socket address ????
		std::cerr << std::setw(10) << "SERVER =" << std::setw(25) << "listen port"
				  << " | " << (*curServ)->listen_port << std::endl;
		// server Directive
		cur = (*curServ)->directives.begin();
		end = (*curServ)->directives.end();
		while (cur != end)
		{
			std::cerr << std::setw(10) << "SERVER =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
			cur++;
		}
		std::vector<Location>::const_iterator curLoc = (*curServ)->locations.begin();
		std::vector<Location>::const_iterator endLoc = (*curServ)->locations.end();
		while (curLoc != endLoc)
		{
			// print content in loc
			std::cerr << "------------------ Location config ------------------" << std::endl;
			std::cerr << std::setw(10) << "LOC =" << std::setw(25) << "Location path"
					  << " | " << curLoc->path << std::endl;
			cur = curLoc->directives.begin();
			end = curLoc->directives.end();
			while (cur != end)
			{
				std::cerr << std::setw(10) << "LOC dir =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
				cur++;
			}
			// cgi
			cur = curLoc->cgi.begin();
			end = curLoc->cgi.end();
			while (cur != end)
			{
				std::cerr << std::setw(10) << "LOC cgi =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
				cur++;
			}

			// upload
			cur = curLoc->upload.begin();
			end = curLoc->upload.end();
			while (cur != end)
			{
				std::cerr << std::setw(10) << "LOC upld =" << std::setw(25) << cur->first << " | " << cur->second << std::endl;
				cur++;
			}
			curLoc++;
		}
		curServ++;
	}
	return;
}
