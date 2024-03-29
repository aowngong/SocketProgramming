#include "SpeedTest.h"
static int receive(int socket, double *speed, int filesize);
static int sending(int ServerSocket, double *speed, int port, char *ip, int packets);
static int repeatTester(struct sockaddr_in server, int ServerSocket, double *DLSpeed, double *UPSpeed, int *packets, int *filesize, int mode);
static void packetSize(int *packets, int *filesize, double DLSpeed, double UPSpeed);

int speedTest(int* ServerSocket, char *IPv4, char *port, double *DLSpeedArray, double *UPSpeedArray, int mode)
{
  struct sockaddr_in server;
  *ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (*ServerSocket == -1)
  {
    perror("socket: ");
    return -1;
  }
  server.sin_family = AF_INET;
  if (inet_pton(AF_INET, IPv4, &(server.sin_addr)) <= 0)
  {
    printf("Invalid IP Address Given\n");
    return -1;
  }
  server.sin_port = htons(strtol(port, NULL, 10));
  bzero(&server.sin_zero, 8);
  if (connect(*ServerSocket, (struct sockaddr *)&server, sizeof(server)) < 0)
  {
    fprintf(stderr, "Connection Failed \n");
    return -1;
  }
  else
  {
    fprintf(stderr, "Connected to server port no %d and IP %s\n", ntohs(server.sin_port), inet_ntoa(server.sin_addr));
  }
  int packets = 1200;
  int filesize = 8;
  double DLSpeed = 0;
  double UPSpeed = 0;
  int flag = 0;
  double progress = 0;
  for (int i = 0; i < DATAPOINTS; i++)
  {
    flag = repeatTester(server, *ServerSocket, &DLSpeed, &UPSpeed, &packets, &filesize, mode);
    packetSize(&packets, &filesize, DLSpeed, UPSpeed);
    //printf("New packets: %d\n",packets);
    //printf("New filesize: %d\n",filesize);
    progress = 100 * ((double)i + 1) / DATAPOINTS;
    if (flag == FULLTEST)
    {
      DLSpeedArray[i] = DLSpeed;
      UPSpeedArray[i] = UPSpeed;
    }
    else if (flag == DOWNLOAD)
    {
      DLSpeedArray[i] = DLSpeed;
    }
    else if (flag == UPLOAD)
    {
      UPSpeedArray[i] = UPSpeed;
    }
    printf("%0.2lf%% done\n", progress);
  }
  //close(*ServerSocket);
  if (flag == FULLTEST)
  {
    return FULLTEST;
  }
  else if (flag == DOWNLOAD)
  {
    return DOWNLOAD;
  }
  else if (flag == UPLOAD)
  {
    return UPLOAD;
  }
  else
  {
    return -1;
  }
}
static int repeatTester(struct sockaddr_in server, int ServerSocket, double *DLSpeed, double *UPSpeed, int *packets, int *filesize, int mode)
{

  if (mode == FULLTEST)
  {
    int downloadCode = receive(ServerSocket, DLSpeed, *filesize);
    int uploadCode = sending(ServerSocket, UPSpeed, ntohs(server.sin_port), inet_ntoa(server.sin_addr), *packets);
    if (uploadCode == 0 && downloadCode == 0)
    {
      return FULLTEST;
    }
    else if (uploadCode == -1 && downloadCode == -1)
    {
      fprintf(stderr, "Error with both download and upload test\n");
      return -FULLTEST;
    }
    else if (uploadCode == -1)
    {
      fprintf(stderr, "Error with upload test\n");
      return -UPLOAD;
    }
    else
    {
      fprintf(stderr, "Error with download test\n");
      return -DOWNLOAD;
    }
  }
  else if (mode == DOWNLOAD)
  {
    if (receive(ServerSocket, DLSpeed, *filesize) == 0)
    {

      return DOWNLOAD;
    }
    else
    {
      fprintf(stderr, "Error with Download Test\n");
      return -DOWNLOAD;
    }
  }
  else if (mode == UPLOAD)
  {
    if (sending(ServerSocket, UPSpeed, ntohs(server.sin_port), inet_ntoa(server.sin_addr), *packets) == 0)
    {
      return UPLOAD;
    }
    else
    {
      fprintf(stderr, "Error with Upload Test\n");
      return -UPLOAD;
    }
  }
  return -1;
}

static int receive(int ServerSocket, double *speed, int filesize)
{
  int data_len = 0;
  //int ServerSocket = *((int *)arg)  ;
  char *recv_buffer = malloc(sizeof(char) * buff_size);
  if (recv_buffer == NULL)
  {
    return -1;
  }
  long bytes = 0;
  //char message[] = "GET /testFiles/4MB.b HTTP/1.1\r\nHost: 192.168.50.254\r\n\r\n";
  char response[] = "HTTP/1.1 200";
  char sizeMessage[] = "Content-Length: ";
  struct timeval stop, start;
  dprintf(ServerSocket, "GET /testFiles/%dMB.b HTTP/1.1\r\nHost: 192.168.50.254\r\n\r\n", filesize);
  //data_len = write(ServerSocket,message,strlen(message));
  data_len = recv(ServerSocket, recv_buffer, buff_size, 0);
  char *EndofHeaderPtr = strstr(recv_buffer, "\r\n\r\n");
  if (EndofHeaderPtr == NULL)
  {
    free(recv_buffer);
    return -1;
  }
  EndofHeaderPtr = &EndofHeaderPtr[4];
  if (EndofHeaderPtr != NULL)
  {
    int Blength = EndofHeaderPtr - recv_buffer;
    bytes = bytes + (data_len - Blength);
    //printf("Found end of header: %d\n",Blength);
  }
  else
  {
    free(recv_buffer);
    return -1;
  }
  char *ptr = strstr(recv_buffer, sizeMessage);
  if (ptr != NULL)
  {

    ptr = &ptr[strlen(sizeMessage)];
    int ctr = 0;
    while (ptr[ctr] != '\n')
    {
      ctr++;
    }
    char *filesizeStr = malloc(sizeof(char) * (ctr));
    if (filesizeStr == NULL)
    {
      return -1;
    }
    memcpy(filesizeStr, ptr, sizeof(char) * ctr);
    long filesize = strtol(filesizeStr, NULL, 10);
    free(filesizeStr);
    if (data_len && (strncmp(recv_buffer, response, strlen(response)) == 0))
    {
      //fprintf(stderr,"HTTP OK received\n");
      gettimeofday(&start, NULL);
      do
      {
        data_len = recv(ServerSocket, recv_buffer, buff_size, 0);
        bytes = bytes + data_len;
        if (bytes >= filesize)
        {
          free(recv_buffer);
          break;
        }
      } while (data_len);
      gettimeofday(&stop, NULL);
      ///printf("took %lu us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
      double usecs = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
      *speed = 8 * bytes / usecs; //becomes Mb/s
      return 0;
    }
    else if (strncmp(recv_buffer, "HTTP/1.1 302", strlen(response)) == 0)
    {
      printf("Download file not found\n");
      free(recv_buffer);
      return -1;
    }
  }
  else
  {
    free(recv_buffer);
    return -1;
  }
  return -1;
}
static int sending(int ServerSocket, double *speed, int port, char *ip, int packets2send)
{
  int size = buff_size * packets2send;
  int sent = 0;
  long bytes = 0;
  char cont[] = "HTTP/1.1 100 Continue";
  //printf("Port: %d\n",port);
  //printf("IP: %s\n",ip);
  //struct timeval stop, start;
  struct timespec start, stop;
  dprintf(ServerSocket, "POST /process.php HTTP/1.1\r\nHost: %s:%d\r\n"
                        "User-Agent: VIPTest\r\nAccept: */*\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n"
                        "Content-Type: multipart/form-data;\r\nExpect: 100-continue\r\n\r\n",
          ip, port, size);
  char *recv_buffer = malloc(sizeof(char) * buff_size);
  if (recv_buffer == NULL)
  {
    return -1;
  }
  recv(ServerSocket, recv_buffer, buff_size, 0);
  if (strncmp(recv_buffer, cont, strlen(cont)) != 0)
  {
    free(recv_buffer);
    fprintf(stderr, "%.12s received\n", recv_buffer);
    return -1;
  }
  else
  {
    //fprintf(stderr,"HTTP/1.1 100 Continue Received\n");
  }
  //printf("%s",recv_buffer);
  unsigned char *send_buffer = malloc(buff_size * sizeof(unsigned char));
  if (send_buffer == NULL)
  {
    free(recv_buffer);
    return -1;
  }
  srand(time(0));
  for (int i = 0; i < buff_size; i++)
  {
    send_buffer[i] = rand() % 255;
  }
  //gettimeofday(&start, NULL);
  clock_gettime(CLOCK_REALTIME, &start);

  while (1)
  {
    sent = send(ServerSocket, send_buffer, buff_size, 0);
    //printf("sent: %d\n",sent);
    //printf("bytes: %ld\n",bytes);
    bytes = bytes + sent;
    if (bytes >= size)
    {
      break;
    }
  }
  recv(ServerSocket, recv_buffer, buff_size, 0);
  clock_gettime(CLOCK_REALTIME, &stop);
  char response[] = "HTTP/1.1 200 OK\r\n";
  //printf("\n%.13s",recv_buffer);
  //printf("\nbytes: %ld,  size: %d\n",bytes,size);
  if ((strncmp(recv_buffer, response, strlen(response)) == 0))
  {
    //printf("HTTP OK received\n");
    //double usecs = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
    double usecs = (stop.tv_sec - start.tv_sec) * 1e6 + (stop.tv_nsec - start.tv_nsec) / 1e3;
    *speed = 8 * bytes / usecs; //becomes Mb/s
  }
  else
  {
    printf("%s", recv_buffer);
    *speed = 0;
    free(recv_buffer);
    free(send_buffer);
    return -1;
  }

  free(recv_buffer);
  free(send_buffer);
  //shutdown(ServerSocket,1);
  return 0;
}
static void packetSize(int *packets, int *filesize, double DLSpeed, double UPSpeed)
{
  if (DLSpeed <= 12.5)
  {
    *filesize = 1;
  }
  else if (DLSpeed <= 25)
  {
    *filesize = 2;
  }
  else if (DLSpeed <= 37.5)
  {
    *filesize = 3;
  }
  else if (DLSpeed <= 50)
  {
    *filesize = 4;
  }
  else if (DLSpeed <= 75)
  {
    *filesize = 6;
  }
  else if (DLSpeed <= 100)
  {
    *filesize = 8;
  }
  else if (DLSpeed <= 150)
  {
    *filesize = 12;
  }
  else if (DLSpeed <= 200)
  {
    *filesize = 16;
  }
  else if (DLSpeed <= 300)
  {
    *filesize = 24;
  }
  else if (DLSpeed <= 400)
  {
    *filesize = 32;
  }
  else if (DLSpeed <= 600)
  {
    *filesize = 48;
  }
  else if (DLSpeed <= 800)
  {
    *filesize = 64;
  }
  else
  {
    *filesize = 128;
  }
  if (UPSpeed <= 3.125)
  {
    *packets = 50;
  }
  if (UPSpeed <= 6.25)
  {
    *packets = 100;
  }
  else if (UPSpeed <= 9.375)
  {
    *packets = 150;
  }
  else if (UPSpeed <= 12.5)
  {
    *packets = 200;
  }
  else if (UPSpeed <= 18.75)
  {
    *packets = 300;
  }
  else if (UPSpeed <= 25)
  {
    *packets = 400;
  }
  else if (UPSpeed <= 37.5)
  {
    *packets = 600;
  }
  else if (UPSpeed <= 50)
  {
    *packets = 800;
  }
  else if (UPSpeed <= 75)
  {
    *packets = 1000;
  }
  else if (UPSpeed <= 100)
  {
    *packets = 1200;
  }
  else if (UPSpeed <= 150)
  {
    *packets = 1800;
  }
  else if (UPSpeed <= 200)
  {
    *packets = 2400;
  }
  else if (UPSpeed <= 300)
  {
    *packets = 3600;
  }
  else if (UPSpeed <= 400)
  {
    *packets = 4800;
  }
  else if (UPSpeed <= 600)
  {
    *packets = 6400;
  }
  else if (UPSpeed <= 800)
  {
    *packets = 9600;
  }
  else
  {
    *packets = 12500;
  }
  return;
}