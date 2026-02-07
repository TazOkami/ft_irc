#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "Types.hpp"

struct Channel {
	std::string name;
	std::string topic;

	bool inviteOnly;   // +i
	bool topicOpOnly;  // +t
	bool noExternal;   // +n
	bool hasKey;       // +k
	std::string key;

	std::set<int> members; // fds
	std::set<int> ops;     // fds
	std::set<std::string> invited; // nicks (lower)

	bool hasLimit;      // +l
	int  limit;         // +l

	Channel():
		name(),
		topic(),
		inviteOnly(false),
		topicOpOnly(false),
		noExternal(false),
		hasKey(false),
		key(),
		hasLimit(false),
		limit(0)
		{}

};

#endif
