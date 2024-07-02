#include "cserver.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CircularBuffer sensorBuffer1;
CircularBuffer sensorBuffer2;
enum currentMode currentMode = MODE_PASSIVE;

long long int runningTotalSensor1 = 0;
long long int runningTotalSensor2 = 0;
long long int runningSquaredTotalSensor1 = 0;
long long int runningSquaredTotalSensor2 = 0;
int runningCountSensor1 = 0;
int runningCountSensor2 = 0;

bool sensor1BufferFull = false;
bool sensor2BufferFull = false;
bool recievedRequest = false;
bool bufferInitError = false;

void initializeBuffer(CircularBuffer* cb, int bufferSize) {
  if (bufferSize <= 0) {
    return;
  }
  cb->data = (int*)malloc(bufferSize * sizeof(int));
  cb->head = 0;
  cb->tail = 0;
  cb->count = 0;
  cb->bufferSize = bufferSize;

  sensor1BufferFull = false;
  sensor2BufferFull = false;
}

void reset() {
  initializeBuffer(&sensorBuffer1, sensorBuffer1.bufferSize);
  initializeBuffer(&sensorBuffer2, sensorBuffer1.bufferSize);
}

void addToBuffer(CircularBuffer* cb, long long int item,
                 int sensor) {

  //printf("Adding %lld to buffer\n", item);

  // Update running statistics
  if (sensor == 1) {
    runningTotalSensor1 += item;
    long long int val1 = item;
    long long int val2 = item;
    long long int result = (val1 * val2);
    runningSquaredTotalSensor1 += result;
    runningCountSensor1++;

  } else if (sensor == 2) {
    runningTotalSensor2 += item;
    long long int val1 = item;
    long long int val2 = item;
    long long int result = (val1 * val2);
    runningSquaredTotalSensor2 += result;
    runningCountSensor2++;
  }

  if (cb->count < cb->bufferSize) {
    cb->data[cb->tail] = item;
    cb->tail = (cb->tail + 1) % cb->bufferSize;
    cb->count++;
  } else {
    // Remove the old item from running statistics
    int oldItem = cb->data[cb->head];

    // Overwrite the oldest item in the buffer
    cb->data[cb->tail] = item;
    cb->tail = (cb->tail + 1) % cb->bufferSize;
    cb->head = (cb->head + 1) % cb->bufferSize;

    if (sensor == 1 && sensor1BufferFull == false) {
      sensor1BufferFull = true;
    } else if (sensor == 2 && sensor2BufferFull == false) {
      sensor2BufferFull = true;
    }
  }
}

int pullFromBuffer(CircularBuffer* cb) {
  int item = -1;
  if (cb->count > 0) {
    item = cb->data[cb->head];
    cb->head = (cb->head + 1) % cb->bufferSize;
    cb->count--;
  }

  return item;
}

double getRunningAverage(int sensor) {
  double total = 0;
  int count = 0;
  if (sensor == 1) {
    if (runningCountSensor1 > 0) {

      total = runningTotalSensor1;
      count = runningCountSensor1;

      double avg = total / count;

      return avg;
    }
  } else if (sensor == 2) {
    if (runningCountSensor2 > 0) {

      total = runningTotalSensor2;
      count = runningCountSensor2;
      double avg = total / count;

      return avg;
    }
  }
  return -1.0;
}

void resetRunningStatistics(int sensor) {
  if (sensor == 1) {
    runningTotalSensor1 = 0;
    runningSquaredTotalSensor1 = 0;
    runningCountSensor1 = 0;
  } else if (sensor == 2) {
    runningTotalSensor2 = 0;
    runningSquaredTotalSensor2 = 0;
    runningCountSensor2 = 0;
  }
}

double getRunningStandardDeviation(int sensor) {

  if (sensor == 1) {
    if (runningCountSensor1 == 0) {
      return -1;
    } else if (runningCountSensor1 < 2) {
      return 0.0;
    }

    double val = runningSquaredTotalSensor1 - runningTotalSensor1 * runningTotalSensor1 / runningCountSensor1;
    double result = val / runningCountSensor1;

    return sqrt(result);
  } else if (sensor == 2) {
    if (runningCountSensor2 == 0) {
      return -1;
    } else if (runningCountSensor2 < 2) {
      return 0.0;
    }
    double val = runningSquaredTotalSensor2 - runningTotalSensor2 * runningTotalSensor2 / runningCountSensor2;
    double result = val / runningCountSensor2;
    return sqrt(result);
  }
  return -1;
}

// function should return the average of the buffer content
double getBufferActual(CircularBuffer* buffer) {
  if (buffer->count == 0) {
    return -1.0;  // Return -1.0 if the buffer is empty
  }

  double sum = 0;
  for (int i = 0; i < buffer->count; i++) {
    int value = buffer->data[(buffer->head + i) % buffer->bufferSize];
    sum += value;
  }

  double average = sum / buffer->count;

  return average;
}

void setBufferSize(CircularBuffer* buffer, int newSize,
                   int sensor) {
  if (newSize <= 0) {
    return;
  }

  int* newData = (int*)malloc(newSize * sizeof(int));
  int count =
    buffer->count < newSize ? buffer->count : newSize;

  for (int i = 0; i < count; i++) {
    newData[i] =
      buffer
        ->data[(buffer->head + i) % buffer->bufferSize];
  }

  free(buffer->data);
  buffer->data = newData;
  buffer->bufferSize = newSize;
  buffer->count = count;
  buffer->head = 0;
  buffer->tail = count % newSize;

  // Reset running statistics if necessary
  if (sensor == 1) {
    resetRunningStatistics(1);
  } else if (sensor == 2) {
    resetRunningStatistics(2);
  }
}

struct response handleRequest(struct stream stream) {

  bool recievedRequest = true;
  // printf("\n! Handling request : START\n");
  struct response response;
  char buffer[256] = { 0 };
  int bufferIndex = 0;

  while (stream.available() > 0 && bufferIndex < sizeof(buffer) - 1) {
    buffer[bufferIndex++] = stream.read();
  }
  buffer[bufferIndex] = '\0';

  // Read the request line
  char* line = strtok(buffer, "\r\n");

  char* header = strtok(NULL, "\r\n\r\n");
  char* body = strtok(NULL, "\0");
  int contentLength = header ? atoi(header + 16) : 0;

  char* method = strtok(line, " ");
  char* uri = strtok(NULL, " ");
  char* protocol = strtok(NULL, " ");

  if (strcmp(method, "PUT") == 0) {
    if (strcmp(uri, "/config/mode") == 0) {
      currentMode =
        MODE_PASSIVE;  // Adjust according to body content
      return (struct response){
        .code = CREATED_201_PUT_MODE_PASSIVE
      };
    } else if (strcmp(uri, "/config/cbuffsize") == 0) {
      int newSize = atoi(body);
      setBufferSize(&sensorBuffer1, newSize,
                    1);  // example size
      setBufferSize(&sensorBuffer2, newSize,
                    2);  // example size
      return (struct response){
        .code = CREATED_201_PUT_CBUFFSIZE
      };
    }
  } else if (strcmp(method, "DELETE") == 0) {
    if (strncmp(uri, "/sensors/1", 10) == 0) {
      initializeBuffer(&sensorBuffer1,
                       sensorBuffer1.bufferSize);
      resetRunningStatistics(1);
      return (struct response){
        .code = CREATED_201_DELETE_MEASUREMENTS
      };
    } else if (strncmp(uri, "/sensors/2", 10) == 0) {
      initializeBuffer(&sensorBuffer2,
                       sensorBuffer2.bufferSize);
      resetRunningStatistics(2);
      response.code = CREATED_201_DELETE_MEASUREMENTS;
      return response;
    }
  } else if (strcmp(method, "POST") == 0) {
    int value = atoi(body);  // Get the value from the body

    if (strncmp(uri, "/sensors/1", 10) == 0) {
      addToBuffer(&sensorBuffer1, value,
                  1);  // Use value from body
      return (struct response){
        .code = CREATED_201_POST_MEASUREMENT
      };
    } else if (strncmp(uri, "/sensors/2", 10) == 0) {
      addToBuffer(&sensorBuffer2, value,
                  2);  // Use value from body
      return (struct response){
        .code = CREATED_201_POST_MEASUREMENT
      };
    } else {
      return (struct response){ .code = NOT_FOUND_404 };
    }
  } else if (strcmp(method, "GET") == 0) {
    if (strncmp(uri, "/sensors/1/avg", 14) == 0) {
      double avg = getRunningAverage(1);
      // printf("GET /sensors/1/avg: %f\n", avg);
      return (struct response){ .code = OK_200_GET_AVG,
                                .get_avg = avg };
    } else if (strncmp(uri, "/sensors/1/stdev", 16) == 0) {
      double stdev = getRunningStandardDeviation(1);
      // printf("GET /sensors/1/stdev: %f\n", stdev);
      return (struct response){ .code = OK_200_GET_STDEV,
                                .get_stdev = stdev };
    } else if (strncmp(uri, "/sensors/1/actual", 17) == 0) {
      double avg = getBufferActual(&sensorBuffer1);
      initializeBuffer(&sensorBuffer1,
                       sensorBuffer1.bufferSize);
      // printf("GET /sensors/1/actual: %f\n", avg);
      return (struct response){ .code = OK_200_GET_ACTUAL,
                                .get_actual = avg };
    } else if (strncmp(uri, "/sensors/2/avg", 14) == 0) {
      double avg = getRunningAverage(2);
      // printf("GET /sensors/2/avg: %f\n", avg);
      return (struct response){ .code = OK_200_GET_AVG,
                                .get_avg = avg };
    } else if (strncmp(uri, "/sensors/2/stdev", 16) == 0) {
      double stdev = getRunningStandardDeviation(2);
      // printf("GET /sensors/2/stdev: %f\n", stdev);
      return (struct response){ .code = OK_200_GET_STDEV,
                                .get_stdev = stdev };
    } else if (strncmp(uri, "/sensors/2/actual", 17) == 0) {
      double avg = getBufferActual(&sensorBuffer2);
      initializeBuffer(&sensorBuffer2,
                       sensorBuffer2.bufferSize);
      // printf("GET /sensors/2/actual: %f\n", avg);
      return (struct response){ .code = OK_200_GET_ACTUAL,
                                .get_actual = avg };
    } else {
      return (struct response){ .code = NOT_FOUND_404 };
    }
  } else {

    // printf("Returning 500 Internal Server Error\n");
    return (struct response){ .code = BAD_REQUEST_400 };
  }
}