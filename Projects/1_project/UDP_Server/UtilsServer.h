
#include <arpa/inet.h>
#include <stdbool.h>
#include "UtilsUDP.h"

#ifndef UTILSSERVER_H
#define UTILSSERVER_H

#define USER_SIZE 10
#define INCREMENT_MESSAGE 10

typedef struct {
    unsigned int clientID; //the unique user ID 
    struct sockaddr_in address; //the address for a client, used in all the real time work
    int numMessages; //the number of messages the user has
    int messageLimit;
    
    bool isLoggedIn;
    bool hasNewMessages;
    ServerMessage *messages; //the pointer to all of the client's messages
} Client;

int getUserIndex(unsigned int id, int numUsers, Client *users);
ServerMessage convertMessage(ClientMessage *clientMessage);

#endif
