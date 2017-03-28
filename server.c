#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>

#include "queue.h"
#include "read_line.c"

#define TRUE 1
#define FALSE 0

// mutex and condition variables for message copy
pthread_mutex_t mutex_msg;
int msg_not_copied = TRUE;
pthread_cond_t cond_msg;


void process_message(char **msg){
  char msg_local[4][256];

  // start critical section
  pthread_mutex_lock(&mutex_msg);

  // copy message to local variable
  int i;
  for(i = 0; i<4; i++){
    memcpy( (char *) msg_local[i], (char *) msg[i], 256);
  }

  // release and end critical section
  msg_not_copied = FALSE;
  pthread_cond_signal(&cond_msg);
  pthread_mutex_unlock(&mutex_msg);

  for(i = 0; i<4; i++){
    printf("msg_local[%d]: %s\n",i, msg_local[i] );
  }

}


int main(int argc, char**argv){

  int port;
  // parse arguments
  if(argc != 3 || strcmp(argv[1], "-p")){
   printf("Invalid arguments, Usage: ./server -p <port number>\n");
   return -1;
  }else{
   port = atoi(argv[2]);
   if(port <= 1024 || port > 65535 ){
    printf("Invalid port number, use one from 1025 to 65535.\n");
    return -1;
   }
  }

  // create thread attributes and set detached
  pthread_attr_t thread_attributes;
  if( pthread_attr_init(&thread_attributes) !=0 ){
    perror("Not possible to initialize thread attributes");
    return 1;
  }
  if( pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED) !=0 ){
    perror("Not possible to set thread attributes to Detached");
    return 1;
  }


  // server and client file descriptor
  int sd, sc;
  // sockets addresses
  struct sockaddr_in serverAddr, clientAddr;

  int size, val;
  //int res;
  //char buffer[50];
  char *buffer;
  buffer = (char *) calloc(50,sizeof(char));


  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  val = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int));

  bzero( (char *)&serverAddr, sizeof(serverAddr) );

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(port);

  if( bind(sd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1){
   perror("Bind error: ");
  }

  listen(sd, 5);
  size = sizeof(clientAddr);

  while(1){
    printf("Waiting for connection \n");

    sc = accept( sd, (struct sockaddr *) &clientAddr, &size);

     int count = recv_msg(sc, buffer, 8);
     printf("%s\n", buffer);
     int length = 0;
     count = recv_msg(sc, (char *)&length, 4);
     length = ntohl(length);
     printf("%d\n", length );
     char username[length];
     count = recv_msg(sc, (char *)&username, length);
     printf("%s\n", username );

    // send(sc, &res, sizeof(int), 0);
    char res = 1;
    //uint8_t res = '1';
    sendto(sc, &res, 1, 0, (struct sockaddr *) &clientAddr,size );

    close(sc);

  }

   free(buffer);
   close(sd);


 return 0;
}
