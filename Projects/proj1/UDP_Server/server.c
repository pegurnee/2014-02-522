/* 
 * File:   server.c
 * Author: eddie
 *
 * Created on May 21, 2014, 9:57 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define MESSAGE_SIZE 100 // Longest size of any message
#define NUM_USERS 10 // number of users in the messaging system

typedef struct sockaddr_in sockaddr_in;

typedef struct {

    enum {
        Login, Notify, Logout
    } message_Type; // same size as an unsigned int
    unsigned int ClientId; // unique client identifier
} NotifyMessage; // an unsigned int is 32 bits = 4 bytes

typedef struct {

    enum {
        Send, Retrieve
    } request_Type; // same size as an unsigned int 
    unsigned int SenderId; // unique client identifier 
    unsigned int RecipientId; // unique client identifier 
    char message[MESSAGE_SIZE]; // text message
} ClientMessage; // an unsigned int is 32 bits = 4 bytes 

typedef struct {

    enum {
        New, Old, No_Message
    } messageType; // same size as an unsigned int
    unsigned int SenderId; //unique client identifier 
    unsigned int RecipientId; // unique client identifier
    char message[MESSAGE_SIZE]; // text message
} ServerMessage; // an unsigned int is 32 bits = 4 bytes

/*
 * 
 */
int main(int argc, char** argv) {
    int sock; /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen; /* Length of incoming message */
    char echoBuffer[MESSAGE_SIZE]; /* Buffer for echo string */
    unsigned short echoServPort; /* Server port */
    int recvMsgSize; /* Size of received message */

    //check parameters, just port number (24564)
    
    //create socket
    
    //create the address structure
    
    //bind() to the address
    
    //main looping
    //1. login, check password
    //2. 
    return (EXIT_SUCCESS);
}

