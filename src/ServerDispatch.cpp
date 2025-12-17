#include "Server.hpp"
#include "Parser.hpp"
#include "Utils.hpp"
#include "Numerics.hpp"

#include <iostream>

// Parse a raw IRC line, enforce PASS gating, and forward to the right handler.
void Server::dispatchLine(int fd, const std::string& line) {
    IrcMessage m = parseIrcLine(line);
    std::string cmd = toLower(m.command);

    std::cerr << "[DBG] fd=" << fd
            << " hasPass=" << (clients[fd].hasPass ? "Y" : "N")
            << " reg=" << (clients[fd].registered ? "Y" : "N")
            << " cmd=" << cmd
            << " line=\"" << line << "\""
            << std::endl;

    if (!password.empty() && !clients[fd].hasPass) {
        if (cmd != "pass" && cmd != "ping" && cmd != "quit" && cmd != "cap") {
            sendNumeric(fd, "451", "*", "You must PASS first");
            return;
        }
    }

    if (cmd == "cap")       h_CAP(fd, m.params, m.trailing);
    else if (cmd == "pass") h_PASS(fd, m.params);
    else if (cmd == "nick") h_NICK(fd, m.params);
    else if (cmd == "user") h_USER(fd, m.params, m.trailing);
    else if (cmd == "ping") h_PING(fd, m.params, m.trailing);
    else if (cmd == "privmsg") h_PRIVMSG(fd, m.params, m.trailing);
    else if (cmd == "notice")  h_NOTICE(fd, m.params, m.trailing);
    else if (cmd == "join") h_JOIN(fd, m.params);
    else if (cmd == "part") h_PART(fd, m.params, m.trailing);
    else if (cmd == "names") h_NAMES(fd, m.params);
    else if (cmd == "topic") h_TOPIC(fd, m.params, m.trailing);
    else if (cmd == "mode") h_MODE(fd, m.params, m.trailing);
    else if (cmd == "invite") h_INVITE(fd, m.params);
    else if (cmd == "kick") h_KICK(fd, m.params, m.trailing);
    else if (cmd == "quit") h_QUIT(fd, m.trailing);
    else if (cmd == "who")   h_WHO(fd, m.params);
    else if (cmd == "motd")  h_MOTD(fd);
    else if (cmd == "lusers") h_LUSERS(fd);
    else if (cmd == "whois") h_WHOIS(fd, m.params);

    else sendNumeric(fd, "421", m.command, "Unknown command");
}
