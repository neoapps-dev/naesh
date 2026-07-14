static int builtin_cd(char **args) {
    char *dir;
    if (!args[1]) {
        dir = getenv("HOME");
        if (!dir) {
            struct passwd *pw = getpwuid(getuid());
            dir = pw ? pw->pw_dir : "/";
        }
    } else {
        dir = args[1];
    }
    if (chdir(dir) != 0) {
        perror("cd");
        return 1;
    }
    return 0;
}

REGISTER_BUILTIN("cd", builtin_cd)
