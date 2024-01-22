#include "config_handler.hpp"

static std::string callPythonCgi(char * const args[], char * const envp[], unsigned int timeout=0)
{

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
		close(pipeCgi[1]);

		int status;
		// Wait for the child process to finish
		waitpid(pid, &status, 0);

		switch (status)
		{
			case 1:
				return createHtmlResponse(500, "Internal Server Error");
			case 2:
				return createHtmlResponse(405, "Method Not Allowed");
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

		return createHtmlResponse(200, output);
	}
	else
	{
		std::cerr << DEBUG_MSG << "Failed to fork for CGI processing" << std::endl;
		return createHtmlResponse(500, "Internal Server Error");
	}
}







// return err code or 0
int	ConfigHandler::checkLocation(std::string &response, t_HttpRequest request, Server *matchedServer) const
{
	const Location *matchedLocation = matchRequestToLocation(request, matchedServer);
	std::cout << BCYN << "Matched location: " << matchedLocation << RESET << std::endl;
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
				std::cerr << "Allowed method: " << method << std::endl;
				std::cerr << "Request method: " << request.method << std::endl;
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

			// TODO: What is this block for? need to find out
			if (rootDirective != matchedServer->directives.end())
			{
				std::string rootDir = rootDirective->second;
				std::string filePath = rootDir + request.path;

				// TODO: get cgi file name may need to add file name by request in below function
				std::string cgiFileName = getCgiFileName(request.method);

				std::cout << BGRN << "rootDir: " << rootDir << RESET << std::endl;

				std::cerr << DEBUG_MSG << "File path: " << filePath << std::endl;

				// Check if the CGI script exists
				if (checkFileExist(filePath))
				{
					std::cerr << BYEL << DEBUG_MSG << "File exists, executing CGI" << RESET << std::endl;

					std::string executable = "/usr/bin/python3";

					// Construct the argument array for execve
					char * const args[] = { (char *)executable.c_str(), (char *)filePath.c_str(), NULL};

					// call python cgi
					response = callPythonCgi(args, environ);

				}
				else
				{
					std::cerr << DEBUG_MSG << "File does not exist" << std::endl;
					response = createHtmlResponse(404, "File not found");
				}

			}
		}
		else if (request.path.find("/infinite") != std::string::npos)
		{
			// infinite loop cgi file name
			std::string cgiFileName = this->_cwd + "/htdocs/cgi-bin/infinite.py";

			// get root directive
			std::map<std::string, std::string>::const_iterator rootDirective = matchedServer->directives.find("root");

			std::string rootDir = "/htdocs";
			if (rootDirective != matchedServer->directives.end())
			{
				rootDir = rootDirective->second;
			}

			// get cgi directive
			std::map<std::string, std::string>::const_iterator cgiDirective = matchedServer->directives.find("cgi");

			if (cgiDirective != matchedServer->directives.end())
			{
				cgiFileName = rootDir + "/htdocs" + cgiDirective->second;

			}
			std::cerr << BGRN << "cgiFileName: " << cgiFileName << RESET << std::endl;

			std::string executable = "/usr/bin/python3";

			char * const args[] = { (char *)executable.c_str(), (char *)cgiFileName.c_str(), NULL};

			// call python cgi
			response = callPythonCgi(args, NULL, 5);
			// response = createHtmlResponse(200, "Recieved");
		}
		else if (request.path.find("/upload/") != std::string::npos)
		{
			std::cerr << DEBUG_MSG << "Upload found" << std::endl;

			std::map<std::string, std::string>::const_iterator rootDirective = matchedServer->directives.find("root");

			std::string rootDir = "/htdocs";
			if (rootDirective != matchedServer->directives.end())
			{
				rootDir = rootDirective->second;
			}

			// construct map for envp
			std::map<std::string, std::string> cgiEnvpMap;
			std::string fileName;
			std::string absoluteFilePath;
			
			if (request.method == "DELETE")
			{
				// std::cout << "TEST PRINT LOC ======" << matchedLocation->path << std::endl;
				// cx if DELETE allow
				// where am i???
				
				// get file name from request.path (request.path = /upload/?file=filename)
				fileName = request.path.substr(request.path.find("file=") + 5);
				absoluteFilePath = rootDir + "/upload/" + fileName;
				if (!checkFileExist(absoluteFilePath))
				{
					response = createHtmlResponse(404, "File not found");
					return 1;
				}
			}
			else
			{
				fileName = request.headers["File-name"];
			}
			// Construct absolute file path
			absoluteFilePath = rootDir + "/upload/" + fileName;
			
			// get cgi file name
			std::string cgiFileName = getCgiFileName(request.method);
			std::string filePath = rootDir + "/cgi-bin" + cgiFileName;

			std::string executable = "/usr/bin/python3";

			std::cerr << DEBUG_MSG << "File path: " << filePath << std::endl;

			// set value in envp map
			cgiEnvpMap["FILE_PATH"] = absoluteFilePath;
			cgiEnvpMap["PATH_INFO"] = request.path;
			cgiEnvpMap["REQUEST_METHOD"] = request.method;
			cgiEnvpMap["UPLOAD_DIR"] = rootDir + "/upload";
			cgiEnvpMap["BODY"] = request.body;
			cgiEnvpMap["CONTENT_TYPE"] = request.headers["Content-Type"];

			// call python cgi
			char * const *cgiEnvp = createCgiEnvp(cgiEnvpMap);

			char * const args[] = { (char *)executable.c_str(), (char *)filePath.c_str(), NULL};
			// char * const args[2] = {const_cast<char *>(filePath.c_str()), NULL};

			try {
				response = callPythonCgi(args, cgiEnvp);
			}
			catch (std::exception &e) {
				std::cerr << BRED << "Error: " << e.what() << RESET << std::endl;
				response = createHtmlResponse(500, "Internal Server Error");
			}

			// response = callPythonCgi(filePath, request.body);

			std::cerr << DEBUG_MSG << "Response: " << response << std::endl;

			// Get the root directory from server configuration
			// response = createHtmlResponse(200, "Recieved");
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

	}
	return 0;
}
