#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analyzer.h"

// Prime bucket count to distribute hash values evenly and minimize collisions
#define HASH_BUCKETS 257

// Node for separate chaining collision resolution in the hash table
typedef struct HashNode {
    char message[MAX_MSG_LEN];
    int count;
    struct HashNode *next;
} HashNode;

// Classic djb2 string hashing algorithm (hash * 33 + char)
static unsigned long djb2_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// Comparator for qsort to sort error frequencies in descending order
static int compare_error_freq(const void *a, const void *b) {
    const ErrorFrequency *fa = (const ErrorFrequency *)a;
    const ErrorFrequency *fb = (const ErrorFrequency *)b;
    return fb->count - fa->count;
}

// Calculates counts and error rate across valid log entries
void analyze_logs(const LogEntry entries[], int count, LogStats *stats) {
    if (stats == NULL) return;

    stats->total = count;
    stats->info_count = 0;
    stats->warning_count = 0;
    stats->error_count = 0;
    stats->error_rate = 0.0f;

    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].level, "INFO") == 0) {
            stats->info_count++;
        } else if (strcmp(entries[i].level, "WARNING") == 0) {
            stats->warning_count++;
        } else if (strcmp(entries[i].level, "ERROR") == 0) {
            stats->error_count++;
        }
    }

    if (stats->total > 0) {
        stats->error_rate = ((float)stats->error_count / stats->total) * 100.0f;
    }
}

// Displays summary statistics table with percentage distribution
void print_statistics(const LogStats *stats) {
    if (stats == NULL) return;

    printf("\n========== LOG STATISTICS ==========\n");
    printf("Total Entries : %d\n", stats->total);
    if (stats->total > 0) {
        printf("INFO          : %d (%.2f%%)\n", stats->info_count,
               ((float)stats->info_count / stats->total) * 100.0f);
        printf("WARNING       : %d (%.2f%%)\n", stats->warning_count,
               ((float)stats->warning_count / stats->total) * 100.0f);
        printf("ERROR         : %d (%.2f%%)\n", stats->error_count,
               ((float)stats->error_count / stats->total) * 100.0f);
    } else {
        printf("INFO          : 0 (0.00%%)\n");
        printf("WARNING       : 0 (0.00%%)\n");
        printf("ERROR         : 0 (0.00%%)\n");
    }
    printf("\nError Rate    : %.2f%%\n", stats->error_rate);
    printf("====================================\n");
}

// Deduplicates errors via hash table, ranks them with qsort, and cleans up memory
int count_recurring_errors(const LogEntry entries[], int count, ErrorFrequency freqs[], int max_freqs, int *overflow_count) {
    if (overflow_count != NULL) *overflow_count = 0;
    if (entries == NULL || freqs == NULL || max_freqs <= 0) return 0;

    HashNode *table[HASH_BUCKETS] = {NULL};
    int total_unique = 0;

    // Ingest all ERROR logs into hash table (O(1) average lookup)
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].level, "ERROR") == 0) {
            unsigned long bucket = djb2_hash(entries[i].message) % HASH_BUCKETS;
            HashNode *node = table[bucket];
            int found = 0;

            while (node != NULL) {
                if (strcmp(node->message, entries[i].message) == 0) {
                    node->count++;
                    found = 1;
                    break;
                }
                node = node->next;
            }

            if (!found) {
                HashNode *new_node = malloc(sizeof(HashNode));
                if (new_node != NULL) {
                    strncpy(new_node->message, entries[i].message, MAX_MSG_LEN - 1);
                    new_node->message[MAX_MSG_LEN - 1] = '\0';
                    new_node->count = 1;
                    new_node->next = table[bucket];
                    table[bucket] = new_node;
                    total_unique++;
                }
            }
        }
    }

    if (total_unique == 0) {
        return 0;
    }

    // Flatten hash table into dynamic array and free all hash nodes
    ErrorFrequency *all_errors = malloc(total_unique * sizeof(ErrorFrequency));
    int copied = 0;

    for (int b = 0; b < HASH_BUCKETS; b++) {
        HashNode *curr = table[b];
        while (curr != NULL) {
            if (all_errors != NULL && copied < total_unique) {
                strncpy(all_errors[copied].message, curr->message, MAX_MSG_LEN - 1);
                all_errors[copied].message[MAX_MSG_LEN - 1] = '\0';
                all_errors[copied].count = curr->count;
                copied++;
            }
            HashNode *next = curr->next;
            free(curr); // Free node to prevent memory leaks
            curr = next;
        }
        table[b] = NULL;
    }

    // Rank errors in descending order of frequency (root cause first)
    if (all_errors != NULL) {
        qsort(all_errors, copied, sizeof(ErrorFrequency), compare_error_freq);

        int return_count = (copied < max_freqs) ? copied : max_freqs;
        for (int i = 0; i < return_count; i++) {
            freqs[i] = all_errors[i];
        }

        if (overflow_count != NULL) {
            *overflow_count = (copied > max_freqs) ? (copied - max_freqs) : 0;
        }

        free(all_errors);
        return return_count;
    }

    return 0;
}

// Discovers and displays recurring error messages in a formatted terminal table
void find_recurring_errors(const LogEntry entries[], int count) {
    ErrorFrequency freqs[MAX_UNIQUE_ERRORS];
    int overflow = 0;
    int unique_count = count_recurring_errors(entries, count, freqs, MAX_UNIQUE_ERRORS, &overflow);

    printf("\n========== RECURRING ERRORS (ROOT CAUSE) ==========\n");
    if (unique_count == 0) {
        printf("No errors found in the log entries!\n");
    } else {
        printf("%-12s | %s\n", "Occurrences", "Error Message");
        printf("-------------+-------------------------------------\n");
        for (int i = 0; i < unique_count; i++) {
            printf("     %-7d | %s\n", freqs[i].count, freqs[i].message);
        }
        if (overflow > 0) {
            printf("-------------+-------------------------------------\n");
            printf("Note: Showing top %d unique errors (%d additional unique message(s) omitted).\n",
                   MAX_UNIQUE_ERRORS, overflow);
        }
    }
    printf("===================================================\n");
}
