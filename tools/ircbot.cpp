#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct IrcLine {
	std::string prefix;
	std::string cmd;
	std::vector<std::string> params;
};

static std::vector<std::string> splitSpaces(const std::string &s) {
	std::vector<std::string> out;
	std::string cur;
	for (std::string::size_type i = 0; i < s.size(); ++i) {
		char c = s[i];
		if (c == ' ') {
			if (!cur.empty()) {
				out.push_back(cur);
				cur.clear();
			}
		} else {
			cur += c;
		}
	}
	if (!cur.empty())
		out.push_back(cur);
	return out;
}

static IrcLine parseLine(const std::string &line) {
	IrcLine out;
	std::string rest = line;
	if (!rest.empty() && rest[0] == ':') {
		std::string::size_type sp = rest.find(' ');
		if (sp != std::string::npos) {
			out.prefix = rest.substr(1, sp - 1);
			rest = rest.substr(sp + 1);
		} else {
			out.prefix = rest.substr(1);
			rest.clear();
		}
	}
	std::string trailing;
	std::string::size_type trail = rest.find(" :");
	std::string head = rest;
	if (trail != std::string::npos) {
		head = rest.substr(0, trail);
		trailing = rest.substr(trail + 2);
	}
	std::vector<std::string> parts = splitSpaces(head);
	if (!trailing.empty() || trail != std::string::npos)
		parts.push_back(trailing);
	if (!parts.empty()) {
		out.cmd = parts[0];
		for (std::size_t i = 1; i < parts.size(); ++i)
			out.params.push_back(parts[i]);
	}
	return out;
}

static std::string nickFromPrefix(const std::string &prefix) {
	std::string::size_type ex = prefix.find('!');
	if (ex == std::string::npos)
		return prefix;
	return prefix.substr(0, ex);
}

static int connectTo(const std::string &host, int port) {
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	std::ostringstream p;
	p << port;

	struct addrinfo *res = 0;
	if (getaddrinfo(host.c_str(), p.str().c_str(), &hints, &res) != 0)
		return -1;

	int sock = -1;
	for (struct addrinfo *it = res; it; it = it->ai_next) {
		sock = ::socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (sock < 0)
			continue;
		if (::connect(sock, it->ai_addr, it->ai_addrlen) == 0)
			break;
		::close(sock);
		sock = -1;
	}
	freeaddrinfo(res);
	return sock;
}

static void sendLine(int sock, const std::string &line, bool quiet) {
	if (!quiet)
		std::cerr << "[BOT->] " << line << std::endl;
	std::string out = line + "\r\n";
	::send(sock, out.c_str(), out.size(), 0);
}

int main(int argc, char **argv) {
	std::string host = "127.0.0.1";
	int port = 6667;
	std::string pass;
	std::string nick = "HelperBot";
	std::string user = "helper";
	std::string real = "ft_irc helper bot";
	std::string channel;
	std::string prefix = "!";
	bool quiet = false;

	for (int i = 1; i < argc; ++i) {
		std::string a = argv[i];
		if (a == "--host" && i + 1 < argc) host = argv[++i];
		else if (a == "--port" && i + 1 < argc) port = std::atoi(argv[++i]);
		else if (a == "--pass" && i + 1 < argc) pass = argv[++i];
		else if (a == "--nick" && i + 1 < argc) nick = argv[++i];
		else if (a == "--user" && i + 1 < argc) user = argv[++i];
		else if (a == "--real" && i + 1 < argc) real = argv[++i];
		else if (a == "--channel" && i + 1 < argc) channel = argv[++i];
		else if (a == "--prefix" && i + 1 < argc) prefix = argv[++i];
		else if (a == "--quiet") quiet = true;
		else if (a == "--help") {
			std::cout << "Usage: ircbot [--host H] [--port P] [--pass PASS] [--nick N] [--user U] [--real R] [--channel #chan] [--prefix !] [--quiet]\n";
			return 0;
		}
	}

	int sock = connectTo(host, port);
	if (sock < 0) {
		std::cerr << "Failed to connect to " << host << ":" << port << std::endl;
		return 1;
	}

	std::srand(static_cast<unsigned int>(std::time(0)));

	if (!pass.empty())
		sendLine(sock, "PASS " + pass, quiet);
	sendLine(sock, "NICK " + nick, quiet);
	sendLine(sock, "USER " + user + " 0 * :" + real, quiet);

	std::string buffer;
	bool joined = false;
	std::string botNick = nick;

	while (true) {
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 200000;
		int ready = ::select(sock + 1, &rfds, 0, 0, &tv);
		if (ready < 0)
			break;
		if (ready == 0)
			continue;
		if (!FD_ISSET(sock, &rfds))
			continue;

		char buf[4096];
		ssize_t n = ::recv(sock, buf, sizeof(buf) - 1, 0);
		if (n <= 0)
			break;
		buf[n] = '\0';
		buffer += buf;

		std::string::size_type pos;
		while ((pos = buffer.find('\n')) != std::string::npos) {
			std::string line = buffer.substr(0, pos);
			buffer.erase(0, pos + 1);
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			if (line.empty())
				continue;
			if (!quiet)
				std::cerr << "[BOT<-] " << line << std::endl;

			IrcLine irc = parseLine(line);

			if (irc.cmd == "PING") {
				std::string token = irc.params.empty() ? "" : irc.params[0];
				sendLine(sock, "PONG :" + token, quiet);
				continue;
			}
			if (irc.cmd == "001" && !channel.empty() && !joined) {
				sendLine(sock, "JOIN " + channel, quiet);
				joined = true;
				continue;
			}
			if (irc.cmd == "433") {
				botNick += "_";
				sendLine(sock, "NICK " + botNick, quiet);
				continue;
			}
			if (irc.cmd != "PRIVMSG" || irc.params.size() < 2)
				continue;

			std::string sender = nickFromPrefix(irc.prefix);
			std::string target = irc.params[0];
			std::string message = irc.params[1];

			if (sender == botNick)
				continue;
			if (!message.empty() && message[0] == '\x01')
				continue;
			if (message.compare(0, prefix.size(), prefix) != 0)
				continue;

			std::string body = message.substr(prefix.size());
			while (!body.empty() && body[0] == ' ')
				body.erase(0, 1);
			if (body.empty())
				continue;

			std::string::size_type sp = body.find(' ');
			std::string cmd = (sp == std::string::npos) ? body : body.substr(0, sp);
			std::string rest = (sp == std::string::npos) ? "" : body.substr(sp + 1);
			for (std::string::size_type i = 0; i < cmd.size(); ++i)
				cmd[i] = static_cast<char>(std::tolower(cmd[i]));

			std::string replyTarget = (target.size() > 0 && target[0] == '#') ? target : sender;

			if (cmd == "help") {
				sendLine(sock, "PRIVMSG " + replyTarget + " :Commands: !help, !ping, !roll, !echo <text>, !time", quiet);
			} else if (cmd == "ping") {
				sendLine(sock, "PRIVMSG " + replyTarget + " :PONG", quiet);
			} else if (cmd == "roll") {
				int value = (std::rand() % 100) + 1;
				std::ostringstream oss;
				oss << "PRIVMSG " << replyTarget << " :rolls " << value << "/100";
				sendLine(sock, oss.str(), quiet);
			} else if (cmd == "echo") {
				if (!rest.empty())
					sendLine(sock, "PRIVMSG " + replyTarget + " :" + rest, quiet);
			} else if (cmd == "time") {
				std::time_t now = std::time(0);
				char tbuf[64];
				std::tm *lt = std::localtime(&now);
				if (lt && std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", lt)) {
					sendLine(sock, "PRIVMSG " + replyTarget + " :" + std::string(tbuf), quiet);
				}
			}
		}
	}

	::close(sock);
	return 0;
}
