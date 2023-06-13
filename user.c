#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define FILE_BUFFER 495

char validFiles[6][6] = {".txt", ".c", ".cpp", ".py", ".tex", ".java"};

int createAndConnectSockToServerIPV4orIPV6(char* ip, int port){
  int sock;
  struct sockaddr_in servAddrV4;
  struct sockaddr_in6 servAddrV6;

  //IPV4
  if(inet_pton(AF_INET, ip, &servAddrV4.sin_addr) == 1){

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("Criação do socket falhou!\n");
      return -1;
    }
    
    memset(&servAddrV4, '0', sizeof(servAddrV4));
    servAddrV4.sin_family = AF_INET;
    servAddrV4.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &servAddrV4.sin_addr) <= 0) {
      printf("Endereço v4 não suportado\n");
      return -1;
    }

    if (connect(sock, (struct sockaddr *)&servAddrV4, sizeof(servAddrV4)) < 0) {
      printf("Connection Failed\n");
      return -1;
    }

    printf("Conectado com sucesso ao server %s porta %d\n", ip, port);

  //IPV6
  }else if(inet_pton(AF_INET6, ip, &servAddrV6.sin6_addr) == 1){

    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
      printf("Criação do socket falhou!\n");
      return -1;
    }
    
    memset(&servAddrV6, '0', sizeof(servAddrV6));
    servAddrV6.sin6_family = AF_INET6;
    servAddrV6.sin6_port = htons(port);

    if (inet_pton(AF_INET6, ip, &servAddrV6.sin6_addr) <= 0) {
      printf("Endereço v6 não suportado\n");
      return -1;
    }

    if (connect(sock, (struct sockaddr *)&servAddrV6, sizeof(servAddrV6)) < 0) {
      printf("Connection Failed\n");
      return -1;
    }

    printf("Conectado com sucesso ao server %s porta %d\n", ip, port);
  }

  return sock;
}

void closeSocket(int sock){
  if(close(sock) == -1){
    printf("Erro ao fechar o socket\n");
  }
}

int main(int argc, char *argv[]){
  char buffer[BUFFER_SIZE];
  char *ip = argv[1];
  int port = atoi(argv[2]);

  printf("Client running!\n");
  printf("Endereço IP: %s\n", argv[1]);
  printf("Porta de conexão: %s\n", argv[2]);

  int sock;

  //Create and connect sock to server
  if((sock = createAndConnectSockToServerIPV4orIPV6(ip, port)) < 0){
    return 0;
  }

  while(1){
    fgets(buffer, sizeof(buffer), stdin);
    //Clear string \n
    buffer[strcspn(buffer, "\n")] = '\0'; 

    if(strstr(buffer, "select file") != NULL){
      continue;
    }else{ 
      closeSocket(sock);
      break;
    } 
  }
  return 0;
}