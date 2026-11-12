/*
 * fd_table.c
 *
 * Show which numbers the descriptor table hands back. Descriptors 0, 1, and 2
 * are open when the program starts. open() returns the lowest number that is
 * not in use, and so does dup().
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

    for (int fd = STDIN_FILENO; fd <= STDERR_FILENO; fd++) {
        if (fcntl(fd, F_GETFD) < 0) {
            die("fcntl");
        }
        printf("descriptor %d is open at start\n", fd);
    }

    int first = open("fd_table.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (first < 0) {
        die("open");
    }
    printf("first open returned %d\n", first);
    if (close(first) < 0) {
        die("close");
    }

    int again = open("fd_table.txt", O_WRONLY | O_APPEND);
    if (again < 0) {
        die("open");
    }
    printf("open after close returned %d\n", again);
    if (close(again) < 0) {
        die("close");
    }

    int private_out = dup(STDOUT_FILENO);
    if (private_out < 0) {
        die("dup");
    }

    if (close(STDOUT_FILENO) < 0) {
        die("close");
    }
    int reclaimed = open("fd_table.txt", O_WRONLY | O_APPEND);
    if (reclaimed < 0) {
        die("open");
    }
    if (dprintf(private_out, "open after close(1) returned %d\n", reclaimed) < 0) {
        die("dprintf");
    }

    if (dup2(private_out, STDOUT_FILENO) < 0) {
        die("dup2");
    }
    if (close(private_out) < 0) {
        die("close");
    }
    printf("stdout restored\n");
    return 0;
}
