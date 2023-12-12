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

t_HttpRequest parseHttpRequest(std::string requestString) {
	t_HttpRequest request;
	std::istringstream requestStream(requestString);
	std::string line;
	bool headerSection = true;

	// Parse the request line
	if (std::getline(requestStream, line) && !line.empty()) {
		std::istringstream lineStream(line);
		lineStream >> request.method >> request.path >> request.httpVersion;
	}

	// Parse headers and body
	while (std::getline(requestStream, line)) {
		// Handle different line endings
		std::string::size_type end = line.find("\r");
		if (end != std::string::npos) {
			line.erase(end);
		}

		// Check for the end of the header section
		if (line.empty() && headerSection) {
			headerSection = false;
			continue;
		}

		if (headerSection) {
			std::string::size_type pos = line.find(":");
			if (pos != std::string::npos) {
				std::string headerName = line.substr(0, pos);
				std::string headerValue = line.substr(pos + 2);  // Skip the ": "
				request.headers[headerName] = headerValue;
			}
		} else {
			// Add to body
			request.body += line;
			if (!requestStream.eof()) {
				request.body += "\n";  // Maintain line breaks in body
			}
		}
	}

	return request;
}

// Function to match the request to a server
Server* matchRequestToServer(const t_HttpRequest& request, const std::vector<Server*>& servers) {
	std::string requestedHost;

	// Find the "Host" header in a const-safe manner
	std::map<std::string, std::string>::const_iterator itHost = request.headers.find("Host");
	if (itHost != request.headers.end()) {
		requestedHost = itHost->second;
	} else {
		// Handle case where "Host" header is not found
		return 0; // Or handle it according to your application logic
	}

	// Optionally, you can parse the port from the requested host if it's in the format "host:port"
	size_t colonPos = requestedHost.find(":");
	if (colonPos != std::string::npos) {
		requestedHost = requestedHost.substr(0, colonPos);
	}

	for (std::vector<Server*>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		std::map<std::string, std::string>::const_iterator itDirective = (*it)->directives.find("server_name");
		if (itDirective != (*it)->directives.end()) {
			// Split the server names by spaces and match each one
			std::istringstream iss(itDirective->second);
			std::string serverName;
			while (iss >> serverName) {
				if (serverName == requestedHost) {
					// Found a matching server
					return *it;
				}
			}
		}
	}
	return 0; // No matching server found
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
HTTPConfig ConfigHandler::_parseHTTPConfig(const std::string& filename)
{
	// std::cerr << "Parsing " << filename << std::endl;

	// Check if the file extension is .conf
	std::string extension = filename.substr(filename.find_last_of(".") + 1);
	if (extension != "conf") {
		std::cerr << "Incorrect file extension" << std::endl;
		throw IncorrectExtensionException();
	}

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
			// Parse http directives
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

		// Set socket to non-blocking
		int flags = fcntl((*serverIt)->listener, F_GETFL, 0);
		if (flags < 0) {
			std::cerr << "Error getting socket flags" << std::endl;
			continue;
		}

		// Set socket options
		struct timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		if (setsockopt((*serverIt)->listener, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
			std::cerr << "Error setting socket options" << std::endl;
			continue;
		}

		// Bind socket
		(*serverIt)->addr.sin_family = AF_INET;
		(*serverIt)->addr.sin_port = htons(atoi((*serverIt)->directives["listen"].c_str()));
		(*serverIt)->addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind((*serverIt)->listener, (struct sockaddr *)&(*serverIt)->addr, sizeof((*serverIt)->addr)) < 0) {
			std::cerr << "Error binding socket" << std::endl;
			continue;
		}
		// Listen on socket
		if (listen((*serverIt)->listener, 1024) < 0) {
			std::cerr << "Error listening on socket" << std::endl;
			continue;
		}
		std::cout << std::endl; // Add a newline for readability between servers
		std::cout << "Listening on port " << (*serverIt)->directives["listen"] << std::endl;
	}
}

void ConfigHandler::execute() const {
	std::cout << "Executing config" << std::endl;

	fd_set readfds;
	std::vector<int> activeSockets;
	std::vector<int>::iterator it;

	while (true) {
		FD_ZERO(&readfds);

		// Add listening sockets to the set
		for (std::vector<Server*>::const_iterator serverIt = _httpConfig.servers.begin(); serverIt != _httpConfig.servers.end(); ++serverIt) {
			FD_SET((*serverIt)->listener, &readfds);
		}

		// Add active sockets to the set
		for (it = activeSockets.begin(); it != activeSockets.end(); ++it) {
			FD_SET(*it, &readfds);
		}

		// Wait for an activity on one of the sockets
		int activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
		if (activity < 0) {
			std::cerr << "Error selecting" << std::endl;
			continue;
		}

		// Accept new connections
		for (std::vector<Server*>::const_iterator serverIt = _httpConfig.servers.begin(); serverIt != _httpConfig.servers.end(); ++serverIt) {
			if (FD_ISSET((*serverIt)->listener, &readfds)) {
				int newSocket = accept((*serverIt)->listener, NULL, NULL);
				if (newSocket < 0) {
					std::cerr << "Error accepting connection" << std::endl;
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
		for (it = activeSockets.begin(); it != activeSockets.end();) {
			if (FD_ISSET(*it, &readfds)) {
				char buffer[4096];
				ssize_t bytesReceived = recv(*it, buffer, sizeof(buffer), 0);

				std::cout << "Received " << bytesReceived << " bytes" << std::endl;
				std::cout << "Buffer: " << buffer << std::endl;

				if (bytesReceived > 0) {
					// Process the data
					//std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello world!";
					//send(*it, response.c_str(), response.length(), 0);
					std::string response = "";
					std::string requestString(buffer, bytesReceived);
					// check host and path
					t_HttpRequest request = parseHttpRequest(requestString);

					Server* matchedServer = matchRequestToServer(request, _httpConfig.servers);
					if (matchedServer) {
						std::cout << "Matched server" << std::endl;
						response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello world!";
					} else {
						std::cout << "No matching server found" << std::endl;
						response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 9\r\n\r\nNot found";
					}

					send(*it, response.c_str(), response.length(), 0);

				} else if (bytesReceived == 0) {
					// Client closed connection
					close(*it);
					it = activeSockets.erase(it); // Erase returns the next iterator
					continue;
				} else {
					// Connection closed or an error occurred
					std::cerr << "Error receiving data" << std::endl;
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
