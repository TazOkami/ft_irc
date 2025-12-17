#include "Server.hpp"
#include "Numerics.hpp"
#include "Utils.hpp"

#include <cstdlib>
#include <sstream>

// Handle TOPIC queries and updates while enforcing operator-only edits.
void Server::h_TOPIC(int fd, const std::vector<std::string>& params, const std::string& trailing) {
    if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "TOPIC :Not enough parameters"); return; }
    std::string name = params[0];
    if (channels.find(name) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, name, "No such channel"); return; }
    Channel &ch = channels[name];

    if (trailing.empty()) {
        if (ch.topic.empty()) queueLine(fd, ":" + serverName + " " + RPL_NOTOPIC + " * " + name + " :No topic is set");
        else queueLine(fd, ":" + serverName + " " + RPL_TOPIC + " * " + name + " :" + ch.topic);
        return;
    }
    if (ch.topicOpOnly && ch.ops.count(fd) == 0) { sendNumeric(fd, ERR_CHANOPRIVSNEEDED, name, "You're not channel operator"); return; }

    ch.topic = trailing;
    std::string msg = prefixFor(fd) + " TOPIC " + name + " :" + ch.topic;
    broadcast(ch.members, -1, msg);
}

// Implement MODE queries and updates for channel flags and operator privileges.
void Server::h_MODE(int fd, const std::vector<std::string>& params, const std::string& trailing) {
    (void)trailing;
    if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "MODE :Not enough parameters"); return; }

    const std::string& target = params[0];

    if (!isChannelName(target)) { sendNumeric(fd, "502", target, "User mode not supported"); return; }

    if (params.size() == 1) {
        if (channels.find(target) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, target, "No such channel"); return; }
        Channel &ch = channels[target];

        std::string flags = "+";
        if (ch.inviteOnly)  flags += "i";
        if (ch.topicOpOnly) flags += "t";
        if (ch.hasKey)      flags += "k";
        if (ch.hasLimit)    flags += "l";

        bool isOp = ch.ops.count(fd) != 0;
        std::string args;
        if (ch.hasKey)   args += isOp ? (" " + ch.key) : " *";
        if (ch.hasLimit) {
            std::ostringstream oss; oss << ch.limit;
            args += " " + oss.str();
        }

        std::string nick = clients[fd].nick.empty() ? "*" : clients[fd].nick;
        queueLine(fd, ":" + serverName + " " + RPL_CHANNELMODEIS + " " + nick + " " + ch.name + " " + flags + args);
        return;
    }

    if (params.size() >= 2 && params[1] == "b") {
        std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
        queueLine(fd, ":" + serverName + " " + RPL_ENDOFBANLIST + " " + me + " " + target + " :End of Channel Ban List");
        return;
    }

    if (channels.find(target) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, target, "No such channel"); return; }
    Channel &ch = channels[target];

    if (!ch.members.count(fd)) { sendNumeric(fd, ERR_NOTONCHANNEL, target, "You're not on that channel"); return; }
    if (ch.ops.count(fd) == 0) { sendNumeric(fd, ERR_CHANOPRIVSNEEDED, target, "You're not channel operator"); return; }

    const std::string& mode = params[1];
    bool add = true;
    size_t pidx = 2;

    std::string changes;
    std::vector<std::string> margs;

    for (size_t i = 0; i < mode.size(); ++i) {
        char c = mode[i];
        if (c == '+') { add = true;  continue; }
        if (c == '-') { add = false; continue; }

        switch (c) {
            case 'i':
                ch.inviteOnly = add;
                changes += (add ? "+i" : "-i");
                break;

            case 't':
                ch.topicOpOnly = add;
                changes += (add ? "+t" : "-t");
                break;

            case 'k':
                if (add) {
                    if (pidx >= params.size()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "MODE +k requires key"); return; }
                    ch.hasKey = true;
                    ch.key = params[pidx++];
                    changes += "+k";
                    margs.push_back(ch.key);
                } else {
                    ch.hasKey = false;
                    ch.key.clear();
                    changes += "-k";
                }
                break;

            case 'l':
                if (add) {
                    if (pidx >= params.size()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "MODE +l requires limit"); return; }
                    const std::string& lims = params[pidx++];
                    char* end = NULL;
                    long lim = ::strtol(lims.c_str(), &end, 10);
                    if (!*lims.c_str() || (end && *end) || lim <= 0) {
                        sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "MODE +l requires positive integer");
                        return;
                    }
                    ch.hasLimit = true;
                    ch.limit = static_cast<int>(lim);
                    changes += "+l";
                    margs.push_back(lims);
                } else {
                    ch.hasLimit = false;
                    ch.limit = 0;
                    changes += "-l";
                }
                break;

            case 'o': {
                if (pidx >= params.size()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "MODE o requires nick"); return; }
                const std::string& nick = params[pidx++];
                std::map<std::string,int>::iterator it = nickToFd.find(toLower(nick));
                if (it == nickToFd.end() || ch.members.count(it->second) == 0) {
                    sendNumeric(fd, ERR_USERNOTINCHANNEL, nick, target + " :They aren't on that channel");
                    return;
                }
                int tfd = it->second;
                if (add) ch.ops.insert(tfd);
                else     ch.ops.erase(tfd);
                changes += (add ? "+o" : "-o");
                margs.push_back(nick);
                break;
            }

            default:
                sendNumeric(fd, ERR_UNKNOWNMODE, std::string(1, c), "is unknown mode char to me");
                return;
        }
    }

    if (!changes.empty()) {
        std::string msg = prefixFor(fd) + " MODE " + target + " " + changes;
        for (size_t i = 0; i < margs.size(); ++i) msg += " " + margs[i];
        broadcast(ch.members, -1, msg);
    }
}

// Handle INVITE by validating operator status and notifying the invited nick.
void Server::h_INVITE(int fd, const std::vector<std::string>& params) {
    if (params.size() < 2) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "INVITE :Not enough parameters"); return; }
    std::string nick = params[0];
    std::string name = params[1];
    if (channels.find(name) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, name, "No such channel"); return; }
    Channel &ch = channels[name];
    if (ch.ops.count(fd) == 0) { sendNumeric(fd, ERR_CHANOPRIVSNEEDED, name, "You're not channel operator"); return; }

    ch.invited.insert(toLower(nick));
    std::map<std::string,int>::iterator it = nickToFd.find(toLower(nick));
    if (it != nickToFd.end()) queueLine(it->second, prefixFor(fd) + " INVITE " + nick + " :" + name);
    queueLine(fd, ":" + serverName + " " + RPL_INVITING + " " + nick + " " + name);
}

// Handle KICK by ejecting the target and pruning channel membership maps.
void Server::h_KICK(int fd, const std::vector<std::string>& params, const std::string& trailing) {
    if (params.size() < 2) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "KICK :Not enough parameters"); return; }
    std::string name = params[0];
    std::string nick = params[1];
    if (channels.find(name) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, name, "No such channel"); return; }
    Channel &ch = channels[name];
    if (ch.ops.count(fd) == 0) { sendNumeric(fd, ERR_CHANOPRIVSNEEDED, name, "You're not channel operator"); return; }

    std::map<std::string,int>::iterator it = nickToFd.find(toLower(nick));
    if (it == nickToFd.end() || ch.members.count(it->second) == 0) {
        sendNumeric(fd, ERR_USERNOTINCHANNEL, nick + " " + name, "They aren't on that channel");
        return;
    }
    int vfd = it->second;
    std::string reason = trailing.empty() ? "kicked" : trailing;
    std::string msg = prefixFor(fd) + " KICK " + name + " " + nick + " :" + reason;
    broadcast(ch.members, -1, msg);

    ch.members.erase(vfd);
    ch.ops.erase(vfd);
    if (ch.members.empty()) channels.erase(name);
}
