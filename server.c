#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

#define BUFFER_SIZE 1024
#define MESSAGE_SIZE 2048

#define MAX_CLIENTS 15
//Message types
#define REQ_ADD 1
#define REQ_REM 2
#define RES_LIST 4
#define MSG 6
#define ERROR 7
#define OK 8

typedef struct {
  int idMsg;
  int idSender;
  int idReceiver;
  char message[2048];
} Payload;

typedef struct {
  int socket;
  int id;
} Client;

Client clients[MAX_CLIENTS];
int numClients = 0;

int createAndConfigureSocketIPV4(int port){
  int sockfd;
  struct sockaddr_in servAddr;

  //Create the server socket
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Criação do socket falhou!\n");
    return -1;
  }

  //printf("Socket criado com sucesso!\n");

  //Config server addr structure
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons(port);

  //Associate a socket to a port and address
  if (bind(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
    printf("Bind do socket à porta e endereço falhou\n");
    return -1;
  }

  //printf("Socket associado a porta e endereço com sucesso!\n");

  ///Wait for a client to connect
  if (listen(sockfd, 1) < 0) {
    printf("Listen do socket falhou!\n");
    return -1;
  }

  //printf("Socket server ouvindo a porta: %d\n", port);

  return sockfd;
}

int receiveClientConnectionIPV4(int serverSock){
  int newSock;
  struct sockaddr_in cliAddr;
  socklen_t cliLen = sizeof(cliAddr);

  // Aceita a conexão do cliente
  if ((newSock = accept(serverSock, (struct sockaddr *) &cliAddr, &cliLen)) < 0) {
    printf("Accept conexão do cliente falhou!\n");
    return -1;
  }

  return newSock;
}

int createAndConfigureSocketIPV6(int port){
  int sockfd;
  struct sockaddr_in6 servAddr;

  //Create the server socket
  if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
    printf("Criação do socket falhou!\n");
    return -1;
  }

  printf("Socket criado com sucesso!\n");

  //Config server addr structure
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin6_family = AF_INET6;
  servAddr.sin6_addr = in6addr_any;
  servAddr.sin6_port = htons(port);

  //Associate a socket to a port and address
  if (bind(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
    printf("Bind do socket à porta e endereço falhou\n");
    return -1;
  }

  printf("Socket associado a porta e endereço com sucesso!\n");

  //Wait for a client to connect
  if (listen(sockfd, 1) < 0) {
    printf("Listen do socket falhou!\n");
    return -1;
  }

  printf("Socket server ouvindo a porta: %d\n", port);

  return sockfd;
}

int receiveClientConnectionIPV6(int serverSock){
  int newSock;
  struct sockaddr_in6 cliAddr;
  socklen_t cliLen = sizeof(cliAddr);

  //Accept client connection
  if ((newSock = accept(serverSock, (struct sockaddr *) &cliAddr, &cliLen)) < 0) {
    printf("Accept conexão do cliente falhou!\n");
    return -1;
  }

  printf("Cliente conectado!\n");

  return newSock;
}

void closeSockets(int serverSock, int cliSock){
  char *serverResponse = "connection closed";

  //Response for client
  if (send(cliSock, serverResponse, strlen(serverResponse), 0) == -1){
    printf("Send message failed\n");
  }

  if(close(serverSock) == -1){
    printf("Erro ao fechar o socket do servidor!\n");
  }

  if(close(cliSock) == -1){
    printf("Erro ao fechar o socket do cliente!\n");
  }
}

void clearPayload(Payload *payload){
  payload->idReceiver = (intptr_t)NULL;
  payload->idSender = (intptr_t)NULL;
  payload->idMsg = (intptr_t)NULL;
}

void *handleClient(void *arg) {
  int cliSock = *(int *)arg;
  Payload cliRequest = {
    .idSender = (intptr_t)NULL,
    .idMsg = (intptr_t)NULL,
    .idReceiver = (intptr_t)NULL
  };
  char bufferRequest[sizeof(Payload)];

  Payload *response = malloc(sizeof(Payload)); 
  *response = (Payload) {
    .idSender = (intptr_t)NULL,
    .idMsg = (intptr_t)NULL,
    .idReceiver = (intptr_t)NULL
  };

  char bufferResponse[sizeof(Payload)];

  while (1){  
    //Receive from client and resolve struct
    int recvReturn = recv(cliSock, bufferRequest, sizeof(Payload), 0);
    memcpy(&cliRequest, bufferRequest, sizeof(Payload));

    if (recvReturn <= 0) {
      break;
    }

    if(cliRequest.idMsg == MSG){    
      //Broadcast message
      if(!cliRequest.idReceiver || cliRequest.idReceiver == (intptr_t)NULL){
        //Retrieve time from system
        time_t systemNow = time(NULL);
        struct tm* localTime = localtime(&systemNow);
        // Format string with retrieved time
        char timeStr[6];
        strftime(timeStr, sizeof(timeStr), "%H:%M", localTime);

        char *finalClientsMessage = malloc(200);
        strcat(finalClientsMessage, "[");
        strcat(finalClientsMessage, timeStr);
        strcat(finalClientsMessage, "] ");
        sprintf(finalClientsMessage + strlen(finalClientsMessage), "%02d", cliRequest.idSender);
        strcat(finalClientsMessage, ": ");
        strcat(finalClientsMessage, cliRequest.message);

        char *finalSelfMessage = malloc(200);
        strcat(finalSelfMessage, "[");
        strcat(finalSelfMessage, timeStr);
        strcat(finalSelfMessage, "] ");
        strcat(finalSelfMessage, "-> all");
        strcat(finalSelfMessage, ": ");
        strcat(finalSelfMessage, cliRequest.message);

        //Build message payload
        response->idMsg = MSG;
        response->idSender = cliRequest.idSender;
        
        //Server prints the message
        printf("%s\n", finalClientsMessage);

        //Send broadcast message
        for(int i=0; i < MAX_CLIENTS; i++){
          //Formatted to the client who is sending the broadcast
          if(clients[i].id != -1 && clients[i].id == cliRequest.idSender){
            strcpy(response->message, finalSelfMessage);
            //Send message to the target
            memcpy(bufferResponse, response, sizeof(Payload));
            send(clients[i].socket, bufferResponse, sizeof(Payload), 0);
          }else if(clients[i].id != -1){
            strcpy(response->message, finalClientsMessage);
            //Send message to the target
            memcpy(bufferResponse, response, sizeof(Payload));
            send(clients[i].socket, bufferResponse, sizeof(Payload), 0);
          }
        }

        clearPayload(response);
        free(finalClientsMessage);
      //Private message
      }else{
        bool userFound = false;
        //Search target of the message
        for(int i=0; i < MAX_CLIENTS; i++){
          if(clients[i].id == cliRequest.idReceiver){
            userFound = true;
            //Build confirmation payload
            response->idMsg = OK;
            strcpy(response->message, cliRequest.message);
            response->idReceiver = cliRequest.idReceiver;
            response->idSender = cliRequest.idSender;

            //Send information that the target was found successfully
            memcpy(bufferResponse, response, sizeof(Payload));
            send(cliSock, bufferResponse, sizeof(Payload), 0);

            //Build message payload
            response->idMsg = MSG;

            //Send message to the target
            memcpy(bufferResponse, response, sizeof(Payload));
            send(clients[i].socket, bufferResponse, sizeof(Payload), 0);
            clearPayload(response);
          }
        }

        if(!userFound){
          //Build payload
          char *message = "Receiver not found";
          response->idMsg = ERROR;
          strcpy(response->message, message);
          response->idReceiver = cliRequest.idSender;

          //Send information that the target was not found
          memcpy(bufferResponse, response, sizeof(Payload));
          send(cliSock, bufferResponse, sizeof(Payload), 0);
          clearPayload(response);
        }
      }

    }else if(cliRequest.idMsg == REQ_REM){
      bool userFound = false;
      //Search user to remove it
      for(int i=0; i < MAX_CLIENTS; i++){
        if(clients[i].id == cliRequest.idSender){
          //Remove client and decrease client counter
          userFound = true;
          clients[i].id = -1;
          clients[i].socket = -1;
          numClients--;
        }
      }

      //if user was removed
      if(userFound){
        //Build payload
        char *message = "Removed Successfully";
        response->idMsg = OK;
        strcpy(response->message, message);
        response->idReceiver = cliRequest.idSender;

        //Send response
        memcpy(bufferResponse, response, sizeof(Payload));
        send(cliSock, bufferResponse, sizeof(Payload), 0);
        clearPayload(response);

        printf("User %02d removed\n", cliRequest.idSender);

        //Build payload
        response->idMsg = REQ_REM;
        response->idSender = cliRequest.idSender;
        //Send broadcast message to all clients telling one client was removed
        for(int j=0; j < MAX_CLIENTS; j++){
          if(clients[j].id != -1){
            //Prepare struct to send through socket
            memcpy(bufferResponse, response, sizeof(Payload));
            send(clients[j].socket, bufferResponse, sizeof(Payload), 0);
          }
        }
      }else{
        //Build payload
        char *message = "User not found";
        response->idMsg = ERROR;
        strcpy(response->message, message);
        response->idReceiver = cliRequest.idSender;

        //Send response
        memcpy(bufferResponse, response, sizeof(Payload));
        send(cliSock, bufferResponse, sizeof(Payload), 0);
        clearPayload(response);
      }
    }
  }

  close(cliSock);
  pthread_exit(NULL);
}

char *concatIntArray(int *clientArray, int numClients) {
  char *result = malloc(numClients * 4 + 1);

  for (int i = 0; i < numClients; i++) {
    sprintf(result + strlen(result), "%d", clientArray[i]);

    if (i < numClients - 1) {
      strcat(result, ", ");
    }
  }

  return result;
}

int main(int argc, char *argv[]){

  for(int i=0; i < MAX_CLIENTS; i++){
    clients[i].id = -1;
    clients[i].socket = -1;
  }

  Payload cliRequest = {
    .idSender = (intptr_t)NULL,
    .idMsg = (intptr_t)NULL,
    .idReceiver = (intptr_t)NULL
  };

  char bufferCliRequest[sizeof(Payload)];

  Payload *response = malloc(sizeof(Payload)); 
  *response = (Payload) {
    .idSender = (intptr_t)NULL,
    .idMsg = (intptr_t)NULL,
    .idReceiver = (intptr_t)NULL
  };

  char bufferResponse[sizeof(Payload)];

  char buffer[BUFFER_SIZE];
  
  char *ipVersion = argv[1];
  int port = atoi(argv[2]);

  //printf("Server running!\n");
  //printf("Versão IP: %s\n", argv[1]);
  //printf("Porta: %s\n", argv[2]);

  int sockfd, cliSock;
  
  if(strcmp(ipVersion, "v4") == 0){
    //printf("Configurando conexão IPV4!\n");

    //Create sock and accept client connection
    if((sockfd = createAndConfigureSocketIPV4(port)) < 0){
      return 0;
    }

    //printf("IPV4 configurado com sucesso!\n");
  }else if(strcmp(ipVersion, "v6") == 0){
    printf("Configurando conexão IPV6!\n");

    //Create sock and accept client connection
    if((sockfd = createAndConfigureSocketIPV6(port)) < 0){
      return 0;
    }

    //printf("IPV6 configurado com sucesso!\n");
  }else{
    printf("Versão ip recebida inválida!\n");
  }

  int recvReturn;

  while(1){
    if(strcmp(ipVersion, "v4") == 0){
      if((cliSock = receiveClientConnectionIPV4(sockfd)) < 0){
        return 0;
      }
    }else if(strcmp(ipVersion, "v6") == 0){
      if((cliSock = receiveClientConnectionIPV6(sockfd)) < 0){
        return 0;
      }
    }
    //Receive from client and resolve struct
    recvReturn = recv(cliSock, bufferCliRequest, sizeof(Payload), 0);
    memcpy(&cliRequest, bufferCliRequest, sizeof(Payload));

    if(recvReturn < 0){
      printf("Erro ao executar recv do client\n");
      return 0;
    }

    //Verifies if has any space for a new cliente and tells them if no space is left
    if (numClients >= MAX_CLIENTS) {
      char *message = "User limit exceeded\n";
      response->idMsg = ERROR;
      strcpy(response->message, message);

      //Prepare struct to send through socket
      memcpy(bufferResponse, response, sizeof(Payload));
      send(cliSock, bufferResponse, sizeof(Payload), 0);
      close(cliSock);
      continue;
    }

    //Found the lowest empty client place, add client, notify all users and send a list of users to new user
    for(int i=0; i < MAX_CLIENTS; i++){
      if(clients[i].id == -1){
        clients[i].id = i+1;
        clients[i].socket = cliSock;
        numClients++;
        printf("User %02d added\n", clients[i].id);

        char *userIdStr = malloc(12);
        char *broadMessage = malloc(100);
        char *part2M = malloc(100);

        //Construct message setup
        if(clients[i].id >= 10){
          strcpy(broadMessage, "User ");
        }else{
          strcpy(broadMessage, "User 0");
        }
        sprintf(userIdStr, "%d", clients[i].id);
        strcpy(part2M, " joined the group!");

        //BuildMessage
        strcat(broadMessage, userIdStr);  
        strcat(broadMessage, part2M);
        //Build broadcast response payload
        response->idMsg = MSG;
        response->idSender = clients[i].id;
        strcpy(response->message, broadMessage);
        
        //Send broadcast message to all clients telling that there is a new client
        for(int j=0; j < MAX_CLIENTS; j++){
          if(clients[j].id != -1){
            //Prepare struct to send through socket
            memcpy(bufferResponse, response, sizeof(Payload));
            send(clients[j].socket, bufferResponse, sizeof(Payload), 0);
          }
        }
        clearPayload(response);

        //List users for new users
        char *messageListUsers = malloc(200);
        int countUsersAdded = 0;
        for(int i=0; i < MAX_CLIENTS; i++){         
          if(clients[i].id != -1){
            if(countUsersAdded > 0){
              strcat(messageListUsers, ", ");
              sprintf(messageListUsers + strlen(messageListUsers), "%d", clients[i].id);   
            }else{
              sprintf(messageListUsers + strlen(messageListUsers), "%d", clients[i].id);
            }   
            countUsersAdded++;
          }
        }

        //Build list users payload
        response->idMsg = RES_LIST;
        strcpy(response->message, messageListUsers);

        //Prepare struct to send through socket
        memcpy(bufferResponse, response, sizeof(Payload));
        send(cliSock, bufferResponse, sizeof(Payload), 0);

        free(userIdStr);
        free(broadMessage);
        free(part2M);
        free(messageListUsers);
        break;
      }   
    }

    pthread_t thread;
    if (pthread_create(&thread, NULL, handleClient, (void *)&cliSock) != 0) {
      perror("Erro ao criar a thread");
      exit(EXIT_FAILURE);
    }

    if(strcmp(buffer, "exit") == 0){
      //Close all sockets
      closeSockets(sockfd, cliSock);
      return 0;
    }
  }
}