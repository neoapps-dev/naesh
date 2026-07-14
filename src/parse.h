#include "naesh.h"
char **naesh_parse_line(char *line) {
    int buf_size = NAESH_BUF_SIZE;
    int tok_size = NAESH_BUF_SIZE;
    int pos = 0;
    int tpos = 0;
    int in_sq = 0;
    int in_dq = 0;
    char *p;
    char *tok;
    char **tokens;
    tokens = (char **)malloc(sizeof(char *) * (size_t)buf_size);
    tok = (char *)malloc((size_t)tok_size);
    if (!tokens || !tok) {
        fprintf(stderr, "naesh: allocation error\n");
        exit(1);
    }
    p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (!*p) break;
        tpos = 0;
        in_sq = 0;
        in_dq = 0;
        while (*p && (in_sq || in_dq || (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '|'))) {
            if (tpos >= tok_size - 1) {
                tok_size *= 2;
                tok = (char *)realloc(tok, (size_t)tok_size);
                if (!tok) {
                    fprintf(stderr, "naesh: allocation error\n");
                    exit(1);
                }
            }
            if (in_sq) {
                if (*p == '\'') {
                    in_sq = 0;
                    p++;
                } else {
                    tok[tpos++] = *p++;
                }
            } else if (in_dq) {
                if (*p == '"') {
                    in_dq = 0;
                    p++;
                } else if (*p == '\\' && (p[1] == '"' || p[1] == '\\' || p[1] == '$')) {
                    p++;
                    tok[tpos++] = *p++;
                } else {
                    tok[tpos++] = *p++;
                }
            } else {
                if (*p == '\'') {
                    in_sq = 1;
                    p++;
                } else if (*p == '"') {
                    in_dq = 1;
                    p++;
                } else if (*p == '\\' && p[1]) {
                    p++;
                    tok[tpos++] = *p++;
                } else {
                    tok[tpos++] = *p++;
                }
            }
        }
        tok[tpos] = '\0';
        if (tpos > 0) {
            if (pos >= buf_size) {
                buf_size *= 2;
                tokens = (char **)realloc(tokens, sizeof(char *) * (size_t)buf_size);
                if (!tokens) {
                    fprintf(stderr, "naesh: allocation error\n");
                    exit(1);
                }
            }
            tokens[pos] = strdup(tok);
            pos++;
        }
        if (*p == '|') {
            if (pos >= buf_size) {
                buf_size *= 2;
                tokens = (char **)realloc(tokens, sizeof(char *) * (size_t)buf_size);
                if (!tokens) {
                    fprintf(stderr, "naesh: allocation error\n");
                    exit(1);
                }
            }
            tokens[pos] = strdup("|");
            pos++;
            p++;
        }
    }
    tokens[pos] = NULL;
    free(tok);
    return tokens;
}

naesh_pipeline *naesh_parse(char **tokens) {
    naesh_pipeline *pl;
    int cmd_count;
    int cmd_idx;
    int i;
    int start;
    pl = (naesh_pipeline *)malloc(sizeof(naesh_pipeline));
    if (!pl) {
        fprintf(stderr, "naesh: allocation error\n");
        exit(1);
    }
    if (!tokens || !tokens[0]) {
        pl->cmds = NULL;
        pl->count = 0;
        return pl;
    }
    cmd_count = 1;
    for (i = 0; tokens[i]; i++) {
        if (strcmp(tokens[i], "|") == 0) cmd_count++;
    }
    for (i = 0; tokens[i]; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            if (i == 0 || !tokens[i + 1] || strcmp(tokens[i + 1], "|") == 0) {
                fprintf(stderr, "naesh: syntax error near '|'\n");
                pl->cmds = NULL;
                pl->count = 0;
                return pl;
            }
        }
    }
    pl->cmds = (char ***)malloc(sizeof(char **) * (size_t)cmd_count);
    if (!pl->cmds) {
        fprintf(stderr, "naesh: allocation error\n");
        exit(1);
    }
    pl->count = cmd_count;
    cmd_idx = 0;
    start = 0;
    for (i = 0; ; i++) {
        if (!tokens[i] || strcmp(tokens[i], "|") == 0) {
            int len = i - start;
            int j;
            pl->cmds[cmd_idx] = (char **)malloc(sizeof(char *) * (size_t)(len + 1));
            if (!pl->cmds[cmd_idx]) {
                fprintf(stderr, "naesh: allocation error\n");
                exit(1);
            }
            for (j = 0; j < len; j++) {
                pl->cmds[cmd_idx][j] = strdup(tokens[start + j]);
            }
            pl->cmds[cmd_idx][len] = NULL;
            cmd_idx++;
            start = i + 1;
        }
        if (!tokens[i]) break;
    }
    return pl;
}

void naesh_pipeline_free(naesh_pipeline *pl) {
    int i;
    int j;
    if (!pl) return;
    if (pl->cmds) {
        for (i = 0; i < pl->count; i++) {
            if (pl->cmds[i]) {
                for (j = 0; pl->cmds[i][j]; j++) free(pl->cmds[i][j]);
                free(pl->cmds[i]);
            }
        }
        free(pl->cmds);
    }
    free(pl);
}
