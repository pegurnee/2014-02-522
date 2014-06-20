#include <stdio.h>
#include <stdlib.h>
#include "UtilsTCP.h"

void dieWithError(char *errorMessage) {
    perror(errorMessage);
    exit(1);
}