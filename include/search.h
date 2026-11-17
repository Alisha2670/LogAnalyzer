#ifndef SEARCH_H
#define SEARCH_H

#include "parser.h"

// Filters logs by level, date, and/or keyword in a single pass. Pass NULL to skip a filter.
void filter_entries(const LogEntry entries[], int count,
                    const char *level, const char *date, const char *keyword);

#endif
