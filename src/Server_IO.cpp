#include "Server.hpp"
#include "Utils.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cctype>
#include <iostream>

static bool isNumericLine(const std::string& line) {
	if (line.size() < 6 || line[0] != ':') return false;
	size_t sp = line.find(' ');
	if (sp == std::string::npos || sp + 4 > line.size()) return false;
	size_t i = sp + 1;
	return std::isdigit(static_cast<unsigned char>(line[i])) &&
		   std::isdigit(static_cast<unsigned char>(line[i + 1])) &&
		   std::isdigit(static_cast<unsigned char>(line[i + 2])) &&
		   line[i + 3] == ' ';
}

// Accept pending connections.
void Server::handleAccept() {
	for (;;) {
		int cfd = ::accept(listenFd, NULL, NULL);
		if (cfd < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) return;
			::perror("accept");
			return;
		}
		setNonBlocking(cfd);
		poller.add(cfd, POLLIN);
		clients[cfd] = Client();
		std::cout << "[+] client fd=" << cfd << " connected\n";
	}
}

// Read available bytes into the client's buffer.
void Server::readFromClient(int fd, Client& c) {
	char tmp[4096];
	ssize_t n = ::recv(fd, tmp, sizeof(tmp), 0);
	if (n > 0) {
		c.readBuf.append(tmp, n);
		if (c.readBuf.size() > MAX_READBUF) { disconnect(fd, "flood"); return; }
	} else if (n == 0) {
		disconnect(fd, "quit");
		return;
	} else {
		if (errno==EAGAIN || errno==EWOULDBLOCK) return;
		disconnect(fd, "recv error");
		return;
	}
}

// Split complete lines and dispatch them.
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

// Read + parse everything available for this fd.
void Server::handleRead(int fd) {
	std::map<int,Client>::iterator it = clients.find(fd);
	if (it == clients.end()) return;
	Client &c = it->second;

	readFromClient(fd, c);
	if (clients.find(fd) == clients.end()) return;

	processIncomingLines(fd, c);
}

// Enable/disable POLLIN for a client fd.
void Server::wantRead(int fd, bool yes) {
	for (size_t i=0;i<poller.fds.size();++i)
		if (poller.fds[i].fd == fd) {
			short e = poller.fds[i].events;
			if (yes) e |= POLLIN; else e &= ~POLLIN;
			poller.fds[i].events = e; return;
		}
}

// Enable/disable POLLOUT for a client fd.
// POLLOUT is level-triggered: most TCP sockets are "writable" most of the time.
// If we keep POLLOUT enabled permanently, poll() will wake up on every loop
// even when we have nothing to send (busy loop). So we enable POLLOUT only
// when there is pending data in the write buffer, and disable it once drained.
void Server::wantWrite(int fd, bool yes) {
	for (size_t i=0;i<poller.fds.size();++i)
		if (poller.fds[i].fd == fd) {
			short e = poller.fds[i].events;
			if (yes) e |= POLLOUT; else e &= ~POLLOUT;
			poller.fds[i].events = e; return;
		}
}

// Queue an outgoing line (CRLF added).
// This is the switch that turns on POLLOUT: when we buffer data here, we
// subscribe to write readiness. When the buffer becomes empty again, we
// turn POLLOUT back off in handleWrite() to avoid constant wakeups.
void Server::queueLine(int fd, const std::string& line) {
	if (clients.find(fd) == clients.end()) return;
	if (isNumericLine(line)) {
		std::cerr << "[NUMERIC] " << line << std::endl;
	}
	Client &c = clients[fd];
	// Enforce IRC 512-byte max line length (CRLF included).
	std::string out = line;
	if (out.size() + 2 > MAX_LINE_LEN) {
		out.resize(MAX_LINE_LEN - 2);
	}
	if (c.writeBuf.size() + out.size() + 2 > MAX_WRITEBUF) {
		c.pausedRead = true;
		wantRead(fd, false);
		return;
	}
	c.writeBuf += out + "\r\n";
	wantWrite(fd, true);
	if (c.writeBuf.size() >= MAX_WRITEBUF) { c.pausedRead = true; wantRead(fd, false); }
}

// Flush pending writes.
// When the buffer drains, we disable POLLOUT again (no pending data).
void Server::handleWrite(int fd) {
	std::map<int,Client>::iterator it = clients.find(fd);
	if (it == clients.end()) return;
	Client &c = it->second;
	if (c.writeBuf.empty()) { wantWrite(fd, false); return; }

	ssize_t s = ::send(fd, c.writeBuf.data(), c.writeBuf.size(), 0);
	if (s > 0) c.writeBuf.erase(0, (size_t)s);
	else if (s < 0 && (errno==EAGAIN || errno==EWOULDBLOCK)) return;
	else { disconnect(fd, "send error"); return; }

	if (c.writeBuf.empty()) {
		wantWrite(fd, false);
		if (c.pausedRead) { c.pausedRead = false; wantRead(fd, true); }
	}
}
