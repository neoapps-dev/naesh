/*neo: ported to C89 from https://github.com/bareutils/barelib (C11, src/esc.c) */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char *esc(const char *in) {
    size_t len;
    char *out;
    size_t j;
    size_t i;
    len = strlen(in);
    out = (char*)malloc(len * 4 + 1);
    if (!out) return NULL;
    j = 0;
    for (i = 0; i < len; i++) {
        if (in[i] == '\\' && i + 1 < len) {
            i++;
            switch (in[i]) {
                case 'n': out[j++] = '\n'; break;
                case 't': out[j++] = '\t'; break;
                case 'r': out[j++] = '\r'; break;
                case 'b': out[j++] = '\b'; break;
                case 'a': out[j++] = '\a'; break;
                case 'v': out[j++] = '\v'; break;
                case 'f': out[j++] = '\f'; break;
                case '\\':
                    out[j++] = '\\';
                    break;

                case 'x': {
                    int h1 = (i + 1 < len) ? hexval(in[i + 1]) : -1;
                    int h2 = (i + 2 < len) ? hexval(in[i + 2]) : -1;
                    if (h1 >= 0 && h2 >= 0) {
                        out[j++] = (char)((h1 << 4) | h2);
                        i += 2;
                    } else {
                        out[j++] = 'x';
                    }
                    break;
                }

                case '0':
                    out[j++] = '\0';
                    break;

                default:
                    out[j++] = '\\';
                    out[j++] = in[i];
                    break;
            }
        } else {
            out[j++] = in[i];
        }
    }

    out[j] = '\0';
    return out;
}
