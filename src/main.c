#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "analyzer.h"
#include "search.h"
#include "report.h"

// Starting capacity for dynamic heap array before doubling
#define INITIAL_CAPACITY 16

// Holds parsed command-line arguments and filter criteria
typedef struct {
    const char *level;
    const char *date;
    const char *keyword;
    int recurring;
    int report;
    const char *export_filepath;
} CliOptions;

// Displays CLI syntax and available option flags
static void print_usage(const char *prog_name) {
    fprintf(stderr, "C-Log: Command-line Log Analysis & Diagnostic Tool\n\n");
    fprintf(stderr, "Usage: %s <path_to_log_file | -> [options]\n\n", prog_name);
    fprintf(stderr, "Input:\n");
    fprintf(stderr, "  <path_to_log_file>    Path to a log file on disk\n");
    fprintf(stderr, "  -                     Read log stream from standard input (stdin)\n\n");
    fprintf(stderr, "Filtering Options (can be combined):\n");
    fprintf(stderr, "  --errors              Filter for ERROR logs\n");
    fprintf(stderr, "  --warnings            Filter for WARNING logs\n");
    fprintf(stderr, "  --info                Filter for INFO logs\n");
    fprintf(stderr, "  --date <YYYY-MM-DD>   Filter logs for a specific calendar date\n");
    fprintf(stderr, "  --search <keyword>    Search messages by keyword (case-insensitive)\n\n");
    fprintf(stderr, "Analysis Options:\n");
    fprintf(stderr, "  --recurring           Show top recurring error messages (root cause)\n");
    fprintf(stderr, "  --report              Generate full diagnostic health report\n");
    fprintf(stderr, "  --export [file]       Export diagnostic report (default: reports/report.txt)\n");
    fprintf(stderr, "  --help, -h            Show this help manual\n");
}

int main(int argc, char *argv[]) {
    // 1. Verify arguments
    if (argc < 2) {
        fprintf(stderr, "Error: No log file or input source specified.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *filepath = argv[1];
    if (strcmp(filepath, "--help") == 0 || strcmp(filepath, "-h") == 0) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    // 2. Open input stream (disk file or standard input)
    int is_stdin = (strcmp(filepath, "-") == 0);
    FILE *file = is_stdin ? stdin : fopen(filepath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file '%s': ", filepath);
        perror("");
        return EXIT_FAILURE;
    }

    // 3. Allocate initial heap buffer for parsed entries
    int capacity = INITIAL_CAPACITY;
    LogEntry *entries = malloc(capacity * sizeof(LogEntry));
    if (entries == NULL) {
        perror("Memory allocation failed");
        if (!is_stdin) fclose(file);
        return EXIT_FAILURE;
    }
    int valid_count = 0;

    int invalid_lines[MAX_INVALID_ENTRIES];
    int invalid_count = 0;

    char line[MAX_LINE_LEN];
    LogEntry entry;
    int line_number = 0;

    // 4. Stream ingestion and parsing loop
    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;

        // Drain remainder of the line if it exceeded MAX_LINE_LEN to keep line numbers exact
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] != '\n' && !feof(file)) {
            int ch;
            while ((ch = fgetc(file)) != '\n' && ch != EOF);
        }

        if (parse_line(line, &entry)) {
            // Geometric heap scaling (2x doubling)
            if (valid_count >= capacity) {
                capacity *= 2;
                LogEntry *temp = realloc(entries, capacity * sizeof(LogEntry));
                if (temp == NULL) {
                    perror("Memory reallocation failed");
                    free(entries);
                    if (!is_stdin) fclose(file);
                    return EXIT_FAILURE;
                }
                entries = temp;
            }
            entries[valid_count] = entry;
            valid_count++;
        } else {
            // Track corrupt line numbers up to MAX_INVALID_ENTRIES
            if (invalid_count < MAX_INVALID_ENTRIES) {
                invalid_lines[invalid_count] = line_number;
            }
            invalid_count++;
        }
    }

    if (!is_stdin) {
        fclose(file);
    }

    // 5. Parse command-line options
    CliOptions opts = {0};
    int has_filter = 0;

    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--errors") == 0) {
            opts.level = "ERROR";
            has_filter = 1;
        } else if (strcmp(arg, "--warnings") == 0) {
            opts.level = "WARNING";
            has_filter = 1;
        } else if (strcmp(arg, "--info") == 0) {
            opts.level = "INFO";
            has_filter = 1;
        } else if (strcmp(arg, "--date") == 0) {
            if (i + 1 >= argc || argv[i + 1][0] == '-') {
                fprintf(stderr, "Error: --date requires a date argument (YYYY-MM-DD).\n");
                free(entries);
                return EXIT_FAILURE;
            }
            opts.date = argv[++i];
            has_filter = 1;
        } else if (strcmp(arg, "--search") == 0) {
            if (i + 1 >= argc || argv[i + 1][0] == '-') {
                fprintf(stderr, "Error: --search requires a keyword argument.\n");
                free(entries);
                return EXIT_FAILURE;
            }
            opts.keyword = argv[++i];
            has_filter = 1;
        } else if (strcmp(arg, "--recurring") == 0) {
            opts.recurring = 1;
        } else if (strcmp(arg, "--report") == 0) {
            opts.report = 1;
        } else if (strcmp(arg, "--export") == 0) {
            opts.report = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                opts.export_filepath = argv[++i];
            } else {
                opts.export_filepath = "reports/report.txt";
            }
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            print_usage(argv[0]);
            free(entries);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", arg);
            fprintf(stderr, "Run '%s --help' for available options.\n", argv[0]);
            free(entries);
            return EXIT_FAILURE;
        }
    }

    const char *source_name = is_stdin ? "<stdin>" : filepath;

    // 6. Execute requested actions
    if (has_filter) {
        filter_entries(entries, valid_count, opts.level, opts.date, opts.keyword);
    }
    if (opts.recurring) {
        find_recurring_errors(entries, valid_count);
    }
    if (opts.report) {
        generate_report(source_name, line_number, entries, valid_count, invalid_lines, invalid_count, opts.export_filepath);
    }

    // Default summary output if no filter or report flag was requested
    if (!has_filter && !opts.recurring && !opts.report) {
        printf("========== VALIDATION SUMMARY ==========\n");
        printf("Analyzed Source       : %s\n", source_name);
        printf("Total Lines Processed : %d\n", line_number);
        printf("Valid Entries Stored  : %d\n", valid_count);
        printf("Invalid Entries       : %d\n", invalid_count);

        if (invalid_count > 0) {
            printf("\nInvalid Entries Found on Line(s):\n");
            int display_count = (invalid_count < MAX_INVALID_ENTRIES) ? invalid_count : MAX_INVALID_ENTRIES;
            for (int i = 0; i < display_count; i++) {
                printf("  - Line %d\n", invalid_lines[i]);
            }
            if (invalid_count > MAX_INVALID_ENTRIES) {
                printf("  - ... and %d more corrupted line(s)\n", invalid_count - MAX_INVALID_ENTRIES);
            }
        }
        printf("========================================\n");

        LogStats stats;
        analyze_logs(entries, valid_count, &stats);
        print_statistics(&stats);
    }

    // 7. Free heap allocated log entries
    free(entries);
    return EXIT_SUCCESS;
}
