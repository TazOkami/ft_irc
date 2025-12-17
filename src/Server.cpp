#include "Server.hpp"
#include "Utils.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <fcntl.h>
#include <cstdio>

// Initialize default sockets, containers, and a friendly server name.
Server::Server() : listenFd(-1), serverName("mini-irc") {}

// Build a reusable, non-blocking listening socket bound to the provided port.
static int createListen(unsigned short port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (!setNonBlocking(fd)) { perror("fcntl"); ::close(fd); return -1; }

    struct sockaddr_in addr; std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); ::close(fd); return -1; }
    if (listen(fd, SOMAXCONN) < 0) { perror("listen"); ::close(fd); return -1; }
    return fd;
}

// Cache the password, open the listening socket, and register it with the poller.
bool Server::start(unsigned short port, const std::string& pass) {
    password = pass;
    listenFd = createListen(port);
    if (listenFd < 0) return false;
    poller.add(listenFd, POLLIN);
    return true;
}

// Drive the poll loop, dispatching accepts/reads/writes and removing dead clients.
void Server::run() {
    std::cout << "[BOOT] " << serverName << " listening. PASS=" << (password.empty()?"<none>":"<set>") << std::endl;
    while (true) {
        int n = poller.wait(1000);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("poll"); break;
        }
        for (size_t i=0;i<poller.fds.size();++i) {
            struct pollfd &ev = poller.fds[i];

            if (ev.fd == listenFd && (ev.revents & POLLIN)) handleAccept();

            if (ev.fd != listenFd && (ev.revents & POLLIN)) handleRead(ev.fd);

            if (ev.fd != listenFd && (ev.revents & POLLOUT)) handleWrite(ev.fd);

            if (ev.fd != listenFd && (ev.revents & (POLLHUP|POLLERR|POLLNVAL))) disconnect(ev.fd, "connection lost");
        }
    }
}

// Disconnect a client, broadcast implicit PARTs, and purge the client from maps.
void Server::disconnect(int fd, const std::string& reason) {
    for (std::map<std::string,Channel>::iterator it = channels.begin(); it != channels.end();) {
        Channel &ch = it->second;
        if (ch.members.count(fd)) {
            std::string msg = prefixFor(fd) + " PART " + ch.name + " :" + reason;
            broadcast(ch.members, -1, msg);
            ch.members.erase(fd);
            ch.ops.erase(fd);
            if (ch.members.empty()) { channels.erase(it++); continue; }
        }
        ++it;
    }
    if (!clients[fd].nick.empty()) nickToFd.erase(toLower(clients[fd].nick));
    ::close(fd);
    poller.del(fd);
    clients.erase(fd);
    std::cout << "[-] client fd=" << fd << " disconnected (" << reason << ")\n";
}
