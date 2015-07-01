
#include "UtilsServer.h"
#include "UtilsUDP.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>

int getUserIndex(unsigned int id, int numUsers, Client *users) {
    int i;
    for (i = 0; i < numUsers; i++) {
        if (users[i].clientID == id) {
            return i;
        }
    }
    return -1;
}

ServerMessage storeMessage(ClientMessage *oldMsg) {
    ServerMessage newMsg;

    strcpy(newMsg.message, oldMsg->message);
    newMsg.senderId = oldMsg->senderId;
    newMsg.recipientId = oldMsg->recipientId;
    newMsg.type = SERVERMSG_TAG;
    newMsg.messageType = NEW;

    return newMsg;
}