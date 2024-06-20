#include "cserver.h"

// extern CircularBuffer buffer1;
// extern CircularBuffer buffer2;

// Example implementation for demonstration purposes.
struct response handleRequest(struct stream stream)
{
    char line[256]; // Buffer to store the line read from the streams
    char method[10];
    char uri[50];
    char body[256];
    char protocol[10];

    int i = 0;
    while (stream.available() > 0 && i < sizeof(line) - 1)
    {
        char c = stream.read();
        if (c == '\n' || c == '\r')
        {
            break; // End of line
        }
        line[i++] = c;
    }
    line[i] = '\0'; // Null-terminate the string

    // Try to read the method, URI, and protocol from the stream
    // This is a simplified approach and assumes well-formed HTTP requests
    if (3 != sscanf(line, "%9s %49s %9s", method, uri, protocol))
    {
        return (struct response){.code = BAD_REQUEST_400};
    }

    // Handle PUT request to set the mode
    if (strcmp(method, "PUT") == 0 && strcmp(uri, "/config/mode") == 0)
    {
        if (strcmp(uri, "/config/mode") == 0)
        {
            // Code to switch mode based on body content; assuming body parsing is handled
            currentMode = MODE_PASSIVE;            // or MODE_ACTIVE depending on the body content
            return (struct response){.code = 201}; // Adjust response code as needed
        }
        else if (strcmp(uri, "/config/cbuffsize") == 0)
        {
            // Code to adjust buffer size based on body content
            setBufferSize(&sensorBuffer1, 10); // or any other value based on the body content
            setBufferSize(&sensorBuffer2, 10); // or any other value based on the body content
            return (struct response){.code = 201};
        }

        // Handle DELETE request to clear sensor data
        if (strcmp(method, "DELETE") == 0 && strncmp(uri, "/sensors/", 9) == 0)
        {
            // Clear the sensor data based on sensor ID in URI
            return (struct response){.code = CREATED_201_DELETE_MEASUREMENTS};
        }

        // Handle POST request to add sensor data
        if (strcmp(method, "POST") == 0 && strncmp(uri, "/sensors/", 9) == 0)
        {
            // Additional logic to parse and store sensor data
            return (struct response){.code = CREATED_201_POST_MEASUREMENT};
        }

        // Handle GET requests for sensor data
        if (strcmp(method, "GET") == 0 && strncmp(uri, "/sensors/", 9) == 0)
        {

            int sensorId = 0;
            char action[20]; // For actions like "avg", "stdev", "actual"

            // Parse the URI to extract sensor ID and the action (e.g., /sensors/1/avg)
            sscanf(uri, "/sensors/%d/%19s", &sensorId, action);

            CircularBuffer *targetBuffer = NULL;
            if (sensorId == 1)
            {
                targetBuffer = &sensorBuffer1;
            }
            else if (sensorId == 2)
            {
                targetBuffer = &sensorBuffer2;
            }

            // Check if the target buffer was set and perform the action
            if (targetBuffer != NULL)
            {
                if (strstr(uri, "/avg"))
                {
                    return (struct response){.code = OK_200_GET_AVG, .get_avg = getBufferAverage(targetBuffer)};
                }
                if (strstr(uri, "/stdev"))
                {
                    return (struct response){.code = OK_200_GET_STDEV, .get_stdev = getBufferStandardDeviation(targetBuffer)};
                }
                if (strstr(uri, "/actual"))
                {
                    return (struct response){.code = OK_200_GET_ACTUAL, .get_actual = getBufferActual(targetBuffer)};
                }
            }
        }
    }

    // If no condition matches, return internal server error
    return (struct response){.code = INTERNAL_SERVER_ERROR_500};
}

// function to initialize the buffer
void initializeBuffer(CircularBuffer *cb)
{
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    cb->bufferSize = MAX_BUFFER_SIZE;
}

// function to add an element to the buffer
void addToBuffer(CircularBuffer *cb, int item)
{
    if (cb->count < cb->bufferSize)
    { // Check if buffer is not full
        cb->data[cb->tail] = item;
        cb->tail = (cb->tail + 1) % cb->bufferSize;
        cb->count++;
    }
    else
    {
        // Handle buffer full (e.g., overwrite oldest data or expand policy)
    }
}

// function to get (and remove) an element from the buffer
int pullFromBuffer(CircularBuffer *cb)
{
    int item = -1; // Default error value
    if (cb->count > 0)
    { // Check if buffer is not empty
        item = cb->data[cb->head];
        cb->head = (cb->head + 1) % cb->bufferSize;
        cb->count--;
    }
    return item;
}

double getBufferAverage(CircularBuffer *buffer)
{
    if (buffer->count == 0)
    {
        return 0.0; // Avoid division by zero; adjust based on your error handling
    }

    long sum = 0;
    for (int i = 0; i < buffer->count; i++)
    {
        int index = (buffer->head + i) % buffer->bufferSize;
        sum += buffer->data[index];
    }

    return sum / (double)buffer->count;
}

double getBufferStandardDeviation(CircularBuffer *buffer)
{
    if (buffer->count <= 1)
    {
        return 0.0; // Avoid division by zero or negative; adjust based on your error handling
    }

    double mean = getBufferAverage(buffer);
    double sum = 0.0;

    for (int i = 0; i < buffer->count; i++)
    {
        int index = (buffer->head + i) % buffer->bufferSize;
        sum += pow(buffer->data[index] - mean, 2);
    }

    return sqrt(sum / (buffer->count - 1));
}

int getBufferActual(CircularBuffer *buffer)
{
    if (buffer->count == 0)
    {
        return -1; // Indicates buffer is empty; adjust based on your error handling
    }

    // The tail points to the next empty slot, so subtract 1 to get the index of the last added item
    int actualIndex = (buffer->tail - 1 + buffer->bufferSize) % buffer->bufferSize;
    return buffer->data[actualIndex];
}

void setBufferSize(CircularBuffer *buffer, int newSize)
{
    if (newSize > 0)
    {
        buffer->bufferSize = newSize;
    }
}
