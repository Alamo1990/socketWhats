#include "server.h"

#define TRUE 1
#define FALSE 0

// mutex and condition variables for message copy
pthread_mutex_t mutex_msg;
int msg_not_copied = TRUE;
pthread_cond_t cond_msg;

struct queue *queueUsers;

// server file descriptor
int sd;
// server buffer
char *buffer;

// Initialize synchronization elements
void initializeSync(){
        if( pthread_mutex_init(&mutex_msg, NULL) != 0) {
                perror("Not possible to initialize mutex");
                exit(0);
        };

        if( pthread_cond_init(&cond_msg, NULL) != 0) {
                perror("Not possible to initialize condition variable");
                exit(0);
        }
}

// destroy synchronization elements
void destroySync(){
        if( pthread_mutex_destroy(&mutex_msg) != 0) {
                perror("Not possible to destroy mutex");
                exit(0);
        };

        if( pthread_cond_destroy(&cond_msg) != 0) {
                perror("Not possible to destroy condition variable");
                exit(0);
        }
}

// retrieve CTRL+C signal
void signal_handler(int sig){
 printf("Signal handler!!\n" );
  // close server port
  close(sd);
  // free buffer
  free(buffer);
  // free queue
  free(queueUsers);
  // destroy synchronization elements
  destroySync();
  // end process
  exit(0);
}


void clientResponse(char response, struct sockaddr_in clientAddr, int sc ){
  // Response to client
  sendto(sc, &response, 1, 0, (struct sockaddr *) &clientAddr,sizeof(clientAddr) );
}


void registerUser(struct argumentWrapper *args){

  char username[sizeof(args->username)];
  int sc;
  struct sockaddr_in clientAddr;

  // start critical section
  pthread_mutex_lock(&mutex_msg);

  strcpy(username,args->username);
  sc = args->clientFD;
  clientAddr = args->clientAddr;

  // release and end critical section
  msg_not_copied = FALSE;
  pthread_cond_signal(&cond_msg);
  pthread_mutex_unlock(&mutex_msg);

  free(args->username);

  /* CHECK IF ALREADY EXISTS */
  if( queue_find(queueUsers, username) == NULL){

    struct userInformation *user = (struct userInformation*)malloc(sizeof(struct userInformation));//si no se aloca desaparece al final del metodo, y en la queue se guarda un puntero a un sitio vacio
    bzero(user, sizeof(struct userInformation));

    strcpy(user->username, username);
    user->status = 0; // default status 0
    // Initialize pending messages
    user->pending_messages = queue_new();

    enqueue(queueUsers, (void *) user);

    printf("username %s\n", user->username);

    //char res = 0;
    //sendto(sc, &res, 1, 0, (struct sockaddr *) &clientAddr,sizeof(clientAddr) );
    clientResponse(0, clientAddr, sc); // success
  }else{
   clientResponse(1, clientAddr, sc); // user exist
  }
  close(sc);

}



void unregisterUser(struct argumentWrapper *args){

  char username[sizeof(args->username)];
  int sc;
  struct sockaddr_in clientAddr;

  // start critical section
  pthread_mutex_lock(&mutex_msg);

  strcpy(username,args->username);
  sc = args->clientFD;
  clientAddr = args->clientAddr;

  // release and end critical section
  msg_not_copied = FALSE;
  pthread_cond_signal(&cond_msg);
  pthread_mutex_unlock(&mutex_msg);

  free(args->username);

  if(queue_find_remove(queueUsers, username))
    clientResponse(0, clientAddr, sc); // success
  else
   clientResponse(1, clientAddr, sc); // user does not exist

  close(sc);
}

void  connectUser(struct argumentWrapper *args){
  char username[sizeof(args->username)];
  int sc;
  int port;
  struct userInformation* user;
  struct sockaddr_in clientAddr;

  // start critical section
  pthread_mutex_lock(&mutex_msg);

  strcpy(username,args->username);
  sc = args->clientFD;
  clientAddr = args->clientAddr;
  port = args->clientPort;
  // release and end critical section
  msg_not_copied = FALSE;
  pthread_cond_signal(&cond_msg);
  pthread_mutex_unlock(&mutex_msg);

  free(args->username);

  if((user = queue_find(queueUsers, username)) != NULL){
      if(user->status == 1) clientResponse(2, clientAddr, sc); // user is already connected
      else{
        user->user_addr = clientAddr.sin_addr;
        user->user_port = port;
        user->status = 1;
        clientResponse(0, clientAddr, sc); // success
      }
  }else clientResponse(1, clientAddr, sc); // user does not exist

  close(sc);
}


int main(int argc, char**argv){

  queueUsers = queue_new();

  // Initialize synchronization elements
  initializeSync();

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


  // client file descriptor
  int sc;
  // sockets addresses
  struct sockaddr_in serverAddr, clientAddr;

  int size, val;

  // Allocate memory for buffer
  buffer = (char *) calloc(BUFFER_SIZE,sizeof(char));


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

  // catch CTRL + C signal and execute handler
  signal(SIGINT, signal_handler);

  while(1){
    printf("Waiting for connection \n");

    sc = accept( sd, (struct sockaddr *) &clientAddr, &size);

    int count = readLine(sc, buffer,256);
    if(count == -1){
      perror("Error reading command: ");
    }else if(count == 0){
      printf("Error: command empty\n");
    }
    printf("Command: %s\n", buffer);


    if(!strcmp(buffer, "REGISTER")){
      char *username;
      username = (char *) calloc(256,sizeof(char));

      count = readLine(sc, username, 256);

      if(count == -1){
        perror("Error reading username: ");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }else if(count == 0){
        printf("Error: username empty\n");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }

      printf("count: %d, username: %s\n", count,username);
     // free(username);

      struct argumentWrapper args;
      args.username = username;
      args.clientFD = sc;
      args.clientAddr = clientAddr;

      pthread_t thid;
      // create new thread with message received
      if( pthread_create( &thid, &thread_attributes, (void *) registerUser, &args) != 0 ){
        perror("Error creating thread");
        close(sc);
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

    }else if(!strcmp(buffer, "UNREGISTER")){
      char *username;
      username = (char *) calloc(256,sizeof(char));
      count = readLine(sc, username, 256);

      if(count == -1){
        perror("Error reading username: ");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }else if(count == 0){
        printf("Error: username empty\n");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }

      printf("count: %d, username: %s\n", count,username);
     // free(username);

      struct argumentWrapper args;
      args.username = username;
      args.clientFD = sc;
      args.clientAddr = clientAddr;

      pthread_t thid;
      // create new thread with message received
      if( pthread_create( &thid, &thread_attributes, (void *) unregisterUser, &args) != 0 ){
        perror("Error creating thread");
        close(sc);
        continue;
    }
  }else if(!strcmp(buffer, "CONNECT")){
      char *username;
      char *portString;
      int port;
      username = (char *) calloc(256,sizeof(char));
      portString = (char *) calloc(8,sizeof(char));
      count = readLine(sc, username, 256);

      if(count == -1){
        perror("Error reading username: ");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }else if(count == 0){
        printf("Error: username empty\n");
        clientResponse(2, clientAddr, sc);
        close(sc);
        continue;
      }
      readLine(sc, portString, 8);
      port = atoi(portString);

      printf("count: %d, username: %s, port: %d\n", count, username, port);
     // free(username);
     free(portString);

      struct argumentWrapper args;
      args.username = username;
      args.clientFD = sc;
      args.clientAddr = clientAddr;
      args.clientPort = port;

      pthread_t thid;
      // create new thread with message received
      if( pthread_create( &thid, &thread_attributes, (void *) connectUser, &args) != 0 ){
        perror("Error creating thread");
        close(sc);
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

  }else printf("Function not found\n");


  }// while

  // free(buffer);
   close(sd);

  // Destroy synchronization elements
  destroySync();

 return 0;
}
