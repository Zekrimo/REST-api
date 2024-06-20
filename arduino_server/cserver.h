#ifndef cserver_h
#define cserver_h

#include "stream.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define MAX_BUFFER_SIZE 20

enum statuscode {
  INTERNAL_SERVER_ERROR_500, // failed to malloc cbuffers
  BAD_REQUEST_400,           // bad request
  NOT_FOUND_404,             // request target not found
  OK_200_GET_AVG,            // get mean
  OK_200_GET_STDEV,          // get standard deviation
  OK_200_GET_ACTUAL,         // empty cbuffer, get its mean
  CREATED_201_PUT_MODE_ACTIVE,    // start reading sensors
  CREATED_201_PUT_MODE_PASSIVE,   // stop reading sensors
  CREATED_201_PUT_CBUFFSIZE,      // send new cbuffer size
  CREATED_201_POST_MEASUREMENT,   // send a measurement
  CREATED_201_DELETE_MEASUREMENTS // clear collected data
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

// buffer for sensor date
typedef struct {
  int data[MAX_BUFFER_SIZE];
  int head;
  int tail;
  int count;
  int bufferSize;
} CircularBuffer;

// CircularBuffer sensorBuffer1;

// CircularBuffer sensorBuffer2;

// int bufferSize = 10;

// currentMode = MODE_PASSIVE;

void initializeBuffer(CircularBuffer* cb);

void addToBuffer(CircularBuffer* cb, int item);

int pullFromBuffer(CircularBuffer* cb);

double getBufferAverage(CircularBuffer* buffer);

double getBufferStandardDeviation(CircularBuffer* buffer);

int getBufferActual(CircularBuffer* buffer);

void setBufferSize(CircularBuffer* buffer, int newSize);

struct response handleRequest(struct stream);

#endif
