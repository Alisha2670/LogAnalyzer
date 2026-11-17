# LogAnalyzer (C-Log)

[![Language: C99](https://img.shields.io/badge/Language-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Platform: Linux](https://img.shields.io/badge/Platform-Ubuntu%20Linux%20%7C%20POSIX-orange.svg)](https://ubuntu.com/)
[![Build: Make](https://img.shields.io/badge/Build-Makefile-green.svg)](Makefile)
[![Tests: 12 Passed](https://img.shields.io/badge/Tests-12%20Passed-brightgreen.svg)](tests/run_tests.sh)
[![Memory Safety: Valgrind](https://img.shields.io/badge/Valgrind-0%20Leaks-brightgreen.svg)](#8-memory-safety--valgrind-verification)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**LogAnalyzer** (C-Log) is a fast, robust command-line log analysis and diagnostic tool written in pure C99 for Linux and POSIX environments. It safely ingests system and application log files or live streaming logs via standard input (`stdin`), parses multi-word entries, validates data integrity, computes severity statistics, isolates root-cause recurring errors, and exports comprehensive diagnostic health reports.

---

## 1. Project Proposal & Problem Statement

Production servers, cloud clusters, and microservices generate millions of lines of unstructured logs every day. When an outage occurs, finding the root cause manually using traditional text viewers is slow, inefficient, and error-prone.

**LogAnalyzer** solves this problem by providing an intelligent diagnostic pipeline that:
* **Ingests files or live piped streams** (`cat file.log | ./clog -`) without crashing on corrupted or oversized lines.
* **Computes analytical severity metrics** (counts and error percentages) instantly.
* **Allows engineers to filter logs** by severity level, calendar date, or case-insensitive keywords—individually or combined in a single pass.
* **Identifies top recurring error messages** to pinpoint the root cause of failures.
* **Exports executive diagnostic summaries** to terminal display or custom disk destinations.
* **Dynamically scales** to arbitrary file sizes using heap memory management with **0 memory leaks**.

---

## 2. Expected Log Format

LogAnalyzer expects standard log entries formatted as:
```text
YYYY-MM-DD HH:MM:SS LEVEL MESSAGE
```

### Example:
```text
2026-09-26 10:15:22 INFO Server started successfully
2026-09-26 10:18:41 WARNING High memory consumption detected
2026-09-26 10:20:15 ERROR Database connection failed
```
* **Date:** 10-character ISO format (`YYYY-MM-DD`).
* **Time:** 8-character 24-hour format (`HH:MM:SS`).
* **Level:** Strictly validated against `INFO`, `WARNING`, or `ERROR`.
* **Message:** Multi-word sentence preserved in full without word truncation.

---

## 3. Architecture & Data Flow

```text
               +-----------------------------------------+
               |  Input Source: File on Disk or stdin (-) |
               +--------------------+--------------------+
                                    |
                                    v
               +-----------------------------------------+
               | fgets() Stream + Safe Long-Line Draining| (Prevents buffer overrun)
               +--------------------+--------------------+
                                    |
                                    v
               +-----------------------------------------+
               |             Parser Engine               | (Validates fields & log levels)
               +----------+-------------------+----------+
                          |                   |
                 Valid    |                   | Invalid
                          v                   v
               +--------------------+   +-------------------+
               | Dynamic Heap Array |   | Invalid Lines Log |
               | (malloc / realloc) |   +-------------------+
               +----------+---------+
                          |
         +----------------+----------------+----------------+
         |                                 |                |
         v                                 v                v
+-----------------+             +-----------------+   +--------------------+
|  Analyzer       |             | Search / Filter |   |  Report Generator  |
|  - Statistics   |             | - Level         |   |  - Terminal stdout |
|  - Error Rate   |             | - Date          |   |  - Disk export     |
|  - Root Causes  |             | - Keyword       |   |    (custom path)   |
|    (Top 100)    |             | - Combined      |   |  - System verdict  |
+-----------------+             +-----------------+   +--------------------+
```

---

## 4. Project Directory Layout

```text
LogAnalyzer/
├── .gitignore          # Ignores compiled binaries, objects, and test artifacts
├── Makefile            # Automated build script (all, clean, rebuild, test, valgrind)
├── README.md           # Project documentation and proposal
├── LICENSE             # Open-source MIT License
├── data/               # Test datasets
│   ├── sample.log      # Primary test log (10 lines with valid & corrupt entries)
│   ├── large_test.log  # Benchmark test log (5,000 lines)
│   ├── empty.log       # Edge-case: 0-byte file
│   └── corrupt.log     # Edge-case: 100% malformed lines
├── include/            # Header interface contracts
│   ├── parser.h        # LogEntry definition, line buffer bounds, parser prototype
│   ├── analyzer.h      # LogStats, ErrorFrequency, and analytics prototypes
│   ├── search.h        # Unified multi-criteria filtering prototype
│   └── report.h        # Diagnostic reporting prototypes
├── reports/            # Output directory for exported reports
│   └── .gitkeep        # Keeps directory tracked in version control
├── src/                # Implementation source code
│   ├── main.c          # Entry point, CLI coordinator, stdin stream handler
│   ├── parser.c        # Safe line parsing and field extraction
│   ├── analyzer.c      # Severity metrics and recurring error counter
│   ├── search.c        # Unified level, date, and keyword query filters
│   └── report.c        # Executive diagnostic report generator
└── tests/              # Automated test suite
    └── run_tests.sh    # End-to-end regression test harness (12 automated checks)
```

---

## 5. Prerequisites & Compilation

### Prerequisites
* **OS:** Ubuntu Linux, macOS, or Windows WSL / MinGW
* **Compiler:** `gcc` (with standard C99 support)
* **Build System:** `make`
* **Shell:** `bash` (for automated test suite)
* **Memory Profiler:** `valgrind` (optional, for memory leak verification)

### Building the Project
Clone the repository and compile using `make`:
```bash
git clone https://github.com/Alisha2670/LogAnalyzer.git
cd LogAnalyzer
make
```

### Running Automated Tests
Run the built-in regression test harness:
```bash
make test
```

### Memory Leak Verification
Run Valgrind leak checking on benchmark logs:
```bash
make valgrind
```

### Cleaning Artifacts
To clean compiled binaries and generated reports:
```bash
make clean
```

---

## 6. CLI Usage & Features

### 1. Overall Summary & Statistics (Default)
When executed without filter flags, LogAnalyzer provides an integrity summary and severity breakdown:
```bash
./clog data/sample.log
```
**Output:**
```text
========== VALIDATION SUMMARY ==========
Analyzed Source       : data/sample.log
Total Lines Processed : 10
Valid Entries Stored  : 8
Invalid Entries       : 2

Invalid Entries Found on Line(s):
  - Line 3
  - Line 5
========================================

========== LOG STATISTICS ==========
Total Entries : 8
INFO          : 3 (37.50%)
WARNING       : 1 (12.50%)
ERROR         : 4 (50.00%)

Error Rate    : 50.00%
====================================
```

---

### 2. Standard Input Piping (`-`)
In accordance with Unix philosophy, LogAnalyzer can ingest streaming logs directly from pipelines using `-` as the input source:
```bash
cat data/sample.log | ./clog - --errors
```
**Output:**
```text
========== FILTERED LOG RESULTS ==========
Filter Level   : ERROR
------------------------------------------
[2026-09-26 10:20:15] ERROR   Database connection failed
[2026-09-26 10:21:02] ERROR   Database connection failed
[2026-09-27 09:12:45] ERROR   Authentication failed
[2026-09-27 09:14:02] ERROR   Database connection failed
------------------------------------------
Total Matches: 4
==========================================
```
*Live pipeline examples:*
```bash
docker logs backend_container 2>&1 | ./clog - --report
journalctl -u nginx | ./clog - --recurring
```

---

### 3. Severity Level Filtering
Filter logs by severity level (`--errors`, `--warnings`, `--info`):
```bash
./clog data/sample.log --warnings
```
**Output:**
```text
========== FILTERED LOG RESULTS ==========
Filter Level   : WARNING
------------------------------------------
[2026-09-26 10:18:41] WARNING High memory usage
------------------------------------------
Total Matches: 1
==========================================
```

---

### 4. Calendar Date Filtering
Filter logs belonging to a specific date (`--date YYYY-MM-DD`):
```bash
./clog data/sample.log --date 2026-09-26
```
**Output:**
```text
========== FILTERED LOG RESULTS ==========
Filter Date    : 2026-09-26
------------------------------------------
[2026-09-26 10:15:22] INFO    Server started successfully
[2026-09-26 10:16:03] INFO    User logged in
[2026-09-26 10:18:41] WARNING High memory usage
[2026-09-26 10:20:15] ERROR   Database connection failed
[2026-09-26 10:21:02] ERROR   Database connection failed
------------------------------------------
Total Matches: 5
==========================================
```

---

### 5. Case-Insensitive Keyword Search
Search the log message body for a specific term (`--search <keyword>`):
```bash
./clog data/sample.log --search database
```
**Output:**
```text
========== FILTERED LOG RESULTS ==========
Filter Keyword : "database"
------------------------------------------
[2026-09-26 10:20:15] ERROR   Database connection failed
[2026-09-26 10:21:02] ERROR   Database connection failed
[2026-09-27 09:14:02] ERROR   Database connection failed
------------------------------------------
Total Matches: 3
==========================================
```
*(Also matches `--search DATABASE` or `--search DataBase` seamlessly).*

---

### 6. Multi-Criteria Combined Filtering
Combine date, severity level, and keyword filters simultaneously:
```bash
./clog data/sample.log --date 2026-09-26 --errors --search database
```
**Output:**
```text
========== FILTERED LOG RESULTS ==========
Filter Level   : ERROR
Filter Date    : 2026-09-26
Filter Keyword : "database"
------------------------------------------
[2026-09-26 10:20:15] ERROR   Database connection failed
[2026-09-26 10:21:02] ERROR   Database connection failed
------------------------------------------
Total Matches: 2
==========================================
```

---

### 7. Root Cause Analysis (Recurring Errors)
Isolate and rank repeating error messages to identify the primary failure point:
```bash
./clog data/sample.log --recurring
```
**Output:**
```text
========== RECURRING ERRORS (ROOT CAUSE) ==========
Occurrences  | Error Message
-------------+-------------------------------------
     3       | Database connection failed
     1       | Authentication failed
===================================================
```

---

### 8. Diagnostic Health Report & Custom Export
Generate an executive diagnostic report and export it to disk:
```bash
# Default export to reports/report.txt:
./clog data/sample.log --report --export

# Custom export destination:
./clog data/sample.log --report --export custom_audit.txt
```
**Report Content Preview:**
```text
============================================================
              C-LOG SYSTEM DIAGNOSTIC REPORT                
============================================================
Analyzed File   : data/sample.log
Total Lines     : 10

------------------------------------------------------------
1. INTEGRITY & VALIDATION SUMMARY
------------------------------------------------------------
Valid Entries Stored  : 8
Corrupted / Invalid   : 2

* Corrupted Line Numbers:
  - Line 3
  - Line 5

------------------------------------------------------------
2. LOG SEVERITY STATISTICS
------------------------------------------------------------
INFO    : 3 (37.50%)
WARNING : 1 (12.50%)
ERROR   : 4 (50.00%)

Overall Error Rate : 50.00%

------------------------------------------------------------
3. TOP RECURRING ERRORS (ROOT CAUSE)
------------------------------------------------------------
Occurrences  | Error Message
-------------+----------------------------------------------
     3       | Database connection failed
     1       | Authentication failed

------------------------------------------------------------
4. SYSTEM HEALTH VERDICT
------------------------------------------------------------
Verdict: [ CRITICAL ATTENTION REQUIRED ]
Reason : Error rate exceeds 20% threshold.
============================================================

[SUCCESS] Diagnostic report exported to: reports/report.txt
```

---

### 9. High-Volume Benchmark (5,000 Lines in ~10ms)
Benchmark processing speed on large server log datasets:
```bash
time ./clog data/large_test.log --report
```
**Output:**
```text
Analyzed File   : data/large_test.log
Total Lines     : 5000
Valid Entries   : 5000
Corrupted       : 0
Overall Error Rate : 25.18%

Verdict: [ CRITICAL ATTENTION REQUIRED ]
Reason : Error rate exceeds 20% threshold.

real    0m0.011s
user    0m0.007s
sys     0m0.004s
```

---

### 10. CLI Help Manual
```bash
./clog --help
```
**Output:**
```text
C-Log: Command-line Log Analysis & Diagnostic Tool

Usage: ./clog <path_to_log_file | -> [options]

Input:
  <path_to_log_file>    Path to a log file on disk
  -                     Read log stream from standard input (stdin)

Filtering Options (can be combined):
  --errors              Filter for ERROR logs
  --warnings            Filter for WARNING logs
  --info                Filter for INFO logs
  --date <YYYY-MM-DD>   Filter logs for a specific calendar date
  --search <keyword>    Search messages by keyword (case-insensitive)

Analysis Options:
  --recurring           Show top recurring error messages (root cause)
  --report              Generate full diagnostic health report
  --export [file]       Export diagnostic report (default: reports/report.txt)
  --help, -h            Show this help manual
```

---

## 7. Automated Testing Suite

The repository includes an automated regression test harness ([tests/run_tests.sh](tests/run_tests.sh)) that validates 12 end-to-end scenarios covering normal execution, edge cases, error exits, and streaming.

Run tests using:
```bash
make test
```
**Test Output:**
```text
==========================================
    LogAnalyzer Automated Test Suite     
==========================================
  Running: Default validation on sample.log... [PASS]
  Running: Corrupted log handling (corrupt.log)... [PASS]
  Running: Empty file handling (empty.log)... [PASS]
  Running: Severity filtering (--errors)... [PASS]
  Running: Combined filtering (--date and --errors)... [PASS]
  Running: Keyword search (--search database)... [PASS]
  Running: Standard input piping (stdin with -)... [PASS]
  Running: Report generation and custom export... [PASS]
  Running: Root cause recurring errors (--recurring)... [PASS]
  Running: Help option (--help)... [PASS]
  Running: Error on missing input file... [PASS] (Correctly failed with code 1)
  Running: Error on invalid option flag... [PASS] (Correctly failed with code 1)
==========================================
Tests Completed: 12 Passed, 0 Failed
==========================================
```

---

## 8. Memory Safety & Valgrind Verification

LogAnalyzer utilizes dynamic heap memory management (`malloc` / `realloc` / `free`) with geometric capacity doubling to scale smoothly without stack overflows or memory leaks.

Run memory analysis with Valgrind on 5,000 lines:
```bash
make valgrind
```

**Valgrind Verification Output:**
```text
==1904== HEAP SUMMARY:
==1904==     in use at exit: 0 bytes in 0 blocks
==1904==   total heap usage: 13 allocs, 13 frees, 10,972,152 bytes allocated
==1904== 
==1904== All heap blocks were freed -- no leaks are possible
==1904== 
==1904== ERROR SUMMARY: 0 errors from 0 contexts
```

---

## 9. Key Engineering Design Decisions

1. **Defensive Input Handling & Line Draining:** `fgets()` is used with safe buffer bounds (`MAX_LINE_LEN`). If a single log line exceeds buffer limits, trailing characters are drained until `\n` to prevent split "ghost" lines and preserve line counting accuracy.
2. **Proportional Buffer Geometry:** `MAX_LINE_LEN` is dynamically derived (`MAX_MSG_LEN + MAX_DATE_LEN + MAX_TIME_LEN + MAX_LEVEL_LEN + 16`). This ensures a full 1,024-byte message is never prematurely clipped by metadata headers.
3. **Hash Table Deduplication & Frequency Ranking (O(N)):** Employs a classic `djb2` hash table with separate chaining (257 buckets) for $O(1)$ average-time error deduplication (improving upon $O(N \times U)$ linear scanning). Aggregated errors are ranked in descending order of frequency via `qsort()`, instantly surfacing top root causes with 0 memory leaks.
4. **Dynamic Memory Scaling:** A geometric growth strategy (`capacity *= 2` via `realloc` using a temporary pointer) allows LogAnalyzer to scale from a handful of entries to hundreds of thousands without data loss.
5. **Unified Multi-Criteria Filter Pass:** Rather than cascading separate filter iterations, `filter_entries()` validates date, level, and keyword criteria in a single unified pass over the entries.
6. **Stream-Agnostic Reporting:** The diagnostic reporting engine accepts any `FILE *` stream, allowing identical formatting for terminal display (`stdout`) and file export (`fopen`).
7. **Pure ISO C99 Portability:** Non-standard GNU extensions (like `strcasestr`) were replaced with a standard C99 case-insensitive substring search using `tolower()`, ensuring universal compilation across GCC, Clang, and MSVC.

---

## 10. Limitations & Future Enhancements

* **Current Limitation:** Log entries must follow the standard `YYYY-MM-DD HH:MM:SS LEVEL MESSAGE` structure.
* **Future Enhancement:** JSON and CSV export formats for seamless ingestion into Prometheus, Grafana, and ELK stack.
* **Future Enhancement:** Time-series error distribution histograms to visualize error frequency spikes across hourly windows.

---

## 11. License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
