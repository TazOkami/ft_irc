# IRSSI Command Coverage (ft_irc)

This is a reference list of the commands shown by `/help` in irssi,
grouped by server coverage, plus the irssi command syntax (v1.4.5).

Legend:
- Full normal IRC behavior: the command maps to an IRC command you implement or
  is a CTCP message that the server will relay.
- Client-side only: no IRC command is sent (UI/config/local behavior).
- Not implemented: irssi sends an IRC command, but the server does not support
  it (typically you will get 421 Unknown command or mode errors).

Note: If a command has no syntax in the local irssi help files, a minimal
placeholder is shown; use `/HELP <command>` in irssi for full options.

---

## Full normal IRC behavior (server-supported)

- /action: /ACTION [-<server tag>] <target> <message> - envoie une action (CTCP ACTION)
- /ctcp: /CTCP <targets> <ctcp command> [<ctcp data>] - envoie une requete CTCP
- /cycle: /CYCLE [<channel>] [<message>] - quitte puis rejoint un channel
- /deop: /DEOP <nicks> - retire le statut operateur (MODE -o)
- /dcc: /DCC CHAT [-passive] [<nick>] | /DCC GET [<nick> [<file>]] | /DCC RESUME [<nick> [<file>]] | /DCC SERVER [+|-scf] [port] | /DCC CLOSE <type> <nick> [<file>] - initie DCC (chat/transfert client-a-client)
- /exec: /EXEC [-] [-nosh] [-out | -msg <target> | -notice <target>] [-name <name>] <cmd line> | /EXEC -out | -window | -msg <target> | -notice <target> | -close | -<signal> id> | /EXEC -in id> <text to send to process> - commande client; **envoie au serveur seulement si** `-msg` ou `-notice` est utilisé
- /invite: /INVITE <nick> [<channel>] - invite un utilisateur dans un channel
- /join: /JOIN [-window] [-invite] [-<server tag>] <channels> [<keys>] - rejoint un ou plusieurs channels
- /kick: /KICK [<channel>] <nicks> [<reason>] - expulse un utilisateur du channel
- /lusers: /LUSERS [<server mask> [<remote server>]] - affiche des stats globales du serveur
- /me: /ME <message> - alias de /action
- /mode: /MODE <your nick>|<channel> [<mode> [<mode parameters>]] - affiche ou modifie les modes
- /motd: /MOTD [<server>|<nick>] - affiche le message du jour du serveur
- /msg: /MSG [-<server tag>] [-channel | -nick] *|<targets> <message> - envoie un message prive (PRIVMSG)
- /query: /QUERY [-window] [-<server tag>] <nick> [<message>] - ouvre une query; si message fourni, envoie PRIVMSG
- /names: /NAMES [-count | -ops -halfops -voices -normal] [<channels> | **] - liste les utilisateurs d'un channel
- /nctcp: /NCTCP <targets> <ctcp command> [<ctcp data>] - CTCP via NOTICE (souvent pour reponses)
- /nick: /NICK <new nick> - change votre pseudo
- /notice: /NOTICE <targets> <message> - envoie un NOTICE (pas de reponse automatique)
- /op: /OP <nicks> - donne le statut operateur (MODE +o)
- /part: /PART [<channels>] [<message>] - quitte un channel
- /ping: /PING [<nick> | <channel> | *] - CTCP PING vers un user/channel ou ping serveur
- /quit: /QUIT [<message>] - quitte le serveur IRC
- /quote: /QUOTE <data> - envoie une ligne IRC brute au serveur
- /topic: /TOPIC [-delete] [<channel>] [<topic>] - affiche ou modifie le topic d'un channel
- /ver: /VER [<nick> | <channel> | *] - envoie un CTCP VERSION
- /who: /WHO [<nicks> | <channels> | **] - affiche des infos sur les utilisateurs
- /whois: /WHOIS [-<server tag>] [<server>] [<nicks>] - infos detaillees sur un utilisateur

---


## Client-side only (no impact on server)

- /accept: /ACCEPT [[-]nick,...]
- /alias: /ALIAS [[-]<alias> [<command>]]
- /beep: /BEEP
- /bind: /BIND [-list] [-delete | -reset] [<key> [<command> [<data>]]]
- /cat: /CAT [-window] <file> [<seek position>]
- /cd: /CD <directory>
- /channel: /CHANNEL LIST | /CHANNEL ADD|MODIFY [-auto | -noauto] [-bots <masks>] [-botcmd <command>] <channel> <network> [<password>] | /CHANNEL REMOVE <channel> <network>
- /clear: /CLEAR [-all] [<refnum>]
- /completion: /COMPLETION [-auto] [-delete] <key> <value>
- /dehilight: /DEHILIGHT <id>|<mask>
- /echo: /ECHO [-window <name>] [-level <level>] <text> - affiche un texte local, rien n’est envoyé au serveur
- /eval: /EVAL <command(s)>
- /flushbuffer: /FLUSHBUFFER
- /foreach: /FOREACH [args]
- /format: /FORMAT [-delete | -reset] [<module>] [<key> [<value>]]
- /hash: /HASH
- /help: /HELP [<command>]
- /hilight: /HILIGHT [-nick | -word | -line] [-mask | -full | -matchcase | -regexp] [-color <color>] [-actcolor <color>] [-level <level>] [-network <network>] [-channels <channels>] <text>
- /ignore: /IGNORE [-regexp | -full] [-pattern <pattern>] [-except] [-replies] [-network <network>] [-channels <channel>] [-time <time>] <mask> [<levels>] | /IGNORE [-regexp | -full] [-pattern <pattern>] [-except] [-replies] [-network <network>] [-time <time>] <channels> [<levels>]
- /ircnet: /IRCNET [args]
- /lastlog: /LASTLOG [-] [-file <filename>] [-window <ref#|name>] [-new | -away] [-<level> -<level...>] [-clear] [-count] [-case] [-date] [-regexp | -word] [-before [<#>]] [-after [<#>]] [-<# before+after>] [<pattern>] [<count> [<start>]]
- /layout: /LAYOUT SAVE | /LAYOUT RESET
- /load: /LOAD [-silent] <module> [<submodule>]
- /log: /LOG OPEN [-noopen] [-autoopen] [-window] [-<server tag>] [-targets <targets>] [-colors] <fname> [<levels>] | /LOG CLOSE <id>|<file> | /LOG START <id>|<file> | /LOG STOP <id>|<file>
- /map: /MAP
- /mircdcc: /MIRCDCC ON|OFF
- /network: /NETWORK ADD|MODIFY [-nick <nick>] [-alternate_nick <nick>] [-user <user>] [-realname <name>] [-host <host>] [-usermode <mode>] [-autosendcmd <cmd>] [-querychans <count>] [-whois <count>] [-msgs <count>] [-kicks <count>] [-modes <count>] [-cmdspeed <ms>] [-cmdmax <count>] [-sasl_mechanism <mechanism>] [-sasl_username <username>] [-sasl_password <password>] <name> | /NETWORK REMOVE <network>
- /netsplit: /NETSPLIT
- /rawlog: /RAWLOG SAVE <file> | /RAWLOG OPEN <file> | /RAWLOG CLOSE
- /recode: /RECODE | /RECODE ADD [[<tag>/]<target>] <charset> | /RECODE REMOVE [<target>]
- /redraw: /REDRAW [args]
- /reload: /RELOAD [<file>]
- /resize: /RESIZE [args]
- /save: /SAVE [<file>]
- /script: /SCRIPT [LIST|LOAD <file>|UNLOAD <name>|RESET|EXEC <code>]
- /scrollback: /SCROLLBACK CLEAR [-all] [<refnum>] | /SCROLLBACK LEVELCLEAR [-all] [-level <level>] [<refnum>] | /SCROLLBACK GOTO <+|-linecount>|<linenum>|<timestamp> | /SCROLLBACK HOME | /SCROLLBACK END | /SCROLLBACK REDRAW
- /set: /SET [-clear | -default | -section] [<key> [<value>]]
- /sethost: /SETHOST [args]
- /setname: /SETNAME [args]
- /statusbar: /STATUSBAR ADD|MODIFY [-disable | -nodisable] [-type window|root] [-placement top|bottom] [-position #] [-visible always|active|inactive] <statusbar> | /STATUSBAR RESET <statusbar> | /STATUSBAR ADDITEM|MODIFYITEM [-before | -after <item>] [-priority #] [-alignment left|right] <item> <statusbar> | /STATUSBAR REMOVEITEM <item> <statusbar> | /STATUSBAR INFO <statusbar>
- /toggle: /TOGGLE <key> [on|off|toggle]
- /ts: /TS
- /unalias: /UNALIAS <alias>
- /unignore: /UNIGNORE <id>|<mask>
- /unnotify: /UNNOTIFY <mask>
- /unquery: /UNQUERY [<nick>]
- /upgrade: /UPGRADE [<irssi binary path>]
- /uptime: /UPTIME
- /unload: /UNLOAD <module> [<submodule>]
- /wait: /WAIT [-<server tag>] <milliseconds>
- /window: /WINDOW LOG on|off|toggle [<filename>] | /WINDOW LOGFILE <file> | /WINDOW NEW [HIDDEN|SPLIT|-right SPLIT] | /WINDOW CLOSE [<first> [<last>]] | /WINDOW REFNUM <number> | /WINDOW GOTO active|<number>|<name> | /WINDOW NEXT | /WINDOW LAST | /WINDOW PREVIOUS | /WINDOW LEVEL [<levels>] | /WINDOW IMMORTAL on|off|toggle | /WINDOW SERVER [-sticky | -unsticky] <tag> | /WINDOW ITEM PREV | /WINDOW ITEM NEXT | /WINDOW ITEM GOTO <number>|<name> | /WINDOW ITEM MOVE <number>|<name> | /WINDOW NUMBER [-sticky] <number> | /WINDOW NAME <name> | /WINDOW HISTORY [-clear] <name> | /WINDOW MOVE PREV | /WINDOW MOVE NEXT | /WINDOW MOVE FIRST | /WINDOW MOVE LAST | /WINDOW MOVE <number>|<direction> | /WINDOW LIST | /WINDOW THEME [-delete] [<name>] | /WINDOW HIDE [<number>|<name>] | /WINDOW SHOW [-right] <number>|<name> | /WINDOW GROW [-right] [<lines>|<columns>] | /WINDOW SHRINK [-right] [<lines>|<columns>] | /WINDOW SIZE [-right] <lines>|<columns> | /WINDOW BALANCE [-right] | /WINDOW UP [-directional] | /WINDOW DOWN [-directional] | /WINDOW LEFT [-directional] | /WINDOW RIGHT [-directional] | /WINDOW STICK [<ref#>] [ON|OFF] | /WINDOW MOVE LEFT [-directional] | /WINDOW MOVE RIGHT [-directional] | /WINDOW MOVE UP [-directional] | /WINDOW MOVE DOWN [-directional] | /WINDOW HIDELEVEL [<levels>]

---

## Connection-related (client-only)

- /connect: /CONNECT [-4 | -6] [-tls_cert <cert>] [-tls_pkey <pkey>] [-tls_pass <password>] [-tls_verify] [-tls_cafile <cafile>] [-tls_capath <capath>] [-tls_ciphers <list>] [-tls_pinned_cert <fingerprint>] [-tls_pinned_pubkey <fingerprint>] [-!] [-noautosendcmd] [-tls | -notls] [-nocap] [-starttls | -disallow_starttls] [-noproxy] [-network <network>] [-host <hostname>] [-rawlog <file>] <address>|<chatnet> [<port> [<password> [<nick>]]] - ouvre une connexion TCP (commande client, pas un message IRC)
- /disconnect: /DISCONNECT *|<tag> [<message>] - ferme la connexion client
- /reconnect: /RECONNECT <tag> [<quit message>]
- /rmreconns: /RMRECONNS
- /rmrejoins: /RMREJOINS
- /server: /SERVER CONNECT [-4 | -6] [-tls | -notls] [-tls_cert <cert>] [-tls_pkey <pkey>] [-tls_pass <password>] [-tls_verify | -notls_verify] [-tls_cafile <cafile>] [-tls_capath <capath>] [-tls_ciphers <list>] [-tls_pinned_cert <fingerprint>] [-tls_pinned_pubkey <fingerprint>] [-!] [-noautosendcmd] [-nocap] [-noproxy] [-network <network>] [-host <hostname>] [-rawlog <file>] [+]<address>|<chatnet> [<port> [<password> [<nick>]]] | /SERVER REMOVE <address> [<port>] [<network>] | /SERVER ADD|MODIFY [-4 | -6] [-cap | -nocap] [-tls_cert <cert>] [-tls_pkey <pkey>] [-tls_pass <password>] [-tls_verify] [-tls_cafile <cafile>] [-tls_capath <capath>] [-tls_ciphers <list>] [-tls | -notls] [-starttls | -nostarttls | -disallow_starttls | -nodisallow_starttls] [-auto | -noauto] [-network <network>] [-host <hostname>] [-cmdspeed <ms>] [-cmdmax <count>] [-port <port>] <address> [<port> [<password>]] | /SERVER LIST | /SERVER PURGE [<target>] - gère les serveurs/connexions côté client (liste, ajoute, connecte)

---

## Not implemented by this server

- /admin: /ADMIN [<server>|<nickname>]
- /away: /AWAY [-one | -all] [<reason>]
- /ban: /BAN [<channel>] [<nicks>] | /BAN [-normal | -user | -host | -domain | -custom <type>] <nicks/masks>
- /devoice: /DEVOICE <nicks>
- /die: /DIE
- /info: /INFO [<server>]
- /ison: /ISON <nicks>
- /notify: /NOTIFY [-away] <mask> [<ircnets>]
- /kickban: /KICKBAN [<channel>] <nicks> <reason>
- /kill: /KILL <nick> <reason>
- /knock: /KNOCK <channel>
- /knockout: /KNOCKOUT [<time>] <nicks> <reason>
- /links: /LINKS [[<server>] <mask>]
- /list: /LIST [-yes] [<channel>]
- /oper: /OPER [<nick> [<password>]]
- /rehash: /REHASH [<option>]
- /restart: /RESTART
- /servlist: /SERVLIST [<mask> [<type>]]
- /silence: /SILENCE [[+|-]<nick!user@host>] | /SILENCE [<nick>]
- /sconnect: /SCONNECT <new server> [[<port>] <existing server>]
- /squery: /SQUERY <service> [<message>]
- /squit: /SQUIT <server>|<mask> <reason>
- /stats: /STATS <type> [<server>]
- /time: /TIME [<server>|<nick>]
- /trace: /TRACE [<server>|<nick>]
- /unban: /UNBAN -first | -last | <id> | <masks>
- /unsilence: /UNSILENCE <nick!user@host>
- /userhost: /USERHOST <nicks>
- /version: /VERSION | /VERSION [<server>|<nick>]
- /voice: /VOICE <nicks>
- /wall: /WALL [<channel>] <message>
- /wallops: /WALLOPS <message>
- /whowas: /WHOWAS [<nicks> [<count> [server]]]

---
