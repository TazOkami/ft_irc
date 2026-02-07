#include "Server.hpp"
#include "Numerics.hpp"
#include "Utils.hpp"
#include "Handlers_Utils.hpp"

// JOIN handler.
void Server::h_JOIN(int fd, const std::vector<std::string>& params) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " JOIN", "Not enough parameters"); return; }
	if (params[0] == "0") {
		std::vector<std::string> toLeave;
		for (std::map<std::string,Channel>::iterator it = channels.begin(); it != channels.end(); ++it) {
			if (it->second.members.count(fd)) toLeave.push_back(it->first);
		}
		for (size_t i = 0; i < toLeave.size(); ++i) {
			const std::string& name = toLeave[i];
			Channel &ch = channels[name];
			std::string msg = prefixFor(fd) + " PART " + name + " :leaving";
			broadcast(ch.members, -1, msg);
			ch.members.erase(fd);
			ch.ops.erase(fd);
			if (ch.members.empty()) channels.erase(name);
		}
		return;
	}

	std::vector<std::string> names = splitComma(params[0]);
	std::vector<std::string> keys;
	if (params.size() >= 2) keys = splitComma(params[1]);

	for (size_t i = 0; i < names.size(); ++i) {
		std::string name = names[i];
		std::string key  = (i < keys.size()) ? keys[i] : "";

		if (!isChannelName(name)) { sendNumeric(fd, ERR_NOSUCHCHANNEL, me + " " + name, "Illegal channel name"); continue; }
		Channel &ch = getOrCreateChan(name);

		Client &c = clients[fd];
		if (ch.members.count(fd)) {
			sendNumeric(fd, ERR_USERONCHANNEL, me + " " + me + " " + name, "is already on channel");
			continue;
		}
		if (ch.inviteOnly && ch.invited.count(toLower(c.nick)) == 0) {
			sendNumeric(fd, ERR_INVITEONLYCHAN, me + " " + name, "Cannot join channel (+i)"); continue;
		}
		if (ch.hasKey && ch.key != key) {
			sendNumeric(fd, ERR_BADCHANNELKEY, me + " " + name, "Cannot join channel (+k)"); continue;
		}
		if (ch.hasLimit && (int)ch.members.size() >= ch.limit) {
			sendNumeric(fd, ERR_CHANNELISFULL, me + " " + name, "Cannot join channel (+l)");
			continue;
		}
		bool wasEmpty = ch.members.empty();
		ch.members.insert(fd);
		ch.invited.erase(toLower(c.nick));
		if (wasEmpty) ch.ops.insert(fd);

		std::string line = prefixFor(fd) + " JOIN " + name;
		broadcast(ch.members, -1, line);
		if (ch.topic.empty()) queueLine(fd, ":" + serverName + " " + RPL_NOTOPIC + " " + c.nick + " " + name + " :No topic is set");
		else                  queueLine(fd, ":" + serverName + " " + RPL_TOPIC   + " " + c.nick + " " + name + " :" + ch.topic);
		sendNames(fd, ch);
	}
}

// PRIVMSG handler.
void Server::h_PRIVMSG(int fd, const std::vector<std::string>& params, const std::string& trailing) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.empty()) { sendNumeric(fd, ERR_NORECIPIENT, me, "No recipient given (PRIVMSG)"); return; }
	if (trailing.empty()) { sendNumeric(fd, ERR_NOTEXTTOSEND, me, "No text to send"); return; }
	std::string target = params[0];
	std::string msg = prefixFor(fd) + " PRIVMSG " + target + " :" + trailing;

	if (isChannelName(target)) {
		if (channels.find(target) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, me + " " + target, "No such channel"); return; }
		Channel &ch = channels[target];
		if (ch.members.count(fd) == 0 && ch.noExternal) {
			sendNumeric(fd, ERR_CANNOTSENDTOCHAN, me + " " + target, "Cannot send to channel");
			return;
		}
		broadcast(ch.members, fd, msg);
	} else {
		std::map<std::string,int>::iterator it = nickToFd.find(toLower(target));
		if (it == nickToFd.end()) { sendNumeric(fd, ERR_NOSUCHNICK, me + " " + target, "No such nick"); return; }
		queueLine(it->second, msg);
	}
}

// TOPIC handler.
void Server::h_TOPIC(int fd, const std::vector<std::string>& params, const std::string& trailing, bool hasTrailing) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " TOPIC", "Not enough parameters"); return; }
	std::string name = params[0];
	if (channels.find(name) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, me + " " + name, "No such channel"); return; }
	Channel &ch = channels[name];

	if (!hasTrailing) {
		if (ch.topic.empty()) queueLine(fd, ":" + serverName + " " + RPL_NOTOPIC + " " + me + " " + name + " :No topic is set");
		else queueLine(fd, ":" + serverName + " " + RPL_TOPIC + " " + me + " " + name + " :" + ch.topic);
		return;
	} // Setting a new topic
	if (!ch.members.count(fd)) { sendNumeric(fd, ERR_NOTONCHANNEL, me + " " + name, "You're not on that channel"); return; } // New check
	if (ch.topicOpOnly && ch.ops.count(fd) == 0) { sendNumeric(fd, ERR_CHANOPRIVSNEEDED, me + " " + name, "You're not channel operator"); return; }

	ch.topic = trailing;
	std::string msg = prefixFor(fd) + " TOPIC " + name + " :" + ch.topic;
	broadcast(ch.members, -1, msg);
}

// INVITE handler.
void Server::h_INVITE(int fd, const std::vector<std::string>& params) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.size() < 2) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " INVITE", "Not enough parameters"); return; }
	std::string nick = params[0];
	std::string name = params[1];
	std::map<std::string,int>::iterator it = nickToFd.find(toLower(nick));
	if (it == nickToFd.end()) { sendNumeric(fd, ERR_NOSUCHNICK, me + " " + nick, "No such nick"); return; }

	if (channels.find(name) != channels.end()) {
		Channel &ch = channels[name];
		if (!ch.members.count(fd)) { sendNumeric(fd, ERR_NOTONCHANNEL, me + " " + name, "You're not on that channel"); return; }
		if (ch.members.count(it->second)) { sendNumeric(fd, ERR_USERONCHANNEL, me + " " + nick + " " + name, "is already on channel"); return; }
		if (ch.inviteOnly && ch.ops.count(fd) == 0) { sendNumeric(fd, ERR_CHANOPRIVSNEEDED, me + " " + name, "You're not channel operator"); return; }
		ch.invited.insert(toLower(nick));
	}

	queueLine(it->second, prefixFor(fd) + " INVITE " + nick + " :" + name);
	queueLine(fd, ":" + serverName + " " + RPL_INVITING + " " + me + " " + nick + " " + name);
}

// KICK handler.
void Server::h_KICK(int fd, const std::vector<std::string>& params, const std::string& trailing) {
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.size() < 2) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " KICK", "Not enough parameters"); return; }
	std::string name = params[0];
	std::string nick = params[1];
	if (channels.find(name) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, me + " " + name, "No such channel"); return; }
	Channel &ch = channels[name];
	if (ch.ops.count(fd) == 0) { sendNumeric(fd, ERR_CHANOPRIVSNEEDED, me + " " + name, "You're not channel operator"); return; }

	std::map<std::string,int>::iterator it = nickToFd.find(toLower(nick));
	if (it == nickToFd.end() || ch.members.count(it->second) == 0) {
		sendNumeric(fd, ERR_USERNOTINCHANNEL, me + " " + nick + " " + name, "They aren't on that channel");
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
