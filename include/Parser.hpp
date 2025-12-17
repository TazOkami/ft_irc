#ifndef PARSER_HPP
#define PARSER_HPP

#include "Types.hpp"

struct IrcMessage {
    std::string prefix;
    std::string command;
    std::vector<std::string> params;
    std::string trailing;
};

IrcMessage parseIrcLine(const std::string& raw);

#endif
