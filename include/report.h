#ifndef REPORT_H
#define REPORT_H

#include "parser.h"

// Prints diagnostic summary to stdout and optionally exports to a file path
void generate_report(const char *filepath, int total_lines,
                     const LogEntry entries[], int valid_count,
                     const int invalid_lines[], int invalid_count,
                     const char *export_filepath);

#endif
