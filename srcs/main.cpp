#include <iostream>
#include "config_parser.hpp"

int	main (int argc, char **argv)
{
	if (argc <= 2)
	{
		std::string fileName = argv[1] ? argv[1] : "default.conf";
		// Construct configPasrser with specified conf file
		ConfigParser configParser = ConfigParser( fileName );
		configParser.printData();
	}
	else
	{
		std::cerr << "Parameters are more than 2" << std::endl;
	}

	return 0;
}

// #include <iostream>
// #include <cstring>
// #include <cstdlib>
// #include <cstdio>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>

// #define PORT 8080
// #define MAX_CLIENTS 30

// int main() {
//     int master_socket, addrlen, new_socket, client_socket[MAX_CLIENTS],
//         activity, i, valread, sd;
//     int max_sd;
//     struct sockaddr_in address;

//     char buffer[1025];  // data buffer of 1K

//     // set of socket descriptors
//     fd_set readfds;

//     // initialize all client_socket[] to 0 so not checked
//     for (i = 0; i < MAX_CLIENTS; i++) {
//         client_socket[i] = 0;
//     }

//     // create a master socket
//     if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
//         perror("socket failed");
//         exit(EXIT_FAILURE);
//     }

//     // set master socket to allow multiple connections
//     int opt = 1;
//     if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1) {
//         perror("setsockopt");
//         exit(EXIT_FAILURE);
//     }

//     // type of socket created
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     // bind the socket to localhost port 8080
//     if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) == -1) {
//         perror("bind failed");
//         exit(EXIT_FAILURE);
//     }
//     printf("Listener on port %d \n", PORT);

//     // try to specify maximum of 3 pending connections for the master socket
//     if (listen(master_socket, 3) == -1) {
//         perror("listen");
//         exit(EXIT_FAILURE);
//     }

//     // accept the incoming connection
//     addrlen = sizeof(address);
//     puts("Waiting for connections ...");

//     while (true) {
//         FD_ZERO(&readfds);

//         FD_SET(master_socket, &readfds);
//         max_sd = master_socket;

//         for (i = 0; i < MAX_CLIENTS; i++) {
//             sd = client_socket[i];

//             if (sd > 0) {
//                 FD_SET(sd, &readfds);
//             }

//             if (sd > max_sd) {
//                 max_sd = sd;
//             }
//         }

//         activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

//         if ((activity < 0) && (errno != EINTR)) {
//             printf("select error");
//         }

//         if (FD_ISSET(master_socket, &readfds)) {
//             if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) == -1) {
//                 perror("accept");
//                 exit(EXIT_FAILURE);
//             }

//             printf("New connection, socket fd is %d, ip is : %s, port : %d\n",
//                    new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

//             const char *message = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 83\r\n\n<html><body><h1>Hello, this is a simple web server!</h1></body></html>";
//             send(new_socket, message, strlen(message), 0);
//             close(new_socket);
//         }
//     }

//     return 0;
// }
