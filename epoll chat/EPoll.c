#define MAX_EVENTS 10
#define READ_SIZE 1024
#include <stdio.h>     // for fprintf()
#include <unistd.h>    // for close(), read()
#include <sys/epoll.h> // for epoll_create1(), epoll_ctl(), struct epoll_event
#include <string.h>    // for strncmp
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
#include <error.h>
#include <fcntl.h>
#include "tree.h"
int listen_sock;
int client_sock;
int SetSocketBlockingEnabled(int fd, int blocking);
void printTree(struct tree *root, char* msg, int sender);
void printTree(struct tree *root, char* msg, int sender)
{
  if (root == NULL){
    return;
  }
  //printf("key: %d\n",root->key);
  if (sender != root->key){
    dprintf(root->key,"User %d: %s\n",sender,msg);
  }
  printTree(root->child[0],msg,sender);
  printTree(root->child[1],msg,sender);
  return;
}
int main(int argc, char ** argv)
{
  struct tree *root = malloc(sizeof(struct tree));
  root->key = -1;
  int newRoot = 0;


  /*
  printf("%d\n",root->child[0]->key);
  printf("%d\n",root->child[1]->key);
  printf("%d\n",root->key);
  */

  if (argc != 2)
  {
    printf("Not enough arguments, enter port number to be opened\n");
    exit(1);
  }
  struct sockaddr_in server,client;
  listen_sock = socket(AF_INET,SOCK_STREAM,0);
  //SOCK_STREAM = TCP
  if (listen_sock == -1){
    perror("socket: ");
    exit(-1);
  }
  printf("Socket created\n");
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(strtol(argv[1],NULL,10));
  bzero(&server.sin_zero,8);
  int b = bind(listen_sock,(struct sockaddr*)&server,sizeof(server));
  while(b == -1){
    server.sin_port = htons(ntohs(server.sin_port)+1);
    b = bind(listen_sock,(struct sockaddr*)&server,sizeof(server));
    }
  printf("Binded to port %d\n",htons(server.sin_port));

  if (listen(listen_sock,MAX_EVENTS) == -1){ //5 pending connections maximum
    perror("listen: ");
    exit(-1);
  }

  unsigned int sockaddr_len = sizeof(struct sockaddr_in);
  struct epoll_event ev, events[MAX_EVENTS];
  int nfds, epollfd;
  int running = 1;
  char read_buffer[READ_SIZE + 1];
  size_t bytes_read;

  epollfd = epoll_create1(0);
  if (epollfd == -1) {
      perror("epoll_create1");
      exit(EXIT_FAILURE);
  }
  ev.events = EPOLLIN;
  ev.data.fd = listen_sock;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
      perror("epoll_ctl: listen_sock");
      exit(EXIT_FAILURE);
  }
  while(running){
      nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
      if (nfds == -1) {
        perror("epoll_wait");
        exit(EXIT_FAILURE);
           }
    for (int n = 0; n < nfds; ++n)
    {

      if (events[n].data.fd == listen_sock) {
        client_sock = accept(listen_sock, (struct sockaddr*)&client, &sockaddr_len);
        printf("New client connected from port no %d and IP %s\n",ntohs(client.sin_port),inet_ntoa(client.sin_addr));
        if (client_sock == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
          }
          SetSocketBlockingEnabled(client_sock,0);
          ev.events = EPOLLIN | EPOLLET;
          ev.data.fd = client_sock;
          if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock,&ev) == -1) {
            perror("epoll_ctl: client_sock");
            exit(EXIT_FAILURE);
            }
    }
    else {
        //do_use_fd(events[n].data.fd);
        bytes_read = read(events[n].data.fd, read_buffer, READ_SIZE);
        //printf("%zd bytes read.\n", bytes_read);
        if (bytes_read == 0){
          printf("User %d Disconnected\n",events[n].data.fd);
          treeDelete(&root,events[n].data.fd);
        }
        else
        {
          read_buffer[bytes_read-1] = '\0';
          if (!newRoot){
            treeDelete(&root,-1);
            newRoot = 1;
          }
          treeInsert(&root,events[n].data.fd);
          printTree(root,read_buffer,events[n].data.fd);
          //dprintf(events[n].data.fd,"You: %s\n",read_buffer);
          printf("User %d: %s\n",events[n].data.fd,read_buffer);
          if(!strncmp(read_buffer, "S9S1", 4))
          {
            if (read_buffer[4] == '\0' || (read_buffer[4] == '\n' && read_buffer[5] == '\0'))
            {
              running = 0;
              printf("Client stopped\n");
              break;
            }
          }
        }
      }
    }
  }
  //ClientSocket = accept(ServerSocket,(struct sockaddr*)&client,&sockaddr_len);
  treeDestroy(&root);
  close(listen_sock);
  close(epollfd);
}
int SetSocketBlockingEnabled(int fd, int blocking)
{
   if (fd < 0) return 0;

#ifdef _WIN32
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? 1 : 0;
#else
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return 0;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? 1 : 0;
#endif
}
