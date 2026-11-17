#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"
#include "search.h"

// Portable ISO C99 case-insensitive substring search (replaces GNU strcasestr)
static int case_insensitive_contains(const char *haystack, const char *needle) {
    if (haystack == NULL || needle == NULL) return 0;
    if (*needle == '\0') return 1;

    size_t nlen = strlen(needle);
    size_t hlen = strlen(haystack);
    if (nlen > hlen) return 0;

    for (size_t i = 0; i <= hlen - nlen; i++) {
        size_t j = 0;
        while (j < nlen && tolower((unsigned char)haystack[i + j]) == tolower((unsigned char)needle[j])) {
            j++;
        }
        if (j == nlen) return 1;
    }
    return 0;
}

// Evaluates and prints log entries matching any combination of level, date, and keyword
void filter_entries(const LogEntry entries[], int count,
                    const char *level, const char *date, const char *keyword) {
    int matches = 0;

    printf("\n========== FILTERED LOG RESULTS ==========\n");
    if (level != NULL)   printf("Filter Level   : %s\n", level);
    if (date != NULL)    printf("Filter Date    : %s\n", date);
    if (keyword != NULL) printf("Filter Keyword : \"%s\"\n", keyword);
    printf("------------------------------------------\n");

    // Single unified traversal across all entries
    for (int i = 0; i < count; i++) {
        if (level != NULL && strcmp(entries[i].level, level) != 0) {
            continue;
        }
        if (date != NULL && strcmp(entries[i].date, date) != 0) {
            continue;
        }
        if (keyword != NULL && !case_insensitive_contains(entries[i].message, keyword)) {
            continue;
        }

        printf("[%s %s] %-7s %s\n",
               entries[i].date, entries[i].time, entries[i].level, entries[i].message);
        matches++;
    }

    printf("------------------------------------------\n");
    printf("Total Matches: %d\n", matches);
    printf("==========================================\n");
}
