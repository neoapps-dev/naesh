#include "naesh.h"
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("the nae shell v%s\n", NAESH_VERSION);
    return naesh_repl();
}
