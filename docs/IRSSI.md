# Irssi Usage Guide (ft_irc)

This guide is focused on using **irssi** to test and demo your IRC server.

## Install

With Homebrew:
```
brew install irssi
```

## Start the server

```
./ircserv 6667 pass
```

## Connect (single client)

Launch irssi and connect:
```
irssi
/connect 127.0.0.1 6667 pass Alice
```

You should see the welcome numerics (001-004).

## Two-client setup

Terminal A:
```
irssi
/connect 127.0.0.1 6667 pass Alice
```

Terminal B:
```
irssi
/connect 127.0.0.1 6667 pass Bob
```

## Core commands to test

```
/join #chan
/names #chan
/topic #chan :hello
/topic #chan :        # clear topic
/msg #chan hello
/msg Bob hi
/notice #chan notice-text
/part #chan :bye
/quit
```

## Modes, invite, kick

As channel operator:
```
/mode #chan +it
/mode #chan +k secret
/mode #chan +l 2
/mode #chan +o Bob
/invite Bob #chan
/kick #chan Bob :reason
```

## PING notes

- `/ping <nick>` sends a CTCP PING to a **nick**.
- `/ping #chan` sends CTCP PING to a **channel**.
- If you run `/ping 127.0.0.1`, irssi treats it as a **nick**, so you may see
  “No such nick/channel” unless a user named `127.0.0.1` exists.

To send a raw IRC PING to the server:
```
/quote PING :test123
```

## Extra: +n (no external messages)

When `+n` is set, only users **in** the channel can message it.

```
/mode #chan +n   # block external messages
/mode #chan -n   # allow external messages
```

## Troubleshooting

- **Not connected**: ensure the server is running and the port/password match.
- **PASS warning**: the server may display a NOTICE about PASS; it is informational.
- **User mode queries**: `/mode <your nick>` returns `221` with `+`. Other-nick
  user mode queries are not supported and return `502`.

## Optional: clean test profile

If you want a clean, temporary irssi profile (no personal settings):
```
irssi --home /tmp/irssi_a -!
```
