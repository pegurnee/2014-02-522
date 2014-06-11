/* 
 * File:   Server.c
 * Author: eddie
 *
 * Created on June 3, 2014, 4:27 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "UtilsTCP.h"
#include "UtilsServer.h"

/*
 * 
 */
int main(int argc, char** argv) {
    //all my variables
    int theSocket; //the socket
    unsigned short serverPort; //the server port
    int numUsers = 0; //current number of users
    int userLimit = 10; //the user limit, will adjust when needed
    int userIndex; //used when addressing users in the data structure

    //structures
    Message incoming; //incoming message
    Client *users; //all the users
    pid_t processID; //the child process ID

    //address variables
    struct sockaddr_in theServerAddress; //the local address
    struct sockaddr_in theClientAddress; //the client address
    unsigned int clientAddressLength = sizeof (theClientAddress); //length of the client address

    //handle command line stuff

    //create socket
    if ((theSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        dieWithError("socket() failed");
    }

    //create the address structure
    memset(&theServerAddress, 0, sizeof (theServerAddress)); // Zero out structure
    theServerAddress.sin_family = AF_INET; // Internet address family
    theServerAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
    theServerAddress.sin_port = htons(serverPort); // Local port

    //bind() to the address
    if (bind(theSocket,
            (struct sockaddr *) &theServerAddress,
            sizeof (theServerAddress))
            < 0) {
        dieWithError("bind() failed");
    }

    processID = fork();

    //two threads, 
    if (processID > 0) { //parent: server commands
        //exit/logout: safely closes down the server
        //who: displays on the server the current users logged in
        //say: says a global message from the server
    } else if (processID == 0) { //child: sever operation, mainly just retrieving and responding to messages
        users = calloc(userLimit, sizeof (Client)); //memmory for data structure

        for (;;) {
            //just the tag
            recvfrom(theSocket, &incoming, sizeof (incoming), 0,
                    (struct sockaddr *) &theClientAddress, &clientAddressLength);
            switch (incoming.type) {
                case TAG_LOGIN:
                    //LOGIN, adds the user to the list of logged in users, sends a message to each other user that the user logged in
                    
                    break;
                case TAG_LOGOUT:
                    //LOGOUT, removes the user from the list of logged out users, sends a message to each other user that the user logged out
                    break;
                case TAG_WHO:
                    //WHO, returns to the user a list of all of the currently logged in users
                    break;
                case TAG_TALK:
                    //TALK, sends a request to start talking with a user
                    break;
                default:
                    //not gonna happen
                    break;
            }
        }
    } else { //forking() no good
        dieWithError("fork() failed");
    }
    return (EXIT_SUCCESS);
}

