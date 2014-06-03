
#include "UtilsServer.h"
#include "UtilsUDP.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <stdlib.h>

int getUserIndex(unsigned int id, int numUsers, Client *users) {
    for (int i = 0; i < numUsers; i++) {
        if (users[i].clientID == id) {
            return i;
        }
    }
    return -1;
}

ServerMessage convertMessage(ClientMessage *clientMessage) {
    ServerMessage textMessage;

    strcpy(textMessage.message, clientMessage->message);
    textMessage.messageType = NEW;
    textMessage.senderId = clientMessage->senderId;
    textMessage.recipientId = clientMessage->recipientId;

    return textMessage;
}