# ft_ping

A `ping` implementation in C using raw ICMP sockets — a 42 school project.

## Quick Start
```bash
make
sudo ./ft_ping google.com
```

## Features

- ICMP ECHO_REQUEST / ECHO_REPLY handling over a raw socket
- Hostname and IPv4 resolution (no reverse DNS on the reply)
- RTT statistics (min/avg/max/mdev)
- Clean shutdown on CTRL+C (SIGINT), with a final summary
- Remote ICMP errors (Destination Unreachable, Time Exceeded) reported via `recvfrom()`
- Local ICMP errors (e.g. an ARP resolution failure) reported via the socket's
  error queue (`IP_RECVERR` / `MSG_ERRQUEUE`), the same mechanism the
  reference `ping` uses for errors the kernel detects before it even sends

## Requirements

- Linux (raw ICMP sockets and `IP_RECVERR` are Linux-specific)
- GCC
- Root privileges, or `CAP_NET_RAW` on the binary (`SOCK_RAW` requires it)

## Installation
```bash
git clone https://github.com/EnzoPasselegue1/ft_ping.git
cd ft_ping
make
sudo ./ft_ping <hostname|IP>
```

## Usage
```bash
# Basic usage
sudo ./ft_ping <hostname|IP>

# Examples
sudo ./ft_ping google.com
sudo ./ft_ping 8.8.8.8
sudo ./ft_ping localhost

# Help
sudo ./ft_ping -?
```

## Options
```
-v          Verbose output (also explains ICMP errors on a probe)
-?          Show usage and exit
```

## Example Output
```
PING google.com (172.217.20.46) 56(84) bytes of data.
64 bytes from google.com (172.217.20.46): icmp_seq=1 ttl=113 time=7.624 ms
64 bytes from google.com (172.217.20.46): icmp_seq=2 ttl=113 time=7.542 ms
64 bytes from google.com (172.217.20.46): icmp_seq=3 ttl=113 time=7.564 ms
^C
--- google.com ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2023ms
rtt min/avg/max/mdev = 7.542/7.577/7.624/0.035 ms
```

## Project Structure
```
ft_ping/
├── includes/
│   └── ft_ping.h           # Shared structs, constants, prototypes
├── srcs/
│   ├── main.c               # Orchestration: setup, then the send/receive loop
│   ├── parser.c             # -v / -? argument parsing
│   ├── dns.c                # Hostname/IP resolution (getaddrinfo)
│   ├── socket.c              # Raw socket creation, SO_RCVTIMEO, IP_RECVERR
│   ├── icmp.c                # ICMP packet construction + checksum
│   ├── send_receive.c        # sendto()/recvfrom()/select() + local-error polling
│   ├── display.c             # Reply and error line formatting
│   ├── stats.c                # min/avg/max/mdev accumulation and summary
│   ├── timing.c               # timeval → ms conversion (RTT, total time)
│   └── signal.c               # SIGINT handling for a clean shutdown
├── Makefile
└── README.md
```

## Technical Details

### ICMP Packet Structure
```
IP Header:      20 bytes
ICMP Header:     8 bytes
  - Type (8)
  - Code (0)
  - Checksum
  - Identifier
  - Sequence Number
Data:           56 bytes
  - A timeval, then a filler pattern
Total:          84 bytes on the wire
```

### How It Works

1. Parse command-line arguments
2. Resolve the hostname or IP argument
3. Open a raw ICMP socket (`SO_RCVTIMEO` + `IP_RECVERR`)
4. Install the SIGINT handler and print the header line
5. Loop until interrupted (CTRL+C):
   - build and send an ICMP Echo Request
   - wait up to ~1s for a reply, an ICMP error (local or remote), or silence
   - update the RTT statistics on a reply
6. Print the final summary and close the socket

## Compilation
```bash
make          # Compile the project
make clean    # Remove object files
make fclean   # Remove object files and the binary
make re       # Rebuild everything
```

## Known Limitations

- IPv4 only (no IPv6 support)
- Linux-specific (`SOCK_RAW` + `IP_RECVERR`/`MSG_ERRQUEUE`)
- Requires root or `CAP_NET_RAW`

## Resources

- RFC 792: Internet Control Message Protocol
- RFC 1071: Computing the Internet Checksum
- `man 7 raw`, `man 2 setsockopt` (`IP_RECVERR`)

## Troubleshooting

### "Lacking privilege for raw socket"
Run with `sudo` — `SOCK_RAW` requires `CAP_NET_RAW`.

### "Name or service not known"
Check DNS configuration, or pass a direct IPv4 address instead.

## Author

École 42 Lyon — 2026
