#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <error.h>
#include <pthread.h>
#include <sys/sendfile.h>
#include <errno.h>
#define buff_size 8192
double receive(int socket);
double sending(int socket);
int speedTest();

int speedTest()
{
  return 0;
}

//argv[1] is ip address of server and argv[2] is port number of serve
int main(int argc, char ** argv)
{
  int ServerSocket = 0;
  if (argc != 3)
  {
    fprintf(stderr,"Not enough arguments\n");
    exit(1);
  }
  struct sockaddr_in server;
  ServerSocket = socket(AF_INET,SOCK_STREAM,0);
  //unsigned int length = sizeof(struct sockaddr);
  //SOCK_STREAM = TCP
  if (ServerSocket == -1){
    perror("socket: ");
    exit(-1);
  }
  printf("Socket created\n");

  server.sin_family = AF_INET;
  if(inet_pton(AF_INET, argv[1], &(server.sin_addr))<=0)
  {
      printf("Invalid IP Address Given\n");
      exit(1);
  }
  server.sin_port = htons(strtol(argv[2],NULL,10));
  bzero(&server.sin_zero,8);
  if (connect(ServerSocket, (struct sockaddr *)&server, sizeof(server)) < 0)
      {
          fprintf(stderr,"Connection Failed \n");
          exit(1);
      }
  else
  {
      fprintf(stderr,"Connected to server port no %d and IP %s\n",ntohs(server.sin_port),inet_ntoa(server.sin_addr));
  }
  double speed = receive(ServerSocket);
  close(ServerSocket);
  return 0;
}

double receive(int ServerSocket){
  int data_len = 0;
  //int ServerSocket = *((int *)arg);
  char recv_buffer[buff_size];
  long bytes = 0;
  char message[] = "GET /5M.b HTTP/1.1\r\nHost: 192.168.50.254\r\n\r\n";
  char response[] = "HTTP/1.1 200";
  struct timeval stop, start;
  write(ServerSocket,message,strlen(message));
  data_len = recv(ServerSocket,recv_buffer,buff_size,0);
  if (data_len && (strncmp(recv_buffer, response, strlen(response))==0))
  {
    printf("HTTP OK received\n");
    gettimeofday(&start, NULL);
    do{
        //printf("waiting for server");
        data_len = recv(ServerSocket,recv_buffer,buff_size,0);
        bytes = bytes + data_len;
        //printf("%s",recv_buffer);
        //printf("Received %d\n",data_len);
        /*
        if (errno == EAGAIN || errno == EWOULDBLOCK){
          return NULL;
        }
        */
        /*
        if (data_len){
          fwrite(recv_buffer,sizeof(char),data_len,fp);
        }
        */
    } while(data_len);
    gettimeofday(&stop, NULL);
    double bits = 8*bytes;
    printf("took %lu us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
    double usecs = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
    double speed = bits/usecs; //becomes Mb/s
    fprintf(stderr,"Average Download Speed: %.3lf Mbit/s\n",speed);
    //pthread_mutex_unlock(&lock);
    return speed;
  }
  else
  {
    return 0;
  }

}
double sending(int ServerSocket)
{
  //int ServerSocket = *((int *)arg);;
  struct timeval stop, start;
  gettimeofday(&start, NULL);
  long bytes = 0;
  int sent = 0;
  do{
    //sent = sendfile(ServerSocket,fd,NULL,buff_size);
    //printf("Sent: %d bytes\n",sent);
    bytes = bytes + sent;
  } while(sent>0);
  gettimeofday(&stop, NULL);
  double secs = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
  double Mb = 8*bytes / (1000000);
  //printf("%ld bytes sent\n",bytes);
  //printf("Time Spent: %lf secs \n",secs);
  double speed = Mb/secs;
  shutdown(ServerSocket,1);
  fprintf(stderr,"Average Upload Speed: %.2lf Mbit/s\n",speed);
  return speed;
}
