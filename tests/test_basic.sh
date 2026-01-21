#!/bin/bash
# tests/test_all.sh

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

pass=0
fail=0

test_case() {
    local name="$1"
    local cmd="$2"
    local expected_exit="$3"

    echo -n "Testing:$name ... "
    eval "$cmd" > /dev/null 2>&1
    ret=$?

    if [ $ret -eq $expected_exit ]; then
        echo -e "${GREEN}PASS${NC}"
        ((pass++))
    else
        echo -e "${RED}FAIL${NC} (exit code:$ret, expected:$expected_exit)"
        ((fail++))
    fi
}

echo "=== ft_ping Test Suite ==="
echo

# Tests de base
test_case "Help flag" "./ft_ping -?" 0
test_case "Missing argument" "./ft_ping" 1
test_case "Invalid hostname" "timeout 2 sudo ./ft_ping invalid.xyz.123" 1
test_case "Localhost" "timeout 3 sudo ./ft_ping 127.0.0.1" 0
test_case "Google DNS" "timeout 3 sudo ./ft_ping 8.8.8.8" 0
test_case "Hostname resolution" "timeout 3 sudo ./ft_ping google.com" 0
test_case "Verbose mode" "timeout 3 sudo ./ft_ping -v google.com" 0

echo
echo "=== Results ==="
echo -e "Passed:${GREEN}$pass${NC}"
echo -e "Failed:${RED}$fail${NC}"

if [ $fail -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed.${NC}"
    exit 1
fi