#include "Server.hpp"
#include "Numerics.hpp"
#include "Utils.hpp"

#include <cstdlib>
#include <sstream>

static void appendModeChange(std::string& changes, char& curSign, bool add, char mode) {
	char sign = add ? '+' : '-';
	if (curSign != sign) {
		changes += sign;
		curSign = sign;
	}
	changes += mode;
}

// MODE handler (channel modes).
void Server::h_MODE(int fd, const std::vector<std::string>& params, const std::string& trailing) {
	(void)trailing;
	std::string me = clients[fd].nick.empty() ? "*" : clients[fd].nick;
	if (params.empty()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " MODE", "Not enough parameters"); return; }

	const std::string& target = params[0];

	if (!isChannelName(target)) {
		std::string nick = clients[fd].nick.empty() ? "*" : clients[fd].nick;
		if (toLower(target) == toLower(nick)) {
			queueLine(fd, ":" + serverName + " " + RPL_UMODEIS + " " + nick + " +");
			return;
		}
		sendNumeric(fd, "502", me + " " + target, "User mode not supported");
		return;
	}

	if (params.size() == 1) {
		if (channels.find(target) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, me + " " + target, "No such channel"); return; }
		Channel &ch = channels[target];

		std::string flags = "+";
		if (ch.inviteOnly)  flags += "i";
		if (ch.topicOpOnly) flags += "t";
		if (ch.noExternal)  flags += "n";
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
		if (channels.find(target) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, me + " " + target, "No such channel"); return; }
		Channel &ch = channels[target];
		if (!ch.members.count(fd)) { sendNumeric(fd, ERR_NOTONCHANNEL, me + " " + target, "You're not on that channel"); return; }
		queueLine(fd, ":" + serverName + " " + RPL_ENDOFBANLIST + " " + me + " " + target + " :End of Channel Ban List");
		return;
	}

	if (channels.find(target) == channels.end()) { sendNumeric(fd, ERR_NOSUCHCHANNEL, me + " " + target, "No such channel"); return; }
	Channel &ch = channels[target];

	if (!ch.members.count(fd)) { sendNumeric(fd, ERR_NOTONCHANNEL, me + " " + target, "You're not on that channel"); return; }
	if (ch.ops.count(fd) == 0) { sendNumeric(fd, ERR_CHANOPRIVSNEEDED, me + " " + target, "You're not channel operator"); return; }

	// Accept split mode tokens like: MODE #chan -i +k key
	std::string mode = params[1];
	size_t pidx = 2;
	while (pidx < params.size()) {
		const std::string& tok = params[pidx];
		if (tok.empty()) break;
		if ((tok[0] == '+' || tok[0] == '-') &&
			tok.find_first_not_of("+-itkoln") == std::string::npos) {
			mode += tok;
			++pidx;
			continue;
		}
		break;
	}
	bool add = true;

	std::string changes;
	char curSign = 0;
	std::vector<std::string> margs;

	for (size_t i = 0; i < mode.size(); ++i) {
		char c = mode[i];
		if (c == '+') { add = true;  continue; }
		if (c == '-') { add = false; continue; }

		switch (c) {
			case 'i':
				ch.inviteOnly = add;
				appendModeChange(changes, curSign, add, 'i');
				break;

			case 't':
				ch.topicOpOnly = add;
				appendModeChange(changes, curSign, add, 't');
				break;

			case 'n':
				ch.noExternal = add;
				appendModeChange(changes, curSign, add, 'n');
				break;

			case 'k':
				if (add) {
					if (pidx >= params.size()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " MODE", "+k requires key"); return; }
					ch.hasKey = true;
					ch.key = params[pidx++];
					appendModeChange(changes, curSign, true, 'k');
					margs.push_back(ch.key);
				} else {
					ch.hasKey = false;
					ch.key.clear();
					appendModeChange(changes, curSign, false, 'k');
				}
				break;

			case 'l':
				if (add) {
					if (pidx >= params.size()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " MODE", "+l requires limit"); return; }
					const std::string& lims = params[pidx++];
					char* end = NULL;
					long lim = ::strtol(lims.c_str(), &end, 10);
					if (!*lims.c_str() || (end && *end) || lim <= 0) {
						sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " MODE", "+l requires positive integer");
						return;
					}
					ch.hasLimit = true;
					ch.limit = static_cast<int>(lim);
					appendModeChange(changes, curSign, true, 'l');
					margs.push_back(lims);
				} else {
					ch.hasLimit = false;
					ch.limit = 0;
					appendModeChange(changes, curSign, false, 'l');
				}
				break;

			case 'o': {
				if (pidx >= params.size()) { sendNumeric(fd, ERR_NEEDMOREPARAMS, me + " MODE", "o requires nick"); return; }
				const std::string& nick = params[pidx++];
				std::map<std::string,int>::iterator it = nickToFd.find(toLower(nick));
				if (it == nickToFd.end() || ch.members.count(it->second) == 0) {
					sendNumeric(fd, ERR_USERNOTINCHANNEL, me + " " + nick + " " + target, "They aren't on that channel");
					return;
				}
				int tfd = it->second;
				if (add) ch.ops.insert(tfd);
				else     ch.ops.erase(tfd);
				appendModeChange(changes, curSign, add, 'o');
				margs.push_back(nick);
				break;
			}

			default:
				sendNumeric(fd, ERR_UNKNOWNMODE, me + " " + std::string(1, c), "is unknown mode char to me");
				return;
		}
	}

	if (!changes.empty()) {
		std::string msg = prefixFor(fd) + " MODE " + target + " " + changes;
		for (size_t i = 0; i < margs.size(); ++i) msg += " " + margs[i];
		broadcast(ch.members, -1, msg);
	}
}
