static int builtin_bg(char **args) {
    int idx;
    if (!args[1]) {
        fprintf(stderr, "bg: usage: bg <job_id>\n");
        return 1;
    }
    idx = atoi(args[1]) - 1;
    if (idx < 0 || idx >= job_count) {
        fprintf(stderr, "bg: %s: no such job\n", args[1]);
        return 1;
    }
    if (job_table[idx].state == JOB_DONE) {
        fprintf(stderr, "bg: %s: job already finished\n", args[1]);
        return 1;
    }
    kill(-job_table[idx].pids[0], SIGCONT);
    job_table[idx].state = JOB_RUNNING;
    return 0;
}

REGISTER_BUILTIN("bg", builtin_bg)
