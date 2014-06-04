/* 
 * File:   UtilsTCP.h
 * Author: eddie
 *
 * Created on June 4, 2014, 4:35 PM
 */
#include <stdbool.h>

#ifndef UTILSTCP_H
#define	UTILSTCP_H

#define TAG_LOGIN 'L'
#define TAG_LOGOUT 'O'
#define TAG_WHO 'W'
#define TAG_TALK 'T'
#define DATA_SIZE 20

typedef struct {
    int type; //the type of message to be sent
    unsigned int senderID; //the user id that sent the message
    
    bool confirm; //if needed, a boolean value to confirm or deny actions
    char data[DATA_SIZE]; //if needed, an array of chars for extra data
} Message;

void dieWithError(char *errorMessage);

#endif	/* UTILSTCP_H */

