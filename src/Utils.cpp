#include "Utils.hpp"
#include <algorithm>
#include <cctype>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

// Return a lowercase copy of the provided ASCII string for case-insensitive lookups.
std::string toLower(const std::string& s) {
    std::string r = s;
    for (size_t i=0;i<r.size();++i) r[i] = (char)std::tolower(r[i]);
    return r;
}

// Trim leading/trailing spaces, tabs, and CR characters when sanitizing input tokens.
std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a<b && (s[a]==' ' || s[a]=='\t' || s[a]=='\r')) ++a;
    while (b>a && (s[b-1]==' ' || s[b-1]=='\t' || s[b-1]=='\r')) --b;
    return s.substr(a, b-a);
}

// Check if the token looks like a channel (leading '#', as mandated by the subject).
bool isChannelName(const std::string& s) {
    return !s.empty() && s[0] == '#';
}

// Put the descriptor into non-blocking mode using fcntl, the project-mandated approach.
bool setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0);
}
