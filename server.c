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
  char buffer[4][256];

  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  val = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int));

  bzero( (char *)&serverAddr, sizeof(serverAddr) );

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(port);

  bind(sd, &serverAddr, sizeof(serverAddr));

  listen(sd, 5);
  size = sizeof(clientAddr);

  while(1){
    printf("Waiting for connection \n");

    sc = accept( sd, (struct sockaddr *) &clientAddr, &size);

    recv_msg(sc, buffer, sizeof(buffer));
    //recv(sc, (char *)&res, sizeof(int), 0);


    // create thread
    pthread_t thid;
    // create new thread with message received
    if( pthread_create( &thid, &thread_attributes, (void *) process_message, &buffer) != 0 ){
      perror("Error creating thread");
      continue;
    }
    // start critical section
    pthread_mutex_lock(&mutex_msg);
    while(msg_not_copied){
      pthread_cond_wait(&cond_msg, &mutex_msg);
    }
    // end critical section
    msg_not_copied = TRUE;
    pthread_mutex_unlock(&mutex_msg);



    // res = res+1;
    // send(sc, &res, sizeof(int), 0);

    close(sc);

  }

   close(sd);


 return 0;
}
