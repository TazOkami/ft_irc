#ifndef NUMERICS_HPP
#define NUMERICS_HPP

// Réponses numérales (subset) — centralisées ici pour lisibilité
// Usage: sendNumeric(fd, RPL_WELCOME, nick, "Welcome ...");

#define RPL_WELCOME    "001"  // Welcome to the IRC Network
#define RPL_YOURHOST   "002"  // Your host is <servername>
#define RPL_CREATED    "003"  // This server was created <date>
#define RPL_MYINFO     "004"  // Server info (name, version, modes)
#define RPL_NAMREPLY   "353"  // List of users in a channel
#define RPL_ENDOFNAMES "366"  // End of NAMES list
#define RPL_NOTOPIC    "331"  // No topic is set
#define RPL_TOPIC      "332"  // Channel topic

#define RPL_INVITING   "341"  // User has been invited

#define RPL_CHANNELMODEIS "324"  // Channel mode is <modes>
#define RPL_WHOREPLY      "352"  // Reply to WHO command
#define RPL_ENDOFWHO      "315"  // End of WHO list
#define RPL_MOTDSTART     "375"  // Start of MOTD
#define RPL_MOTD          "372"  // MOTD line
#define RPL_ENDOFMOTD     "376"  // End of MOTD
#define RPL_LUSERCLIENT   "251"  // Number of users online
#define RPL_LUSERME       "255"  // Number of clients on this server
#define RPL_WHOISUSER     "311"  // WHOIS user info
#define RPL_ENDOFWHOIS    "318"  // End of WHOIS

#define RPL_BANLIST      "367"  // Ban list entry
#define RPL_ENDOFBANLIST "368"  // End of ban list

#define ERR_NOSUCHNICK        "401"  // No such nick/channel
#define ERR_NOSUCHCHANNEL     "403"  // No such channel
#define ERR_CANNOTSENDTOCHAN  "404"  // Cannot send to channel
#define ERR_TOOMANYCHANNELS   "405"  // You have joined too many channels
#define ERR_NORECIPIENT       "411"  // No recipient given
#define ERR_NOTEXTTOSEND      "412"  // No text to send
#define ERR_NONICKNAMEGIVEN   "431"  // No nickname given
#define ERR_ERRONEUSNICKNAME  "432"  // Erroneous nickname
#define ERR_NICKNAMEINUSE     "433"  // Nickname is already in use
#define ERR_USERNOTINCHANNEL  "441"  // User is not in that channel
#define ERR_NOTONCHANNEL      "442"  // You're not on that channel
#define ERR_USERONCHANNEL     "443"  // User is already on channel
#define ERR_NEEDMOREPARAMS    "461"  // Not enough parameters
#define ERR_ALREADYREGISTRED  "462"  // Already registered
#define ERR_PASSWDMISMATCH    "464"  // Password incorrect
#define ERR_CHANNELISFULL     "471"  // Cannot join channel (+l)
#define ERR_UNKNOWNMODE       "472"  // Unknown mode char
#define ERR_INVITEONLYCHAN    "473"  // Cannot join channel (+i)
#define ERR_BADCHANNELKEY     "475"  // Cannot join channel (+k)
#define ERR_CHANOPRIVSNEEDED  "482"  // You're not channel operator

#endif
