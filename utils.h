#ifndef UTILS_H
#define UTILS_H

extern unsigned int mopen_stdout;
int mopen_status;
void mopen(char *argv[]);

extern unsigned int print_debug;
int deprintf(const char *restrict fmt, ...);

#endif
