#define MAX_EVENTS 10
#define READ_SIZE 1024
#define buff_size 8192
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
#include <sys/sendfile.h>
#include <fcntl.h>

int listen_sock;
int client_sock;
FILE *fp;
int SetSocketBlockingEnabled(int fd, int blocking);
void sending(void* arg);
void epollThread();
void epollThread()
{
  struct sockaddr_in client;
  unsigned int sockaddr_len = sizeof(struct sockaddr_in);
  struct epoll_event ev, events[MAX_EVENTS];
  int nfds, epollfd;
  int running = 1;
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
          //SetSocketBlockingEnabled(client_sock,0);
          ev.events = EPOLLIN | EPOLLET;
          ev.data.fd = client_sock;
          if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock,&ev) == -1) {
            perror("epoll_ctl: client_sock");
            exit(EXIT_FAILURE);
            }
          printf("Sending to client with fd: %d\n",events[n].data.fd);
          sending(&client_sock);
    }
    else {
        //do_use_fd(events[n].data.fd);
        //bytes_read = read(events[n].data.fd, read_buffer, READ_SIZE);
        //printf("%zd bytes read.\n", bytes_read);
      }
    }
  }
    close(epollfd);
}
int main(int argc, char ** argv)
{
  fp = fopen("5M.b","r");
  if (fp == NULL){
    printf("invalid file opened\n");
    exit(1);
  }
  if (argc != 2)
  {
    printf("Not enough arguments, enter port number to be opened\n");
    exit(1);
  }
  //struct sockaddr_in server,client;
  struct sockaddr_in server;
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
  epollThread();
  close(listen_sock);

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
void sending(void* arg)
{
  int ClientSocket = *((int *)arg);;
  int fd = fileno(fp); //file descriptor for file about to be sent

  long bytes = 0;
  time_t startTime = time(NULL);
  int sent = 0;
  do{
    sent = sendfile(ClientSocket,fd,NULL,buff_size);
    if (sent<0){
      perror("gg:");
    }
    printf("Sent: %d bytes\n",sent);
    bytes = bytes + sent;
  } while(sent>0);
  time_t endTime = time(NULL);
  double secs = difftime(endTime,startTime);
  double Mb = 8*bytes / (1000000);
  //printf("%ld bytes sent\n",bytes);
  //printf("Time Spent: %lf secs \n",secs);
  double speed = Mb/secs;
  printf("Average Download Speed: %.2lf Mbit/s\n",speed);
  shutdown(ClientSocket,1);
  fseek( fp, 0, SEEK_SET );
  return;
}
