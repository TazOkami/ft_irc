#include "Poller.hpp"
#include <cstddef>

// Add an fd to the poll set.
void Poller::add(int fd, short events)
{
	struct pollfd p;
	p.fd = fd;
	p.events = events;
	p.revents = 0;
	fds.push_back(p);
}

// Update events for an fd.
void Poller::mod(int fd, short events)
{
	for (std::size_t i = 0; i < fds.size(); ++i)
		if (fds[i].fd == fd)
		{
			fds[i].events = events;
			return;
		}
}

// Remove fd from poll set.
void Poller::del(int fd)
{
	for (std::size_t i = 0; i < fds.size(); ++i)
		if (fds[i].fd == fd)
		{
			fds.erase(fds.begin() + i);
			return;
		}
}

// poll(2) wrapper.
int Poller::wait(int timeout_ms)
{
	if (fds.empty()) { ::poll(NULL, 0, timeout_ms); return 0; }
	return ::poll(&fds[0], fds.size(), timeout_ms);
}
