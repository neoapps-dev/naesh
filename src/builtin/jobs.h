static int builtin_jobs(char **args) {
    int i;
    const char *state_str;
    (void)args;
    cleanup_jobs();
    for (i = 0; i < job_count; i++) {
        if (job_table[i].state == JOB_RUNNING) state_str = "Running";
        else if (job_table[i].state == JOB_STOPPED) state_str = "Stopped";
        else state_str = "Done";
        printf("[%d] %s %s\n", i + 1, state_str, job_table[i].cmd_str);
    }
    return 0;
}

REGISTER_BUILTIN("jobs", builtin_jobs)
