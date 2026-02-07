# IRC_new (ft_irc)

Minimal IRC server in C++98 using a single poll() loop. The goal is to follow
docs/subject_IRC.pdf and support a real client (irssi, weechat, etc.).

## What is implemented

Subject-required features:
- Registration via PASS, NICK, USER with welcome numerics after completion.
- Channels: JOIN, TOPIC, MODE with flags +i +t +k +o +l.
- Messaging: PRIVMSG (channels and users) and broadcast to channel members.
- Operator commands: KICK, INVITE (plus topic/mode restrictions).
- All sockets are non-blocking and driven by one poll(2) loop.

Additional implemented (beyond subject requirements):
- PART, NAMES.
- NOTICE, WHO, WHOIS, LUSERS, MOTD.
- Minimal CAP flow (LS, REQ->NAK, END) so common clients connect cleanly.
- PING → PONG and accept client PONG.
- MODE +n (no external messages).

## Build

```
make
```

Binary: `ircserv`

## Run

```
./ircserv <port> <password>
```

Notes:
- The password argument is required by the binary. Pass an empty string to
  disable PASS enforcement:
  `./ircserv 6667 ""`
- Make sure the port is free, or pick another (e.g. 6668).

## Connect with Irssi

One-shot connect:

```
irssi -c 127.0.0.1 -p 6667 -n mynick -w mypass
```

Interactive:

```
/connect 127.0.0.1 6667 mypass mynick
```

If a password is configured, PASS must be sent before NICK/USER. If you send
NICK/USER first, you will get numeric 451 (must PASS first).

## Quick smoke test (raw TCP)

Interactive netcat session:

```
# Terminal A
./ircserv 6667 secretpass

# Terminal B
nc -v 127.0.0.1 6667
```

Then type these IRC commands inside the nc session:

```
PASS secretpass
NICK piped
USER user 0 * :Pipe
JOIN #nc
PRIVMSG #nc :hello from nc
QUIT :bye
```

One-shot netcat script (CRLF-terminated):

```
printf 'PASS secretpass\r\nNICK piped\r\nUSER user 0 * :Pipe\r\nJOIN #nc\r\nPRIVMSG #nc :hi\r\nQUIT :bye\r\n' | nc -v 127.0.0.1 6667
```

## Manual test checklist

1. Connect a client and verify welcome numerics (001-004).
2. JOIN a channel, check NAMES, set TOPIC.
3. Toggle MODE +i +t +k +o +l and confirm behavior.
4. Invite a second client; verify INVITE / key requirements.
5. Exchange PRIVMSG/NOTICE; issue KICK and verify notifications.
6. Optionally test WHO, WHOIS, LUSERS, MOTD.

## Optional: Bonus bot

This repo includes a tiny IRC bot that connects as a client and responds to
simple `!` commands. Build with `make bot`. See `docs/BOT.md`.

## Project map (where to change what)

- Core loop / poll: `src/Server_Loop.cpp`
- I/O and buffering: `src/Server_IO.cpp`
- Command dispatch + PASS gating: `src/Server_Dispatch.cpp`
- IRC commands: `src/Handlers_*.cpp`
- Parsing utilities: `src/Parser.cpp`, `include/Parser.hpp`
- Server config and limits: `include/Server.hpp`

Team split (for evaluation clarity):
- Mate 1: `src/Server_Loop.cpp`, `src/Server_IO.cpp`, `src/Server_Dispatch.cpp`, `src/Parser.cpp`, `src/Poller.cpp`, `src/Utils.cpp`
- Mate 2: `src/Handlers_Registration.cpp`, `src/Handlers_Core.cpp`, `src/Handlers_Channel_Modes.cpp`, `src/Handlers_Utils.cpp`
- You (bonus/extras): `src/Handlers_Extras.cpp`, `tools/ircbot.cpp`, `docs/BOT.md`

## POLLOUT subtlety (important)

We do NOT keep POLLOUT enabled all the time. POLLOUT is level-triggered, and
most TCP sockets are writable most of the time. Keeping POLLOUT always enabled
would make poll() wake up continuously even when there is nothing to send.

Current design:
- When we queue data (`queueLine`), we enable POLLOUT for that fd.
- When the write buffer drains (`handleWrite`), we disable POLLOUT again.

This avoids busy loops while still sending data as soon as the socket can
accept it.

## References

- `docs/subject_IRC.pdf` (spec that guided the implementation)
- `docs/CHEATSHEET.md` (quick orientation for contributors)
- `docs/IRSSI.md` (irssi-only usage guide)
- `docs/EVALUATION_REPORT.md` (local test log vs. ft_irc.pdf)

## Troubleshooting

- "You must PASS first": send PASS (if configured) before NICK/USER.
- "You have not registered": you must complete NICK + USER (and PASS if set).
- No output on client: ensure all replies are sent via `queueLine()`.
