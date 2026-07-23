static int builtin_fg(char **args) {
    int idx;
    int status;
    int i;
    status = 0;
    if (!args[1]) {
        fprintf(stderr, "fg: usage: fg <job_id>\n");
        return 1;
    }
    idx = atoi(args[1]) - 1;
    if (idx < 0 || idx >= job_count) {
        fprintf(stderr, "fg: %s: no such job\n", args[1]);
        return 1;
    }
    if (job_table[idx].state == JOB_DONE) {
        fprintf(stderr, "fg: %s: job already finished\n", args[1]);
        return 1;
    }
    kill(-job_table[idx].pids[0], SIGCONT);
    tcsetpgrp(STDIN_FILENO, job_table[idx].pids[0]);
    for (i = 0; i < job_table[idx].pid_count; i++) {
        do {
            waitpid(job_table[idx].pids[i], &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    tcsetpgrp(STDIN_FILENO, getpid());
    job_table[idx].state = JOB_DONE;
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

REGISTER_BUILTIN("fg", builtin_fg)
