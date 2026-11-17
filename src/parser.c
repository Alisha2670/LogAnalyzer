#include <stdio.h>
#include <string.h>
#include "parser.h"

// Parses: "YYYY-MM-DD HH:MM:SS LEVEL MESSAGE"
int parse_line(const char *line, LogEntry *entry) {
    if (line == NULL || entry == NULL) {
        return 0;
    }

    // Bounded format specifiers prevent buffer overflows
    int fields = sscanf(line, "%15s %15s %15s %1023[^\r\n]",
                        entry->date,
                        entry->time,
                        entry->level,
                        entry->message);

    // Require all 4 fields to be present
    if (fields < 4) {
        return 0;
    }

    // Validate that severity level matches expected keywords
    if (strcmp(entry->level, "INFO") != 0 &&
        strcmp(entry->level, "WARNING") != 0 &&
        strcmp(entry->level, "ERROR") != 0) {
        return 0;
    }

    return 1;
}
