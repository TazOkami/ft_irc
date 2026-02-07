# IA Findings (Codex review, 2026-01-27)

This is a historical log of fixes applied during review. File references below
reflect the **current** code layout.

## Critical - FIXED

- TOPIC empty clear: `TOPIC #chan :` clears topic (uses `hasTrailing` to distinguish query vs set). `Handlers_Core.cpp`
- TOPIC membership: non-members get `ERR_NOTONCHANNEL` when trying to change topic. `Handlers_Core.cpp`
- PASS/NICK/USER order (RFC 2812): PASS must be sent before NICK/USER when a password is set; NICK/USER before PASS returns 451. `Server_Dispatch.cpp`
- PASS after NICK/USER: rejected until PASS; registration completes after PASS+NICK+USER. `Handlers_Registration.cpp`

Tests performed:
- `nc`: PASS-first registration returns 001-004; NICK/USER before PASS gets 451 until PASS is sent.
- `irssi`: connects cleanly (PASS sent first); welcome numerics shown.

## Protocol / Permission mismatches (RFC alignment) - FIXED (2026-01-28)

- INVITE allows non-op members unless +i; +i requires chanop. `Handlers_Core.cpp`
- INVITE errors on unknown nick (`ERR_NOSUCHNICK`). `Handlers_Core.cpp`
- INVITE errors when target already in channel (`ERR_USERONCHANNEL`). `Handlers_Core.cpp`
- INVITE allows invites to non-existent channels (RFC-style). `Handlers_Core.cpp`
- INVITE errors if inviter is not on the channel (`ERR_NOTONCHANNEL`). `Handlers_Core.cpp`
- MODE #chan b checks channel existence/membership before end-of-ban-list. `Handlers_Channel_Modes.cpp`
- MODE query on missing channel includes nick in `ERR_NOSUCHCHANNEL`. `Handlers_Channel_Modes.cpp`

## Command behavior mismatches (RFC alignment) - FIXED (2026-01-28)

- NAMES without parameters lists visible channels (or `*` if none). `Handlers_Extras.cpp`
- PRIVMSG to a channel when not on it returns `ERR_CANNOTSENDTOCHAN` (404). `Handlers_Core.cpp`
- PING without parameters returns `ERR_NOORIGIN` (409). `Handlers_Extras.cpp`
- PASS mismatch disconnects after `ERR_PASSWDMISMATCH`. `Handlers_Registration.cpp`
- JOIN 0 leaves all channels. `Handlers_Core.cpp`
- JOIN parses comma-separated channel/key lists. `Handlers_Core.cpp`

## Edge / compatibility - FIXED (2026-01-28)

- Re-JOIN returns `ERR_USERONCHANNEL` instead of re-broadcasting JOIN. `Handlers_Core.cpp`
- Invite list cleared after successful join (no indefinite rejoin without new INVITE). `Handlers_Core.cpp`
- MODE change output groups signs (+it instead of +i+t). `Handlers_Channel_Modes.cpp`
- Parser skips repeated spaces (no empty params on `"NICK  foo"`). `Parser.cpp`
- Channel name validation rejects invalid names like `#` or `#a,b`. `Utils.cpp`
- `handleAccept` drains all pending connections per poll wakeup. `Server_IO.cpp`
- Write buffer growth is capped; lines are dropped once `MAX_WRITEBUF` would be exceeded. `Server_IO.cpp`
