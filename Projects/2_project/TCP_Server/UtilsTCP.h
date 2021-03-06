/* 
 * File:   UtilsTCP.h
 * Author: eddie
 *
 * Created on June 4, 2014, 4:35 PM
 */
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#ifndef UTILSTCP_H
#define	UTILSTCP_H

#define TAG_LOGIN 'L'
#define TAG_LOGOUT 'O'
#define TAG_WHO 'W'
#define TAG_TALK_REQ 'T'
#define TAG_TALK_RES 'R'
#define SERVER_ID 0 //the server's id is 0
#define DATA_SIZE 20
#define DEFAULT_PORT 24564
#define USER_PROMPT printf("> ")

typedef struct {
    int type; //the type of message to be sent
    unsigned int senderID; //the user id that sent the message
    
    bool confirm; //if needed, a boolean value to confirm or deny actions
    char data[DATA_SIZE]; //if needed, an array of chars for extra data
    struct sockaddr_in theAddress; //somebody's address
    struct sockaddr_in theOtherAddress;
} Message;

void dieWithError(char *errorMessage);

#endif	/* UTILSTCP_H */

