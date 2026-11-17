#ifndef ANALYZER_H
#define ANALYZER_H

#include "parser.h"

// Maximum unique errors to display in root cause ranking table
#define MAX_UNIQUE_ERRORS 100

// Summary severity statistics
typedef struct {
    int total;
    int info_count;
    int warning_count;
    int error_count;
    float error_rate;
} LogStats;

// Frequency counter for a unique error message
typedef struct {
    char message[MAX_MSG_LEN];
    int count;
} ErrorFrequency;

void analyze_logs(const LogEntry entries[], int count, LogStats *stats);
void print_statistics(const LogStats *stats);

// Deduplicates errors using a hash table and returns sorted top frequencies
int count_recurring_errors(const LogEntry entries[], int count, ErrorFrequency freqs[], int max_freqs, int *overflow_count);

void find_recurring_errors(const LogEntry entries[], int count);

#endif
