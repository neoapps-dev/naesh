#define NAESH_HISTORY_SIZE 256
#define NAESH_LINE_MAX 4096
#include <termios.h>
#include <signal.h>
static char *history[NAESH_HISTORY_SIZE];
static int history_count = 0;
static int history_index = -1;
static struct termios orig_termios;
static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\033[?25h");
    fflush(stdout);
}

static int enable_raw_mode(void) {
    struct termios raw;
    if (!isatty(STDIN_FILENO)) return -1;
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    raw = orig_termios;
    raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(tcflag_t)(IXON | ICRNL);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf("\033[?25l");
    fflush(stdout);
    return 0;
}

static void history_add(const char *line) {
    char *dup;
    if (!line || !line[0]) return;
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) return;
    dup = strdup(line);
    if (!dup) return;
    if (history_count < NAESH_HISTORY_SIZE) {
        history[history_count++] = dup;
    } else {
        free(history[0]);
        memmove(history, history + 1, sizeof(char *) * (NAESH_HISTORY_SIZE - 1));
        history[NAESH_HISTORY_SIZE - 1] = dup;
    }
}

static void history_free(void) {
    int i;
    for (i = 0; i < history_count; i++) {
        free(history[i]);
        history[i] = NULL;
    }
    history_count = 0;
    history_index = -1;
}

static void render_line(const char *buf, size_t len, size_t cursor) {
    size_t i;
    printf("\033[?25l");
    printf("\r\033[K%s", NAESH_PROMPT);
    for (i = 0; i < len; i++) putchar(buf[i]);
    printf("\r\033[%luC", (int)strlen(NAESH_PROMPT) + cursor);
    printf("\033[?25h");
    fflush(stdout);
}

static char *naesh_read_line_plain(void);
static char *naesh_read_line(void) {
    char buf[NAESH_LINE_MAX];
    size_t len = 0;
    size_t cursor = 0;
    char c;
    char seq[3];
    if (enable_raw_mode() != 0) {
        return naesh_read_line_plain();
    }

    memset(buf, 0, NAESH_LINE_MAX);
    render_line(buf, len, cursor);
    while (1) {
        if (sigint_received) {
            len = 0;
            buf[0] = '\0';
            cursor = 0;
            sigint_received = 0;
            render_line(buf, len, cursor);
            continue;
        }
        {
            fd_set rfds;
            struct timeval tv;
            int maxfd = STDIN_FILENO > signal_read_fd ? STDIN_FILENO : signal_read_fd;
            FD_ZERO(&rfds);
            FD_SET(STDIN_FILENO, &rfds);
            if (signal_read_fd != -1) {
                FD_SET(signal_read_fd, &rfds);
            }
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            if (select(maxfd + 1, &rfds, NULL, NULL, &tv) == -1) {
                if (errno == EINTR) {
                    char sigbyte;
                    if (signal_read_fd != -1) {
                        read(signal_read_fd, &sigbyte, 1);
                    }
                    continue;
                }
                break;
            }
            if (signal_read_fd != -1 && FD_ISSET(signal_read_fd, &rfds)) {
                char sigbyte;
                if (read(signal_read_fd, &sigbyte, 1) == 1) {
                    (void)sigbyte;
                }
                continue;
            }
            if (FD_ISSET(STDIN_FILENO, &rfds)) {
                if (read(STDIN_FILENO, &c, 1) != 1) break;
            } else {
                continue;
            }
        }
        if (c == '\n' || c == '\r') {
            buf[len] = '\0';
            disable_raw_mode();
            putchar('\n');
            fflush(stdout);
            if (len > 0) history_add(buf);
            history_index = -1;
            if (len == 0) {
                render_line(buf, len, cursor);
                return naesh_read_line();
            }
            return strdup(buf);
        }

        if (c == 27) {
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    if (read(STDIN_FILENO, &seq[2], 1) != 1) continue;
                    if (seq[1] == '3' && seq[2] == '~') {
                        if (cursor < len) {
                            memmove(buf + cursor, buf + cursor + 1, len - cursor - 1);
                            len--;
                            buf[len] = '\0';
                        }
                    } else if (seq[1] == '5' && seq[2] == '~') {
                        if (history_index == -1 && history_count > 0) {
                            history_index = 0;
                            len = strlen(history[history_index]);
                            memcpy(buf, history[history_index], len);
                            cursor = len;
                        } else if (history_index > 0) {
                            history_index = 0;
                            len = strlen(history[history_index]);
                            memcpy(buf, history[history_index], len);
                            cursor = len;
                        } else if (history_index == 0) {
                            history_index = -1;
                            len = 0;
                            buf[0] = '\0';
                            cursor = 0;
                        }
                    } else if (seq[1] == '6' && seq[2] == '~') {
                        if (history_count > 0 && history_index < history_count - 1) {
                            if (history_index == -1) {
                                buf[len] = '\0';
                            }
                            history_index = history_count - 1;
                            len = strlen(history[history_index]);
                            memcpy(buf, history[history_index], len);
                            cursor = len;
                        } else if (history_index == history_count - 1) {
                            history_index = -1;
                            len = 0;
                            buf[0] = '\0';
                            cursor = 0;
                        }
                    }
                } else if (seq[1] == 'A') {
                    if (history_index == -1) {
                        if (history_count > 0) {
                            history_index = history_count - 1;
                            len = strlen(history[history_index]);
                            memcpy(buf, history[history_index], len);
                            cursor = len;
                        }
                    } else if (history_index > 0) {
                        history_index--;
                        len = strlen(history[history_index]);
                        memcpy(buf, history[history_index], len);
                        cursor = len;
                    } else if (history_index == 0) {
                        history_index = -1;
                        len = 0;
                        buf[0] = '\0';
                        cursor = 0;
                    }
                } else if (seq[1] == 'B') {
                    if (history_index >= 0 && history_index < history_count - 1) {
                        history_index++;
                        len = strlen(history[history_index]);
                        memcpy(buf, history[history_index], len);
                        cursor = len;
                    } else if (history_index == history_count - 1) {
                        history_index = -1;
                        len = 0;
                        buf[0] = '\0';
                        cursor = 0;
                    }
                } else if (seq[1] == 'C') {
                    if (cursor < len) cursor++;
                } else if (seq[1] == 'D') {
                    if (cursor > 0) cursor--;
                } else if (seq[1] == 'H') {
                    cursor = 0;
                } else if (seq[1] == 'F') {
                    cursor = len;
                }
            } else if (seq[0] == 'O') {
                if (seq[1] == 'H') cursor = 0;
                else if (seq[1] == 'F') cursor = len;
            }
        } else if (c == 127 || c == 8) {
            if (cursor > 0) {
                memmove(buf + cursor - 1, buf + cursor, len - cursor);
                cursor--;
                len--;
                buf[len] = '\0';
            }
        } else if (c == 3) {
            len = 0;
            cursor = 0;
            buf[0] = '\0';
        } else if (c == 1) {
            cursor = 0;
        } else if (c == 5) {
            cursor = len;
        } else if (c == 11) {
            buf[cursor] = '\0';
            len = cursor;
        } else if (c == 12) {
            printf("\033[2J\033[H");
            render_line(buf, len, cursor);
            continue;
        } else if (c >= 32) {
            if (len < NAESH_LINE_MAX - 1) {
                memmove(buf + cursor + 1, buf + cursor, len - cursor);
                buf[cursor] = c;
                cursor++;
                len++;
                buf[len] = '\0';
            }
        }

        render_line(buf, len, cursor);
    }

    disable_raw_mode();
    return NULL;
}

static char *naesh_read_line_plain(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    nread = getline(&line, &len, stdin);
    if (nread == -1) {
        free(line);
        return NULL;
    }
    if (line[nread - 1] == '\n') line[nread - 1] = '\0';
    if (line[0]) history_add(line);
    return line;
}
