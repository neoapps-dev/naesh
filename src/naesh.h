#ifndef NAESH_H
#define NAESH_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <dirent.h>
#define NAESH_VERSION "0.0.0" /*neo: i guess bro */
#define NAESH_PROMPT "$ " /*neo: TODO: make it use $PROMPT */
#define NAESH_BUF_SIZE 1024
typedef struct {
    int count;
    char ***cmds;
} naesh_pipeline;
int naesh_repl(void);
char **naesh_parse_line(char *line);
naesh_pipeline *naesh_parse(char **tokens);
void naesh_pipeline_free(naesh_pipeline *pl);
int naesh_exec_pipeline(naesh_pipeline *pl);
int naesh_execute(char **args);
#endif
