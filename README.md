*This project has been created as part of the 42 curriculum by cacheoeil.*

# ft_irc

## Description

**ft_irc** is an Internet Relay Chat (IRC) server written in C++98 as part of the 42 school project. The goal is to implement a functional IRC server that handles multiple clients simultaneously using **non-blocking I/O** and a single process, following the IRC protocol (RFC 1459 / RFC 2812).

Clients connect over TCP, authenticate with a server password, register with `NICK` and `USER`, then interact through standard IRC commands: joining and leaving channels, sending private and channel messages, managing topics, inviting users, kicking members, and setting user/channel modes.

The server is built around three core modules:

- **server** — socket setup, `epoll`-based event loop, connection lifecycle, command dispatch
- **client** — per-connection state (nickname, buffers, registration, channels joined)
- **channel** — channel membership, operators, modes, topic, invite list

## Features

| Category | Supported commands |
|---|---|
| Connection | `CAP`, `PASS`, `NICK`, `USER`, `PING`, `QUIT` |
| Messaging | `PRIVMSG` (users and channels), built-in `Bot` (`ping` → `pong`) |
| Channels | `JOIN`, `PART`, `TOPIC`, `INVITE`, `KICK` |
| Modes | `MODE` (user `+i`, channel `+/-itkol`) |

Additional behaviour:

- IRC case-insensitive nick/channel comparison (`irc_utils.hpp`)
- Non-blocking read/write with output buffering
- Graceful shutdown on `SIGINT` / `SIGTERM`
- Compiled with `-Wall -Wextra -Werror`

## Project structure

```
ft_irc/
├── main.cpp              # Entry point, signal handling, argument parsing
├── Makefile
├── includes/             # Headers
│   ├── server.hpp
│   ├── client.hpp
│   ├── channel.hpp
│   └── irc_utils.hpp
├── server/
│   ├── server.cpp        # Network I/O and event loop
│   ├── server_utils.cpp  # Shared helpers (send, lookup, registration)
│   ├── cmd_dispatch.cpp  # Command parsing and routing
│   ├── cmd_registration.cpp
│   ├── cmd_channels.cpp
│   ├── cmd_mode.cpp
│   └── cmd_messaging.cpp
├── client/
│   └── client.cpp
└── channel/
    └── channel.cpp
```

## Instructions

### Requirements

- A C++ compiler with C++98 support (`g++` / `clang++`)
- Linux (uses `epoll`, `socket`, `fcntl`)
- An IRC client for testing (e.g. **irssi**, **HexChat**, **WeeChat**)

### Compilation

```bash
make
```

Other targets:

```bash
make clean    # remove object files
make fclean   # remove object files and binary
make re       # rebuild from scratch
```

This produces the executable **`ircserv`** at the project root.

### Execution

```bash
./ircserv <port> <password>
```

| Argument | Description |
|---|---|
| `port` | TCP port to listen on (1–65535) |
| `password` | Server connection password (required for `PASS`) |

Example:

```bash
./ircserv 6667 mysecretpassword
```

Stop the server with `Ctrl+C`.

### Testing with an IRC client

Example with **irssi**:

```bash
irssi
```

Inside irssi:

```
/connect localhost 6667 mysecretpassword
/nick alice
/join #general
/msg #general Hello everyone!
/msg Bot ping
/quit
```

Replace `6667` and `mysecretpassword` with the values used when starting the server.

## Resources

### IRC protocol and networking

- [RFC 1459 — Internet Relay Chat Protocol](https://www.rfc-editor.org/rfc/rfc1459)
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812)
- [RFC 2813 — Internet Relay Chat: Server Protocol](https://www.rfc-editor.org/rfc/rfc2813)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- `man 7 epoll`, `man 2 poll`, `man 2 socket`, `man 2 fcntl`

### 42 subject and references

- [42 ft_irc subject (PDF)](https://github.com/rphlr/42-Subjects/) — community mirror; refer to your local subject for the official version
- [Modern IRC documentation (ircdocs.horse)](https://modern.ircdocs.horse/)

### AI usage

AI (Cursor / Claude) was used during this project for the following tasks:

| Task | Details |
|---|---|
| **Architecture refactoring** | Splitting the monolithic `server_commands.cpp` into separate files by responsibility (`cmd_dispatch`, `cmd_registration`, `cmd_channels`, `cmd_mode`, `cmd_messaging`, `server_utils`) without changing business logic |
| **`KICK` command** | Completing the unfinished implementation and refactoring it to an iterative pattern (`kick` + `kickOneChannel`), consistent with `JOIN` |
| **Build fixes** | Resolving `-Wall -Wextra -Werror` warnings (unused parameters, sign comparisons, member initialization order) |
| **`includes/` layout** | Moving headers into a shared `includes/` directory and updating Makefile include paths |
| **Documentation** | Drafting this README according to the 42 README requirements |

All generated suggestions were reviewed and integrated manually. Core protocol logic, class design, and final code decisions remain the responsibility of the project author.
