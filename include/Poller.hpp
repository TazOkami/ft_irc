#ifndef POLLER_HPP
#define POLLER_HPP

#include <vector>
#include <poll.h>

class Poller {
public:
	std::vector<struct pollfd> fds;

	void add(int fd, short events);
	void mod(int fd, short events);
	void del(int fd);
	int  wait(int timeout_ms);
};

#endif
