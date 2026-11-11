/*
 * pipe_basic.c
 *
 * One pipe between a parent and a child. The parent writes and closes its
 * copy of the write end. The child closes its copy of the write end before it
 * reads, so the read returns zero once the payload runs out. The child prints
 * every line, and the parent prints after the child exits, so the order of
 * the output is fixed.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static const char message[] = "read,read\nwrite,write\n";

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

    int fds[2];
    if (pipe(fds) < 0) {
        die("pipe");
    }
    printf("parent: pipe read end is %d, write end is %d\n", fds[0], fds[1]);

    pid_t child = fork();
    if (child < 0) {
        die("fork");
    }

    if (child == 0) {
        if (close(fds[1]) < 0) {
            die("close");
        }
        char buf[64];
        size_t total = 0;
        for (;;) {
            ssize_t n = read(fds[0], buf + total, sizeof buf - 1 - total);
            if (n < 0) {
                die("read");
            }
            if (n == 0) {
                break;
            }
            total += (size_t) n;
        }
        buf[total] = '\0';
        printf("child: read %zu bytes\n", total);
        for (size_t i = 0; i < total; i++) {
            if (buf[i] == '\n') {
                buf[i] = '|';
            }
        }
        printf("child: body \"%s\"\n", buf);
        printf("child: read returned 0 after every write end closed\n");
        if (close(fds[0]) < 0) {
            die("close");
        }
        exit(0);
    }

    if (close(fds[0]) < 0) {
        die("close");
    }
    ssize_t written = write(fds[1], message, strlen(message));
    if (written < 0 || (size_t) written != strlen(message)) {
        die("write");
    }
    if (close(fds[1]) < 0) {
        die("close");
    }

    int status = 0;
    pid_t reaped = waitpid(child, &status, 0);
    if (reaped < 0) {
        die("waitpid");
    }
    if (!WIFEXITED(status)) {
        fprintf(stderr, "child did not exit normally\n");
        return 1;
    }
    printf("parent: reaped the child, exit status %d\n", WEXITSTATUS(status));
    printf("parent: child pid matched: %s\n", reaped == child ? "yes" : "no");
    return 0;
}
