#include "config_handler.hpp"

// move server core here 


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


// ==================== EXECUTE ========================================================================



static ConfigHandler& instance() {
	static ConfigHandler configHandler;
	return configHandler;
}

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

// bool	ConfigHandler::matchPort()
// {

// }

// ============ EXECUTE ================

void ConfigHandler::run() const
{
	// handle signal
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGQUIT, signalHandler);

	std::cerr << DEBUG_MSG << "Executing config" << std::endl;

	fd_set readfds;
	std::vector<int> activeSockets;
	std::vector<int>::iterator it;

	int		resNum = 0;

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

				std::cout << BGRN << SVMSG << "WHAT WE GOT" << RESET << std::endl;
				std::cout << "Buffer: " << buffer << std::endl;
				std::cout << "END " << std::endl;

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
						
						std::cerr << "\033[1;31m" << "Matched server: " << matchedServer << "\033[0m" << std::endl;

						// if (!matchPort(request, matchedServer)) {
						// 	continue ;
						// }

						// If Server Matched, Check Location ...
						resNum = checkLocation(response, request, matchedServer);
						if (resNum == 1)
							continue ;
						
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
						response = createHtmlResponse(404, readHtmlFile(this->_cwd + "/htdocs/error/404.html"));
						// }
					}
					std::cout << "TEST-9999 RESPONSE == " << response << std::endl;
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


// bool		ConfigHandler::matchPort(t_HttpRequest request, Server *matchedServer)
// {

// }

