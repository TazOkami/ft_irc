#include "Utils.hpp"
#include <cctype>
#include <fcntl.h>

// Lowercase helper (ASCII).
std::string toLower(const std::string& s) {
	std::string r = s;
	for (size_t i=0;i<r.size();++i) r[i] = (char)std::tolower(static_cast<unsigned char>(r[i]));
	return r;
}

// Trim spaces/tabs/CR.
std::string trim(const std::string& s) {
	size_t a = 0, b = s.size();
	while (a<b && (s[a]==' ' || s[a]=='\t' || s[a]=='\r')) ++a;
	while (b>a && (s[b-1]==' ' || s[b-1]=='\t' || s[b-1]=='\r')) --b;
	return s.substr(a, b-a);
}

// Channel name check (starts with '#', no spaces/commas/colons, length >= 2).
bool isChannelName(const std::string& s) {
	if (s.size() < 2 || s[0] != '#') return false;
	for (size_t i = 1; i < s.size(); ++i) {
		unsigned char c = static_cast<unsigned char>(s[i]);
		if (c <= 0x20 || c == 0x7F) return false;
		if (c == ',' || c == ':' || c == 0x07) return false;
	}
	return true;
}

// Set non-blocking.
bool setNonBlocking(int fd) {
	return (fcntl(fd, F_SETFL, O_NONBLOCK) == 0);
}

// Format time for IRC welcome.
std::string formatTime(std::time_t t) {
	char buf[64];
	std::tm* tm = std::localtime(&t);
	if (!tm) return "unknown";
	if (std::strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y %Z", tm) == 0) return "unknown";
	return std::string(buf);
}
