/*
 * fork_exec: the child replaces itself with another program.
 *
 * The child calls execvp and never returns from it. The line after the call
 * runs only when execvp fails. The parent waits for the child, so the echo
 * output always precedes the parent's line.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t rc = fork();

    if (rc < 0) {
        perror("fork");
        return 1;
    }

    if (rc == 0) {
        char *args[3];
        args[0] = "echo";
        args[1] = "the child is now echo";
        args[2] = NULL;
        execvp(args[0], args);
        perror("execvp");
        exit(127);
    }

    if (wait(NULL) < 0) {
        perror("wait");
        return 1;
    }

    printf("parent: the child finished\n");
    return 0;
}
