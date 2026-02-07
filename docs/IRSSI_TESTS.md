# IRSSI Test Checklist (ft_irc)

This file is a step-by-step command log template. Run each section in order and
fill in the **Observed** lines.

Assumes server is running:
```
./ircserv 6667 pass
```

Launch irssi (manual):
```
irssi
/connect 127.0.0.1 6667 pass Alice
```

One-shot connect:
```
irssi -c 127.0.0.1 -p 6667 -w pass -n Alice
```

Notes:
- If `-p` is omitted, irssi uses port 6667 by default.
- If `-n` is omitted, irssi uses your local username as the nick.
---

## 0) Connection scenarios (client-side failures)

### 0.1 Wrong port

/connect 127.0.0.1 6668 pass Alice

11:24 [1272] -!- Irssi: Looking up 127.0.0.1
11:24 [1272] -!- Irssi: Connecting to 127.0.0.1 [127.0.0.1] port 6668
11:24 -!- Irssi: Unable to connect server 127.0.0.1 port 6668 [Connection refused]

### 0.2 Wrong address

/connect 127.0.0.2 6667 pass Alice

11:52 [128] -!- Irssi: Unable to connect server 128.0.0.2 port 6667 [Connection timed out]

## 1) PASS/NICK/USER order

### 1.1 Correct (single command)

/connect 127.0.0.1 6667 pass Alice

/connect 127.0.0.1 6667
/quote PASS pass
/quote NICK Alice
/quote USER Alice 0 * :Alice

### 1.2 No PASS (server requires PASS)
Command:

/connect 127.0.0.1 6667

11:55 [1275] -!- Irssi: Looking up 127.0.0.1
11:55 [1275] -!- Irssi: Connecting to 127.0.0.1 [127.0.0.1] port 6667
11:55 [1275] -!- Irssi: Connection to 127.0.0.1 established
11:55 [1275] -!- Capabilities supported: 
11:55 [1275] -!- You must PASS first
11:55 [1275] -!- You must PASS first

### 1.3 Wrong PASS

/connect 127.0.0.1 6667 wrongpass Alice

11:55 [1275] -!- Irssi: Looking up 127.0.0.1
11:55 [1275] -!- Irssi: Connecting to 127.0.0.1 [127.0.0.1] port 6667
11:55 [1275] -!- Irssi: Connection to 127.0.0.1 established
11:55 [1275] -!- Capabilities supported: 
11:55 [1275] -!- You must PASS first
11:55 [1275] -!- You must PASS first

### 1.4 Wrong order (connect, then PASS/NICK/USER)

/connect 127.0.0.1 6667
/quote USER alice 0 * :Alice

11:55 [1275] -!- You must PASS first
11:55 [1275] -!- You must PASS first

## 2) Basic channel flow (Alice only)

### 2.1 JOIN + NAMES + TOPIC

/join #chan
/names #chan
/topic #chan

- JOIN broadcast
- Names list (353)
- End of names (366)
- No topic is set (331) if none

### 2.2 PART

/part #chan :bye

- PART broadcast



## 3) Two-client setup (Alice + Bob)

Terminal A (Alice):
```
/connect 127.0.0.1 6667 pass Alice
/join #chan
```

Terminal B (Bob):
```
/connect 127.0.0.1 6667 pass Bob
/join #chan
```

### 3.1 PRIVMSG to channel
From Alice:
```
/msg #chan hello
```
Expected:
- Bob sees “Alice: hello”
Observed:
- 

### 3.2 PRIVMSG to user
From Alice:
```
/msg Bob hi
```
Expected:
- Bob receives direct message
Observed:
- 

### 3.3 NOTICE to channel (no error numerics)
From Alice:
```
/notice #chan notice-text
```
Expected:
- Bob sees NOTICE
Observed:
- 

---

## 4) TOPIC

### 4.1 Set topic
From Alice (op):
```
/topic #chan :Hello
```
Expected:
- Topic change broadcast
- Topic numeric (332)
Observed:
- 

### 4.2 Clear topic
From Alice:
```
/quote TOPIC #chan :
```
Expected:
- Topic cleared
- No topic is set (331)
Observed:
- 

### 4.3 +t restricts TOPIC to ops
From Alice:
```
/mode #chan +t
```
From Bob (non-op):
```
/topic #chan :Nope
```
Expected:
- Error 482 (You're not channel operator)
Observed:
- 

---

## 5) MODE +i +k +l +o (+n optional)

### 5.1 Invite-only (+i)
From Alice (op):
```
/mode #chan +i
```
From Bob (not invited, try rejoin after /part):
```
/part #chan :bye
/join #chan
```
Expected:
- Error 473 (Cannot join channel +i)
Observed:
- 

### 5.2 INVITE (when +i)
From Alice:
```
/invite Bob #chan
```
From Bob:
```
/join #chan
```
Expected:
- Bob can join
Observed:
- 

### 5.3 Key (+k)
From Alice:
```
/mode #chan +k secret
```
From Bob (rejoin without key):
```
/part #chan :bye
/join #chan
```
Expected:
- Error 475 (Cannot join channel +k)
Observed:
- 

From Bob (with key):
```
/join #chan secret
```
Expected:
- Join succeeds
Observed:
- 

### 5.4 Limit (+l)
From Alice:
```
/mode #chan +l 1
```
From Bob (try join):
```
/part #chan :bye
/join #chan
```
Expected:
- Error 471 (Cannot join channel +l)
Observed:
- 

### 5.5 Operator (+o)
From Alice:
```
/mode #chan +o Bob
```
Expected:
- Bob shows as op (names list with @)
Observed:
- 

### 5.6 No external messages (+n)
From Alice:
```
/mode #chan +n
```
From Bob (not in channel, then message):
```
/part #chan :bye
/msg #chan hello
```
Expected:
- Error 404 (Cannot send to channel)
Observed:
- 

From Alice:
```
/mode #chan -n
```
From Bob (still not in channel):
```
/msg #chan hello
```
Expected:
- Message delivered to channel members
Observed:
- 

---

## 6) KICK

From Alice (op):
```
/kick #chan Bob :reason
```
Expected:
- Bob is removed from channel
Observed:
- 

From Bob (non-op, try to kick):
```
/kick #chan Alice :nope
```
Expected:
- Error 482 (You're not channel operator)
Observed:
- 

---

## 7) MODE queries and errors

### 7.1 Channel modes
Command:
```
/mode #chan
```
Expected:
- Channel modes (324) with current flags
Observed:
- 

### 7.2 Missing channel
Command:
```
/mode #missing
```
Expected:
- Error 403 (No such channel)
Observed:
- 

### 7.3 User mode query (self)
Command:
```
/mode Alice
```
Expected:
- User modes (221), usually empty
Observed:
- 

### 7.4 User mode query (other user)
Command:
```
/mode Bob
```
Expected:
- Error 502 (User mode not supported)
Observed:
- 

---

## 8) WHO / WHOIS / LUSERS / MOTD

### 8.1 WHO #chan
Command:
```
/who #chan
```
Expected:
- WHO entries (352)
- End of WHO (315)
Observed:
- 

### 8.2 WHOIS Bob
Command:
```
/whois Bob
```
Expected:
- WHOIS user (311)
- End of WHOIS (318)
Observed:
- 

### 8.3 LUSERS (server)
Command:
```
/lusers
```
Expected:
- User summary (251)
- Server summary (255)
Observed:
- 

### 8.4 LLUSERS (client)
Command:
```
/llusers
```
Expected:
- Client error “Unknown command: llusers”
Observed:
- 

### 8.5 MOTD
Command:
```
/motd
```
Expected:
- MOTD start (375)
- MOTD line(s) (372)
- MOTD end (376)
Observed:
- 

---

## 9) NOTICE behavior (no error numerics)

From Alice:
```
/notice #missing test
```
Expected:
- No error numeric
Observed:
- 

---

## 10) Unknown command

Command:
```
/quote FOO bar
```
Expected:
- Error 421 (Unknown command)
Observed:
- 
