#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static struct {
    bool needhelp:1;
    const char* filename;
} options = {0};

void printhelp(const char* progname) {
    printf("Usage:\n");
    // printf("%s -h", progname);
    printf("\t%s <options> filename\n", progname);
    printf("\t%s [-h|--help]\n", progname);
}

int main(const int argc, const char* const argv[]) {
    enum {
        STATE_FILENAME = 0,
    } state;
    for (int i = 1; i < argc; i++) {
        switch (state) {
        case STATE_FILENAME:
            if (*argv[i] == '-') {
                if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                    options.needhelp = true;
                }
                continue;
            }
            else options.filename = argv[i];
        }
    }
    if (options.needhelp) {
        printhelp(argv[0]);
        return 0;
    }
    if (options.filename == NULL) {
        printhelp(argv[0]);
        return 1;
    }
    printf("Hello world!\n");
    return 0;
}