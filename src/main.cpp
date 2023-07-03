#include <iostream>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>

std::vector<int> client_sockets;

void handleClient(int client_socket) {
	const char* response = "[Message received by server]";

	while (true) {
		size_t msg_size;
		read(client_socket, &msg_size, sizeof(size_t));
		char* buffer;
		read(client_socket, buffer, msg_size);

		// Broadcast message to all connected clients
		std::cout << buffer << std::endl;
		for (int i = 0; i < client_sockets.size(); ++i) {
			if (client_sockets[i] != client_socket) {
				send(client_sockets[i], buffer, strlen(buffer), 0);
			}
		}

		send(client_socket, response, strlen(response), 0);
	}

	close(client_socket);
	for (auto it = client_sockets.begin(); it != client_sockets.end(); ++it) {
		if (*it == client_socket) {
			client_sockets.erase(it);
			break;
		}
	}
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
		return 1;
	}

	int server_fd, new_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY; // Bind to any available interface
	address.sin_port = htons(std::stoi(argv[1])); // Port number from command line argument

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 3) < 0) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	while (true) {
		if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept failed");
			exit(EXIT_FAILURE);
		}
		
		client_sockets.push_back(new_socket);

		std::thread client_thread(handleClient, new_socket);
		client_thread.detach();
	}

	return 0;
}

