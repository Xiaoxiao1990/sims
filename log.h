#ifndef LOG_H
#define LOG_H

#include <stdio.h>

FILE *misc_log;

int Log_Init(void);
int close_logs(void);
int logs(FILE *log_file, const char *format, ...);

#endif // LOG_H
