#include "SpeedTest.h"
#define SECS 10800 //seconds in 3 hours
//argv[1] = IPv4 Address
//argv[2] = Port number to be used
static int sendSpeed(char *IPv4, char *Port);
int getRandomInt(int low, int high, unsigned int* randval);
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Not enough arguments\n Enter IP address and Port Number to connect to. \n Example: 10.0.0.1 9009\n");
        exit(1);
    }
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    printf("Current GMT time and date: %s", asctime(timeinfo));
    unsigned int randval = 0;
    if (!getRandomInt(0, SECS, &randval))
    {
        randval = rand() % SECS;
    }
    if (timeinfo->tm_hour >= 19 && timeinfo->tm_hour <= 22)
    {
        sleep(randval);
        sendSpeed(argv[1], argv[2]);
    }
    sendSpeed(argv[1], argv[2]);
    //sleep(300);
    return 0;
}
static int sendSpeed(char *IPv4, char *Port)
{
    double *DLSpeedArray = malloc(sizeof(double) * DATAPOINTS);
    if (DLSpeedArray == NULL)
    {
        return -1;
    }
    double *UPSpeedArray = malloc(sizeof(double) * DATAPOINTS);
    if (UPSpeedArray == NULL)
    {
        free(DLSpeedArray);
        return -1;
    }
    int mode = speedTest(IPv4, Port, DLSpeedArray, UPSpeedArray, FULLTEST);
    if (mode == FULLTEST)
    {
        //fprintf(stderr,"Average Download Speed: %.3lf Mbit/s\n",DLSpeed);
        //fprintf(stderr,"Average Upload Speed: %.3lf Mbit/s\n",UPSpeed);
        //fprintf(fp,"%.3lf,%.3lf,\n",DLSpeed,UPSpeed);
        for (int i = 0; i < DATAPOINTS; i++)
        {
            fprintf(stderr, "Download[%d]: %lf\nUpload[%d]: %lf\n\n", i, DLSpeedArray[i], i, UPSpeedArray[i]);
        }
    }
    else if (mode == DOWNLOAD)
    {
        //fprintf(stderr,"Average Download Speed: %.3lf Mbit/s\n",DLSpeed);
        for (int i = 0; i < DATAPOINTS; i++)
        {
            fprintf(stderr, "%.3lf,N/A,\n", DLSpeedArray[i]);
        }
    }
    else if (mode == UPLOAD)
    {
        //fprintf(stderr,"Average Upload Speed: %.3lf Mbit/s\n",UPSpeed);
        for (int i = 0; i < DATAPOINTS; i++)
        {
            fprintf(stderr, "N/A,%.3lf,\n", UPSpeedArray[i]);
        }
    }
    else if (mode == -1)
    {
        return -1;
    }
    free(DLSpeedArray);
    free(UPSpeedArray);
    return 0;
}
int getRandomInt(int low, int high, unsigned int* randval)
{
    FILE *f;
    f = fopen("/dev/urandom", "r");
    if (f == NULL)
    {
        return -1;
    }
    if (fread(randval, sizeof(randval), 1, f))
    {
        *randval = *randval % (high - low);
        fclose(f);
        return 1;
    }
    fclose(f);
    return -1;
}