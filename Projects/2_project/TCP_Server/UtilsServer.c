
#include "UtilsServer.h"
#include "UtilsTCP.h"

int getUserIndex(unsigned int id, int numUsers, Client *users) {
    int i;
    for (i = 0; i < numUsers; i++) {
        if (users[i].clientID == id) {
            return i;
        }
    }
    return -1;
}
