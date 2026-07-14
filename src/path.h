static char *naesh_path_resolve(const char *cmd) {
    char *pathenv;
    char *pathcpy;
    char *dir;
    char fullpath[NAESH_BUF_SIZE];
    if (cmd[0] == '/' || cmd[0] == '.') {
        if (access(cmd, X_OK) == 0) return strdup(cmd);
        return NULL;
    }

    pathenv = getenv("PATH");
    if (!pathenv) return NULL;
    pathcpy = strdup(pathenv);
    if (!pathcpy) return NULL;
    dir = strtok(pathcpy, ":");
    while (dir) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, cmd);
        if (access(fullpath, X_OK) == 0) {
            char *result = strdup(fullpath);
            free(pathcpy);
            return result;
        }
        dir = strtok(NULL, ":");
    }

    free(pathcpy);
    return NULL;
}
