
#include <stdio.h>  /* for perror() */
#include <stdlib.h> /* for exit() */
#include "UtilsUDP.h"

void dieWithError(char *errorMessage) {
    perror(errorMessage);
    exit(1);
}
