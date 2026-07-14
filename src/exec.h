#include "naesh.h"
int naesh_execute(char **args) {
    pid_t pid;
    int status;
    char *resolved;
    if (!args || !args[0]) return 0;
    if (is_builtin(args[0])) return run_builtin(args);
    resolved = naesh_path_resolve(args[0]);
    pid = fork();
    if (pid == 0) {
        if (resolved) {
            execvp(resolved, args);
        } else {
            execvp(args[0], args);
        }
        perror(args[0]);
        _exit(127);
    } else if (pid > 0) {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    } else {
        perror("fork");
        free(resolved);
        return 1;
    }

    free(resolved);
    return 0;
}

int naesh_exec_pipeline(naesh_pipeline *pl) {
    int i;
    int prev_fd;
    int pipefd[2];
    pid_t *pids;
    int status;
    int last_status;
    if (!pl || pl->count == 0) return 0;
    if (pl->count == 1) {
        char **args = pl->cmds[0];
        char *resolved;
        pid_t pid;
        if (!args || !args[0]) return 0;
        if (is_builtin(args[0])) return run_builtin(args);
        resolved = naesh_path_resolve(args[0]);
        pid = fork();
        if (pid == 0) {
            if (resolved) execvp(resolved, args);
            else execvp(args[0], args);
            perror(args[0]);
            _exit(127);
        } else if (pid > 0) {
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            free(resolved);
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        } else {
            perror("fork");
            free(resolved);
            return 1;
        }
    }
    pids = (pid_t *)malloc(sizeof(pid_t) * (size_t)pl->count);
    if (!pids) {
        fprintf(stderr, "naesh: allocation error\n");
        return 1;
    }
    prev_fd = -1;
    for (i = 0; i < pl->count; i++) {
        if (i < pl->count - 1) {
            if (pipe(pipefd) == -1) {
                perror("pipe");
                free(pids);
                return 1;
            }
        }
        pids[i] = fork();
        if (pids[i] == 0) {
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < pl->count - 1) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }
            {
                char *resolved = naesh_path_resolve(pl->cmds[i][0]);
                if (resolved) {
                    execvp(resolved, pl->cmds[i]);
                } else {
                    execvp(pl->cmds[i][0], pl->cmds[i]);
                }
                perror(pl->cmds[i][0]);
                _exit(127);
            }
        } else if (pids[i] < 0) {
            perror("fork");
            if (prev_fd != -1) close(prev_fd);
            if (i < pl->count - 1) {
                close(pipefd[0]);
                close(pipefd[1]);
            }
            free(pids);
            return 1;
        }
        if (prev_fd != -1) close(prev_fd);
        if (i < pl->count - 1) {
            close(pipefd[1]);
            prev_fd = pipefd[0];
        }
    }
    last_status = 0;
    for (i = 0; i < pl->count; i++) {
        do {
            waitpid(pids[i], &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
    free(pids);
    return last_status;
}
