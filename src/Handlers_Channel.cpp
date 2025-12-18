#include "Server.hpp"
#include "Numerics.hpp"
#include "Utils.hpp"

// Return an existing channel or create/store a fresh Channel struct when missing.
Channel& Server::getOrCreateChan(const std::string& name) {
    if (channels.find(name) == channels.end()) {
        Channel ch; ch.name = name;
        channels[name] = ch;
    }
    return channels[name];
}

// Send the current channel roster (including operator prefixes) via RPL_NAMREPLY.
void Server::sendNames(int fd, Channel& ch) {
    std::string list;
    for (std::set<int>::iterator it = ch.members.begin(); it != ch.members.end(); ++it) {
        Client &m = clients[*it];
        bool isOp = ch.ops.count(*it) != 0;
        if (!list.empty()) list += " ";
        list += (isOp ? "@" : "") + (m.nick.empty() ? std::string("*") : m.nick);
    }
    std::string nick = clients[fd].nick.empty() ? "*" : clients[fd].nick;
    queueLine(fd, ":" + serverName + " " + RPL_NAMREPLY + " " + nick + " = " + ch.name + " :" + list);
    queueLine(fd, ":" + serverName + " " + RPL_ENDOFNAMES + " " + nick + " " + ch.name + " :End of /NAMES list.");
}

// Broadcast a fully formed IRC line to every member (-1) except an optional sender.
void Server::broadcast(const std::set<int>& targets, int excludeFd, const std::string& line) {
    for (std::set<int>::const_iterator it=targets.begin(); it!=targets.end(); ++it) {
        if (*it == excludeFd) continue;
        queueLine(*it, line);
    }
}

// Process JOIN requests, enforcing invite/key/limit rules and broadcasting entry.
void Server::h_JOIN(int fd, const std::vector<std::string>& params) {
    if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "JOIN :Not enough parameters"); return; }
    std::string name = params[0];
    std::string key  = (params.size() >= 2) ? params[1] : "";

    if (!isChannelName(name)) { sendNumeric(fd, ERR_NOSUCHCHANNEL, name, "Illegal channel name"); return; }
    Channel &ch = getOrCreateChan(name);

    Client &c = clients[fd];
    if (ch.inviteOnly && ch.invited.count(toLower(c.nick)) == 0) {
        sendNumeric(fd, ERR_INVITEONLYCHAN, name, "Invite-only channel"); return;
    }
    if (ch.hasKey && ch.key != key) {
        sendNumeric(fd, ERR_BADCHANNELKEY, name, "Cannot join channel (+k)"); return;
    }
    if (ch.hasLimit && (int)ch.members.size() >= ch.limit) {
        sendNumeric(fd, ERR_CHANNELISFULL, name, "Cannot join channel (+l)");
        return;
    }
    bool wasEmpty = ch.members.empty();
    ch.members.insert(fd);
    if (wasEmpty) ch.ops.insert(fd);

    std::string line = prefixFor(fd) + " JOIN " + name;
    broadcast(ch.members, -1, line);
    if (ch.topic.empty()) queueLine(fd, ":" + serverName + " " + RPL_NOTOPIC + " " + c.nick + " " + name + " :No topic is set");
    else                  queueLine(fd, ":" + serverName + " " + RPL_TOPIC   + " " + c.nick + " " + name + " :" + ch.topic);
    sendNames(fd, ch);
}

// Process PART requests, notifying members and deleting empty channels.
void Server::h_PART(int fd, const std::vector<std::string>& params, const std::string& trailing) {
    if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "PART :Not enough parameters"); return; }
    std::string name = params[0];
    if (channels.find(name) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, name, "No such channel"); return; }
    Channel &ch = channels[name];
    if (ch.members.count(fd) == 0) { sendNumeric(fd, ERR_NOTONCHANNEL, name, "You're not on that channel"); return; }

    std::string msg = prefixFor(fd) + " PART " + name + " :" + (trailing.empty() ? "leaving" : trailing);
    broadcast(ch.members, -1, msg);

    ch.members.erase(fd);
    ch.ops.erase(fd);
    if (ch.members.empty()) channels.erase(name);
}

// Answer NAMES queries for known channels or emit an immediate end marker.
void Server::h_NAMES(int fd, const std::vector<std::string>& params) {
    if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "NAMES :Not enough parameters"); return; }
    std::string name = params[0];
    if (channels.find(name) == channels.end()) {
        std::string nick = clients[fd].nick.empty() ? "*" : clients[fd].nick;
        queueLine(fd, ":" + serverName + " " + RPL_ENDOFNAMES + " " + nick + " " + name + " :End of /NAMES list.");
        return;
    }
    sendNames(fd, channels[name]);
}
