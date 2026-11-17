#!/bin/bash

# Cross-platform test runner for Linux and Windows (Git Bash)
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

if [[ -f "./clog.exe" ]]; then
    BINARY="./clog.exe"
elif [[ -f "./clog" ]]; then
    BINARY="./clog"
else
    echo -e "${RED}Error:${NC} Binary (clog or clog.exe) not found. Compile first."
    exit 1
fi

PASSED=0
FAILED=0

run_test() {
    local desc="$1"
    local cmd="$2"
    local expected="$3"

    echo -ne "  ${BLUE}Running:${NC} $desc... "
    local output
    if output=$(eval "$cmd" 2>&1); then
        if [[ -n "$expected" ]] && ! echo "$output" | grep -q "$expected"; then
            echo -e "${RED}[FAIL]${NC} (Expected '$expected' not found)"
            echo "Output was: $output"
            FAILED=$((FAILED + 1))
            return
        fi
        echo -e "${GREEN}[PASS]${NC}"
        PASSED=$((PASSED + 1))
    else
        local exit_code=$?
        if [[ "$expected" == "EXPECT_FAILURE" ]] && [[ $exit_code -ne 0 ]]; then
            echo -e "${GREEN}[PASS]${NC} (Correctly failed with code $exit_code)"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}[FAIL]${NC} (Unexpected exit code $exit_code)"
            echo "Output was: $output"
            FAILED=$((FAILED + 1))
        fi
    fi
}

echo "=========================================="
echo "    LogAnalyzer Automated Test Suite     "
echo "=========================================="

run_test "Default validation on sample.log" \
    "$BINARY data/sample.log" \
    "Valid Entries Stored  : 8"

run_test "Corrupted log handling (corrupt.log)" \
    "$BINARY data/corrupt.log" \
    "Valid Entries Stored  : 0"

run_test "Empty file handling (empty.log)" \
    "$BINARY data/empty.log" \
    "Total Lines Processed : 0"

run_test "Severity filtering (--errors)" \
    "$BINARY data/sample.log --errors" \
    "Total Matches: 4"

run_test "Combined filtering (--date and --errors)" \
    "$BINARY data/sample.log --date 2026-09-26 --errors" \
    "Total Matches: 2"

run_test "Keyword search (--search database)" \
    "$BINARY data/sample.log --search database" \
    "Total Matches: 3"

run_test "Standard input piping (stdin with -)" \
    "cat data/sample.log | $BINARY - --errors" \
    "Total Matches: 4"

run_test "Report generation and custom export" \
    "$BINARY data/sample.log --report --export reports/test_report.txt && test -s reports/test_report.txt" \
    "C-LOG SYSTEM DIAGNOSTIC REPORT"

run_test "Root cause recurring errors (--recurring)" \
    "$BINARY data/sample.log --recurring" \
    "Database connection failed"

run_test "Help option (--help)" \
    "$BINARY --help" \
    "Usage:"

run_test "Error on missing input file" \
    "$BINARY nonexistent_file.log" \
    "EXPECT_FAILURE"

run_test "Error on invalid option flag" \
    "$BINARY data/sample.log --invalid-flag" \
    "EXPECT_FAILURE"

echo "=========================================="
echo -e "Tests Completed: ${GREEN}$PASSED Passed${NC}, ${RED}$FAILED Failed${NC}"
echo "=========================================="

# Clean up test artifact
rm -f reports/test_report.txt

if [[ $FAILED -gt 0 ]]; then
    exit 1
fi
exit 0

