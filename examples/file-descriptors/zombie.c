/*
 * zombie.c
 *
 * A child that exits without being waited for stays in the process table as a
 * zombie. This example reads its state from /proc, waits for it, and then
 * reads /proc again. The state field of /proc/<pid>/stat is the third field,
 * so the code reads up to the closing parenthesis of the command name and
 * takes the character after the next space.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static char proc_state(pid_t pid)
{
    char path[64];
    snprintf(path, sizeof path, "/proc/%ld/stat", (long) pid);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return '\0';
    }
    char buf[512];
    ssize_t n = read(fd, buf, sizeof buf - 1);
    if (close(fd) < 0) {
        die("close");
    }
    if (n <= 0) {
        return '\0';
    }
    buf[n] = '\0';

    char *close_paren = strrchr(buf, ')');
    if (close_paren == NULL || close_paren[1] != ' ') {
        return '\0';
    }
    return close_paren[2];
}

int main(void)
{
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        die("setvbuf");
    }

    pid_t child = fork();
    if (child < 0) {
        die("fork");
    }
    if (child == 0) {
        _exit(7);
    }

    char state = '\0';
    for (int tries = 0; tries < 2000; tries++) {
        state = proc_state(child);
        if (state == 'Z') {
            break;
        }
        nap_ms(1);
    }
    printf("parent: /proc state before wait is %c\n", state);

    int status = 0;
    pid_t reaped = waitpid(child, &status, 0);
    if (reaped < 0) {
        die("waitpid");
    }
    printf("parent: waitpid reaped the pid it forked: %s\n",
           reaped == child ? "yes" : "no");
    printf("parent: WIFEXITED %d, WEXITSTATUS %d\n",
           WIFEXITED(status) ? 1 : 0,
           WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    printf("parent: /proc state after wait is %s\n",
           proc_state(child) == '\0' ? "gone" : "present");
    return 0;
}
