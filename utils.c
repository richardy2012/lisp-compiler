#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "utils.h"

int mopen_stdout = 0;
void mopen(char *argv[]) {
    mopen_status = 1;
    pid_t pid;
    int pipe = 0;
    if( (pid = fork()) < 0 ) {
        fputs("Forking error\n", stdout);
    } else if (pid == 0) { // child
        if(mopen_stdout) dup2(pipe, STDOUT_FILENO);
        if(execvp(argv[0], argv) < 0) {
            fputs(argv[0], stdout);
            if(errno == EACCES) {
                fputs(": permission denied", stdout);
            } else if (errno == ENOEXEC) {
                fputs(": can't be executed", stdout);
            } else {
                printf(": error code %i", errno);
            }
            fputc('\n',stdout);
        }
    } else { // parent
        mopen_status = 1;
        waitpid(pid, &mopen_status, 0);
        if(mopen_status > 255) mopen_status -= 255;
    }
}

int print_debug = 0;
int deprintf(const char *restrict fmt, ...)
{
    if(!print_debug) return 0;
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return ret;
}
