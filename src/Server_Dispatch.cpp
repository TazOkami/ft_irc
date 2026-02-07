#include "Server.hpp"
#include "Parser.hpp"
#include "Utils.hpp"
#include <iostream>

static void logDispatch(int fd, const Client& c, const std::string& cmd, const std::string& line) {
	std::cerr << "[DBG] fd=" << fd
			  << " hasPass=" << (c.hasPass ? "Y" : "N")
			  << " reg=" << (c.registered ? "Y" : "N")
			  << " cmd=" << cmd
			  << " line=\"" << line << "\""
			  << std::endl;
}

static bool isAllowedBeforePass(const std::string& cmd) {
	return cmd == "pass" || cmd == "ping" || cmd == "pong" || cmd == "quit" || cmd == "cap";
}

static bool isAllowedBeforeRegister(const std::string& cmd) {
	return cmd == "pass" || cmd == "nick" || cmd == "user" || cmd == "ping" || cmd == "pong" || cmd == "quit" || cmd == "cap";
}

// Parse one IRC line, apply PASS gating, then dispatch.
void Server::dispatchLine(int fd, const std::string& line) {
	IrcMessage m = parseIrcLine(line);
	std::string cmd = toLower(m.command);
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;

	logDispatch(fd, clients[fd], cmd, line);

	if (!password.empty() && !clients[fd].hasPass) {
		if (!isAllowedBeforePass(cmd)) {
			sendNumeric(fd, "451", me, "You must PASS first");
			return;
		}
	}

	if (!clients[fd].registered) {
		if (!isAllowedBeforeRegister(cmd)) {
			sendNumeric(fd, "451", me, "You have not registered");
			return;
		}
	}

	if (cmd == "cap")       h_CAP(fd, m.params, m.trailing);
	else if (cmd == "pass") h_PASS(fd, m.params);
	else if (cmd == "nick") h_NICK(fd, m.params);
	else if (cmd == "user") h_USER(fd, m.params, m.trailing);
	else if (cmd == "ping") h_PING(fd, m.params, m.trailing);
	else if (cmd == "pong") h_PONG(fd, m.params, m.trailing);
	else if (cmd == "privmsg") h_PRIVMSG(fd, m.params, m.trailing);
	else if (cmd == "notice")  h_NOTICE(fd, m.params, m.trailing);
	else if (cmd == "join") h_JOIN(fd, m.params);
	else if (cmd == "part") h_PART(fd, m.params, m.trailing);
	else if (cmd == "names") h_NAMES(fd, m.params);
	else if (cmd == "topic") h_TOPIC(fd, m.params, m.trailing, m.hasTrailing);
	else if (cmd == "mode") h_MODE(fd, m.params, m.trailing);
	else if (cmd == "invite") h_INVITE(fd, m.params);
	else if (cmd == "kick") h_KICK(fd, m.params, m.trailing);
	else if (cmd == "quit") h_QUIT(fd, m.trailing);
	else if (cmd == "who")   h_WHO(fd, m.params);
	else if (cmd == "motd")  h_MOTD(fd);
	else if (cmd == "lusers") h_LUSERS(fd);
	else if (cmd == "whois") h_WHOIS(fd, m.params);

	else sendNumeric(fd, "421", me + " " + m.command, "Unknown command");
}
