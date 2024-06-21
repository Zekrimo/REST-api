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
  printf("Buffer initialized with size: %d\n", bufferSize);
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
  printf("Added %d to buffer. Head: %d, Tail: %d, Count: %d\n", item, cb->head, cb->tail, cb->count);
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
  printf("Pulled %d from buffer. Head: %d, Tail: %d, Count: %d\n", item, cb->head, cb->tail, cb->count);
  return item;
}

double getBufferAverage(CircularBuffer *buffer)
{
  if (buffer->count == 0)
  {
    printf("Buffer is empty, returning -1.0\n");
    return -1.0; // Return -1.0 if the buffer is empty
  }

  double sum = 0;
  for (int i = 0; i < buffer->count; i++)
  {
    int value = buffer->data[(buffer->head + i) % buffer->bufferSize];
    sum += value;
    printf("Adding value %d to sum, current sum: %f\n", value, sum);
  }
  double avg = sum / buffer->count;
  printf("Calculated average: %f\n", avg);
  return avg;
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
    printf("Diff squared for value %d: %f, current sum: %f\n", buffer->data[(buffer->head + i) % buffer->bufferSize], diff * diff, sum);
  }
  double stdev = sqrt(sum / (buffer->count - 1));
  printf("Calculated standard deviation: %f\n", stdev);
  return stdev;
}

int getBufferActual(CircularBuffer *buffer)
{
  if (buffer->count == 0)
    return -1;
  int actual = buffer->data[(buffer->tail - 1 + buffer->bufferSize) % buffer->bufferSize];
  printf("Retrieved actual value from buffer: %d\n", actual);
  return actual;
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
  printf("Buffer size set to: %d. Head: %d, Tail: %d, Count: %d\n", newSize, buffer->head, buffer->tail, buffer->count);
}

struct response handleRequest(struct stream stream)
{
  char line[256];
  char method[10];
  char uri[50];
  char protocol[10];
  char body[256] = {0};
  int contentLength = 0;
  int i = 0;

  ////read stream data

  // read request line
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
  i = 0;

  printf(" !ALERT! Received request line: %s\n", line);

  // handle data
  printf("Received request line: %s\n", line);

  if (3 != sscanf(line, "%9s %49s %9s", method, uri, protocol))
  {
    printf("Invalid request format\n");
    return (struct response){.code = BAD_REQUEST_400};
  }

  printf("Method: %s, URI: %s, Protocol: %s\n", method, uri, protocol);

  // read body
  //  Read headers to find Content-Length
  while (stream.available() > 0)
  {
    i = 0;
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

    if (sscanf(line, "Content-Length: %d", &contentLength) == 1)
    {
      printf("Content-Length: %d\n", contentLength);
      continue;
    }

    // End of headers
    if (line[0] == '\0')
    {
      break;
    }
  }

  // Read the body
  i = 0;
  while (stream.available() > 0 && i < contentLength)
  {
    body[i++] = stream.read();
  }
  body[i] = '\0';
  int value = atoi(body);

  printf("Body value: %d\n", value);

  if (strcmp(method, "PUT") == 0)
  {
    if (strcmp(uri, "/config/mode") == 0)
    {
      currentMode = MODE_PASSIVE; // Adjust according to body content
      printf("Set mode to passive\n");
      return (struct response){.code = CREATED_201_PUT_MODE_PASSIVE};
    }
    else if (strcmp(uri, "/config/cbuffsize") == 0)
    {
      setBufferSize(&sensorBuffer1, 10); // example size
      setBufferSize(&sensorBuffer2, 10); // example size
      printf("Set circular buffer sizes\n");
      return (struct response){.code = CREATED_201_PUT_CBUFFSIZE};
    }
  }
  else if (strcmp(method, "DELETE") == 0)
  {
    if (strncmp(uri, "/sensors/1", 10) == 0)
    {
      initializeBuffer(&sensorBuffer1, sensorBuffer1.bufferSize);
      printf("Deleted measurements for sensor 1\n");
      return (struct response){.code = CREATED_201_DELETE_MEASUREMENTS};
    }
    else if (strncmp(uri, "/sensors/2", 10) == 0)
    {
      initializeBuffer(&sensorBuffer2, sensorBuffer2.bufferSize);
      printf("Deleted measurements for sensor 2\n");
      return (struct response){.code = CREATED_201_DELETE_MEASUREMENTS};
    }
  }
  else if (strcmp(method, "POST") == 0)
  {

    if (strncmp(uri, "/sensors/1", 10) == 0)
    {
      addToBuffer(&sensorBuffer1, 228); // example value
      printf("Posted measurement 228 to sensor 1\n");
      return (struct response){.code = CREATED_201_POST_MEASUREMENT};
    }
    else if (strncmp(uri, "/sensors/2", 10) == 0)
    {
      addToBuffer(&sensorBuffer2, 51); // example value
      printf("Posted measurement 51 to sensor 2\n");
      return (struct response){.code = CREATED_201_POST_MEASUREMENT};
    }
  }
  else if (strcmp(method, "GET") == 0)
  {
    if (strncmp(uri, "/sensors/1/avg", 14) == 0)
    {
      double avg = getBufferAverage(&sensorBuffer1);
      printf("GET /sensors/1/avg: %f\n", avg);
      return (struct response){
          .code = OK_200_GET_AVG,
          .get_avg = avg};
    }
    else if (strncmp(uri, "/sensors/1/stdev", 16) == 0)
    {
      double stdev = getBufferStandardDeviation(&sensorBuffer1);
      printf("GET /sensors/1/stdev: %f\n", stdev);
      return (struct response){
          .code = OK_200_GET_STDEV,
          .get_stdev = stdev};
    }
    else if (strncmp(uri, "/sensors/1/actual", 17) == 0)
    {
      int actual = getBufferActual(&sensorBuffer1);
      printf("GET /sensors/1/actual: %d\n", actual);
      return (struct response){
          .code = OK_200_GET_ACTUAL,
          .get_actual = actual};
    }
    else if (strncmp(uri, "/sensors/2/avg", 14) == 0)
    {
      double avg = getBufferAverage(&sensorBuffer2);
      printf("GET /sensors/2/avg: %f\n", avg);
      return (struct response){
          .code = OK_200_GET_AVG,
          .get_avg = avg};
    }
    else if (strncmp(uri, "/sensors/2/stdev", 16) == 0)
    {
      double stdev = getBufferStandardDeviation(&sensorBuffer2);
      printf("GET /sensors/2/stdev: %f\n", stdev);
      return (struct response){
          .code = OK_200_GET_STDEV,
          .get_stdev = stdev};
    }
    else if (strncmp(uri, "/sensors/2/actual", 17) == 0)
    {
      int actual = getBufferActual(&sensorBuffer2);
      printf("GET /sensors/2/actual: %d\n", actual);
      return (struct response){
          .code = OK_200_GET_ACTUAL,
          .get_actual = actual};
    }
  }

  printf("Returning 500 Internal Server Error\n");
  return (struct response){.code = INTERNAL_SERVER_ERROR_500};
}
