#ifndef PARSER_H
#define PARSER_H

#define MAX_DATE_LEN 16
#define MAX_TIME_LEN 16
#define MAX_LEVEL_LEN 16
#define MAX_MSG_LEN 1024

// Line buffer accounts for message, headers, and delimiter padding
#define MAX_LINE_LEN (MAX_MSG_LEN + MAX_DATE_LEN + MAX_TIME_LEN + MAX_LEVEL_LEN + 16)

// Maximum corrupt line numbers stored for summary printing
#define MAX_INVALID_ENTRIES 100

// Represents a successfully validated log entry
typedef struct {
    char date[MAX_DATE_LEN];
    char time[MAX_TIME_LEN];
    char level[MAX_LEVEL_LEN];
    char message[MAX_MSG_LEN];
} LogEntry;

// Parses and validates a raw log line. Returns 1 if valid, 0 if corrupt.
int parse_line(const char *line, LogEntry *entry);

#endif
