#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>


#include "read_line.c"

#define BUFFER_SIZE 256
#define CONNECTED 1
#define OFF 0
#define TRUE 1
#define FALSE 0

struct argumentWrapper {
        char *username;
        char *usernameD;
        char *msg;
        int clientFD;
        struct sockaddr_in clientAddr;
        int clientPort;
};

struct messages {
        //char type;
        unsigned int message_id;
        char message[256];
        char sender[256];
        char receiver[256];
};
struct userInformation{
  char username[256];
  char status;
  struct in_addr user_addr;
  unsigned short user_port;
  //struct messages *pending_messages;
  struct queue *pending_messages;
  unsigned int last_message_id;
};



#include "queue.h"

// FIXME
#include "queue.c"


#endif
