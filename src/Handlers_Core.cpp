#include "Server.hpp"
#include "Numerics.hpp"
#include "Utils.hpp"
#include "Parser.hpp"
#include <sstream>



// Send a numeric reply with the standard server prefix and nickname target.
void Server::sendNumeric(int fd, const std::string& code, const std::string& target, const std::string& text) {
    queueLine(fd, ":" + serverName + " " + code + " " + (target.empty()?"*":target) + " :" + text);
}

// Check whether a lowercase nickname is already bound to a client fd.
bool Server::nickInUse(const std::string& nick) const {
    return nickToFd.find(toLower(nick)) != nickToFd.end();
}

// Update a client's nickname, enforcing uniqueness and refreshing lookup maps.
void Server::setNick(int fd, const std::string& nick) {
    if (nick.empty()) { sendNumeric(fd, ERR_NONICKNAMEGIVEN, "*", "No nickname given"); return; }
    if (nickInUse(nick)) { sendNumeric(fd, ERR_NICKNAMEINUSE, "*", nick + " :Nickname is already in use"); return; }

    Client &c = clients[fd];
    if (!c.nick.empty()) nickToFd.erase(toLower(c.nick));
    c.nick = nick;
    nickToFd[toLower(nick)] = fd;
}

// Build the nick!user@host prefix expected by downstream handlers.
std::string Server::prefixFor(int fd) const {
    std::map<int, Client>::const_iterator it = clients.find(fd);
    std::string nick = (it==clients.end() || it->second.nick.empty()) ? "unknown" : it->second.nick;
    std::string user = (it==clients.end() || it->second.user.empty()) ? "user" : it->second.user;
    return ":" + nick + "!" + user + "@" + serverName;
}

// Complete registration once PASS/NICK/USER are set, sending 001/002/003 numerics.
void Server::maybeWelcome(int fd) {
    Client &c = clients[fd];
    if (!password.empty() && !c.hasPass) return;
    if (!c.registered && !c.nick.empty() && !c.user.empty()) {
        c.registered = true;
        sendNumeric(fd, RPL_WELCOME, c.nick, "Welcome to " + serverName + ", " + c.nick + "!" + c.user + "@" + serverName);
        sendNumeric(fd, RPL_YOURHOST, c.nick, "Your host is " + serverName + ", running 0.1");
        sendNumeric(fd, RPL_CREATED, c.nick, "This server was created just now");
        queueLine(fd, ":" + serverName + " " + RPL_MYINFO + " " + c.nick + " " + serverName + " 0.1 o o");
    }
}

// Minimal CAP handler to satisfy clients that probe for IRCv3 capabilities.
void Server::h_CAP(int fd, const std::vector<std::string>& params, const std::string& trailing) {
    std::string nick = clients[fd].nick.empty() ? "*" : clients[fd].nick;
    if (!params.empty()) {
        std::string sub = toLower(params[0]);
        if (sub == "ls") {
            queueLine(fd, ":" + serverName + " CAP " + nick + " LS :");
            return;
        } else if (sub == "req") {
            queueLine(fd, ":" + serverName + " CAP " + nick + " NAK :" + trailing);
            return;
        } else if (sub == "end") {
            return;
        }
    }
    queueLine(fd, ":" + serverName + " CAP " + nick + " LS :");
}

// Store the PASS value, mark hasPass when valid, and block other commands otherwise.
void Server::h_PASS(int fd, const std::vector<std::string>& params) {
    if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "PASS :Not enough parameters"); return; }
    Client &c = clients[fd];
    if (c.registered) { sendNumeric(fd, ERR_ALREADYREGISTRED, "*", "You may not reregister"); return; }
    c.pass = params[0];
    if (password.empty() || c.pass == password) c.hasPass = true;
    else sendNumeric(fd, ERR_PASSWDMISMATCH, "*", "Password incorrect");
}

// Apply the requested nickname and trigger registration completion if possible.
void Server::h_NICK(int fd, const std::vector<std::string>& params) {
    if (params.empty()) { sendNumeric(fd, ERR_NONICKNAMEGIVEN, "*", "No nickname given"); return; }
    setNick(fd, params[0]);
    maybeWelcome(fd);
}

// Capture username/realname data and trigger registration completion if possible.
void Server::h_USER(int fd, const std::vector<std::string>& params, const std::string& trailing) {
    if (params.size() < 3) { sendNumeric(fd, ERR_NEEDMOREPARAMS, "*", "USER :Not enough parameters"); return; }
    Client &c = clients[fd];
    if (c.registered) { sendNumeric(fd, ERR_ALREADYREGISTRED, "*", "You may not reregister"); return; }
    c.user = params[0];
    c.realname = trailing;
    maybeWelcome(fd);
}

// Reply to PING probes to keep the connection alive and reassure clients.
void Server::h_PING(int fd, const std::vector<std::string>& params, const std::string& trailing) {
    std::string token = trailing.empty() ? (params.empty() ? "keepalive" : params[0]) : trailing;
    queueLine(fd, ":" + serverName + " PONG " + serverName + " :" + token);
}

// Close the connection, notifying peers of the quit reason.
void Server::h_QUIT(int fd, const std::string& trailing) {
    disconnect(fd, trailing.empty() ? "quit" : trailing);
}

// Deliver PRIVMSG payloads to channels or directly addressed users with error checks.
void Server::h_PRIVMSG(int fd, const std::vector<std::string>& params, const std::string& trailing) {
    if (params.empty()) { sendNumeric(fd, ERR_NORECIPIENT, "*", "No recipient given (PRIVMSG)"); return; }
    if (trailing.empty()) { sendNumeric(fd, ERR_NOTEXTTOSEND, "*", "No text to send"); return; }
    std::string target = params[0];
    std::string msg = prefixFor(fd) + " PRIVMSG " + target + " :" + trailing;

    if (isChannelName(target)) {
        if (channels.find(target) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, target, "No such channel"); return; }
        Channel &ch = channels[target];
        if (ch.members.count(fd) == 0) { sendNumeric(fd, ERR_NOTONCHANNEL, target, "You're not on that channel"); return; }
        broadcast(ch.members, fd, msg);
    } else {
        std::map<std::string,int>::iterator it = nickToFd.find(toLower(target));
        if (it == nickToFd.end()) { sendNumeric(fd, ERR_NOSUCHNICK, target, "No such nick"); return; }
        queueLine(it->second, msg);
    }
}
