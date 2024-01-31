#include "config_handler.hpp"

std::string ConfigHandler::getCgiFileName(std::string method) {
	if (method == "POST") {
		return "/upload.py";
	}
	else if (method == "DELETE") {
		return "/delete.py";
	}
	return "";
}

std::string ConfigHandler::createHtmlTextResponse(int statusCode, const std::string &htmlContent) const
{
	std::ostringstream response;
	response << "HTTP/1.1 " << statusCode << " " << getHttpStatusString(statusCode) << "\r\n"
			 << "Content-Type: text/html\r\n"
			 << "Content-Length: " << htmlContent.length() << "\r\n\r\n"
			 << htmlContent;
	return response.str();
}


std::string ConfigHandler::createHtmlResponse(int statusCode, const std::string &htmlContent) const
{
	// find if there are default error page in http scope
	std::stringstream	ss;
	ss << statusCode;
	if (_httpConfig.defaultErrorPages.find(ss.str()) != _httpConfig.defaultErrorPages.end()) {
		std::map<std::string, std::string>	path = _httpConfig.defaultErrorPages;
		std::string	ret = createFileResponse(statusCode, path[ss.str()]);
		if (ret.find("Error: ") == std::string::npos) {
			return ret;
		}
	}
	return createHtmlTextResponse(statusCode, htmlContent);
}


std::string createHtmlResponseOnlyHead(int statusCode)
{
	std::ostringstream response;
	response << "HTTP/1.1 " << statusCode << " " << getHttpStatusString(statusCode) << "\r\n"
			 << "Content-Type: text/html\r\n"
			 << "Content-Length: " << 0 << "\r\n\r\n";
	return response.str();
}

bool	ConfigHandler::checkImageFile(std::string path) const
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

const Location *ConfigHandler::matchRequestToLocation(std::string requestPath, Server *server) const
{
	// Iterate through locations to find a match
	for (std::vector<Location>::const_iterator it = server->locations.begin(); it != server->locations.end(); ++it)
	{
		const Location &location = *it;

		// Check if the requested path starts with the location path
		if (requestPath == location.path)
		{
			// Found a matching location
			return &(*it);
		}
	}

	// No matching location found
	return 0;
}

// std::vector<Server *>::const_iterator	*checkServerPort(std::string port, std::string servName, std::vector<Server *>::const_iterator *it)
// {
	 
// }


// Function to match the request to a server
Server *ConfigHandler::matchRequestToServer(const t_HttpRequest &request, const std::vector<Server *> &servers) const
{
	std::string requestedHost;

	// Find the "Host" header in a const-safe manner
	std::map<std::string, std::string>::const_iterator itHost = request.headers.find("Host");
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
	std::string	port = "";
	bool	isPort = false;
	// Optionally, you can parse the port from the requested host if it's in the format "host:port"
	size_t colonPos = requestedHost.find(":");
	if (colonPos != std::string::npos)
	{
		port = requestedHost.substr(colonPos + 1);
		isPort = true;
		requestedHost = requestedHost.substr(0, colonPos);
	}

	// find server name and find port
	for (std::vector<Server *>::const_iterator it = servers.begin(); it != servers.end(); ++it)
	{
		std::map<std::string, std::string>::const_iterator itDirective = (*it)->directives.find("server_name");
		if (itDirective != (*it)->directives.end())
		{
			// Split the server names by spaces and match each one
			std::istringstream iss(itDirective->second);
			std::string serverName, serverPort;


			std::map<std::string, std::string>::const_iterator	itPort = (*it)->directives.find("listen");
			if (itPort != (*it)->directives.end()) 
			{
				serverPort = itPort->second;
			}
			
			iss >> serverName;
			if ((serverName == requestedHost || requestedHost == "localhost") && (port == serverPort || !isPort))
			{
				return *it;
			}
		}
	}
	return 0; // No matching server found
}

std::string readHtmlFile(const std::string &filePath)
{
	std::ifstream htmlFile(filePath.c_str());
	std::ostringstream buffer;
	buffer << htmlFile.rdbuf();

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
	case 301:
		return "Moved Permanently";
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
	case 408:
		return "Request Timeout";
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

std::string formatSize(size_t size) {
	std::ostringstream stream; // Declaring an ostringstream object
	stream << size << " bytes"; // Using the insertion operator to format the string
	return stream.str(); // Returning the formatted string
}

void timeoutHandler(int signum) {
	if (signum == SIGALRM) {
		std::cerr << BRED <<  "Timeout reached. The child process took too long to execute." << RESET << std::endl;
		exit(_POSIX_TIMEOUTS);
	}
}

char * const *createCgiEnvp( const std::map<std::string, std::string> &cgiEnv )
{
	// Create a new environment array
	char **envp = new char *[cgiEnv.size() + 1];

	// Copy the environment variables from the map to the array
	int i = 0;
	for (std::map<std::string, std::string>::const_iterator it = cgiEnv.begin(); it != cgiEnv.end(); ++it)
	{
		std::string envVar = it->first + "=" + it->second;
		// print envVar in green
		envp[i] = new char[envVar.length() + 1];
		strcpy(envp[i], envVar.c_str());
		i++;
	}

	// Add a null terminator to the end of the array
	envp[i] = NULL;

	return envp;
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

std::string ConfigHandler::createFileResponse(int statusCode, const std::string &filePath) const
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

