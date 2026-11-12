/*
 * pipe_eof.c
 *
 * A write end that survives in a process the reader never forked.
 *
 * The reader drains the first payload. Then it reads again while the pipe is
 * empty, and that read returns eight more bytes rather than zero, because one
 * write end is still open. Only after the holder closes does a read on the
 * empty pipe return zero.
 *
 * The reader prints every line, so the order of the output is fixed.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define FIRST "one,one\n"
#define SECOND "two,two\n"
#define PAYLOAD_LEN 8

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

    int data[2];
    int stop[2];
    if (pipe(data) < 0) die("pipe data");
    if (pipe(stop) < 0) die("pipe stop");

    pid_t middle = fork();
    if (middle < 0) {
        die("fork");
    }

    if (middle == 0) {
        if (close(data[0]) < 0) die("close");
        if (close(stop[1]) < 0) die("close");

        pid_t holder = fork();
        if (holder < 0) {
            die("fork");
        }
        if (holder == 0) {
            if (write(data[1], FIRST, PAYLOAD_LEN) != PAYLOAD_LEN) {
                die("write first");
            }
            char byte = 0;
            if (read(stop[0], &byte, 1) != 1) die("read stop");
            if (write(data[1], SECOND, PAYLOAD_LEN) != PAYLOAD_LEN) {
                die("write second");
            }
            if (close(data[1]) < 0) die("close");
            _exit(0);
        }

        if (close(data[1]) < 0) die("close");
        if (close(stop[0]) < 0) die("close");
        _exit(0);
    }

    if (close(data[1]) < 0) die("close");
    if (close(stop[0]) < 0) die("close");

    char body[32];
    size_t total = 0;
    while (total < PAYLOAD_LEN) {
        ssize_t n = read(data[0], body + total, PAYLOAD_LEN - total);
        if (n <= 0) {
            break;
        }
        total += (size_t) n;
    }
    body[total] = '\0';
    for (size_t i = 0; i < total; i++) {
        if (body[i] == '\n') {
            body[i] = '|';
        }
    }
    printf("parent: read %zu bytes \"%s\"\n", total, body);

    char go = 1;
    if (write(stop[1], &go, 1) != 1) die("write stop");

    total = 0;
    while (total < PAYLOAD_LEN) {
        ssize_t n = read(data[0], body + total, PAYLOAD_LEN - total);
        if (n <= 0) {
            break;
        }
        total += (size_t) n;
    }
    body[total] = '\0';
    for (size_t i = 0; i < total; i++) {
        if (body[i] == '\n') {
            body[i] = '|';
        }
    }
    printf("parent: read %zu more bytes \"%s\"\n", total, body);

    ssize_t last = read(data[0], body, sizeof body - 1);
    if (last < 0) {
        die("read");
    }
    printf("parent: read after the holder closes returns %zd\n", last);

    int status = 0;
    if (waitpid(middle, &status, 0) < 0) {
        die("waitpid");
    }
    printf("parent: reaped the middle child, exit status %d\n",
           WIFEXITED(status) ? WEXITSTATUS(status) : -1);

    if (close(stop[1]) < 0) die("close");
    if (close(data[0]) < 0) die("close");
    return 0;
}
