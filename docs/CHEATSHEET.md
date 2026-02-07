# ft_irc Study Cheat Sheet

A rapid orientation guide for teammates who need to understand this IRC server
implementation quickly.

## Architecture at a Glance

- **Process**: single-threaded event loop using `poll(2)` on non-blocking sockets.
- **Core objects**: `Server` owns the listening socket, `Poller`, `clients` map, and
  `channels` map. Each `Client` stores protocol state (nick, user, buffers). Each
  `Channel` tracks members, operators, topic, and mode flags.
- **Source layout**:
  - `src/Server_Loop.cpp`: lifecycle (`start`, `run`, `disconnect`).
  - `src/Server_IO.cpp`: accept/read/write helpers plus buffering.
  - `src/Server_Dispatch.cpp`: parses raw lines and routes to handlers.
  - `src/Handlers_*.cpp`: implement the protocol commands (core, channel, bonus).
  - `src/Utils.cpp`: helper utilities (lowercase, trim, channel check, O_NONBLOCK).

## Event Flow

1. `main` parses CLI, builds `Server`, and calls `start`/`run`.
2. `Server::run` loops on `poller.wait` and calls `handleAccept`, `handleRead`,
   `handleWrite`, or `disconnect` depending on events.
3. `handleRead` fills `Client::readBuf`, `processIncomingLines` slices CRLF lines,
   `dispatchLine` converts them to `IrcMessage` objects, and the appropriate
   `h_*` handler executes.
4. Responses are enqueued via `queueLine`, and `handleWrite` flushes them.

## poll(2) at a Glance

```
poll() -> ready fds
  listen fd + POLLIN   => accept()
  client fd + POLLIN   => recv() -> parse -> dispatch
  client fd + POLLOUT  => send() buffered output
  fd + HUP/ERR/NVAL    => disconnect()
```

Key idea: `poll()` tells you which sockets are safe to read/write so the server
never blocks on one client.

## Command Handlers (implemented)

| Command | Location | Purpose |
|---------|----------|---------|
| `PASS`/`NICK`/`USER` | `Handlers_Registration.cpp` | Registration flow; PASS must come before NICK/USER when a password is set. |
| `PING`/`PONG` | `Handlers_Extras.cpp` | Keep connections alive / avoid unknown-command noise. |
| `PRIVMSG` | `Handlers_Core.cpp` | Deliver channel or direct messages. |
| `JOIN` | `Handlers_Core.cpp` | Maintain membership and broadcast JOIN. |
| `PART`/`NAMES` | `Handlers_Extras.cpp` | Leave a channel and request roster numerics. |
| `TOPIC` | `Handlers_Core.cpp` | View/change channel topic (subject-required). |
| `MODE` (`i/t/k/o/l` + optional `n`) | `Handlers_Channel_Modes.cpp` | Channel mode changes and queries. |
| `INVITE`/`KICK` | `Handlers_Core.cpp` | Manage access and moderation. |

Extra commands (`NOTICE`, `WHO`, `WHOIS`, `LUSERS`, `MOTD`, CAP) are in
`Handlers_Extras.cpp` and mirror common client expectations.

## Data Structures

- `clients`: `std::map<int, Client>` keyed by socket fd.
- `nickToFd`: lowercase nick → fd map for fast lookups.
- `channels`: `std::map<std::string, Channel>` storing member sets, operator sets,
  topics, keys, and limits.

## Adding/Modifying Commands

1. Update `Server.hpp` with new handler signatures if needed.
2. Implement logic in the relevant `Handlers_*.cpp` file.
3. Add dispatch case inside `Server_Dispatch.cpp::dispatchLine`.
4. Update numerics in `include/Numerics.hpp` or constants as required.

## Testing Workflow

- Build: `make`
- Manual checklist: connect via irssi/weechat (or `nc`) and walk through
  PASS/NICK/USER, channel creation, topic/mode toggles, INVITE/KICK, and optional
  extra commands.
- Irssi-only guide: `docs/IRSSI.md`

## Quick Reference Commands

```text
PASS <password>
NICK <nick>
USER <user> 0 * :Real Name
JOIN #channel
MODE #channel +itkol <key> <limit>
INVITE <nick> #channel
KICK #channel <nick> :reason
PRIVMSG #channel :message
```

To connect manually:

1. Terminal A: `./ircserv 6667 secretpass`
2. Terminal B: `nc -v 127.0.0.1 6667`
3. Type the IRC commands above inside Terminal B (each on its own line). The shell prompt is **not** where you run `PASS`/`NICK`; those go through the TCP connection managed by `nc`.

Alternatively, pipe them via `nc` automatically:

```bash
printf 'PASS secretpass\r\nNICK piped\r\nUSER user 0 * :Pipe\r\nJOIN #nc\r\nPRIVMSG #nc :hello\r\nQUIT :bye\r\n' | nc -v 127.0.0.1 6667
```

## Glossary

- **poll(2)**: POSIX system call the server uses to monitor all sockets (listen,
  clients) for readability/writability in a single loop.
- **Client**: Represents a connected socket plus nick/user state. Stored in
  `Server::clients` keyed by fd.
- **Channel**: Keeps member FDs, operator set, topic, key, and +i/+t/+k/+l flags.
- **PASS/NICK/USER**: Three-step registration (PASS required if configured).
  PASS must be sent before NICK/USER when a password is set.
- **JOIN/PART**: Adds/removes a client from a channel, triggering broadcasts and
  `RPL_NAMREPLY`/`RPL_ENDOFNAMES` numerics.
- **PRIVMSG/NOTICE**: Deliver text either to a channel (fan-out) or to a single
  user (looked up via `nickToFd`). NOTICE never triggers numeric errors.
- **MODE (i/t/k/o/l; +n optional)**: Channel operator commands enabling
  invite-only, topic lock, key protection, operator assignment, user limits, and
  optional no-external-messages.
- **INVITE/KICK**: Grants access to invite-only channels or removes disruptive
  users, updating membership sets accordingly.
- **WHO/WHOIS/LUSERS/MOTD**: Optional informational commands implemented in
  `Handlers_Extras.cpp` for a more complete IRC experience.

Keep `subject_IRC.pdf` nearby for authoritative behavior details.

## Evaluation Quick Answers

- **How do we prove everything is non-blocking?** Think of `poll(2)` as a
  waiter: it watches multiple sockets and taps us on the shoulder when one is
  ready. We never block on a single client because we:
  1. call `setNonBlocking` (see `Utils.cpp`) on the listening socket and every
     accepted client so `recv`/`send` return immediately if no data is pending;
  2. sit in `Server::run`, which calls `Poller::wait` to learn which fds are
     readable/writable; only then do `handleAccept/handleRead/handleWrite`
     perform I/O; and
  3. avoid `sleep`, busy loops, or blocking syscalls anywhere else. If the
  evaluator asks, point them to `Server_IO.cpp` to show that `recv`/`send`
  always check for `EAGAIN` rather than waiting.
- **Que signifient les "événements" ?** Après `poller.wait`, chaque entrée
  `pollfd` possède un champ `revents` avec des drapeaux POSIX :
  - `POLLIN` : ce descripteur est lisible. Sur le socket d'écoute, ça veut dire
    qu'un client attend un `accept`. Sur un client, `handleRead` peut récupérer
    des octets sans bloquer.
  - `POLLOUT` : on peut écrire sans bloquer : c'est le signal pour vider notre
    buffer via `handleWrite`.
  - `POLLHUP`, `POLLERR`, `POLLNVAL` : le pair a fermé la connexion ou le fd est
    invalide, donc on appelle `disconnect`.
  La boucle `Server::run` regarde simplement ces bits et appelle la fonction
  adaptée, ce qui permet de traiter des dizaines de sockets dans un seul thread.
- **What commands are mandatory vs. extra?** Subject-required: `PASS`, `NICK`,
  `USER`, `PRIVMSG`, `JOIN`, `TOPIC`, `MODE` (+itkol), `INVITE`, `KICK`
  (plus channel broadcast). Implemented extras: `PART`, `NAMES`, `NOTICE`,
  `WHO`, `WHOIS`, `LUSERS`, `MOTD`, minimal CAP, `PING`/`PONG`, and `MODE +n`.
- **How do we test quickly during defense?** Use the `nc` recipe above or the
  `irssi` command (`irssi --home /tmp/irssi_tmp -! -c 127.0.0.1 -p 6667 -n TestBot -w secretpass`).
- **Where are operator permissions enforced?** `Handlers_Channel_Modes.cpp`
  checks membership/operator status for channel MODE changes. `Handlers_Core.cpp`
  enforces op checks for `KICK` and `TOPIC` (when +t), and requires ops for
  `INVITE` only when the channel is `+i`.
- **How are numerics generated?** `Handlers_Utils.cpp::sendNumeric` uses the
  constants declared in `include/Numerics.hpp` (e.g. `RPL_WELCOME`,
  `ERR_NOSUCHNICK`). Every handler calls it with the appropriate numeric so all
  replies are centralized and consistent with the subject.
