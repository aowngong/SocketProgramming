#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <error.h>
#include <pthread.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#define buff_size 8192
void *receive(void *arg);
void *sending(void *arg);
void InitializeSSL();
void DestroySSL();
void ShutdownSSL();
pthread_t tid[2];
pthread_mutex_t lock;
SSL_CTX *sslctx;
SSL *cSSL;
void main(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Not enough arguments\n");
    return;
  }
  int ServerSock, ClientSocket;
  struct sockaddr_in server, client;
  InitializeSSL();
  ServerSock = socket(AF_INET, SOCK_STREAM, 0);
  //SOCK_STREAM = TCP
  if (ServerSock == -1)
  {
    perror("socket: ");
    exit(-1);
  }
  puts("Socket created");
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(strtol(argv[1], NULL, 10));
  bzero(&server.sin_zero, 8);
  int b = bind(ServerSock, (struct sockaddr *)&server, sizeof(server));
  while (b == -1)
  {
    server.sin_port = htons(ntohs(server.sin_port) + 1);
    b = bind(ServerSock, (struct sockaddr *)&server, sizeof(server));
  }
  printf("Binded to port %d\n", htons(server.sin_port));

  if (listen(ServerSock, 2) == -1)
  { //5 pending connections maximum
    perror("listen: ");
    exit(-1);
  }
  char recv_buffer[buff_size];
  int sockaddr_len = sizeof(struct sockaddr_in);
  ClientSocket = -1;
  ClientSocket = accept(ServerSock, (struct sockaddr *)&client, &sockaddr_len);
  if (ClientSocket == -1) //client is empty but it will get info from accept function
  {
    printf("ClientSocket: %d\n", ClientSocket);
    perror("accept: ");
    exit(-1);
  }
  else
  {
    printf("New client connected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
  }
  sslctx = SSL_CTX_new(SSLv23_server_method());
  SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
  int use_cert = SSL_CTX_use_certificate_file(sslctx, "/home/vip/SSL/server.cert", SSL_FILETYPE_PEM);
  int use_prv = SSL_CTX_use_PrivateKey_file(sslctx, "/home/vip/SSL/server.key", SSL_FILETYPE_PEM);
  cSSL = SSL_new(sslctx);
  SSL_set_fd(cSSL, ClientSocket);
  //Here is the SSL Accept portion.  Now all reads and writes must use SSL
  int ssl_err = SSL_accept(cSSL);
  if (ssl_err <= 0)
  {
    printf("Non SSL Connection\n");
    ShutdownSSL();
    printf("Exit\n");
    return;
  }
  if (pthread_mutex_init(&lock, NULL) != 0)
  {
    printf("\n mutex init has failed\n");
    return;
  }
  int rtv = pthread_create(&tid[0], NULL, receive, (void *)&ClientSocket);
  if (rtv != 0)
  {
    printf(" ERROR;  pthread_create1()   returns   %d\n", rtv);
    return;
  }
  int rtv1 = pthread_create(&tid[1], NULL, sending, (void *)&ClientSocket);
  if (rtv1 != 0)
  {
    printf(" ERROR;  pthread_create2()   returns   %d\n", rtv1);
    return;
  }
  rtv = pthread_join(tid[0], NULL);
  if (rtv != 0)
  {
    printf(" ERROR ; pthread_join1()   returns   %d\n", rtv);
    return;
  }
  rtv1 = pthread_join(tid[1], NULL);
  if (rtv1 != 0)
  {
    printf(" ERROR ; pthread_join2()   returns   %d\n", rtv1);
    return;
  }
  pthread_mutex_destroy(&lock);

  printf("Client Disconnected\n");
  close(ClientSocket);
  close(ServerSock);
  return;
}

void *receive(void *arg)
{
  int data_len = 0;
  int ClientSocket = *((int *)arg);
  char recv_buffer[buff_size];
  while (1)
  {
    //data_len = recv(ClientSocket, recv_buffer, buff_size, 0);
    data_len = SSL_read(cSSL, (char *)recv_buffer, buff_size);
    if (data_len)
    {
      pthread_mutex_lock(&lock);
      recv_buffer[data_len] = '\0';
      printf("Received: %s\n", recv_buffer);
      pthread_mutex_unlock(&lock);
    }
    else
    {
      pthread_cancel(tid[1]);
      //printf("receive thread ended\n");
      break;
    }
  }
  //pthread_mutex_unlock(&lock);
  return NULL;
}
void *sending(void *arg)
{
  //printf("Sending Thread\n");
  int ClientSocket = *((int *)arg);
  ;
  //int ClientSocket = *((int *)arg);
  int replySize = 0;
  int data_len = 0;
  char reply[buff_size];
  do
  {
    if (fgets(reply, buff_size, stdin) != NULL)
    {
      pthread_mutex_lock(&lock);
      replySize = strcspn(reply, "\n") + 1;
      //printf("replySize: %d",replySize);
      //name[strcspn(name, "\n")] = 0;
      //data_len = send(ClientSocket, reply, replySize, 0);
      data_len = SSL_write(cSSL, reply, replySize);
      //printf("data len: %d\n",data_len);
      pthread_mutex_unlock(&lock);
    }

  } while (data_len);

  return NULL;
}

void InitializeSSL()
{
  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();
}

void DestroySSL()
{
  ERR_free_strings();
  EVP_cleanup();
}

void ShutdownSSL()
{
  SSL_shutdown(cSSL);
  SSL_free(cSSL);
}