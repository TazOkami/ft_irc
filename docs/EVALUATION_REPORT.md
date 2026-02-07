ft_irc Evaluation Checklist (ft_irc.pdf)
Date: 2026-01-28
Reference client: irssi 1.4.5

Basic checks
- Makefile / build / binary name: PASS
  - Command: make
  - Result: binary ./ircserv present
- Single poll() in code: PASS
  - Evidence: rg -n "poll(" src include -> only Poller.cpp wrapper and call site
- poll() before accept/read/write: PASS (code review)
  - Event loop in src/Server_Loop.cpp calls poller.wait() then dispatches handleAccept/handleRead/handleWrite on revents
- fcntl usage: PASS
  - Evidence: only fcntl(fd, F_SETFL, O_NONBLOCK) in src/Utils.cpp

Networking
- Listens on all interfaces: PASS
  - Evidence: lsof -nP -iTCP:6720 -sTCP:LISTEN -> TCP *:6720 (LISTEN)
- nc connect + server replies: PASS
  - Command: printf 'PASS..NICK..USER..PING :ping1\r\n' | nc -w 2 127.0.0.1 6720
  - Output includes:
    - :mini-irc 001 nctest :Welcome...
    - :mini-irc PONG mini-irc :ping1
- Reference client (irssi) can connect: PASS
  - Command: irssi --home /tmp/irssi_eval -! -c 127.0.0.1 -p 6720 -n EvalBot -w pass
  - Evidence: welcome numerics shown (001-004)
- Multiple connections simultaneously: PASS
  - multi1 connected while multi2 PING/PONG succeeds
- Channel broadcast: PASS
  - recv side saw: :send4!send4@mini-irc PRIVMSG #chan4 :hello_chan4

Networking specials
- Partial command (nc) + other clients still OK: PASS
  - Partial send: "PI" + "NG :com" + "mand\r\n" -> PONG :command observed
  - Other connection during partial also got PONG
- Unexpectedly kill a client: PASS
  - Killed nc process; new client still gets PONG
- Kill nc mid-command: PASS
  - Half command sent then connection closed; new client still gets PONG
- Stop client (^Z) + flood + resume: PASS
  - SIGSTOP receiver, 20 PRIVMSG flooded, SIGCONT -> receiver got 20 messages
- Memory leaks during flood: NOT VERIFIED
  - valgrind/leaks not run

Client commands
- Auth/NICK/USER/JOIN via nc: PASS
- Auth/NICK/USER/JOIN via irssi: PASS
- PRIVMSG + NOTICE (channel/user): PASS
  - Receiver saw NOTICE #notice and NOTICE user

Operator privileges / permissions
- Non-op blocked for MODE +i, TOPIC (with +t), and KICK: PASS (482)
- INVITE behavior (RFC-aligned):
  - Non-op member can INVITE when channel is not +i (saw RPL_INVITING)
  - +i requires chanop to INVITE (non-op got 482 when +i set)
  - ERR_NOSUCHNICK / ERR_USERONCHANNEL / ERR_NOTONCHANNEL handled
  - Note: this differs from strict op-only expectations in older checklists

Bonus
- File transfer: NOT IMPLEMENTED / NOT TESTED
- Bot: IMPLEMENTED later (`tools/ircbot.cpp`), not covered in this report

Tests performed (command appendix)
- Build: make
- nc basic connect:
  - printf 'PASS pass\r\nNICK nctest\r\nUSER u 0 * :u\r\nPING :ping1\r\n' | nc -w 2 127.0.0.1 6720
- Multi-connection (simultaneous):
  - (printf 'PASS pass\r\nNICK multi1\r\nUSER u 0 * :u\r\n'; sleep 5) | nc -w 7 127.0.0.1 6720 > /tmp/multi1.out &
  - printf 'PASS pass\r\nNICK multi2\r\nUSER u 0 * :u\r\nPING :multi\r\n' | nc -w 2 127.0.0.1 6720
- Channel broadcast:
  - Receiver: (printf 'PASS pass\r\nNICK recv4\r\nUSER u 0 * :u\r\nJOIN #chan4\r\n'; tail -f /dev/null) | nc 127.0.0.1 6720 > /tmp/recv4.out &
  - Sender: printf 'PASS pass\r\nNICK send4\r\nUSER u 0 * :u\r\nJOIN #chan4\r\nPRIVMSG #chan4 :hello_chan4\r\n' | nc -w 2 127.0.0.1 6720
- Partial command:
  - { printf 'PASS pass\r\nNICK part\r\nUSER u 0 * :u\r\n'; printf 'PI'; sleep 0.5; printf 'NG :com'; sleep 0.5; printf 'mand\r\n'; } | nc -w 2 127.0.0.1 6720
- Partial command + other client:
  - (partial sender) & printf 'PASS pass\r\nNICK other2\r\nUSER u 0 * :u\r\nPING :ok\r\n' | nc -w 2 127.0.0.1 6720
- Kill client:
  - (printf 'PASS pass\r\nNICK killme\r\nUSER u 0 * :u\r\n'; tail -f /dev/null) | nc 127.0.0.1 6720 & then kill <pid>
  - verify: printf 'PASS pass\r\nNICK alive\r\nUSER u 0 * :u\r\nPING :alive\r\n' | nc -w 2 127.0.0.1 6720
- Half command then disconnect:
  - printf 'PASS pass\r\nNICK half\r\nUSER u 0 * :u\r\nPRIV' | nc -w 1 127.0.0.1 6720
  - verify: printf 'PASS pass\r\nNICK afterhalf\r\nUSER u 0 * :u\r\nPING :ok\r\n' | nc -w 2 127.0.0.1 6720
- SIGSTOP flood test:
  - receiver: (printf 'PASS pass\r\nNICK stopme\r\nUSER u 0 * :u\r\nJOIN #flood\r\n'; tail -f /dev/null) | nc 127.0.0.1 6720 > /tmp/stopme.out &
  - kill -STOP <pid>
  - sender: printf 'PASS pass\r\nNICK flooder\r\nUSER u 0 * :u\r\nJOIN #flood\r\n' then 20 PRIVMSG lines
  - kill -CONT <pid>
- NOTICE channel/user:
  - receiver: (printf 'PASS pass\r\nNICK nrecv\r\nUSER u 0 * :u\r\nJOIN #notice\r\n'; tail -f /dev/null) | nc 127.0.0.1 6720 > /tmp/nrecv.out &
  - sender: printf 'PASS pass\r\nNICK nsend\r\nUSER u 0 * :u\r\nJOIN #notice\r\nNOTICE #notice :notice_chan\r\nNOTICE nrecv :notice_user\r\n' | nc -w 2 127.0.0.1 6720
- Operator tests (nc, RFC behavior, port 6726):
  - target: (printf 'PASS pass\r\nNICK target\r\nUSER u 0 * :u\r\n'; sleep 25) | nc -w 28 127.0.0.1 6726 > /tmp/target_eval5.out &
  - op:     (printf 'PASS pass\r\nNICK op\r\nUSER u 0 * :u\r\nJOIN #perm\r\n'; sleep 1; printf 'MODE #perm +t\r\n'; sleep 10; printf 'MODE #perm +i\r\nMODE #perm +k key\r\nMODE #perm +l 5\r\nMODE #perm +o member\r\nINVITE target #perm\r\nKICK #perm member :bye\r\n'; sleep 5) | nc -w 28 127.0.0.1 6726 > /tmp/op_eval5.out &
  - member: (printf 'PASS pass\r\nNICK member\r\nUSER u 0 * :u\r\nJOIN #perm\r\n'; sleep 2; printf 'MODE #perm +i\r\nTOPIC #perm :nope\r\nKICK #perm op :nope\r\nINVITE target #perm\r\n'; sleep 8) | nc -w 28 127.0.0.1 6726 > /tmp/member_eval5.out &
- irssi connect:
  - irssi --home /tmp/irssi_eval -! -c 127.0.0.1 -p 6720 -n EvalBot -w pass

Memory leaks (leaks)
- Tool: /usr/bin/leaks
- Command: leaks <ircserv_pid>
- Result: 0 leaks for 0 total leaked bytes
- Note: leaks run while server was idle after a JOIN/PRIVMSG session
