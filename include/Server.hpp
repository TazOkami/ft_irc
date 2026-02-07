#ifndef SERVER_HPP
#define SERVER_HPP

#include "Types.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Poller.hpp"
#include <ctime>

struct Server {
	// config
	int         listenFd;
	std::string password;
	std::string serverName;
	std::time_t startTime;

	// état
	Poller                           poller;
	std::map<int, Client>            clients;     // fd -> client
	std::map<std::string, int>       nickToFd;    // lower(nick) -> fd
	std::map<std::string, Channel>   channels;    // "#x" -> channel

	// limites
	static const size_t MAX_LINE_LEN   = 512;       // CRLF inclus
	static const size_t MAX_READBUF    = 64 * 1024; // anti-flood
	static const size_t MAX_WRITEBUF   = 64 * 1024; // back-pressure

	Server();

	// boot
	bool start(unsigned short port, const std::string& pass);

	// boucle
	void run();

	// accept/read/write/close
	void handleAccept();
	void handleRead(int fd);
	void handleWrite(int fd);
	void disconnect(int fd, const std::string& reason);

	// I/O utils
	void queueLine(int fd, const std::string& line);
	void wantRead(int fd, bool yes);
	void wantWrite(int fd, bool yes);

	// helpers IRC
	void maybeWelcome(int fd);
	bool nickInUse(const std::string& nick) const;
	void setNick(int fd, const std::string& nick);
	std::string prefixFor(int fd) const;
	Channel& getOrCreateChan(const std::string& name);
	void sendNames(int fd, Channel& ch);
	void broadcast(const std::set<int>& targets, int excludeFd, const std::string& line);

	// lecture/parse
	void readFromClient(int fd, Client& c);
	void processIncomingLines(int fd, Client& c);

	// dispatcher
	void dispatchLine(int fd, const std::string& line);

	// handlers (core)
	void h_PASS(int fd, const std::vector<std::string>& params);
	void h_NICK(int fd, const std::vector<std::string>& params);
	void h_USER(int fd, const std::vector<std::string>& params, const std::string& trailing);
	void h_QUIT(int fd, const std::string& trailing);
	void h_PRIVMSG(int fd, const std::vector<std::string>& params, const std::string& trailing);

	// extras
	void h_CAP(int fd, const std::vector<std::string>& params, const std::string& trailing);
	void h_PING(int fd, const std::vector<std::string>& params, const std::string& trailing);
	void h_PONG(int fd, const std::vector<std::string>& params, const std::string& trailing);
	void h_NOTICE(int fd, const std::vector<std::string>& params, const std::string& trailing);
	void h_PART(int fd, const std::vector<std::string>& params, const std::string& trailing);
	void h_NAMES(int fd, const std::vector<std::string>& params);
	void h_WHO(int fd, const std::vector<std::string>& params);
	void h_MOTD(int fd);
	void h_LUSERS(int fd);
	void h_WHOIS(int fd, const std::vector<std::string>& params);

	// handlers (channels)
	void h_JOIN(int fd, const std::vector<std::string>& params);
	void h_TOPIC(int fd, const std::vector<std::string>& params, const std::string& trailing, bool hasTrailing);
	void h_MODE(int fd, const std::vector<std::string>& params, const std::string& trailing);
	void h_INVITE(int fd, const std::vector<std::string>& params);
	void h_KICK(int fd, const std::vector<std::string>& params, const std::string& trailing);

	// numerics utils
	void sendNumeric(int fd, const std::string& code, const std::string& target, const std::string& text);
};

#endif
