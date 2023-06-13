#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

int createAndConfigureSocketIPV4(int port){
  int sockfd;
  struct sockaddr_in servAddr;

  //Create the server socket
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Criação do socket falhou!\n");
    return -1;
  }

  printf("Socket criado com sucesso!\n");

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

  printf("Socket associado a porta e endereço com sucesso!\n");

  ///Wait for a client to connect
  if (listen(sockfd, 1) < 0) {
    printf("Listen do socket falhou!\n");
    return -1;
  }

  printf("Socket server ouvindo a porta: %d\n", port);

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

  printf("Cliente conectado!\n");

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

int main(int argc, char *argv[]){
  char buffer[BUFFER_SIZE]; 
  
  char *ipVersion = argv[1];
  int port = atoi(argv[2]);

  printf("Server running!\n");
  printf("Versão IP: %s\n", argv[1]);
  printf("Porta: %s\n", argv[2]);

  int sockfd, cliSock;
  
  if(strcmp(ipVersion, "v4") == 0){
    printf("Configurando conexão IPV4!\n");

    //Create sock and accept client connection
    if((sockfd = createAndConfigureSocketIPV4(port)) < 0){
      return 0;
    }
    if((cliSock = receiveClientConnectionIPV4(sockfd)) < 0){
      return 0;
    }

    printf("IPV4 configurado com sucesso!\n");
  }else if(strcmp(ipVersion, "v6") == 0){
    printf("Configurando conexão IPV6!\n");

    //Create sock and accept client connection
    if((sockfd = createAndConfigureSocketIPV6(port)) < 0){
      return 0;
    }
    if((cliSock = receiveClientConnectionIPV6(sockfd)) < 0){
      return 0;
    }

    printf("IPV6 configurado com sucesso!\n");
  }else{
    printf("Versão ip recebida inválida!\n");
  }

  int recvReturn;

  while(1){
    memset(buffer, 0, sizeof(buffer));
    recvReturn = recv(cliSock, buffer, sizeof(buffer), 0);
    
    if(recvReturn < 0){
      printf("Erro ao executar recv do client\n");
      return 0;
    }

    //Print on the default exit the message received from client
    printf("Mensagem recebida do cliente: %s\n", buffer);

    if(strcmp(buffer, "exit") == 0){
      //Close all sockets
      closeSockets(sockfd, cliSock);
      return 0;
    }
  }
}