#include "Server.hpp"
#include "Numerics.hpp"
#include "Utils.hpp"
#include <sstream>

// Forward NOTICE data to channels or users without generating numeric errors.
void Server::h_NOTICE(int fd, const std::vector<std::string>& params, const std::string& trailing) {
    if (params.empty() || trailing.empty()) return;
    const std::string& target = params[0];
    const std::string msg = prefixFor(fd) + " NOTICE " + target + " :" + trailing;

    if (isChannelName(target)) {
        std::map<std::string, Channel>::iterator chit = channels.find(target);
        if (chit == channels.end()) return;
        Channel &ch = chit->second;
        if (!ch.members.count(fd)) return;
        broadcast(ch.members, fd, msg);
    } else {
        std::map<std::string,int>::iterator it = nickToFd.find(toLower(target));
        if (it == nickToFd.end()) return;
        queueLine(it->second, msg);
    }
}

// Handle WHO requests for channels (listing members) or individual nick targets.
void Server::h_WHO(int fd, const std::vector<std::string>& params) {
    std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
    if (params.empty()) {
        queueLine(fd, ":" + serverName + " " + RPL_ENDOFWHO + " " + me + " * :End of WHO list");
        return;
    }
    std::string name = params[0];
    if (isChannelName(name)) {
        if (channels.find(name) == channels.end()) {
            queueLine(fd, ":" + serverName + " " + RPL_ENDOFWHO + " " + me + " " + name + " :End of WHO list");
            return;
        }
        Channel &ch = channels[name];
        for (std::set<int>::iterator it = ch.members.begin(); it != ch.members.end(); ++it) {
            Client &m = clients[*it];
            std::string nick = m.nick.empty() ? "*" : m.nick;
            std::string user = m.user.empty() ? "user" : m.user;
            std::string host = serverName;
            std::string flags = "H";
            if (ch.ops.count(*it)) flags += "@";
            queueLine(fd, ":" + serverName + " " + RPL_WHOREPLY + " " + me + " " + ch.name + " " +
                           user + " " + host + " " + serverName + " " + nick + " " + flags + " :0 " +
                           (m.realname.empty() ? nick : m.realname));
        }
        queueLine(fd, ":" + serverName + " " + RPL_ENDOFWHO + " " + me + " " + ch.name + " :End of WHO list");
    } else {
        std::map<std::string,int>::iterator itfd = nickToFd.find(toLower(name));
        if (itfd != nickToFd.end()) {
            Client &m = clients[itfd->second];
            std::string nick = m.nick.empty() ? "*" : m.nick;
            std::string user = m.user.empty() ? "user" : m.user;
            std::string host = serverName;
            queueLine(fd, ":" + serverName + " " + RPL_WHOREPLY + " " + me + " * " +
                           user + " " + host + " " + serverName + " " + nick + " H :0 " +
                           (m.realname.empty() ? nick : m.realname));
        }
        queueLine(fd, ":" + serverName + " " + RPL_ENDOFWHO + " " + me + " " + name + " :End of WHO list");
    }
}

// Emit a minimal three-line MOTD sequence matching the subject's numerics.
void Server::h_MOTD(int fd) {
    std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
    queueLine(fd, ":" + serverName + " " + RPL_MOTDSTART + " " + me + " :- " + serverName + " Message of the day -");
    queueLine(fd, ":" + serverName + " " + RPL_MOTD      + " " + me + " :- No MOTD set");
    queueLine(fd, ":" + serverName + " " + RPL_ENDOFMOTD + " " + me + " :End of /MOTD command.");
}

// Send a simplified LUSERS summary using the 251 and 255 numerics.
void Server::h_LUSERS(int fd) {
    std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
    int users = (int)clients.size();
    {
        std::ostringstream oss;
        oss << "There are " << users << " users and 0 services on 1 servers";
        queueLine(fd, ":" + serverName + " " + RPL_LUSERCLIENT + " " + me + " :" + oss.str());
    }
    {
        std::ostringstream oss;
        oss << "I have " << users << " clients and 0 servers";
        queueLine(fd, ":" + serverName + " " + RPL_LUSERME + " " + me + " :" + oss.str());
    }
}

// Provide WHOIS responses with basic identity lines (311 + 318 numerics).
void Server::h_WHOIS(int fd, const std::vector<std::string>& params) {
    std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
    if (params.empty()) { sendNumeric(fd, ERR_NONICKNAMEGIVEN, "*", "WHOIS :No nickname given"); return; }
    std::string target = params[0];
    std::map<std::string,int>::iterator it = nickToFd.find(toLower(target));
    if (it == nickToFd.end()) {
        sendNumeric(fd, ERR_NOSUCHNICK, target, "No such nick");
        queueLine(fd, ":" + serverName + " " + RPL_ENDOFWHOIS + " " + me + " " + target + " :End of WHOIS list");
        return;
    }
    Client &u = clients[it->second];
    std::string nick = u.nick.empty() ? "*" : u.nick;
    std::string user = u.user.empty() ? "user" : u.user;
    std::string host = serverName;
    queueLine(fd, ":" + serverName + " " + RPL_WHOISUSER + " " + me + " " + nick + " " + user + " " + host + " * :" + 
                   (u.realname.empty() ? nick : u.realname));
    queueLine(fd, ":" + serverName + " " + RPL_ENDOFWHOIS + " " + me + " " + nick + " :End of WHOIS list");
}
