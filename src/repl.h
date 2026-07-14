#include "naesh.h"
int naesh_repl(void) {
    char *line;
    char **args;
    naesh_pipeline *pipeline;
    int lastexit;
    lastexit = 0;
    setup_signals();
    while (running) {
        sigint_received = 0;
        line = naesh_read_line();
        if (!line) break;
        if (sigint_received) {
            free(line);
            sigint_received = 0;
            continue;
        }
        args = naesh_parse_line(line);
        pipeline = naesh_parse(args);
        if (pipeline) {
            if (pipeline->count > 0) {
                lastexit = naesh_exec_pipeline(pipeline);
            }
            naesh_pipeline_free(pipeline);
        }
        {
            int i;
            for (i = 0; args[i]; i++) free(args[i]);
        }
        free(args);
        free(line);
    }
    history_free();
    free_signals();
    return lastexit;
}
