static int builtin_exit(char **args) {
    if (running != 0) running = 0;
    if (args[1] == NULL) return 0;
    return atoi(args[1]); /*neo: yeah. it'll do ASCII instead of an integer because of args[] lmao */
}

REGISTER_BUILTIN("exit", builtin_exit)
