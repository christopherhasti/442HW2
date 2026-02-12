#include <stdio.h>
#include <stdlib.h>
#include "error.h"

void ERROR(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}