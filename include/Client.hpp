#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Types.hpp"

struct Client {
	std::string readBuf;
	std::string writeBuf;

	std::string nick;
	std::string user;
	std::string pass;
	std::string realname;

	bool hasPass;
	bool registered;
	bool closed;
	bool pausedRead;

	Client() : hasPass(false), registered(false), closed(false), pausedRead(false) {}
};

#endif
