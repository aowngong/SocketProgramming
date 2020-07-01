#include "SpeedTest.h"
#define SECS 10800 //seconds in 3 hours
#define MAXLength 10000
//argv[1] = IPv4 Address
//argv[2] = Port number to be used
static int sendSpeed(char *IPv4, char *Port);
int getRandomInt(int low, int high, unsigned int *randval);
static void catString(char *start, int *currentPos, char *str2Copy);
static int createJSON(double *DLSpeedArray, double *UPSpeedArray, char *JSON, int *JSONlength);
static void getMaxMinAvg(double *DLSpeedArray, double *max, double *min, double *average);
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
        for (int i = 0; i < DATAPOINTS; i++)
        {
            fprintf(stderr, "Download[%d]: %lf\nUpload[%d]: %lf\n\n", i, DLSpeedArray[i], i, UPSpeedArray[i]);
        }
    }
    else if (mode == DOWNLOAD)
    {
        for (int i = 0; i < DATAPOINTS; i++)
        {
            fprintf(stderr, "%.3lf,N/A,\n", DLSpeedArray[i]);
        }
    }
    else if (mode == UPLOAD)
    {
        for (int i = 0; i < DATAPOINTS; i++)
        {
            fprintf(stderr, "N/A,%.3lf,\n", UPSpeedArray[i]);
        }
    }
    else if (mode == -1)
    {
        return -1;
    }
    char *JSON = malloc(sizeof(char) * MAXLength);
    if (JSON == NULL)
    {
        return -1;
    }
    int length = createJSON(DLSpeedArray, UPSpeedArray, JSON, &length);
    free(JSON);
    free(DLSpeedArray);
    free(UPSpeedArray);
    return 0;
}
int getRandomInt(int low, int high, unsigned int *randval)
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

static int createJSON(double *DLSpeedArray, double *UPSpeedArray, char *JSON, int *JSONlength)
{

    FILE *fp = fopen("test.json", "w");
    int position = 0;
    catString(JSON, &position, "{\n");
    catString(JSON, &position, "\"SpeedTest\": {\n");
    catString(JSON, &position, "    \"Download\": {\n");
    catString(JSON, &position, "        \"Values\": [\n");
    if (DLSpeedArray == NULL || UPSpeedArray == NULL)
    {
        return -1;
    }
    char speedBuffer[50];
    for (int i = 0; i < DATAPOINTS; i++)
    {

        if (i != DATAPOINTS - 1)
        {
            snprintf(speedBuffer, 50, "            %0.3lf,\n", DLSpeedArray[i]);
            catString(JSON, &position, speedBuffer);
        }
        else
        {
            snprintf(speedBuffer, 50, "            %0.3lf\n        ],\n", DLSpeedArray[i]);
            catString(JSON, &position, speedBuffer);
        }
    }
    double DLMax = -99999999999999;
    double DLMin = 99999999999999;
    double DLAvg = 0;
    getMaxMinAvg(DLSpeedArray, &DLMax, &DLMin, &DLAvg);
    snprintf(speedBuffer, 50, "        \"Max\": %.3lf,\n", DLMax);
    catString(JSON, &position, speedBuffer);
    snprintf(speedBuffer, 50, "        \"Min\": %.3lf,\n", DLMin);
    catString(JSON, &position, speedBuffer);
    snprintf(speedBuffer, 50, "        \"Average\": %.3lf\n", DLAvg);
    catString(JSON, &position, speedBuffer);
    catString(JSON, &position, "      },\n");
    catString(JSON, &position, "    \"Upload\": {\n");
    catString(JSON, &position, "        \"Values\": [\n");
    for (int i = 0; i < DATAPOINTS; i++)
    {
        if (i != DATAPOINTS - 1)
        {
            snprintf(speedBuffer, 50, "            %0.3lf,\n", UPSpeedArray[i]);
            catString(JSON, &position, speedBuffer);
        }
        else
        {
            snprintf(speedBuffer, 50, "            %0.3lf\n        ],\n", UPSpeedArray[i]);
            catString(JSON, &position, speedBuffer);
        }
    }
    double UPMax = -99999999999999;
    double UPMin = 99999999999999;
    double UPAvg = 0;
    getMaxMinAvg(UPSpeedArray, &UPMax, &UPMin, &UPAvg);
    snprintf(speedBuffer, 50, "        \"Max\": %.3lf,\n", UPMax);
    catString(JSON, &position, speedBuffer);
    snprintf(speedBuffer, 50, "        \"Min\": %.3lf,\n", UPMin);
    catString(JSON, &position, speedBuffer);
    snprintf(speedBuffer, 50, "        \"Average\": %.3lf\n", UPAvg);
    catString(JSON, &position, speedBuffer);
    catString(JSON,&position,"      }\n   }\n}");
    fprintf(fp, "%s", JSON);
    fclose(fp);
    *JSONlength = position;
    printf("%d\n",*JSONlength);
    return 0;
}
static void catString(char *start, int *currentPos, char *str2Copy)
{
    char *ptr = &start[*currentPos];
    int i = 0;
    while (str2Copy[i] != '\0')
    {
        ptr[i] = str2Copy[i];
        i++;
    }
    ptr[i] = '\0';
    *currentPos = *currentPos + i;
    return;
}
static void getMaxMinAvg(double *DLSpeedArray, double *max, double *min, double *average)
{
    double curr = 0;
    double total = 0;
    for (int i = 0; i < DATAPOINTS; i++)
    {
        curr = DLSpeedArray[i];
        if (curr > *max)
        {
            *max = curr;
        }
        if (curr < *min)
        {
            *min = curr;
        }
        total = total + curr;
    }
    *average = total / DATAPOINTS;
    return;
}