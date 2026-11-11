/*
 * orphan.c
 *
 * A process that outlives its parent. The parent forks a child, the child
 * forks a grandchild, and the child exits. The grandchild now has no parent of
 * its own, so the kernel gives it a new one.
 *
 * The middle child waits on a pipe until the grandchild reports that it has
 * read its own parent pid. Only then does the middle child exit. Without that
 * handshake the middle child can exit first, the grandchild reads the new pid
 * as its original one, and the example reports no change.
 *
 * The parent prints the whole transcript, so the order of the output is fixed.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void die(const char *what)
{
    perror(what);
    exit(1);
}

static void nap_ms(long ms)
{
    struct timespec request = { ms / 1000, (ms % 1000) * 1000000L };
    while (nanosleep(&request, &request) < 0 && errno == EINTR) {
    }
}

int main(void)
{
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        die("setvbuf");
    }

    int sync[2];
    int ack[2];
    if (pipe(sync) < 0) die("pipe sync");
    if (pipe(ack) < 0) die("pipe ack");

    pid_t middle = fork();
    if (middle < 0) {
        die("fork");
    }

    if (middle == 0) {
        if (close(ack[0]) < 0) die("close");

        pid_t grandchild = fork();
        if (grandchild < 0) {
            die("fork");
        }
        if (grandchild == 0) {
            pid_t original = getppid();
            char byte = 1;
            if (write(sync[1], &byte, 1) != 1) die("write sync");
            if (close(sync[1]) < 0) die("close");
            if (close(sync[0]) < 0) die("close");

            pid_t now = original;
            for (int tries = 0; tries < 4000 && now == original; tries++) {
                nap_ms(1);
                now = getppid();
            }
            byte = now == original ? 0 : 1;
            if (write(ack[1], &byte, 1) != 1) die("write ack");
            if (close(ack[1]) < 0) die("close");
            _exit(0);
        }

        if (close(sync[1]) < 0) die("close");
        if (close(ack[1]) < 0) die("close");
        char byte = 0;
        if (read(sync[0], &byte, 1) != 1) die("read sync");
        _exit(0);
    }

    if (close(ack[1]) < 0) die("close");
    if (close(sync[1]) < 0) die("close");
    if (close(sync[0]) < 0) die("close");

    int status = 0;
    if (waitpid(middle, &status, 0) < 0) {
        die("waitpid");
    }
    printf("parent: reaped the middle child, exit status %d\n",
           WIFEXITED(status) ? WEXITSTATUS(status) : -1);

    char byte = 0;
    if (read(ack[0], &byte, 1) != 1) {
        die("read");
    }
    printf("parent: the grandchild observed a new parent pid: %s\n",
           byte == 1 ? "yes" : "no");

    if (close(ack[0]) < 0) {
        die("close");
    }
    return 0;
}
