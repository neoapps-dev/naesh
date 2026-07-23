static int builtin_disown(char **args) {
    int idx;
    int i;
    int new_count;
    if (!args[1]) {
        fprintf(stderr, "disown: usage: disown <job_id>\n");
        return 1;
    }
    idx = atoi(args[1]) - 1;
    if (idx < 0 || idx >= job_count) {
        fprintf(stderr, "disown: %s: no such job\n", args[1]);
        return 1;
    }
    free(job_table[idx].cmd_str);
    new_count = job_count - 1;
    for (i = idx; i < new_count; i++) {
        job_table[i] = job_table[i + 1];
    }
    job_count = new_count;
    return 0;
}

REGISTER_BUILTIN("disown", builtin_disown)
