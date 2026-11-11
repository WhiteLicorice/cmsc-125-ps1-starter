/*
 * fork_basic: one call to fork, two processes, two return values.
 *
 * The program prints nothing before the call, so no buffered output can be
 * copied into the child. Both processes print their own line. Nothing decides
 * which line reaches the standard output first, so the check reads the two
 * lines as a set.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    pid_t rc = fork();

    if (rc < 0) {
        perror("fork");
        return 1;
    }

    if (rc == 0) {
        printf("child: pid %d fork 0\n", (int)getpid());
    } else {
        printf("parent: pid %d fork %d\n", (int)getpid(), (int)rc);
    }

    return 0;
}
