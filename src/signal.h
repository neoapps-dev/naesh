#include "naesh.h"
#include <sys/select.h>
static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t sigint_received = 0;
static int signal_fd = -1;
static void handlesig(int sig) {
    int saved_errno;
    char byte = (char)sig;
    saved_errno = errno;
    if (write(signal_fd, &byte, 1) == -1) {
        (void)0;
    }
    errno = saved_errno;
    if (sig == SIGINT) {
        sigint_received = 1;
    }
}

static void setup_signals(void) {
    int fds[2];
    struct sigaction sa;
    if (pipe(fds) == -1) {
        perror("pipe");
        exit(1);
    }
    signal_fd = fds[1];
    sa.sa_handler = handlesig;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
}

static void free_signals(void) {
    if (signal_fd != -1) {
        close(signal_fd);
        signal_fd = -1;
    }
}
