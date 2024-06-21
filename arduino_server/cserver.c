#include "cserver.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

CircularBuffer sensorBuffer1;
CircularBuffer sensorBuffer2;
enum currentMode currentMode = MODE_PASSIVE;

void initializeBuffer(CircularBuffer *cb, int bufferSize)
{
  cb->data = (int *)malloc(bufferSize * sizeof(int));
  cb->head = 0;
  cb->tail = 0;
  cb->count = 0;
  cb->bufferSize = bufferSize;
}

void addToBuffer(CircularBuffer *cb, int item)
{
  if (cb->count < cb->bufferSize)
  {
    cb->data[cb->tail] = item;
    cb->tail = (cb->tail + 1) % cb->bufferSize;
    cb->count++;
  }
  else
  {
    cb->data[cb->tail] = item;
    cb->tail = (cb->tail + 1) % cb->bufferSize;
    cb->head = (cb->head + 1) % cb->bufferSize;
  }
}

int pullFromBuffer(CircularBuffer *cb)
{
  int item = -1;
  if (cb->count > 0)
  {
    item = cb->data[cb->head];
    cb->head = (cb->head + 1) % cb->bufferSize;
    cb->count--;
  }
  return item;
}

double getBufferAverage(CircularBuffer* buffer) {
    if (buffer->count == 0) return -1.0; // Return -1.0 if the buffer is empty

    double sum = 0;
    for (int i = 0; i < buffer->count; i++) {
        sum += buffer->data[(buffer->head + i) % buffer->bufferSize];
    }
    return sum / buffer->count;
}

double getBufferStandardDeviation(CircularBuffer *buffer)
{
  if (buffer->count < 2)
    return 0.0; // Return 0.0 if fewer than 2 elements
  double mean = getBufferAverage(buffer);
  double sum = 0;
  for (int i = 0; i < buffer->count; i++)
  {
    double diff = buffer->data[(buffer->head + i) % buffer->bufferSize] - mean;
    sum += diff * diff;
  }
  return sqrt(sum / (buffer->count - 1));
}

int getBufferActual(CircularBuffer *buffer)
{
  if (buffer->count == 0)
    return -1;
  return buffer->data[(buffer->tail - 1 + buffer->bufferSize) % buffer->bufferSize];
}

void setBufferSize(CircularBuffer *buffer, int newSize)
{
  buffer->bufferSize = newSize;
  buffer->data = (int *)realloc(buffer->data, newSize * sizeof(int));
  buffer->head = 0;
  buffer->tail = buffer->count % newSize;
  if (buffer->count > newSize)
  {
    buffer->count = newSize;
  }
}

struct response handleRequest(struct stream stream)
{
  char line[256];
  char method[10];
  char uri[50];
  char protocol[10];
  int i = 0;
  while (stream.available() > 0 && i < sizeof(line) - 1)
  {
    char c = stream.read();
    if (c == '\n' || c == '\r')
    {
      break;
    }
    line[i++] = c;
  }
  line[i] = '\0';

  if (3 != sscanf(line, "%9s %49s %9s", method, uri, protocol))
  {
    return (struct response){.code = BAD_REQUEST_400};
  }

  if (strcmp(method, "PUT") == 0)
  {
    if (strcmp(uri, "/config/mode") == 0)
    {
      currentMode = MODE_PASSIVE; // Adjust according to body content
      return (struct response){.code = CREATED_201_PUT_MODE_PASSIVE};
    }
    else if (strcmp(uri, "/config/cbuffsize") == 0)
    {
      setBufferSize(&sensorBuffer1, 10); // example size
      setBufferSize(&sensorBuffer2, 10); // example size
      return (struct response){.code = CREATED_201_PUT_CBUFFSIZE};
    }
  }
  else if (strcmp(method, "DELETE") == 0)
  {
    if (strncmp(uri, "/sensors/1", 10) == 0)
    {
      initializeBuffer(&sensorBuffer1, sensorBuffer1.bufferSize);
      return (struct response){.code = CREATED_201_DELETE_MEASUREMENTS};
    }
    else if (strncmp(uri, "/sensors/2", 10) == 0)
    {
      initializeBuffer(&sensorBuffer2, sensorBuffer2.bufferSize);
      return (struct response){.code = CREATED_201_DELETE_MEASUREMENTS};
    }
  }
  else if (strcmp(method, "POST") == 0)
  {
    if (strncmp(uri, "/sensors/1", 10) == 0)
    {
      addToBuffer(&sensorBuffer1, 228); // example value
      return (struct response){.code = CREATED_201_POST_MEASUREMENT};
    }
    else if (strncmp(uri, "/sensors/2", 10) == 0)
    {
      addToBuffer(&sensorBuffer2, 51); // example value
      return (struct response){.code = CREATED_201_POST_MEASUREMENT};
    }
  }
  else if (strcmp(method, "GET") == 0)
  {
    if (strncmp(uri, "/sensors/1/avg", 14) == 0)
    {
      return (struct response){
          .code = OK_200_GET_AVG,
          .get_avg = getBufferAverage(&sensorBuffer1)};
    }
    else if (strncmp(uri, "/sensors/1/stdev", 16) == 0)
    {
      return (struct response){
          .code = OK_200_GET_STDEV,
          .get_stdev = getBufferStandardDeviation(&sensorBuffer1)};
    }
    else if (strncmp(uri, "/sensors/1/actual", 17) == 0)
    {
      return (struct response){
          .code = OK_200_GET_ACTUAL,
          .get_actual = getBufferActual(&sensorBuffer1)};
    }
    else if (strncmp(uri, "/sensors/2/avg", 14) == 0)
    {
      return (struct response){
          .code = OK_200_GET_AVG,
          .get_avg = getBufferAverage(&sensorBuffer2)};
    }
    else if (strncmp(uri, "/sensors/2/stdev", 16) == 0)
    {
      return (struct response){
          .code = OK_200_GET_STDEV,
          .get_stdev = getBufferStandardDeviation(&sensorBuffer2)};
    }
    else if (strncmp(uri, "/sensors/2/actual", 17) == 0)
    {
      return (struct response){
          .code = OK_200_GET_ACTUAL,
          .get_actual = getBufferActual(&sensorBuffer2)};
    }
  }

  return (struct response){.code = INTERNAL_SERVER_ERROR_500};
}
