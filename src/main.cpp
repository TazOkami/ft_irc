#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <csignal>

// Parse args and run the server.
int main(int argc, char** argv) {
	// Avoid crashing on SIGPIPE when writing to a disconnected client.
	::signal(SIGPIPE, SIG_IGN);

	if (argc < 3) {
		std::cerr << "Usage: ./ircserv <port> <password>\n";
		return 1;
	}
	std::string portStr = argv[1];
	if (portStr.empty()) {
		std::cerr << "Usage: ./ircserv <port> <password>\n";
		return 1;
	}
	for (size_t i = 0; i < portStr.size(); ++i) {
		if (!std::isdigit(static_cast<unsigned char>(portStr[i]))) {
			std::cerr << "Invalid port.\n";
			return 1;
		}
	}
	long portLong = std::strtol(portStr.c_str(), NULL, 10);
	if (portLong <= 0 || portLong > 65535) {
		std::cerr << "Invalid port.\n";
		return 1;
	}
	unsigned short port = static_cast<unsigned short>(portLong);
	std::string pass = argv[2];
	Server s;
	if (!s.start(port, pass)) {
		std::cerr << "Failed to start server.\n";
		return 1;
	}
	s.run();
	return 0;
}
