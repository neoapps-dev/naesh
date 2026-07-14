#define NAESH_MAX_BUILTINS 64 /*neo: why? idk either. felt like a good number */
typedef int (*builtin_fn)(char **args);
typedef struct {
    const char *name;
    builtin_fn func;
} naesh_builtin_entry;
static naesh_builtin_entry naesh_builtin[NAESH_MAX_BUILTINS];
static int naesh_builtin_count = 0;
static int is_builtin(const char *cmd) {
    int i;
    for (i = 0; i < naesh_builtin_count; i++) {
        if (strcmp(cmd, naesh_builtin[i].name) == 0) return 1;
    }
    return 0;
}
static int run_builtin(char **args) {
    int i;
    for (i = 0; i < naesh_builtin_count; i++) {
        if (strcmp(args[0], naesh_builtin[i].name) == 0) {
            return naesh_builtin[i].func(args);
        }
    }
    return -1;
}
#define REGISTER_BUILTIN(mname, mfunc) /*neo: `m`-prefixed parameters are for macro */ \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wunused-function\"") \
    _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") \
    static void __attribute__((constructor)) register_##mfunc(void) { \
        naesh_builtin[naesh_builtin_count].name = mname; \
        naesh_builtin[naesh_builtin_count].func = mfunc; \
        naesh_builtin_count++; \
    } \
    _Pragma("clang diagnostic pop") /*neo: i know. i know. */

#include "builtin/master.h"
