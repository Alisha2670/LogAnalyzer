#include <stdio.h>
#include <string.h>
#include "report.h"
#include "analyzer.h"

// Formats full diagnostic summary and error rankings to any output stream
static void write_report_content(FILE *stream, const char *filepath, int total_lines,
                                const LogEntry entries[], int valid_count,
                                const int invalid_lines[], int invalid_count) {
    LogStats stats;
    analyze_logs(entries, valid_count, &stats);

    fprintf(stream, "============================================================\n");
    fprintf(stream, "              C-LOG SYSTEM DIAGNOSTIC REPORT                \n");
    fprintf(stream, "============================================================\n");
    fprintf(stream, "Analyzed Source : %s\n", filepath);
    fprintf(stream, "Total Lines     : %d\n", total_lines);

    fprintf(stream, "\n------------------------------------------------------------\n");
    fprintf(stream, "1. INTEGRITY & VALIDATION SUMMARY\n");
    fprintf(stream, "------------------------------------------------------------\n");
    fprintf(stream, "Valid Entries Stored  : %d\n", valid_count);
    fprintf(stream, "Corrupted / Invalid   : %d\n", invalid_count);

    if (invalid_count > 0) {
        fprintf(stream, "\n* Corrupted Line Numbers:\n");
        int display_limit = (invalid_count < MAX_INVALID_ENTRIES) ? invalid_count : MAX_INVALID_ENTRIES;
        for (int i = 0; i < display_limit; i++) {
            fprintf(stream, "  - Line %d\n", invalid_lines[i]);
        }
        if (invalid_count > MAX_INVALID_ENTRIES) {
            fprintf(stream, "  - ... and %d more corrupted line(s)\n", invalid_count - MAX_INVALID_ENTRIES);
        }
    }

    fprintf(stream, "\n------------------------------------------------------------\n");
    fprintf(stream, "2. LOG SEVERITY STATISTICS\n");
    fprintf(stream, "------------------------------------------------------------\n");
    if (stats.total > 0) {
        fprintf(stream, "INFO    : %d (%.2f%%)\n", stats.info_count,
                ((float)stats.info_count / stats.total) * 100.0f);
        fprintf(stream, "WARNING : %d (%.2f%%)\n", stats.warning_count,
                ((float)stats.warning_count / stats.total) * 100.0f);
        fprintf(stream, "ERROR   : %d (%.2f%%)\n", stats.error_count,
                ((float)stats.error_count / stats.total) * 100.0f);
    }
    fprintf(stream, "\nOverall Error Rate : %.2f%%\n", stats.error_rate);

    fprintf(stream, "\n------------------------------------------------------------\n");
    fprintf(stream, "3. TOP RECURRING ERRORS (ROOT CAUSE)\n");
    fprintf(stream, "------------------------------------------------------------\n");

    ErrorFrequency freqs[MAX_UNIQUE_ERRORS];
    int overflow = 0;
    int unique_count = count_recurring_errors(entries, valid_count, freqs, MAX_UNIQUE_ERRORS, &overflow);

    if (unique_count == 0) {
        fprintf(stream, "No errors detected.\n");
    } else {
        fprintf(stream, "%-12s | %s\n", "Occurrences", "Error Message");
        fprintf(stream, "-------------+----------------------------------------------\n");
        for (int i = 0; i < unique_count; i++) {
            fprintf(stream, "     %-7d | %s\n", freqs[i].count, freqs[i].message);
        }
        if (overflow > 0) {
            fprintf(stream, "-------------+----------------------------------------------\n");
            fprintf(stream, "Note: Showing top %d unique errors (%d additional unique message(s) omitted).\n",
                    MAX_UNIQUE_ERRORS, overflow);
        }
    }

    // Determine system health based on error rate thresholds
    fprintf(stream, "\n------------------------------------------------------------\n");
    fprintf(stream, "4. SYSTEM HEALTH VERDICT\n");
    fprintf(stream, "------------------------------------------------------------\n");
    if (stats.error_rate > 20.0f) {
        fprintf(stream, "Verdict: [ CRITICAL ATTENTION REQUIRED ]\n");
        fprintf(stream, "Reason : Error rate exceeds 20%% threshold.\n");
    } else if (stats.error_rate > 5.0f) {
        fprintf(stream, "Verdict: [ WARNING - ELEVATED ERRORS ]\n");
        fprintf(stream, "Reason : Error rate exceeds 5%% threshold.\n");
    } else {
        fprintf(stream, "Verdict: [ STABLE / HEALTHY ]\n");
        fprintf(stream, "Reason : Error rate is within normal operating limits.\n");
    }
    fprintf(stream, "============================================================\n");
}

// Prints report to stdout, and exports to disk if export_filepath is non-NULL
void generate_report(const char *filepath, int total_lines,
                     const LogEntry entries[], int valid_count,
                     const int invalid_lines[], int invalid_count,
                     const char *export_filepath) {
    
    write_report_content(stdout, filepath, total_lines, entries, valid_count, invalid_lines, invalid_count);

    if (export_filepath != NULL) {
        FILE *file = fopen(export_filepath, "w");
        if (file == NULL) {
            fprintf(stderr, "\n[ERROR] Failed to export report to '%s': ", export_filepath);
            perror("");
            return;
        }
        write_report_content(file, filepath, total_lines, entries, valid_count, invalid_lines, invalid_count);
        fclose(file);
        printf("\n[SUCCESS] Diagnostic report exported to: %s\n", export_filepath);
    }
}
