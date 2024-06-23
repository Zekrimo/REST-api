#include "cserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

CircularBuffer sensorBuffer1;
CircularBuffer sensorBuffer2;
enum currentMode currentMode = MODE_PASSIVE;

void initializeBuffer(CircularBuffer *cb, int bufferSize)
{
  if (bufferSize <= 0)
  {
    printf("Invalid buffer size: %d\n", bufferSize);
    return;
  }
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
    return -1.0; // Return -1.0 if the buffer is empty
  }

  double sum = 0;
  for (int i = 0; i < buffer->count; i++)
  {
    int value = buffer->data[(buffer->head + i) % buffer->bufferSize];
    sum += value;
  }
  return sum / buffer->count;
}

double getBufferStandardDeviation(CircularBuffer *buffer)
{
  if (buffer->count < 2)
  {
    return 0.0; // Return 0.0 if fewer than 2 elements
  }

  double mean = getBufferAverage(buffer);
  double sum = 0;
  for (int i = 0; i < buffer->count; i++)
  {
    double value = buffer->data[(buffer->head + i) % buffer->bufferSize];
    double diff = value - mean;
    sum += diff * diff;
  }
  double variance = sum / buffer->count;
  double stdev = sqrt(variance);

  printf("Calculated average: %f\n", mean);
  printf("Sum of squared differences: %f\n", sum);
  printf("Calculated variance: %f\n", variance);
  printf("Calculated standard deviation: %f\n", stdev);

  return stdev; // Return the calculated standard deviation without rounding
}

double getBufferActual(CircularBuffer *buffer)
{
  if (buffer->count == 0)
  {
    return -1.0; // Indicate the buffer is empty
  }
  double mean = getBufferAverage(buffer);
  printf("Calculated mean (actual) value from buffer: %.6f\n", mean); // Increased precision

  // Print all values in buffer for debugging
  printf("Buffer values: ");
  for (int i = 0; i < buffer->count; i++)
  {
    int value = buffer->data[(buffer->head + i) % buffer->bufferSize];
    printf("%d ", value);
  }
  printf("\n");
  return mean;
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
  printf("\n! Handling request : START\n");

  char buffer[1024] = {0};
  int bufferIndex = 0;

  while (stream.available() > 0 && bufferIndex < sizeof(buffer) - 1)
  {
    buffer[bufferIndex++] = stream.read();
  }
  buffer[bufferIndex] = '\0';

  // Print the received request
  // printf("Received request: %s\n", buffer);

  // Read the request line
  char *line = strtok(buffer, "\r\n");
  char method[10], uri[50], protocol[10];
  sscanf(line, "%s %s %s", method, uri, protocol);

  // Print data
  printf("Method: %s, URI: %s, Protocol: %s\n", method, uri, protocol);

  char *header = strtok(NULL, "\r\n\r\n");
  char *body = strtok(NULL, "\0");
  int contentLength = header ? atoi(header + 16) : 0;

  printf("Header: %s\n", header);
  printf("Content Length int value: %d\n", contentLength);

  if (strcmp(method, "PUT") == 0)
  {
    if (strcmp(uri, "/config/mode") == 0)
    {
      currentMode = (strcmp(body, "active") == 0) ? MODE_ACTIVE : MODE_PASSIVE;
      printf("Set mode to %s\n", (currentMode == MODE_ACTIVE) ? "active" : "passive");
      return (struct response){.code = CREATED_201_PUT_MODE_PASSIVE};
    }
    else if (strcmp(uri, "/config/cbuffsize") == 0)
    {
      int value = atoi(body); // Get the value from the body
      printf("Value from body: %d\n", value);

      setBufferSize(&sensorBuffer1, value); // Example size
      setBufferSize(&sensorBuffer2, value); // Example size
      printf("Set circular buffer size to %d\n", value);
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
    int value = atoi(body); // Get the value from the body
    printf("Value from body: %d\n", value);

    if (strncmp(uri, "/sensors/1", 10) == 0)
    {
      addToBuffer(&sensorBuffer1, value); // Use value from body
      printf("Posted measurement %d to sensor 1\n", value);
      return (struct response){.code = CREATED_201_POST_MEASUREMENT};
    }
    else if (strncmp(uri, "/sensors/2", 10) == 0)
    {
      addToBuffer(&sensorBuffer2, value); // Use value from body
      printf("Posted measurement %d to sensor 2\n", value);
      return (struct response){.code = CREATED_201_POST_MEASUREMENT};
    }
  }
  else if (strcmp(method, "GET") == 0)
  {
    if (strncmp(uri, "/sensors/1/avg", 14) == 0)
    {
      double avg = getBufferAverage(&sensorBuffer1);
      printf("GET /sensors/1/avg: %.6f\n", avg); // Increased precision
      return (struct response){.code = OK_200_GET_AVG, .get_avg = avg};
    }
    else if (strncmp(uri, "/sensors/1/stdev", 16) == 0)
    {
      double stdev = getBufferStandardDeviation(&sensorBuffer1);
      printf("GET /sensors/1/stdev: %.6f\n", stdev); // Increased precision
      return (struct response){.code = OK_200_GET_STDEV, .get_stdev = stdev};
    }
    else if (strncmp(uri, "/sensors/1/actual", 17) == 0)
    {
      double actual = getBufferActual(&sensorBuffer1);
      printf("GET /sensors/1/actual: %.6f\n", actual); // Increased precision
      return (struct response){.code = OK_200_GET_ACTUAL, .get_actual = actual};
    }
    else if (strncmp(uri, "/sensors/2/avg", 14) == 0)
    {
      double avg = getBufferAverage(&sensorBuffer2);
      printf("GET /sensors/2/avg: %.6f\n", avg); // Increased precision
      return (struct response){.code = OK_200_GET_AVG, .get_avg = avg};
    }
    else if (strncmp(uri, "/sensors/2/stdev", 16) == 0)
    {
      double stdev = getBufferStandardDeviation(&sensorBuffer2);
      printf("GET /sensors/2/stdev: %.6f\n", stdev); // Increased precision
      return (struct response){.code = OK_200_GET_STDEV, .get_stdev = stdev};
    }
    else if (strncmp(uri, "/sensors/2/actual", 17) == 0)
    {
      double actual = getBufferActual(&sensorBuffer2);
      printf("GET /sensors/2/actual: %.6f\n", actual); // Increased precision
      return (struct response){.code = OK_200_GET_ACTUAL, .get_actual = actual};
    }
  }

  printf("Returning 500 Internal Server Error\n");
  return (struct response){.code = INTERNAL_SERVER_ERROR_500};
}
