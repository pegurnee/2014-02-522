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
#include "UtilsTCP.h"

/*
 * 
 */
int main(int argc, char** argv) {
    //all my variables
    int theSocket; //the socket
    unsigned short serverPort; /* Server port */
    
    //handle command line stuff

    //create socket
    if ((theSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        dieWithError("socket() failed");
    }
    //bind to port

    //two threads, parent: server commands
    //exit/logout: safely closes down the server
    //who: displays on the server the current users logged in
    //say: says a global message from the server

    //child: sever operation, mainly just retrieving and responding to messages
    //LOGIN, adds the user to the list of logged in users, sends a message to each other user that the user logged in
    //LOGOUT, removes the user from the list of logged out users, sends a message to each other user that the user logged out
    //WHO, returns to the user a list of all of the currently logged in users
    //TALK, sends a request to start talking with a user

    return (EXIT_SUCCESS);
}

