/*
 * buffering: the line printed before fork lives in a stdio buffer.
 *
 * Standard output is line buffered on a terminal, so the line leaves the
 * buffer at the newline and the child receives an empty buffer. Standard
 * output is fully buffered on a pipe or a file, so the line is still in the
 * buffer when fork copies the process. Both copies flush that buffer on exit,
 * and the line appears twice.
 *
 * Run with the word "flush" to push the line out before fork returns. On a
 * terminal both modes print the same three lines.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    printf("before fork\n");

    if (argc > 1 && strcmp(argv[1], "flush") == 0) {
        fflush(stdout);
    }

    pid_t rc = fork();

    if (rc < 0) {
        perror("fork");
        return 1;
    }

    if (rc == 0) {
        printf("child\n");
        exit(0);
    }

    if (wait(NULL) < 0) {
        perror("wait");
        return 1;
    }

    printf("parent\n");
    return 0;
}
