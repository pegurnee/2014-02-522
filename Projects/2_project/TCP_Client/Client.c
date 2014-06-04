/* 
 * File:   Client.c
 * Author: eddie
 *
 * Created on June 3, 2014, 4:27 PM
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * 
 */
int main(int argc, char** argv) {
    //handle command line stuff
    
    //create socket
    //bind to port
    
    //three threads, parent thread: communicating with the server, when in talk mode, sending messages to the other person
    //LOGIN, logs into the server
    //LOGOUT, logs out from the server
    //WHO, requests from the user to list all of the currently logged in users
    //EXIT, exits the program
    //TALK, starts a talk request with a user, waits for the user's response (via server) 
    //once the user is in talk mode
    //SEND, everything that the user sends in talk mode is displayed on the other client's screen
    //QUIT, by typing in ":quit" or ":q" the user can exit
    
    //child thread: receiving communication from server
    //TALK, after receiving a talk request from a user, 
    //  sets a shared boolean to true letting the parent thread know that the client has received a request to talk, 
    //  if already talking, respond with a negative message
    //LOGIN, after receiving a login message from the server, displays that user id to the client
    
    //child thread: the TCP client that receives communication from other user while in talk mode, displays messages on screen
    return (EXIT_SUCCESS);
}

