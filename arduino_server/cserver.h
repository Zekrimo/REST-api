#ifndef CSERVER_H
#define CSERVER_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER_SIZE 20

enum statuscode {
  INTERNAL_SERVER_ERROR_500,
  BAD_REQUEST_400,
  NOT_FOUND_404,
  OK_200_GET_AVG,
  OK_200_GET_STDEV,
  OK_200_GET_ACTUAL,
  CREATED_201_PUT_MODE_ACTIVE,
  CREATED_201_PUT_MODE_PASSIVE,
  CREATED_201_PUT_CBUFFSIZE,
  CREATED_201_POST_MEASUREMENT,
  CREATED_201_DELETE_MEASUREMENTS
};

struct response {
  enum statuscode code;
  union {
    double get_avg;
    double get_stdev;
    double get_actual;
  };
};

enum currentMode {
  MODE_ACTIVE = 1,
  MODE_PASSIVE = 2,
};

typedef struct {
  int* data;
  int head;
  int tail;
  int count;
  int bufferSize;
} CircularBuffer;

extern CircularBuffer sensorBuffer1;
extern CircularBuffer sensorBuffer2;
extern enum currentMode currentMode;

// extern double runningTotalSensor1 = 0;
// extern double runningTotalSensor2 = 0;
// extern double runningSquaredTotalSensor1 = 0;
// extern double runningSquaredTotalSensor2 = 0;
// extern int runningCountSensor1 = 0;
// extern int runningCountSensor2 = 0;

void initializeBuffer(CircularBuffer* cb, int bufferSize);
void addToBuffer(CircularBuffer* cb, int item, int sensor);
int pullFromBuffer(CircularBuffer* cb);
double getBufferAverage(CircularBuffer* buffer);
double getBufferStandardDeviation(CircularBuffer* buffer);
double getBufferActual(CircularBuffer* buffer);
void setBufferSize(CircularBuffer* buffer, int newSize,
                   int sensor);
double getRunningAverage(int sensor);
double getRunningStandardDeviation(int sensor);
void resetRunningStatistics(int sensor);

struct stream {
  int (*available)(void);
  char (*read)(void);
  char (*peek)(void);
};

struct response handleRequest(struct stream stream);

// void (*logfunction)(const char*);
// void initLogger(void (*func)(const char*));
// void logmsg(char* msg);

#endif
