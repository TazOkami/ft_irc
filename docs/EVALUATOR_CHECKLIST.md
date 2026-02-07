# Evaluator Manual Command Checklist (ft_irc) — ~60 minutes

This is a **hands-on command list** you can run exactly as an evaluator would.
No logging, no scripts—just copy/paste into terminals. It’s structured as a
~1 hour walkthrough with increasing complexity.

## 0) Build (2–3 min)
```
make
```

## 1) Start server (1 min)
```
./ircserv 6667 pass
```

## 2) Basic nc registration + PING (3–4 min)
Terminal B:
```
nc -v 127.0.0.1 6667
```
Then type:
```
PASS pass
NICK nctest
USER user 0 * :Real Name
PING :ping1
```
Expected: welcome numerics 001–004, PONG reply.

## 3) Multi‑client (nc + nc) (3–4 min)
Terminal B:
```
nc -v 127.0.0.1 6667
PASS pass
NICK multi1
USER u 0 * :u
```
Terminal C:
```
nc -v 127.0.0.1 6667
PASS pass
NICK multi2
USER u 0 * :u
PING :multi
```
Expected: both connect; PONG works while another client is connected.

## 4) Channel JOIN + broadcast (5 min)
Terminal B:
```
PASS pass
NICK recv
USER u 0 * :u
JOIN #chan
```
Terminal C:
```
PASS pass
NICK send
USER u 0 * :u
JOIN #chan
PRIVMSG #chan :hello_chan
```
Expected: recv sees PRIVMSG.

## 5) PART + QUIT (2 min)
```
PART #chan :bye
QUIT :leaving
```

## 6) NAMES / TOPIC (4–5 min)
```
JOIN #topic
NAMES #topic
TOPIC #topic :hello
TOPIC #topic :
TOPIC #topic
```
Expected: topic set, then cleared, then “no topic”.

## 7) MODE +i +t +k +l +o (op vs non‑op) (8–10 min)
Terminal B (becomes op by creating channel):
```
JOIN #mode
MODE #mode +t
MODE #mode +i
MODE #mode +k key123
MODE #mode +l 2
MODE #mode +o userB
```
Terminal C (non‑op):
```
JOIN #mode
MODE #mode +i
TOPIC #mode :nope
KICK #mode userA :nope
```
Expected: non‑op gets 482 errors.

## 8) INVITE / KICK (5–6 min)
Terminal B (op):
```
INVITE userB #mode
KICK #mode userB :reason
```
Expected: userB receives INVITE, then KICK.

## 9) NOTICE + PRIVMSG (user + channel) (5 min)
Terminal B:
```
JOIN #pm
```
Terminal C:
```
JOIN #pm
PRIVMSG #pm :privmsg_channel
NOTICE #pm :notice_channel
PRIVMSG userB :privmsg_user
NOTICE userB :notice_user
```
Expected: channel and direct messages arrive; NOTICE has no error replies.

## 10) Networking specials (partial commands) (6–8 min)
Terminal B:
```
PASS pass
NICK part
USER u 0 * :u
```
Then send partial:
```
PI
NG :com
mand
```
Expected: PONG :command.

While partial is in progress, connect another client and PING:
```
PASS pass
NICK other
USER u 0 * :u
PING :ok
```
Expected: PONG :ok even while partial is in-flight.

## 11) Kill client / half command (6–8 min)
Terminal B:
```
PASS pass
NICK killme
USER u 0 * :u
```
Then **close** terminal suddenly or kill nc process.
Terminal C should still connect and get PONG.

Half command:
```
PASS pass
NICK half
USER u 0 * :u
PRIV
```
Close. Server should remain OK for new connections.

## 12) Optional: irssi quick connect (3–4 min)
```
irssi
/connect 127.0.0.1 6667 pass MyNick
```
Expected: welcome numerics, no user‑mode error spam.

# Extra RFC / edge checks (15–20 min)

These match common evaluator “gotchas.”

## A) PASS ordering
```
NICK a1
USER u 0 * :u
PASS pass
```
Expected: 451 until PASS is sent; after PASS, resend NICK/USER and get 001–004.

## B) PING without params
```
PING
```
Expected: `409 No origin specified`.

## C) PRIVMSG to channel when not joined
```
PRIVMSG #nope :hi
```
Expected: `404 Cannot send to channel`.

## D) JOIN 0 (leave all channels)
```
JOIN #a
JOIN #b
JOIN 0
```
Expected: PART from all channels.

## E) JOIN comma lists
```
JOIN #c,#d keyc,keyd
```
Expected: joins both channels.

## F) NAMES without params
```
NAMES
```
Expected: list visible channels (or “*” if none).

## G) Re‑JOIN
```
JOIN #a
JOIN #a
```
Expected: `443 is already on channel`.

## H) Invite list cleared after join
1) Op sets `+i` and INVITEs `userB`.
2) `userB` joins, PARTs, then tries to re‑JOIN without new INVITE.
Expected: re‑JOIN is blocked (`473`).

## I) MODE query on missing channel
```
MODE #missing
```
Expected: `403 <nick> #missing :No such channel`.

## J) MODE +b query on missing channel
```
MODE #missing b
```
Expected: `403 <nick> #missing :No such channel`.

## K) Invalid channel names
```
JOIN #
JOIN #a,b
```
Expected: `403 Illegal channel name`.

## L) Multiple spaces parsing
```
NICK  spaced
```
Expected: nick becomes `spaced` (no empty param error).

## M) User mode query (client hygiene)
```
MODE <yourNick>
```
Expected: `221 <yourNick> +` (no error spam).
