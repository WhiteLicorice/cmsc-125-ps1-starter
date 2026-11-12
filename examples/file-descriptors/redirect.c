/*
 * redirect.c
 *
 * Point descriptor 1 at a file. The writes after dup2() go to the file, and
 * the write before it goes to the terminal. dup2() copies the reference to
 * the open file description, so the original descriptor can be closed.
 */
#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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

    printf("terminal: before the redirect\n");

    int saved = dup(STDOUT_FILENO);
    if (saved < 0) {
        die("dup");
    }

    int fd = open("redirect.out", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        die("open");
    }
    if (dup2(fd, STDOUT_FILENO) < 0) {
        die("dup2");
    }
    if (close(fd) < 0) {
        die("close");
    }

    printf("file: after the redirect\n");
    if (fflush(stdout) != 0) {
        die("fflush");
    }

    if (dup2(saved, STDOUT_FILENO) < 0) {
        die("dup2");
    }
    if (close(saved) < 0) {
        die("close");
    }

    int back = open("redirect.out", O_RDONLY);
    if (back < 0) {
        die("open");
    }
    char buf[128];
    ssize_t n = read(back, buf, sizeof buf - 1);
    if (n < 0) {
        die("read");
    }
    buf[n] = '\0';
    if (close(back) < 0) {
        die("close");
    }

    printf("redirect.out holds: %s", buf);
    return 0;
}
