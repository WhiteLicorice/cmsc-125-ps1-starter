/*
 * fork_wait: the parent waits for the child, so the order is fixed.
 *
 * The parent blocks in wait until the child exits. The child's line is
 * therefore always the first line, and wait returns the child's process id.
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
        printf("child: pid %d\n", (int)getpid());
        exit(0);
    }

    pid_t done = wait(NULL);
    if (done < 0) {
        perror("wait");
        return 1;
    }

    printf("parent: pid %d waited for %d\n", (int)getpid(), (int)done);
    return 0;
}
