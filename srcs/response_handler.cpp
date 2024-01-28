#include "config_handler.hpp"

// prototype of helper funciton
static bool							checkMethod(Location const *, t_HttpRequest);
static bool 						checkRedirect(Location const *, std::string &);
int									execute(Location const *, std::string &, t_HttpRequest);
std::map<std::string, std::string>	createCgiEnvp(std::string, std::string, t_HttpRequest);
int									getResponseCode(std::string);

static std::string callPythonCgi(char * const args[], char * const envp[], unsigned int timeout=10)
{
	// Set up pipes for communication
	int pipeCgi[2];
	pipe(pipeCgi);

	char cwd[1024];  // You may need to adjust the buffer size accordingly
	getcwd(cwd, sizeof(cwd));

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
				return createHtmlResponse(404, readHtmlFile(std::string(cwd) + "/htdocs/error/500.html"));
			case 2:
				return createHtmlResponse(405, "Method Not Allowed");
			case 3:
				return createHtmlResponse(404, readHtmlFile(std::string(cwd) + "/htdocs/error/404.html"));
			case 4:
				return createHtmlResponse(403, "Forbidden");
			default:
				break ;
		}

		// Check the exit status of the child process
		if (WIFEXITED(status))
		{
			// std::cerr << DEBUG_MSG << "Child process exited with status " << WEXITSTATUS(status) << std::endl;
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

		// Close the read end of the pipe
		close(pipeCgi[0]);

		std::cout << BRED << "Response: " << output << RESET << std::endl;
		int code = getResponseCode(output);
		std::cout << BRED << "Code: " << code << RESET << std::endl;
		return output.c_str();
	}
	else
	{
		std::cerr << DEBUG_MSG << "Failed to fork for CGI processing" << std::endl;
		return createHtmlResponse(500, "Internal Server Error");
	}
}


std::string	createRedirectResponse(Location const *);

// Main check for location
// return err code or 0
int	ConfigHandler::checkLocation(std::string &response, t_HttpRequest request, Server *matchedServer) const
{
	// find second slash
	size_t pathSlashPos = request.path.find("/", 1);
	size_t argSlashPos = request.path.rfind("/");
	// get request path until second slash

	request.requestPath = request.path.substr(0, pathSlashPos);
	// get arg path after second slash else empty string
	request.argPath = request.path.substr(argSlashPos);
	if (request.argPath == request.requestPath)
		request.argPath = "";

	// locahost/index/454

	const Location *matchedLocation = matchRequestToLocation(request.requestPath, matchedServer);

	if (matchedLocation)
	{
		// cx method
		if (checkMethod(matchedLocation, request) || request.method == "HEAD")
		{
			if (request.method == "HEAD")
				response = createHtmlResponseOnlyHead(405);
			else
				response = createHtmlResponse(405, getHttpStatusString(405));
			return 405;
		}
		// cx redirect
		if (checkRedirect(matchedLocation, response))
		{
			// add new to response
			response = createRedirectResponse(matchedLocation);
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
		response = createHtmlResponse(404, readHtmlFile(this->_cwd + "/htdocs/error/404.html"));
		return 404;
	}

	return 0;
}

std::string ConfigHandler::getAutoIndex(std::string path, std::string retPath) const
{
	DIR *dir;
	struct dirent *ent;
	struct stat fileInfo;
	std::string fullPath;
	char timeBuff[20];
	std::string responseTemp = "<html>\n<head>\n<title>Directory Listing</title>\n</head>\n<body>\n<h1>Index Of ";
	responseTemp += retPath;
	responseTemp += "</h1>\n<table>\n<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n";

	dir = opendir(path.c_str());
	if (dir != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			fullPath = path + "/" + ent->d_name;

			if(stat(fullPath.c_str(), &fileInfo) == 0) {
				std::strftime(timeBuff, 20, "%Y-%m-%d %H:%M:%S", std::localtime(&fileInfo.st_mtime));

				std::string href = "";
				if (!std::strcmp(ent->d_name, "."))
				{
					href = retPath;
				}
				else if (!std::strcmp(ent->d_name, ".."))
				{
					size_t pos = retPath.rfind("/");
					href = retPath.substr(0, pos);
					if (href == "")
						href = "/";
				}
				else
				{
					href = retPath + "/" + ent->d_name;
				}

				responseTemp += "<tr><td><a href=\"";
				responseTemp += href;
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
		std::cerr << "Error opening directory: " << path << std::endl;
		return createHtmlResponse(404, readHtmlFile(this->_cwd + "/htdocs/error/404.html"));
	}
}

bool	isDirectory(std::string path)
{
	struct stat s;

	if (stat(path.c_str(), &s) == 0)
	{
		if (s.st_mode & S_IFDIR)
		{
			// it's a directory
			return true;
		}
	}
	return false;
}

int		ConfigHandler::execute(Location const *mLoc, std::string &response, t_HttpRequest request) const
{
	std::map<std::string, std::string> directives = mLoc->directives;
	std::string fullPath;
	std::string reqPath = request.requestPath;


	// Check if its autoindex
	if (reqPath == "/autoindex")
	{
		std::string fullArgPath = directives["root"] + reqPath + request.argPath;

		// if arg path, it's file. otherwise, directory
		if (request.argPath.length() == 0 || isDirectory(fullArgPath))
		{
			fullPath = directives["root"] + reqPath + request.argPath;

			std::string retPath = reqPath + request.argPath;

			response = getAutoIndex(fullPath, retPath);
			return 200;
		}
		else
		{
			fullPath = directives["root"] + request.path;

			// Check if file exist
			if (!checkFileExist(fullPath)) {
				response = createHtmlResponse(404, readHtmlFile(this->_cwd + "/htdocs/error/404.html"));
				return 404;
			}
			// if file exist, return file
			response = createFileResponse(200, fullPath);
			return 200;
		}
	}

	// if request has no path argument, get default file and return
	if (request.argPath.length() == 0)
	{
		fullPath = directives["root"] + "/" + directives["default_file"];

		if (!checkFileExist(fullPath)) {
			response = createHtmlResponse(404, readHtmlFile(this->_cwd + "/htdocs/error/404.html"));
			return 404;
		}

		response = createHtmlResponse(200, readHtmlFile(fullPath));
		return 200;
	}
	// otherwise, call cgi
	else
	{
		std::map<std::string, std::string>	cgiMap = mLoc->cgi;
		std::map<std::string, std::string>	uploadMap = mLoc->upload;


		// call cgi for the location
		std::string	cgiPath = "";

		if (request.requestPath != "/cgi-bin")
			cgiPath = directives["root"] + "/cgi-bin" + request.requestPath + cgiMap["extension"];
		else
			cgiPath = directives["root"] + request.requestPath + "/" + request.argPath;

		std::string	execPath = cgiMap["executable"];
		std::string	absFilePath = directives["root"] + "/upload/" + request.argPath;

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
			response = createHtmlResponse(404, readHtmlFile(this->_cwd + "/htdocs/error/404.html"));
			return 404;
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
	(void) response;
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
	(void) rootDir;
	(void) absFilePath;
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
		ret[key] = value;
	}

	ret["FILE_PATH"] = request.path;
	ret["REQUEST_METHOD"] = request.method;
	ret["REQUEST_URI"] = header["Origin"] + request.path;
	ret["BODY"] = request.body;

	return ret;
}

int getResponseCode(std::string response)
{
	std::cout << BBLU << "Response: " << response << RESET << std::endl;
	// get beginning of code
	size_t start = response.find("HTTP/1.1 ");

	if (start == std::string::npos)
		return 0;

	// get end of code
	size_t end = response.find("OK\r\n", start);

	if (end == std::string::npos)
		return 0;

	// get code
	std::string code = response.substr(start + 9, end - start - 9);
	std::cout << BBLU << "Code: " << code << RESET << std::endl;

	// std::string code = response.substr(response.find("HTTP/1.1 ") + 1);
	// code = code.substr(0, code.find(" "));
	return std::atoi(code.c_str());
}
