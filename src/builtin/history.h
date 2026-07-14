static int builtin_history(char **args) {
    int i;
    (void)args;
    for (i = 0; i < history_count; i++) printf("%5d  %s\n", i + 1, history[i]);
    return 0;
}

REGISTER_BUILTIN("history", builtin_history)
