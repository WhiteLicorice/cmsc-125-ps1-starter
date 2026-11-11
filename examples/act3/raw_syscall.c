/*
 * raw_syscall.c
 *
 * A library call and a raw system call reach the same kernel service.
 *
 * The C library hides a trap inside write(2). The syscall(2) entry point
 * sends the trap itself, with the kernel's number for the service. This
 * program prints the same message both ways.
 *
 * syscall(2) is a Linux interface, not a POSIX one. _DEFAULT_SOURCE exposes
 * it under this build.
 *
 * Build:
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *      raw_syscall.c -o raw_syscall
 *
 * Expected output:
 *   limited direct execution
 *   limited direct execution
 *   libc write returned 25
 *   syscall write returned 25
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static const char message[] = "limited direct execution\n";

int main(void)
{
    size_t length = strlen(message);

    ssize_t libc_result = write(STDOUT_FILENO, message, length);
    ssize_t raw_result = (ssize_t)syscall(SYS_write, STDOUT_FILENO,
                                          message, (unsigned long)length);

    printf("libc write returned %zd\n", libc_result);
    printf("syscall write returned %zd\n", raw_result);

    return 0;
}
