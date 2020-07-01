#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <error.h>
#include <time.h>
#define FULLTEST 2
#define UPLOAD 1
#define DOWNLOAD 0
#define buff_size 8192
#define DATAPOINTS 8
int speedTest(char *IPv4, char *port, double *DLSpeedArray, double *UPSpeedArray, int mode);
