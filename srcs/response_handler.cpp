#include "config_handler.hpp"

static std::string callPythonCgi(char * const args[], char * const envp[], unsigned int timeout=10)
{
	std::cout << BRED << "Args " << args[0] << RESET << std::endl;

	// Set up pipes for communication
	int pipeCgi[2];
	pipe(pipeCgi);

	pid_t pid = fork();
	if (pid == 0)
	{
		// Child process

		// Redirect stdout to the write end of the pipe
		dup2(pipeCgi[1], STDOUT_FILENO);
		// Close the read end of the pipe, it's not needed
		close(pipeCgi[0]);
		close(pipeCgi[1]);

		// catch signal for timeout
		signal(SIGALRM, timeoutHandler);

		// Set timeout for child process
		alarm(timeout);

		// Execute the CGI script
		execve(args[0], args, envp);

		// Exit the child process when done
		exit(0);
	}
	else if (pid > 0)
	{
		// Parent process

		// Close the write end of the pipe, it's not needed

		int status;
		// Wait for the child process to finish
		waitpid(pid, &status, 0);
		// dup2(pipeCgi[0], STDIN_FILENO);
		close(pipeCgi[1]);

		switch (status)
		{
			case 1:
				return createHtmlResponse(500, "Internal Server Error");
			case 2:
				return createHtmlResponse(405, "Method Not Allowed");
			case 3:
				return createHtmlResponse(404, "File Not Found");
			case 4:
				return createHtmlResponse(403, "Forbidden");
			default:
				break ;
		}

		// Check the exit status of the child process
		if (WIFEXITED(status))
		{
			std::cerr << DEBUG_MSG << "Child process exited with status " << WEXITSTATUS(status) << std::endl;
		}
		else if (WIFSIGNALED(status))
		{
			if (WTERMSIG(status) == SIGALRM) {
				std::cerr << BRED << "Timeout reached. The child process took too long to execute." << RESET << std::endl;
				return createHtmlResponse(408, "Request Timeout");
			}
		}
		else if (WIFSTOPPED(status))
		{
			std::cerr << DEBUG_MSG << "Child process was stopped by signal " << WSTOPSIG(status) << std::endl;
		}

		// Read from the pipe
		char buffer[4096];
		std::string output;
		ssize_t bytesRead;

		while ((bytesRead = read(pipeCgi[0], buffer, sizeof(buffer) - 1)) > 0)
		{
			buffer[bytesRead] = '\0'; // Null-terminate the buffer
			output += buffer;
		}

		std::cerr << "CALL CGI Output: " << output << std::endl;

		// Close the read end of the pipe
		close(pipeCgi[0]);

		return output.c_str();
	}
	else
	{
		std::cerr << DEBUG_MSG << "Failed to fork for CGI processing" << std::endl;
		return createHtmlResponse(500, "Internal Server Error");
	}
}

// prototype of helper funciton
static bool							checkMethod(Location const *, t_HttpRequest);
static bool 						checkRedirect(Location const *, std::string &);
int									execute(Location const *, std::string &, t_HttpRequest);
std::map<std::string, std::string>	createCgiEnvp(std::string, std::string, t_HttpRequest);


std::string	createRedirectResponse(Location const *);

// Main check for location
// return err code or 0
int	ConfigHandler::checkLocation(std::string &response, t_HttpRequest request, Server *matchedServer) const
{
	// find second slash
	size_t slashPos = request.path.find("/", 1);
	// get request path until second slash

	std::cout << "request.path" << request.path << std::endl;
	request.requestPath = request.path.substr(0, slashPos);
	std::cout << "request.requestPath" << request.requestPath << std::endl;
	// get arg path after second slash else empty string
	request.argPath = request.path.substr(slashPos + 1);
	if (request.argPath == request.requestPath)
		request.argPath = "";
	std::cout << "request.argPath" << request.argPath << std::endl;

	// locahost/index/454

	const Location *matchedLocation = matchRequestToLocation(request.requestPath, matchedServer);
	std::cout << BCYN << "Matched location: " << matchedLocation << RESET << std::endl;

	// std::string svmsg = "[SV-MSG] ";
	if (matchedLocation)
	{
		std::cout << SVMSG << "Location Matched" << std::endl;
		// cx method
		std::cout << SVMSG << "Checking method" << std::endl;
		if (checkMethod(matchedLocation, request))
		{
			std::cout << SVMSG << "Method NOT ALLOW" << std::endl;
			response = createHtmlResponse(405, getHttpStatusString(405));
			return 405;
		}
		std::cout << SVMSG << "Method OK" << std::endl;
		// cx redirect
		if (checkRedirect(matchedLocation, response))
		{
			// add new to response
			response = createRedirectResponse(matchedLocation);
			std::cout << SVMSG << "Returning 301 response" << std::endl;
			return 301;
		}
		// find root location
		vector<Location> locations = matchedServer->locations;
		Location rootLocation;
		for (std::vector<Location>::const_iterator it = locations.begin(); it != locations.end(); it++)
		{
			if (it->path == "/")
				rootLocation = *it;
		}

		map<std::string, std::string> rootDirective = rootLocation.directives;
		map<std::string, std::string> matchedDirective = matchedLocation->directives;
		std::string matchedDefaultFile = matchedDirective["default_file"];
		std::string rootDefaultFile = rootDirective["default_file"];

		if (matchedDefaultFile == rootDefaultFile)
		{
			return this->execute(const_cast<Location *>(&rootLocation), response, request);
		}
		// matchLocation = root if they share the same default file
		// or go to cgi or html file
		return this->execute(matchedLocation, response, request);

	}
	else
	{
		response = createHtmlResponse(404, getHttpStatusString(404));
		return 404;
	}

	return 0;
}

std::string getAutoIndex(std::string filePath, std::string requestPath)
{
	DIR *dir;
	struct dirent *ent;
	struct stat fileInfo;
	std::string fullPath;
	char timeBuff[20];
	std::string responseTemp = "<html>\n<head>\n<title>Directory Listing</title>\n</head>\n<body>\n<h1>Index Of ";
	responseTemp += requestPath;
	responseTemp += "</h1>\n<table>\n<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n";

	dir = opendir(filePath.c_str());
	if (dir != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			fullPath = filePath + "/" + ent->d_name;

			if(stat(fullPath.c_str(), &fileInfo) == 0) {
				std::strftime(timeBuff, 20, "%Y-%m-%d %H:%M:%S", std::localtime(&fileInfo.st_mtime));

				responseTemp += "<tr><td><a href=\"";
				responseTemp += requestPath + "/" + ent->d_name;
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
		return createHtmlResponse(200, responseTemp);
	} else {
		std::cerr << "Error opening directory: " << filePath << std::endl;
		return createHtmlResponse(404, "Not Found");
	}
}

int		ConfigHandler::execute(Location const *mLoc, std::string &response, t_HttpRequest request) const
{
	std::cout << SVMSG << "execute" << std::endl;
	std::map<std::string, std::string> directives = mLoc->directives;
	std::string fullPath;
	std::string reqPath;

	std::cout << DEBUG_MSG << "argPath: " << request.argPath.length() << std::endl;
	std::cout << DEBUG_MSG << "argPath: " << request.argPath << std::endl;

	std::cout << DEBUG_MSG << "request.path: " << request.path << std::endl;

	// if request has no path argument, get default file and return
	if (request.argPath.length() == 0)
	{
		fullPath = directives["root"] + "/" + directives["default_file"];

		if (!checkFileExist(fullPath)) {
			response = createHtmlResponse(404, getHttpStatusString(404));
			return 404;
		}

		reqPath = fullPath + request.path;

		// std::cerr << DEBUG_MSG << "reqPath: " << reqPath << std::endl;

		struct stat s;
		if (stat(reqPath.c_str(), &s) == 0)
		{
			if (s.st_mode & S_IFDIR)
			{
				// it's a directory
				response = getAutoIndex(reqPath, reqPath);
			}
			else
				response = createHtmlResponse(200, readHtmlFile(fullPath));
		}
		return 200;
	}

	// otherwise, call cgi
	else
	{
		std::map<std::string, std::string>	cgiMap = mLoc->cgi;
		std::map<std::string, std::string>	uploadMap = mLoc->upload;

		// std::cerr << DEBUG_MSG << "Checking CGI" << std::endl
		// 	<< "requestPath" << request.requestPath << std::endl;

		// call cgi for the location
		std::string	cgiPath = "";

		if (request.requestPath != "/cgi-bin")
			cgiPath = directives["root"] + "/cgi-bin" + request.requestPath + cgiMap["extension"];
		else
			cgiPath = directives["root"] + request.requestPath + "/" + request.argPath;

		std::string	execPath = cgiMap["executable"];
		std::string	absFilePath = directives["root"] + "/upload/" + request.argPath;

		// std::cerr << DEBUG_MSG << "cgiPath: " << cgiPath << std::endl;
		// std::cerr << DEBUG_MSG << "execPath: " << execPath << std::endl;
		// std::cerr << DEBUG_MSG << "absFilePath: " << absFilePath << std::endl;

		// create envp
		std::map<std::string, std::string>	cgiEnvpMap = createCgiEnvp(directives["root"], absFilePath, request);
		// to exec cgi

		char * const *cgiEnvp = createCgiEnvp(cgiEnvpMap);

		char * const args[] = { (char *)execPath.c_str(), (char *)cgiPath.c_str(), NULL};
		// char * const args[2] = {const_cast<char *>(filePath.c_str()), NULL};

		try {
			response = callPythonCgi(args, cgiEnvp);
		}
		catch (std::exception &e) {
			std::cerr << BRED << "Error: " << e.what() << RESET << std::endl;
			response = createHtmlResponse(500, "Internal Server Error");
		}

	}

	return 0;
}


// Start of helper

static bool	checkMethod(Location const *mLoc, t_HttpRequest Req)
{
	std::map<std::string, std::string>::const_iterator	allowMethod = mLoc->directives.find("methods");

	if (allowMethod == mLoc->directives.end())
		return 1;
	if (allowMethod->second.find(Req.method) == std::string::npos)
		return 1;
	return 0;
}

static bool checkRedirect(Location const *mLoc, std::string &response)
{
	// find return
	std::map<std::string, std::string>::const_iterator	it = mLoc->directives.find("return");
	if (it == mLoc->directives.end())
		return false;
	return true;
}

std::string	createRedirectResponse(Location const *mLoc)
{
	std::stringstream	tmpRes;
	std::map<std::string, std::string>	loc = mLoc->directives;
	std::string			newAdd = loc["return"];

	newAdd = newAdd.substr(newAdd.find(" "));

	tmpRes << "HTTP/1.1 301 " << getHttpStatusString(301) << "\r\n"
			<< "Location: " << newAdd << "\r\n"
			<< "Content-Type: text/html; charset=UTF-8" << "\r\n"
			<< "Content-Length: 0"
			<< "\r\n\r\n";

	// can redirect to another url
	// make redirect to another page in server

	return tmpRes.str();
}

std::map<std::string, std::string>	createCgiEnvp(std::string rootDir, std::string absFilePath, t_HttpRequest request)
{
	std::map<std::string, std::string>	ret;

	// remove / from argPath
	std::string argPath = request.argPath;
	if (argPath[0] == '/')
		argPath = argPath.substr(1);

	std::map<std::string, std::string> header = request.headers;

	// loop all key and value in header
	for (std::map<std::string, std::string>::iterator it = header.begin(); it != header.end(); it++)
	{
		std::string key = it->first;
		std::string value = it->second;
		std::transform(key.begin(), key.end(), key.begin(), ::toupper);
		std::replace(key.begin(), key.end(), '-', '_');
		std::cout << "key: " << key << " value: " << value << std::endl;
		ret[key] = value;
	}

	std::cout << "Origin: " << header["Origin"] << std::endl;

	ret["FILE_PATH"] = request.path;
	ret["REQUEST_METHOD"] = request.method;
	ret["REQUEST_URI"] = header["Origin"] + request.path;
	ret["BODY"] = request.body;

	return ret;
}

// return err code or 0
// int	ConfigHandler::checkLocation(std::string &response, t_HttpRequest request, Server *matchedServer) const
// {
// 	const Location *matchedLocation = matchRequestToLocation(request, matchedServer);
// 	std::cout << BCYN << "Matched location: " << matchedLocation << RESET << std::endl;
// 	if (matchedLocation)
// 	{
// 		std::cerr << DEBUG_MSG << "Matched location" << std::endl;

// 		// If Location Matched, Check Method ...
// 		// Check if method is allowed
// 		std::map<std::string, std::string>::const_iterator allowedMethod = matchedLocation->directives.find("methods");

// 		if (allowedMethod != matchedLocation->directives.end())
// 		{
// 			// check if request method is allowed
// 			std::cerr << DEBUG_MSG << "Method: " << request.method << std::endl;

// 			bool methodAllowed = false;

// 			// print all Allowed Methods
// 			std::istringstream iss(allowedMethod->second);
// 			std::string method;
// 			while (iss >> method)
// 			{
// 				std::cerr << "Allowed method: " << method << std::endl;
// 				std::cerr << "Request method: " << request.method << std::endl;
// 				if (method == request.method)
// 				{
// 					methodAllowed = true;
// 					break;
// 				}
// 			}

// 			if (!methodAllowed)
// 			{
// 				std::cerr << DEBUG_MSG << "Method not allowed" << std::endl;
// 				response = createHtmlResponse(405, "Method not allowed");
// 			}
// 			else
// 			{
// 				// check redirect
// 				std::map<std::string, std::string>::const_iterator itret = matchedLocation->directives.find("return:");
// 				if (itret != matchedLocation->directives.end())
// 				{
// 					std::stringstream tmpRes;
// 					std::string		tmp;
// 					tmp = itret->second;
// 					tmp = tmp.substr(4);
// 					tmpRes << "HTTP/1.1 " << "301 " << getHttpStatusString(301) << "\r\n"
// 						<< "Location: " << tmp << "\r\n"
// 						<<  "Content-Type: text/html\r\n"
// 						<< "Content-Length: 0\r\n"
// 						<< "Connection: close\r\n\r\n";
// 					response = tmpRes.str();

// 					return 301;
// 				}

// 				// check if file exist
// 				std::map<std::string, std::string>::const_iterator rootDirective = matchedLocation->directives.find("root");
// 				std::map<std::string, std::string>::const_iterator defaultFile = matchedLocation->directives.find("default_file");

// 				if (rootDirective == matchedLocation->directives.end())
// 				{
// 					rootDirective = matchedServer->directives.find("root");
// 				}

// 				std::string filePath = rootDirective->second + request.path;
// 				std::cerr << DEBUG_MSG << "TEST TEST File path: " << filePath << std::endl;

// 				// check if directory or file
// 				struct stat s;
// 				if (stat(filePath.c_str(), &s) == 0)
// 				{
// 					if (s.st_mode & S_IFDIR)
// 					{
// 						// it's a directory

// 						// check if default file exist
// 						if (defaultFile != matchedLocation->directives.end())
// 						{
// 							filePath = rootDirective->second + "/" + defaultFile->second;
// 							std::cerr << "\033[1;31m" << DEBUG_MSG << "Default file path: " << filePath << "\033[0m" << std::endl;

// 							if (checkFileExist(filePath))
// 							{
// 								std::cerr<< "\033[1;31m" << DEBUG_MSG << "Default file exist" << "\033[0m" << std::endl;

// 								response = createHtmlResponse(200, readHtmlFile(filePath));
// 							}
// 							else
// 							{
// 								std::cerr << "\033[1;31m" << DEBUG_MSG << "Default file not exist" << "\033[0m" << std::endl;

// 								response = createHtmlResponse(404, "File not found");
// 							}
// 						}
// 						else
// 						{
// 							// check if auto index is on
// 							std::map<std::string, std::string>::const_iterator autoIndexDirective = matchedLocation->directives.find("autoindex");

// 							if (autoIndexDirective != matchedLocation->directives.end())
// 							{
// 								// auto index is on
// 								std::cerr << "\033[1;31m" << DEBUG_MSG << "Auto index is on" << "\033[0m" << std::endl;

// 								DIR *dir;
// 								struct dirent *ent;
// 								struct stat fileInfo;
// 								std::string fullPath;
// 								char timeBuff[20];
// 								std::string responseTemp = "<html>\n<head>\n<title>Directory Listing</title>\n</head>\n<body>\n<h1>Index Of ";
// 								responseTemp += request.path;
// 								responseTemp += "</h1>\n<table>\n<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n";

// 								dir = opendir(filePath.c_str());
// 								if (dir != NULL) {
// 									while ((ent = readdir(dir)) != NULL) {
// 										fullPath = filePath + "/" + ent->d_name;

// 										if(stat(fullPath.c_str(), &fileInfo) == 0) {
// 											std::strftime(timeBuff, 20, "%Y-%m-%d %H:%M:%S", std::localtime(&fileInfo.st_mtime));

// 											responseTemp += "<tr><td><a href=\"";
// 											responseTemp += request.path + "/" + ent->d_name;
// 											responseTemp += "\">";
// 											responseTemp += ent->d_name;
// 											responseTemp += "</a></td><td>";
// 											responseTemp += timeBuff;
// 											responseTemp += "</td><td>";
// 											responseTemp += formatSize(fileInfo.st_size);
// 											responseTemp += "</td></tr>\n";
// 										}
// 									}
// 									closedir(dir);
// 									responseTemp += "</table>\n</body>\n</html>";
// 									response = createHtmlResponse(200, responseTemp);
// 								} else {
// 									std::cerr << "Error opening directory: " << filePath << std::endl;
// 									response = createHtmlResponse(404, "Not Found");
// 								}
// 							}
// 							else
// 							{
// 								// auto index is off
// 								std::cerr << "\033[1;31m" << DEBUG_MSG << "Auto index is off" << "\033[0m" << std::endl;

// 								response = createHtmlResponse(404, "File not found");
// 							}
// 						}
// 					}
// 					else if (s.st_mode & S_IFREG)
// 					{
// 						// it's a file
// 						if (checkFileExist(filePath))
// 						{
// 							std::cerr << "\033[1;31m" << DEBUG_MSG << "File exist 2" << "\033[0m" << std::endl;

// 							response = createHtmlResponse(200, readHtmlFile(filePath));
// 						}
// 						else
// 						{
// 							std::cerr << "\033[1;31m" << DEBUG_MSG << "File not exist" << "\033[0m" << std::endl;

// 							response = createHtmlResponse(404, "File not found");
// 						}
// 					}
// 					else
// 					{
// 						// something else
// 					}
// 				}
// 				else
// 				{
// 					// error
// 					std::cerr << "\033[1;31m" << DEBUG_MSG << "File not exist" << "\033[0m" << std::endl;

// 					response = createHtmlResponse(404, "File not found");
// 				}

// 			}
// 		}
// 	}
// 	else
// 	{
// 		std::cerr << DEBUG_MSG << "No matching location found" << std::endl;
// 		std::cerr << DEBUG_MSG << "Checking CGI" << std::endl;
// 		if (request.path.find("/cgi-bin/") != std::string::npos && (request.path.find(".py") != std::string::npos || request.path.find(".sh") != std::string::npos)){
// 			std::cerr << DEBUG_MSG << "CGI found" << std::endl;
// 			// Get the root directory from server configuration

// 			std::map<std::string, std::string>::const_iterator rootDirective = matchedServer->directives.find("root");

// 			// TODO: What is this block for? need to find out
// 			if (rootDirective != matchedServer->directives.end())
// 			{
// 				std::string rootDir = rootDirective->second;
// 				std::string filePath = rootDir + request.path;

// 				// TODO: get cgi file name may need to add file name by request in below function
// 				std::string cgiFileName = getCgiFileName(request.method);

// 				std::cout << BGRN << "rootDir: " << rootDir << RESET << std::endl;

// 				std::cerr << DEBUG_MSG << "File path: " << filePath << std::endl;

// 				// Check if the CGI script exists
// 				if (checkFileExist(filePath))
// 				{
// 					std::cerr << BYEL << DEBUG_MSG << "File exists, executing CGI" << RESET << std::endl;

// 					std::string executable = "/usr/bin/python3";

// 					// Construct the argument array for execve
// 					char * const args[] = { (char *)executable.c_str(), (char *)filePath.c_str(), NULL};

// 					// call python cgi
// 					response = callPythonCgi(args, environ);

// 				}
// 				else
// 				{
// 					std::cerr << DEBUG_MSG << "File does not exist" << std::endl;
// 					response = createHtmlResponse(404, "File not found");
// 				}

// 			}
// 		}
// 		else if (request.path.find("/infinite") != std::string::npos)
// 		{
// 			// infinite loop cgi file name
// 			std::string cgiFileName = this->_cwd + "/htdocs/cgi-bin/infinite.py";

// 			// get root directive
// 			std::map<std::string, std::string>::const_iterator rootDirective = matchedServer->directives.find("root");

// 			std::string rootDir = "/htdocs";
// 			if (rootDirective != matchedServer->directives.end())
// 			{
// 				rootDir = rootDirective->second;
// 			}

// 			// get cgi directive
// 			std::map<std::string, std::string>::const_iterator cgiDirective = matchedServer->directives.find("cgi");

// 			if (cgiDirective != matchedServer->directives.end())
// 			{
// 				cgiFileName = rootDir + "/htdocs" + cgiDirective->second;

// 			}
// 			std::cerr << BGRN << "cgiFileName: " << cgiFileName << RESET << std::endl;

// 			std::string executable = "/usr/bin/python3";

// 			char * const args[] = { (char *)executable.c_str(), (char *)cgiFileName.c_str(), NULL};

// 			// call python cgi
// 			response = callPythonCgi(args, NULL, 5);
// 			// response = createHtmlResponse(200, "Recieved");
// 		}
// 		else if (request.path.find("/upload/") != std::string::npos)
// 		{
// 			std::cerr << DEBUG_MSG << "Upload found" << std::endl;

// 			std::map<std::string, std::string>::const_iterator rootDirective = matchedServer->directives.find("root");

// 			std::string rootDir = "/htdocs";
// 			if (rootDirective != matchedServer->directives.end())
// 			{
// 				rootDir = rootDirective->second;
// 			}

// 			// construct map for envp
// 			std::map<std::string, std::string> cgiEnvpMap;
// 			std::string fileName;
// 			std::string absoluteFilePath;

// 			if (request.method == "DELETE")
// 			{
// 				// std::cout << "TEST PRINT LOC ======" << matchedLocation->path << std::endl;
// 				// cx if DELETE allow
// 				// where am i???

// 				// get file name from request.path (request.path = /upload/?file=filename)
// 				fileName = request.path.substr(request.path.find("file=") + 5);
// 				absoluteFilePath = rootDir + "/upload/" + fileName;
// 				if (!checkFileExist(absoluteFilePath))
// 				{
// 					response = createHtmlResponse(404, "File not found");
// 					return 1;
// 				}
// 			}
// 			else
// 			{
// 				fileName = request.headers["File-name"];
// 			}
// 			// Construct absolute file path
// 			absoluteFilePath = rootDir + "/upload/" + fileName;

// 			// get cgi file name
// 			std::string cgiFileName = getCgiFileName(request.method);
// 			std::string filePath = rootDir + "/cgi-bin" + cgiFileName;

// 			std::string executable = "/usr/bin/python3";

// 			std::cerr << DEBUG_MSG << "File path: " << filePath << std::endl;

// 			// set value in envp map
// 			cgiEnvpMap["FILE_PATH"] = absoluteFilePath;
// 			cgiEnvpMap["PATH_INFO"] = request.path;
// 			cgiEnvpMap["REQUEST_METHOD"] = request.method;
// 			cgiEnvpMap["UPLOAD_DIR"] = rootDir + "/upload";
// 			cgiEnvpMap["BODY"] = request.body;
// 			cgiEnvpMap["CONTENT_TYPE"] = request.headers["Content-Type"];

// 			// call python cgi
// 			char * const *cgiEnvp = createCgiEnvp(cgiEnvpMap);

// 			char * const args[] = { (char *)executable.c_str(), (char *)filePath.c_str(), NULL};
// 			// char * const args[2] = {const_cast<char *>(filePath.c_str()), NULL};

// 			try {
// 				response = callPythonCgi(args, cgiEnvp);
// 			}
// 			catch (std::exception &e) {
// 				std::cerr << BRED << "Error: " << e.what() << RESET << std::endl;
// 				response = createHtmlResponse(500, "Internal Server Error");
// 			}

// 			// response = callPythonCgi(filePath, request.body);

// 			std::cerr << DEBUG_MSG << "Response: " << response << std::endl;

// 			// Get the root directory from server configuration
// 			// response = createHtmlResponse(200, "Recieved");
// 		}
// 		else
// 		{
// 			std::cerr << DEBUG_MSG << "CGI not found" << std::endl;
// 			std::cerr << DEBUG_MSG << "Checking if File Existed" << std::endl;

// 			std::map<std::string, std::string>::const_iterator rootDirective = matchedServer->directives.find("root");
// 			std::string filePath = rootDirective->second + request.path;
// 			std::cerr << DEBUG_MSG << "File path: " << filePath << std::endl;

// 			struct stat s;
// 			if (stat(filePath.c_str(), &s) == 0)
// 			{
// 				if (s.st_mode & S_IFDIR)
// 				{
// 					// if directory return 404
// 					response = createHtmlResponse(404, "File not found");
// 				}
// 				else
// 				{
// 					if (checkFileExist(filePath))
// 					{
// 						std::cerr << "\033[1;31m" << DEBUG_MSG << "File exist 3" << "\033[0m" << std::endl;

// 						// response = createHtmlResponse(200, readHtmlFile(filePath));
// 						response = createFileResponse(200, filePath);
// 					}
// 					else
// 					{
// 						std::cerr << "\033[1;31m" << DEBUG_MSG << "File not exist" << "\033[0m" << std::endl;

// 						response = createHtmlResponse(404, "File not found");
// 					}
// 				}
// 			}
// 			else
// 			{
// 				// error
// 				std::cerr << "\033[1;31m" << DEBUG_MSG << "File not exist" << "\033[0m" << std::endl;

// 				response = createHtmlResponse(404, "File not found");
// 			}

// 		}

// 	}
// 	return 0;
// }
