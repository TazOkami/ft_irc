#include "Server.hpp"
#include "Utils.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <cstdio>

// Accept as many pending connections as possible and initialize each Client struct.
void Server::handleAccept() {
    for (;;) {
        int cfd = ::accept(listenFd, NULL, NULL);
        if (cfd < 0) {
            if (errno==EAGAIN || errno==EWOULDBLOCK) break;
            perror("accept"); break;
        }
        setNonBlocking(cfd);
        poller.add(cfd, POLLIN);
        clients[cfd] = Client();
        if (!password.empty())
            queueLine(cfd, ":" + serverName + " NOTICE * :This server requires PASS before NICK/USER.");
        std::cout << "[+] client fd=" << cfd << " connected\n";
    }
}

// Flip the POLLIN flag for a tracked fd within the poller vector.
void Server::wantRead(int fd, bool yes) {
    for (size_t i=0;i<poller.fds.size();++i)
        if (poller.fds[i].fd == fd) {
            short e = poller.fds[i].events;
            if (yes) e |= POLLIN; else e &= ~POLLIN;
            poller.fds[i].events = e; return;
        }
}

// Flip the POLLOUT flag for a tracked fd within the poller vector.
void Server::wantWrite(int fd, bool yes) {
    for (size_t i=0;i<poller.fds.size();++i)
        if (poller.fds[i].fd == fd) {
            short e = poller.fds[i].events;
            if (yes) e |= POLLOUT; else e &= ~POLLOUT;
            poller.fds[i].events = e; return;
        }
}

// Queue an outgoing IRC line and pause reading if the buffer exceeds MAX_WRITEBUF.
void Server::queueLine(int fd, const std::string& line) {
    Client &c = clients[fd];
    c.writeBuf += line + "\r\n";
    wantWrite(fd, true);
    if (c.writeBuf.size() >= MAX_WRITEBUF) { c.pausedRead = true; wantRead(fd, false); }
}

// Flush as much of the pending write buffer as the kernel will accept, resuming reads when drained.
void Server::handleWrite(int fd) {
    Client &c = clients[fd];
    while (!c.writeBuf.empty()) {
        ssize_t s = ::send(fd, c.writeBuf.data(), c.writeBuf.size(), 0);
        if (s > 0) c.writeBuf.erase(0, (size_t)s);
        else if (s < 0 && (errno==EAGAIN || errno==EWOULDBLOCK)) break;
        else { disconnect(fd, "send error"); return; }
    }
    if (c.writeBuf.empty()) {
        wantWrite(fd, false);
        if (c.pausedRead) { c.pausedRead = false; wantRead(fd, true); }
    }
}

// Drain bytes from the socket into the client's read buffer, disconnecting on EOF/errors.
void Server::readFromClient(int fd, Client& c) {
    char tmp[4096];
    for (;;) {
        ssize_t n = ::recv(fd, tmp, sizeof(tmp), 0);
        if (n > 0) {
            c.readBuf.append(tmp, n);
            if (c.readBuf.size() > MAX_READBUF) { disconnect(fd, "flood"); return; }
        } else if (n == 0) { disconnect(fd, "quit"); return; }
        else {
            if (errno==EAGAIN || errno==EWOULDBLOCK) break;
            disconnect(fd, "recv error"); return;
        }
    }
}

// Split CRLF-delimited lines and push them to the dispatcher until no full line remains.
void Server::processIncomingLines(int fd, Client& c) {
    for (;;) {
        std::string::size_type lf = c.readBuf.find('\n');
        if (lf == std::string::npos) break;
        std::string raw = c.readBuf.substr(0, lf);
        c.readBuf.erase(0, lf+1);
        if (!raw.empty() && raw[raw.size()-1]=='\r') raw.erase(raw.size()-1);
        if (raw.empty()) continue;
        if (raw.size() + 2 > MAX_LINE_LEN) {
            continue;
        }
        dispatchLine(fd, raw);
        if (clients.find(fd) == clients.end()) return;
    }
}

// Read, parse, and process every available line for the given fd in the poller list.
void Server::handleRead(int fd) {
    std::map<int,Client>::iterator it = clients.find(fd);
    if (it == clients.end()) return;
    Client &c = it->second;

    readFromClient(fd, c);
    if (clients.find(fd) == clients.end()) return;

    processIncomingLines(fd, c);
}
