#ifndef UTILS_H
#define UTILS_H

extern int mopen_stdout;
int mopen_status;
void mopen(char *argv[]);

extern int print_debug;
int deprintf(const char *restrict fmt, ...);
void error(const char *restrict fmt, ...);

#endif
