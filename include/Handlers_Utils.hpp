#ifndef HANDLERUTILS_HPP
#define HANDLERUTILS_HPP

#include <string>
#include <vector>

bool isNickFirstChar(char ch);
bool isNickChar(char ch);
bool isValidNick(const std::string& nick);
std::vector<std::string> splitComma(const std::string& s);

#endif
