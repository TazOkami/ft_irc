#include "Poller.hpp"
#include <algorithm>

// Track a new file descriptor along with the events of interest inside poller state.
void Poller::add(int fd, short events) {
    struct pollfd p; p.fd = fd; p.events = events; p.revents = 0;
    fds.push_back(p);
}

// Update the subscribed events for an already tracked descriptor in place.
void Poller::mod(int fd, short events) {
    for (size_t i=0;i<fds.size();++i)
        if (fds[i].fd == fd) { fds[i].events = events; return; }
}

// Stop tracking the provided descriptor by removing it from the vector.
void Poller::del(int fd) {
    for (size_t i=0;i<fds.size();++i)
        if (fds[i].fd == fd) { fds.erase(fds.begin()+i); return; }
}

// Block in poll(2) until events fire or the timeout elapses; returns poll(2)'s result.
int Poller::wait(int timeout_ms) {
    return ::poll(&fds[0], fds.size(), timeout_ms);
}
