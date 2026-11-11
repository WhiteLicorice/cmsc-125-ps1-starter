/*
 * signal_kill.c
 *
 * Send a signal to a child and read the outcome out of its exit status. The
 * child announces that it is about to block. The parent waits for that
 * announcement on a pipe before it sends SIGTERM. The child never returns
 * from pause(), because SIGTERM ends it.
 */
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *what)
{
    perror(what);
    exit(1);
}

int main(void)
{
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        die("setvbuf");
    }

    int ready[2];
    if (pipe(ready) < 0) {
        die("pipe");
    }

    pid_t child = fork();
    if (child < 0) {
        die("fork");
    }

    if (child == 0) {
        if (close(ready[0]) < 0) {
            die("close");
        }
        char byte = 1;
        if (write(ready[1], &byte, 1) != 1) {
            die("write");
        }
        if (close(ready[1]) < 0) {
            die("close");
        }
        for (;;) {
            pause();
        }
    }

    if (close(ready[1]) < 0) {
        die("close");
    }
    char byte = 0;
    if (read(ready[0], &byte, 1) != 1) {
        die("read");
    }
    if (close(ready[0]) < 0) {
        die("close");
    }
    printf("parent: child announced it is about to pause\n");

    if (kill(child, SIGTERM) != 0) {
        die("kill");
    }
    printf("parent: sent SIGTERM\n");

    int status = 0;
    if (waitpid(child, &status, 0) < 0) {
        die("waitpid");
    }
    printf("parent: stopped by a signal: %s\n",
           WIFSIGNALED(status) ? "yes" : "no");
    printf("parent: terminating signal number: %d\n",
           WIFSIGNALED(status) ? WTERMSIG(status) : -1);
    printf("parent: SIGTERM number: %d\n", SIGTERM);
    return 0;
}
