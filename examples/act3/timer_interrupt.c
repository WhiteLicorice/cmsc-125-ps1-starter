/*
 * timer_interrupt.c
 *
 * Show a timer that interrupts a running loop.
 *
 * This program is a user-level illustration. It is not the kernel's own
 * preemption timer. setitimer(2) asks the kernel to raise SIGALRM on a
 * schedule. The handler counts the ticks. Control then returns to the loop
 * at the interrupted instruction.
 *
 * The loop asks the kernel for nothing and never yields. The ticks still
 * arrive, so the loop stops after the twentieth one.
 *
 * Build:
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *      timer_interrupt.c -o timer_interrupt
 *
 * Output:
 *   timer fired 20 times
 *   loop iterations: <count>
 */
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>

#define TARGET_TICKS 20

static volatile sig_atomic_t ticks = 0;

static void on_tick(int signo)
{
    (void)signo;
    ticks++;
}

int main(void)
{
    struct sigaction action = {0};
    action.sa_handler = on_tick;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    struct itimerval timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        return 1;
    }

    unsigned long iterations = 0;
    int seen = 0;
    while ((seen = ticks) < TARGET_TICKS) {
        iterations++;
    }

    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        return 1;
    }

    printf("timer fired %d times\n", seen);
    printf("loop iterations: %lu\n", iterations);

    return 0;
}
