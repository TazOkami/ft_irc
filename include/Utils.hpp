#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

std::string toLower(const std::string& s);
std::string trim(const std::string& s);
bool        isChannelName(const std::string& s);
bool        setNonBlocking(int fd);

#endif
