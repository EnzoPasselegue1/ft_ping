#!/bin/bash
# tests/compare_with_ping.sh

HOST="google.com"
COUNT=5

echo "=== Comparison: ft_ping vs ping ==="
echo

echo "Running ft_ping..."
sudo timeout ${COUNT} ./ft_ping $HOST > /tmp/ft_ping_output.txt 2>&1

echo "Running system ping..."
timeout ${COUNT} ping -4 $HOST > /tmp/ping_output.txt 2>&1

echo
echo "ft_ping output:"
cat /tmp/ft_ping_output.txt

echo
echo "ping output:"
cat /tmp/ping_output.txt

echo
echo "Compare the RTT values (should be ±30ms)"