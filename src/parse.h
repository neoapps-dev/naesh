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
        while (*p && (in_sq || in_dq || (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '|' && *p != '>' && *p != '<'))) {
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
        if (*p == '|' || *p == '>' || *p == '<') {
            if (*p == '>' && p[1] == '>') {
                if (pos >= buf_size) {
                    buf_size *= 2;
                    tokens = (char **)realloc(tokens, sizeof(char *) * (size_t)buf_size);
                    if (!tokens) {
                        fprintf(stderr, "naesh: allocation error\n");
                        exit(1);
                    }
                }
                tokens[pos] = strdup(">>");
                pos++;
                p += 2;
            } else {
                if (pos >= buf_size) {
                    buf_size *= 2;
                    tokens = (char **)realloc(tokens, sizeof(char *) * (size_t)buf_size);
                    if (!tokens) {
                        fprintf(stderr, "naesh: allocation error\n");
                        exit(1);
                    }
                }
                {
                    char op[2];
                    op[0] = *p;
                    op[1] = '\0';
                    tokens[pos] = strdup(op);
                }
                pos++;
                p++;
            }
        }
    }
    tokens[pos] = NULL;
    free(tok);
    return tokens;
}

static void naesh_cmd_free(naesh_cmd *cmd) {
    int j;
    if (!cmd) return;
    if (cmd->args) {
        for (j = 0; cmd->args[j]; j++) free(cmd->args[j]);
        free(cmd->args);
    }
    free(cmd->redir_in);
    free(cmd->redir_out);
    free(cmd->redir_err);
}

static char *expand_env(const char *token, int lastexit) {
    char *result;
    int rsize;
    int rpos;
    const char *p;
    if (!token) return NULL;
    if (!strchr(token, '$')) return strdup(token);
    rsize = NAESH_BUF_SIZE;
    result = (char *)malloc((size_t)rsize);
    if (!result) {
        fprintf(stderr, "naesh: allocation error\n");
        exit(1);
    }
    rpos = 0;
    p = token;
    while (*p) {
        if (*p == '$') {
            p++;
            if (!*p || *p == '\'' || *p == '"') {
                if (rpos >= rsize - 1) {
                    rsize *= 2;
                    result = (char *)realloc(result, (size_t)rsize);
                    if (!result) { fprintf(stderr, "naesh: allocation error\n"); exit(1); }
                }
                result[rpos++] = '$';
            } else if (*p == '?') {
                char buf[32];
                int len;
                len = snprintf(buf, sizeof(buf), "%d", lastexit);
                while (rpos + len >= rsize) {
                    rsize *= 2;
                    result = (char *)realloc(result, (size_t)rsize);
                    if (!result) { fprintf(stderr, "naesh: allocation error\n"); exit(1); }
                }
                memcpy(result + rpos, buf, (size_t)len);
                rpos += len;
                p++;
            } else if (*p == '$') {
                char buf[32];
                int len;
                len = snprintf(buf, sizeof(buf), "%d", (int)getpid());
                while (rpos + len >= rsize) {
                    rsize *= 2;
                    result = (char *)realloc(result, (size_t)rsize);
                    if (!result) { fprintf(stderr, "naesh: allocation error\n"); exit(1); }
                }
                memcpy(result + rpos, buf, (size_t)len);
                rpos += len;
                p++;
            } else if (*p == '#') {
                if (rpos >= rsize - 1) {
                    rsize *= 2;
                    result = (char *)realloc(result, (size_t)rsize);
                    if (!result) { fprintf(stderr, "naesh: allocation error\n"); exit(1); }
                }
                result[rpos++] = '0';
                p++;
            } else if (*p == '!') {
                if (rpos >= rsize - 1) {
                    rsize *= 2;
                    result = (char *)realloc(result, (size_t)rsize);
                    if (!result) { fprintf(stderr, "naesh: allocation error\n"); exit(1); }
                }
                result[rpos++] = '0';
                p++;
            } else if (*p == '{') {
                const char *start;
                const char *val;
                int varlen;
                p++;
                start = p;
                while (*p && *p != '}') p++;
                varlen = (int)(p - start);
                if (*p == '}') p++;
                {
                    char varname[256];
                    int copylen = varlen < 255 ? varlen : 255;
                    memcpy(varname, start, (size_t)copylen);
                    varname[copylen] = '\0';
                    val = getenv(varname);
                }
                if (val) {
                    int vlen = (int)strlen(val);
                    while (rpos + vlen >= rsize) {
                        rsize *= 2;
                        result = (char *)realloc(result, (size_t)rsize);
                        if (!result) { fprintf(stderr, "naesh: allocation error\n"); exit(1); }
                    }
                    memcpy(result + rpos, val, (size_t)vlen);
                    rpos += vlen;
                }
            } else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_') {
                const char *start = p;
                const char *val;
                int varlen;
                while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_') p++;
                varlen = (int)(p - start);
                {
                    char varname[256];
                    int copylen = varlen < 255 ? varlen : 255;
                    memcpy(varname, start, (size_t)copylen);
                    varname[copylen] = '\0';
                    val = getenv(varname);
                }
                if (val) {
                    int vlen = (int)strlen(val);
                    while (rpos + vlen >= rsize) {
                        rsize *= 2;
                        result = (char *)realloc(result, (size_t)rsize);
                        if (!result) { fprintf(stderr, "naesh: allocation error\n"); exit(1); }
                    }
                    memcpy(result + rpos, val, (size_t)vlen);
                    rpos += vlen;
                }
            } else {
                if (rpos >= rsize - 2) {
                    rsize *= 2;
                    result = (char *)realloc(result, (size_t)rsize);
                    if (!result) { fprintf(stderr, "naesh: allocation error\n"); exit(1); }
                }
                result[rpos++] = '$';
                result[rpos++] = *p++;
            }
        } else {
            if (rpos >= rsize - 1) {
                rsize *= 2;
                result = (char *)realloc(result, (size_t)rsize);
                if (!result) { fprintf(stderr, "naesh: allocation error\n"); exit(1); }
            }
            result[rpos++] = *p++;
        }
    }
    result[rpos] = '\0';
    return result;
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
    pl->cmds = (naesh_cmd *)calloc((size_t)cmd_count, sizeof(naesh_cmd));
    if (!pl->cmds) {
        fprintf(stderr, "naesh: allocation error\n");
        exit(1);
    }
    pl->count = cmd_count;
    cmd_idx = 0;
    start = 0;
    for (i = 0; ; i++) {
        if (!tokens[i] || strcmp(tokens[i], "|") == 0) {
            int j;
            int arg_count;
            char *redir_in;
            char *redir_out;
            int append_out;
            char *redir_err;
            int append_err;
            int arg_idx;
            arg_count = 0;
            redir_in = NULL;
            redir_out = NULL;
            append_out = 0;
            redir_err = NULL;
            append_err = 0;
            for (j = start; j < i; j++) {
                if (strcmp(tokens[j], "<") == 0) {
                    if (j + 1 >= i) {
                        fprintf(stderr, "naesh: syntax error: missing filename\n");
                        naesh_pipeline_free(pl);
                        return NULL;
                    }
                    free(redir_in);
                    redir_in = strdup(tokens[j + 1]);
                    j++;
                } else if (strcmp(tokens[j], ">") == 0) {
                    if (j + 1 >= i) {
                        fprintf(stderr, "naesh: syntax error: missing filename\n");
                        naesh_pipeline_free(pl);
                        return NULL;
                    }
                    free(redir_out);
                    redir_out = strdup(tokens[j + 1]);
                    append_out = 0;
                    j++;
                } else if (strcmp(tokens[j], ">>") == 0) {
                    if (j + 1 >= i) {
                        fprintf(stderr, "naesh: syntax error: missing filename\n");
                        naesh_pipeline_free(pl);
                        return NULL;
                    }
                    free(redir_out);
                    redir_out = strdup(tokens[j + 1]);
                    append_out = 1;
                    j++;
                } else if (strlen(tokens[j]) == 1 && tokens[j][0] >= '0' && tokens[j][0] <= '9' && j + 1 < i && (strcmp(tokens[j + 1], ">") == 0 || strcmp(tokens[j + 1], ">>") == 0 || strcmp(tokens[j + 1], "<") == 0)) {
                    char fd;
                    char *op;
                    char *filename;
                    fd = tokens[j][0];
                    op = tokens[j + 1];
                    filename = (j + 2 < i) ? tokens[j + 2] : NULL;
                    if (!filename) {
                        fprintf(stderr, "naesh: syntax error: missing filename\n");
                        naesh_pipeline_free(pl);
                        return NULL;
                    }
                    if (fd == '0' || (fd != '1' && fd != '2' && strcmp(op, "<") == 0)) {
                        free(redir_in);
                        redir_in = strdup(filename);
                    } else if (fd == '2' || (fd != '0' && fd != '1' && strcmp(op, "<") != 0)) {
                        free(redir_err);
                        redir_err = strdup(filename);
                        append_err = (strcmp(op, ">>") == 0);
                    } else {
                        free(redir_out);
                        redir_out = strdup(filename);
                        append_out = (strcmp(op, ">>") == 0);
                    }
                    j += 2;
                } else {
                    arg_count++;
                }
            }
            pl->cmds[cmd_idx].args = (char **)malloc(sizeof(char *) * (size_t)(arg_count + 1));
            if (!pl->cmds[cmd_idx].args) {
                fprintf(stderr, "naesh: allocation error\n");
                exit(1);
            }
            arg_idx = 0;
            for (j = start; j < i; j++) {
                if (strcmp(tokens[j], "<") == 0 || strcmp(tokens[j], ">") == 0 || strcmp(tokens[j], ">>") == 0) {
                    j++;
                    if (j < i) j++;
                } else if (strlen(tokens[j]) == 1 && tokens[j][0] >= '0' && tokens[j][0] <= '9' && j + 1 < i && (strcmp(tokens[j + 1], ">") == 0 || strcmp(tokens[j + 1], ">>") == 0 || strcmp(tokens[j + 1], "<") == 0)) {
                    j += 2;
                } else {
                    pl->cmds[cmd_idx].args[arg_idx++] = strdup(tokens[j]);
                }
            }
            pl->cmds[cmd_idx].args[arg_idx] = NULL;
            pl->cmds[cmd_idx].redir_in = redir_in;
            pl->cmds[cmd_idx].redir_out = redir_out;
            pl->cmds[cmd_idx].append_out = append_out;
            pl->cmds[cmd_idx].redir_err = redir_err;
            pl->cmds[cmd_idx].append_err = append_err;
            cmd_idx++;
            start = i + 1;
        }
        if (!tokens[i]) break;
    }
    return pl;
}

void naesh_pipeline_free(naesh_pipeline *pl) {
    int i;
    if (!pl) return;
    if (pl->cmds) {
        for (i = 0; i < pl->count; i++) {
            naesh_cmd_free(&pl->cmds[i]);
        }
        free(pl->cmds);
    }
    free(pl);
}
