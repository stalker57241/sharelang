#include "tracer.h"


struct tracer_arg {
    struct tracer_arg* next;
    int offset;
    char* typename;
    char* varname;
};