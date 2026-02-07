#include "Server.hpp"
#include "Utils.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <cstdio>
#include <ctime>

Server::Server() : listenFd(-1), serverName("mini-irc"), startTime(0) {}

// Create the listening socket.
static int createListen(unsigned short port) {
	int fd = ::socket(AF_INET, SOCK_STREAM, 0);                     // Create IPv4 TCP socket
	if (fd < 0) { perror("socket"); return -1; }          

	int yes = 1;                                                    // Enable reuse of local address
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	if (!setNonBlocking(fd)) {                                      // Put socket into non-blocking mode
		perror("fcntl");                                 
		::close(fd);                                      
		return -1;                                        
	}

	struct sockaddr_in addr; std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;                                      // IPv4
	addr.sin_addr.s_addr = htonl(INADDR_ANY);                       // Bind to all local interfaces (0.0.0.0)
	addr.sin_port = htons(port);                                    // Set listen port (network byte order)

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {      // Bind address/port
		perror("bind");
		::close(fd);
		return -1;
	}
	if (listen(fd, SOMAXCONN) < 0) {                                // Mark socket as listening
		perror("listen");
		::close(fd);
		return -1;
	}
	return fd;
}

// Start listening.
bool Server::start(unsigned short port, const std::string& pass) {
	password = pass;
	startTime = std::time(NULL);
	listenFd = createListen(port);
	if (listenFd < 0) return false;
	poller.add(listenFd, POLLIN);
	return true;
}

// Main poll loop.
void Server::run() {
	std::cout << "[BOOT] " << serverName << " listening. PASS=" << (password.empty()?"<none>":"<set>") << std::endl;
	while (true) {
		int n = poller.wait(1000);
		if (n < 0) {
			if (errno == EINTR) continue;
			perror("poll"); break;
		}
		// poller.fds can be mutated by accept/disconnect; iterate on a snapshot.
		std::vector<struct pollfd> fired = poller.fds;
		for (size_t i=0;i<fired.size();++i) {
			const struct pollfd ev = fired[i];

			if (ev.fd == listenFd && (ev.revents & POLLIN)) {
				handleAccept();
				continue;
			}
			if (ev.fd == listenFd) continue;

			if (ev.revents & (POLLHUP|POLLERR|POLLNVAL)) {
				if (clients.find(ev.fd) != clients.end()) disconnect(ev.fd, "connection lost");
				continue;
			}
			if (ev.revents & POLLIN) handleRead(ev.fd);
			if (ev.revents & POLLOUT) handleWrite(ev.fd);
		}
	}
}

// Drop a client and notify peers.
void Server::disconnect(int fd, const std::string& reason) {
	std::set<int> notify;
	std::string quitMsg = prefixFor(fd) + " QUIT :" + reason;
	for (std::map<std::string,Channel>::iterator it = channels.begin(); it != channels.end();) {
		Channel &ch = it->second;
		if (ch.members.count(fd)) {
			for (std::set<int>::iterator m = ch.members.begin(); m != ch.members.end(); ++m) {
				if (*m != fd) notify.insert(*m);
			}
			ch.members.erase(fd);
			ch.ops.erase(fd);
			if (ch.members.empty()) { channels.erase(it++); continue; }
		}
		++it;
	}
	if (!notify.empty()) broadcast(notify, -1, quitMsg);
	if (!clients[fd].nick.empty()) nickToFd.erase(toLower(clients[fd].nick));
	::close(fd);
	poller.del(fd);
	clients.erase(fd);
	std::cout << "[-] client fd=" << fd << " disconnected (" << reason << ")\n";
}
