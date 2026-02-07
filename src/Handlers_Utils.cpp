#include "Server.hpp"
#include "Numerics.hpp"
#include "Handlers_Utils.hpp"

std::vector<std::string> splitComma(const std::string& s) {
	std::vector<std::string> out;
	size_t start = 0;
	while (start <= s.size()) {
		size_t comma = s.find(',', start);
		if (comma == std::string::npos) comma = s.size();
		out.push_back(s.substr(start, comma - start));
		if (comma == s.size()) break;
		start = comma + 1;
	}
	return out;
}

// Helper for numeric replies.
void Server::sendNumeric(int fd, const std::string& code, const std::string& target, const std::string& text) {
	queueLine(fd, ":" + serverName + " " + code + " " + (target.empty()?"*":target) + " :" + text);
}

// Build standard IRC message prefix ":nick!user@server".
std::string Server::prefixFor(int fd) const {
	std::map<int, Client>::const_iterator it = clients.find(fd);
	std::string nick = (it==clients.end() || it->second.nick.empty()) ? "unknown" : it->second.nick;
	std::string user = (it==clients.end() || it->second.user.empty()) ? "user" : it->second.user;
	return ":" + nick + "!" + user + "@" + serverName;
}

// Get channel or create it.
Channel& Server::getOrCreateChan(const std::string& name) {
	if (channels.find(name) == channels.end()) {
		Channel ch; ch.name = name;
		channels[name] = ch;
	}
	return channels[name];
}

// Send NAMES list.
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

// Broadcast to a set of fds.
void Server::broadcast(const std::set<int>& targets, int excludeFd, const std::string& line) {
	for (std::set<int>::const_iterator it=targets.begin(); it!=targets.end(); ++it) {
		if (*it == excludeFd) continue;
		queueLine(*it, line);
	}
}

 
