#include "Server.hpp"
#include <iostream>
#include <cstdlib>

// Entry point: validate CLI arguments, configure Server, and run until shutdown.
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: ./ircserv <port> <password>\n";
        return 1;
    }
    unsigned short port = static_cast<unsigned short>(std::atoi(argv[1]));
    std::string pass = argv[2];
    Server s;
    if (!s.start(port, pass)) {
        std::cerr << "Failed to start server.\n";
        return 1;
    }
    s.run();
    return 0;
}
