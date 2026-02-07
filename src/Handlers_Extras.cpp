#include "Server.hpp"
#include "Numerics.hpp"
#include "Utils.hpp"
#include <sstream>

// NOTICE handler (no error numerics).
void Server::h_NOTICE(int fd, const std::vector<std::string>& params, const std::string& trailing) {
	if (params.empty() || trailing.empty()) return;
	const std::string& target = params[0];
	const std::string msg = prefixFor(fd) + " NOTICE " + target + " :" + trailing;

	if (isChannelName(target)) {
		std::map<std::string, Channel>::iterator chit = channels.find(target);
		if (chit == channels.end()) return;
		Channel &ch = chit->second;
		if (!ch.members.count(fd) && ch.noExternal) return;
		broadcast(ch.members, fd, msg);
	} else {
		std::map<std::string,int>::iterator it = nickToFd.find(toLower(target));
		if (it == nickToFd.end()) return;
		queueLine(it->second, msg);
	}
}

// Minimal CAP support (enough for common clients).
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

// PING -> PONG.
void Server::h_PING(int fd, const std::vector<std::string>& params, const std::string& trailing) {
	if (params.empty() && trailing.empty()) { sendNumeric(fd, ERR_NOORIGIN, clients[fd].nick.empty() ? "*" : clients[fd].nick, "No origin specified"); return; }
	std::string token = trailing.empty() ? params[0] : trailing;
	queueLine(fd, ":" + serverName + " PONG " + serverName + " :" + token);
}

// Client PONG (we don't track keepalives yet).
void Server::h_PONG(int fd, const std::vector<std::string>& params, const std::string& trailing) {
	(void)fd;
	(void)params;
	(void)trailing;
}

// PART handler (extra).
void Server::h_PART(int fd, const std::vector<std::string>& params, const std::string& trailing) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " PART", "Not enough parameters"); return; }
	std::string name = params[0];
	if (channels.find(name) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, me + " " + name, "No such channel"); return; }
	Channel &ch = channels[name];
	if (ch.members.count(fd) == 0) { sendNumeric(fd, ERR_NOTONCHANNEL, me + " " + name, "You're not on that channel"); return; }

	std::string msg = prefixFor(fd) + " PART " + name + " :" + (trailing.empty() ? "leaving" : trailing);
	broadcast(ch.members, -1, msg);

	ch.members.erase(fd);
	ch.ops.erase(fd);
	if (ch.members.empty()) channels.erase(name);
}

// NAMES handler (extra).
void Server::h_NAMES(int fd, const std::vector<std::string>& params) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.empty()) {
		if (channels.empty()) {
			std::string nick = clients[fd].nick.empty() ? "*" : clients[fd].nick;
			queueLine(fd, ":" + serverName + " " + RPL_ENDOFNAMES + " " + nick + " * :End of /NAMES list.");
			return;
		}
		for (std::map<std::string,Channel>::iterator it = channels.begin(); it != channels.end(); ++it) {
			sendNames(fd, it->second);
		}
		return;
	}
	std::string name = params[0];
	if (channels.find(name) == channels.end()) {
		std::string nick = clients[fd].nick.empty() ? "*" : clients[fd].nick;
		queueLine(fd, ":" + serverName + " " + RPL_ENDOFNAMES + " " + nick + " " + name + " :End of /NAMES list.");
		return;
	}
	sendNames(fd, channels[name]);
}

// WHO handler.
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

// MOTD handler.
void Server::h_MOTD(int fd) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	queueLine(fd, ":" + serverName + " " + RPL_MOTDSTART + " " + me + " :Start of /MOTD command.");
	queueLine(fd, ":" + serverName + " " + RPL_MOTD      + " " + me + " :- ------------------------------");
	queueLine(fd, ":" + serverName + " " + RPL_MOTD      + " " + me + " :- Welcome to " + serverName + " (ft_irc)");
	queueLine(fd, ":" + serverName + " " + RPL_MOTD      + " " + me + " :- Modes: +i +t +k +l +o +n");
	queueLine(fd, ":" + serverName + " " + RPL_MOTD      + " " + me + " :- Tip: /join #chan  |  /help");
	queueLine(fd, ":" + serverName + " " + RPL_MOTD      + " " + me + " :- ------------------------------");
	queueLine(fd, ":" + serverName + " " + RPL_ENDOFMOTD + " " + me + " :End of /MOTD command.");
}

// LUSERS handler.
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

// WHOIS handler.
void Server::h_WHOIS(int fd, const std::vector<std::string>& params) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.empty()) { sendNumeric(fd, ERR_NONICKNAMEGIVEN, me, "No nickname given"); return; }
	std::string target = params[0];
	std::map<std::string,int>::iterator it = nickToFd.find(toLower(target));
	if (it == nickToFd.end()) {
		sendNumeric(fd, ERR_NOSUCHNICK, me + " " + target, "No such nick");
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
