/*
 * shell_loop: a minimal fork, exec, wait loop.
 *
 * The loop reads one command per line from the standard input and runs it.
 * It prints no prompt of its own, so its output is the output of the commands
 * it runs. It waits for each child before it reads the next line, so no child
 * outlives the loop.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 16
#define MAX_WORDS 15

int main(void)
{
    char *line = NULL;
    size_t cap = 0;

    while (getline(&line, &cap, stdin) > 0) {
        char *args[MAX_ARGS];
        int n = 0;
        char *save = NULL;

        for (char *word = strtok_r(line, " \t\n", &save);
             word != NULL && n < MAX_WORDS;
             word = strtok_r(NULL, " \t\n", &save)) {
            args[n++] = word;
        }
        args[n] = NULL;

        if (n == 0) {
            continue;
        }

        pid_t rc = fork();
        if (rc < 0) {
            perror("fork");
            return 1;
        }

        if (rc == 0) {
            execvp(args[0], args);
            perror("execvp");
            exit(127);
        }

        if (wait(NULL) < 0) {
            perror("wait");
            return 1;
        }
    }

    free(line);
    return 0;
}
