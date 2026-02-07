# IRC Bot (bonus)

This bot is a minimal IRC client written in C++98. It connects to your ft_irc
server and responds to simple commands. It demonstrates the "bot" bonus
requirement without changing server code.

## Build

```
make bot
```

This produces `./ircbot` in the repo root.

## Run

```
./ircbot --host 127.0.0.1 --port 6667 --pass pass --nick HelperBot --channel #chan
```

If you do not use a server password, omit `--pass`.

## Commands (in channel or private message)

- `!help`  -> list available commands
- `!ping`  -> replies PONG
- `!roll`  -> rolls 1-100
- `!echo <text>` -> repeats text
- `!time`  -> local time on the bot machine

## Notes

- The bot joins the channel after it receives numeric 001.
- It responds only to messages starting with `!`.
- DCC and file transfer remain client-to-client and are not handled by the bot.
