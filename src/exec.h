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

static int apply_redirects(naesh_cmd *cmd) {
    int fd;
    if (cmd->redir_in) {
        fd = open(cmd->redir_in, O_RDONLY);
        if (fd == -1) {
            perror(cmd->redir_in);
            return -1;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (cmd->redir_out) {
        fd = open(cmd->redir_out, O_WRONLY | O_CREAT | (cmd->append_out ? O_APPEND : O_TRUNC), 0644);
        if (fd == -1) {
            perror(cmd->redir_out);
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    if (cmd->redir_err) {
        fd = open(cmd->redir_err, O_WRONLY | O_CREAT | (cmd->append_err ? O_APPEND : O_TRUNC), 0644);
        if (fd == -1) {
            perror(cmd->redir_err);
            return -1;
        }
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
    return 0;
}

int naesh_exec_pipeline(naesh_pipeline *pl) {
    int i;
    int prev_fd;
    int pipefd[2];
    pid_t *pids;
    int status;
    int last_status;
    int background;
    if (!pl || pl->count == 0) return 0;
    background = pl->background;
    if (pl->count == 1) {
        naesh_cmd *cmd = &pl->cmds[0];
        if (!cmd->args || !cmd->args[0]) {
            if (cmd->redir_in || cmd->redir_out || cmd->redir_err) {
                if (apply_redirects(cmd) == -1) return 1;
            }
            return 0;
        }
        if (!background && !cmd->redir_in && !cmd->redir_out && !cmd->redir_err && is_builtin(cmd->args[0])) {
            return run_builtin(cmd->args);
        }
        if (background) {
            pid_t pid;
            pid = fork();
            if (pid == 0) {
                setpgid(0, 0);
                if (apply_redirects(cmd) == -1) _exit(1);
                {
                    char *resolved = naesh_path_resolve(cmd->args[0]);
                    if (resolved) {
                        execvp(resolved, cmd->args);
                    } else {
                        execvp(cmd->args[0], cmd->args);
                    }
                }
                perror(cmd->args[0]);
                _exit(127);
            } else if (pid > 0) {
                pid_t pids_arr[1];
                setpgid(pid, pid);
                pids_arr[0] = pid;
                add_job(pids_arr, 1, cmd->args[0], JOB_RUNNING);
                fprintf(stderr, "[%d] %d\n", job_count, (int)pid);
                return 0;
            } else {
                perror("fork");
                return 1;
            }
        }
        {
            int sync_pipe[2];
            pid_t pid;
            if (pipe(sync_pipe) == -1) {
                perror("pipe");
                return 1;
            }
            pid = fork();
            if (pid == 0) {
                char c;
                close(sync_pipe[1]);
                setpgid(0, 0);
                if (apply_redirects(cmd) == -1) _exit(1);
                read(sync_pipe[0], &c, 1);
                close(sync_pipe[0]);
                {
                    char *resolved = naesh_path_resolve(cmd->args[0]);
                    if (resolved) {
                        execvp(resolved, cmd->args);
                    } else {
                        execvp(cmd->args[0], cmd->args);
                    }
                }
                perror(cmd->args[0]);
                _exit(127);
            } else if (pid > 0) {
                char c = 'y';
                setpgid(pid, pid);
                tcsetpgrp(STDIN_FILENO, pid);
                close(sync_pipe[0]);
                write(sync_pipe[1], &c, 1);
                close(sync_pipe[1]);
                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
                tcsetpgrp(STDIN_FILENO, getpid());
                return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
            } else {
                perror("fork");
                close(sync_pipe[0]);
                close(sync_pipe[1]);
                return 1;
            }
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
            if (i == 0) setpgid(0, 0);
            else setpgid(0, pids[0]);
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < pl->count - 1) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }
            if (apply_redirects(&pl->cmds[i]) == -1) _exit(1);
            {
                char *resolved = naesh_path_resolve(pl->cmds[i].args[0]);
                if (resolved) {
                    execvp(resolved, pl->cmds[i].args);
                } else {
                    execvp(pl->cmds[i].args[0], pl->cmds[i].args);
                }
                perror(pl->cmds[i].args[0]);
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
        if (i == 0) setpgid(pids[i], pids[i]);
        if (prev_fd != -1) close(prev_fd);
        if (i < pl->count - 1) {
            close(pipefd[1]);
            prev_fd = pipefd[0];
        }
    }
    if (background) {
        add_job(pids, pl->count, pl->cmds[0].args[0], JOB_RUNNING);
        fprintf(stderr, "[%d] %d\n", job_count, (int)pids[pl->count - 1]);
        free(pids);
        return 0;
    }
    tcsetpgrp(STDIN_FILENO, pids[0]);
    last_status = 0;
    for (i = 0; i < pl->count; i++) {
        do {
            waitpid(pids[i], &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
    tcsetpgrp(STDIN_FILENO, getpid());
    free(pids);
    return last_status;
}
