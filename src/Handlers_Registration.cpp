#include "Server.hpp"
#include "Numerics.hpp"
#include "Utils.hpp"

#include <cctype>

// Nick helpers (registration-related).
bool isNickFirstChar(char ch) {
	if (std::isalnum(static_cast<unsigned char>(ch))) return true;
	switch (ch) {
		case '[': case ']': case '\\': case '`':
		case '_': case '^': case '{': case '|': case '}':
			return true;
		default:
			return false;
	}
}

bool isNickChar(char ch) {
	if (isNickFirstChar(ch)) return true;
	if (std::isdigit(static_cast<unsigned char>(ch))) return true;
	return ch == '-';
}

bool isValidNick(const std::string& nick) {
	if (nick.empty()) return false;
	if (!isNickFirstChar(nick[0])) return false;
	for (size_t i = 0; i < nick.size(); ++i) {
		unsigned char c = static_cast<unsigned char>(nick[i]);
		if (c <= 0x20 || c == 0x7F) return false;
		if (nick[i] == ':' || nick[i] == '#' || nick[i] == '!' || nick[i] == '@') return false;
		if (!isNickChar(nick[i])) return false;
	}
	return true;
}

// Finish registration once PASS/NICK/USER are set.
void Server::maybeWelcome(int fd) {
	Client &c = clients[fd];
	if (!password.empty() && !c.hasPass) return;
	if (!c.registered && !c.nick.empty() && !c.user.empty()) {
		c.registered = true;
		sendNumeric(fd, RPL_WELCOME, c.nick,
					"Welcome to the Internet Relay Network " + c.nick + "!" + c.user + "@" + serverName);
		sendNumeric(fd, RPL_YOURHOST, c.nick, "Your host is " + serverName + ", running version 0.1");
		sendNumeric(fd, RPL_CREATED, c.nick, "This server was created " + formatTime(startTime));
		queueLine(fd, ":" + serverName + " " + RPL_MYINFO + " " + c.nick + " " +
						  serverName + " 0.1 - itkoln");
		h_MOTD(fd); // Send MOTD upon welcome.
	}
}

// PASS handler.
void Server::h_PASS(int fd, const std::vector<std::string>& params) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " PASS", "Not enough parameters"); return; }
	Client &c = clients[fd];
	if (c.registered) { sendNumeric(fd, ERR_ALREADYREGISTRED, me, "You are already registered"); return; }
	c.pass = params[0];
	if (password.empty() || c.pass == password) {
		c.hasPass = true;
		maybeWelcome(fd);
	} else {
		sendNumeric(fd, ERR_PASSWDMISMATCH, me, "Password incorrect");
		disconnect(fd, "bad password");
	}
}

// NICK handler.
void Server::h_NICK(int fd, const std::vector<std::string>& params) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.empty()) { sendNumeric(fd, ERR_NONICKNAMEGIVEN, me, "No nickname given"); return; }
	setNick(fd, params[0]);
	maybeWelcome(fd);
}

// USER handler.
void Server::h_USER(int fd, const std::vector<std::string>& params, const std::string& trailing) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.size() < 3) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " USER", "Not enough parameters"); return; }
	Client &c = clients[fd];
	if (c.registered) { sendNumeric(fd, ERR_ALREADYREGISTRED, me, "You are already registered"); return; }
	c.user = params[0];
	c.realname = trailing;
	maybeWelcome(fd);
}

// QUIT handler.
void Server::h_QUIT(int fd, const std::string& trailing) {
	disconnect(fd, trailing.empty() ? "quit" : trailing);
}

// Is this nick already taken?
bool Server::nickInUse(const std::string& nick) const {
	return nickToFd.find(toLower(nick)) != nickToFd.end();
}

// Set/replace nickname and update lookup map.
void Server::setNick(int fd, const std::string& nick) {
	Client &c = clients[fd];
	std::string me = c.nick.empty() ? "*" : c.nick;
	if (nick.empty()) { sendNumeric(fd, ERR_NONICKNAMEGIVEN, me, "No nickname given"); return; }
	if (!isValidNick(nick)) { sendNumeric(fd, ERR_ERRONEUSNICKNAME, me + " " + nick, "Erroneous nickname"); return; }

	std::string want = toLower(nick);
	std::string cur = toLower(c.nick);
	if (!c.nick.empty() && cur == want) return;

	std::map<std::string,int>::iterator taken = nickToFd.find(want);
	if (taken != nickToFd.end() && taken->second != fd) {
		sendNumeric(fd, ERR_NICKNAMEINUSE, me + " " + nick, "Nickname is already in use");
		return;
	}

	std::string oldNick = c.nick;
	if (!c.nick.empty()) nickToFd.erase(cur);
	c.nick = nick;
	nickToFd[want] = fd;

	if (c.registered && !oldNick.empty()) {
		std::set<int> notify;
		for (std::map<std::string,Channel>::iterator it = channels.begin(); it != channels.end(); ++it) {
			Channel &ch = it->second;
			if (!ch.members.count(fd)) continue;
			for (std::set<int>::iterator m = ch.members.begin(); m != ch.members.end(); ++m) notify.insert(*m);
		}
		if (!notify.empty()) {
			std::string msg = ":" + oldNick + "!" + c.user + "@" + serverName + " NICK :" + c.nick;
			broadcast(notify, -1, msg);
		}
	}
}
