/*
 * fork_copy: after fork the two processes hold separate copies of x.
 *
 * The child sets x to 400 and prints it. The parent waits, then prints its own
 * x and finds 100, the value it held before the call. The two copies never
 * meet.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int x = 100;
    pid_t rc = fork();

    if (rc < 0) {
        perror("fork");
        return 1;
    }

    if (rc == 0) {
        x = 400;
        printf("child: x = %d\n", x);
        exit(0);
    }

    if (wait(NULL) < 0) {
        perror("wait");
        return 1;
    }

    printf("parent: x = %d\n", x);
    x = 300;
    printf("parent: x = %d after the change\n", x);
    return 0;
}
