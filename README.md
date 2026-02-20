# ft_ping

A simple ping implementation in C using raw sockets.

## Quick Start
```bash
make
sudo ./ft_ping google.com
```

## Features

- ICMP ECHO_REQUEST/REPLY handling
- DNS resolution
- RTT statistics (min/avg/max/mdev)
- Signal handling (Ctrl+C)
- Packet loss calculation

## Requirements

- Linux operating system
- GCC compiler
- Root privileges (sudo)
- Standard C library

## Installation
```bash
# Clone the repository
git clone https://github.com/yourusername/ft_ping.git
cd ft_ping

# Compile
make

# Run
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
sudo ./ft_ping -h
```

## Options
```
-h          Display help message
-v          Verbose output
```

## Example Output
```
PING google.com (172.217.20.46) 56(64) bytes of data.
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
│   └── ft_ping.h           # Header file
├── srcs/
│   ├── main.c              # Entry point
│   ├── parser.c            # Argument parsing
│   ├── dns.c               # DNS resolution
│   ├── socket.c            # Socket creation
│   ├── icmp.c              # ICMP packet handling
│   ├── ping.c              # Send/receive functions
│   └── utils.c             # Helper functions
├── tests/
│   └── mega_test.sh        # Test suite
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
  - ID
  - Sequence
Data:           56 bytes
  - Timestamp
  - Padding
Total:          84 bytes
```

### How It Works

1. Parse command-line arguments
2. Resolve hostname to IP address
3. Create raw ICMP socket
4. Build ICMP ECHO_REQUEST packet
5. Send packet to destination
6. Wait for ICMP ECHO_REPLY
7. Calculate RTT and update statistics
8. Display results and statistics on exit

## Testing
```bash
# Run test suite
sudo ./tests/mega_test.sh

# Test with valgrind
sudo valgrind --leak-check=full ./ft_ping localhost

# Manual testing
sudo ./ft_ping localhost
sudo ./ft_ping 8.8.8.8
sudo ./ft_ping google.com
```

## Compilation
```bash
make          # Compile project
make clean    # Remove object files
make fclean   # Remove object files and binary
make re       # Recompile everything
```

## Known Limitations

- IPv4 only (no IPv6 support)
- Basic functionality only
- Requires root privileges
- Linux-specific implementation

## Resources

- RFC 792: Internet Control Message Protocol
- RFC 1071: Computing the Internet Checksum
- man ping: System ping documentation

## Troubleshooting

### "Operation not permitted"
Run with sudo. Raw sockets require root privileges.

### "Unknown host"
Check DNS configuration or use direct IP address.

### "Network is unreachable"
Check network connectivity and firewall settings.

## Author

Ecole 42 Lyon - 2026

## License

This project is licensed under the MIT License.
