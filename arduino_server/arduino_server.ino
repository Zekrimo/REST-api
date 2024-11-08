#include <Ethernet.h>

extern "C" {
#include "cserver.h"
extern bool sensor1BufferFull;
extern bool sensor2BufferFull;
extern bool recievedRequest;
extern bool bufferInitError;
extern enum currentMode currentMode;
extern CircularBuffer sensorBuffer1;
extern CircularBuffer sensorBuffer2;
}

const int ledPinRed1 = 2;
const int ledPinRed2 = 4;
const int ledPinGreen = 6;
const int ledPinYellow = 8;

long long unsigned int lastMillis = 0;
long long unsigned int timer1 = 0;
long long unsigned int timer2 = 0;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 21);
EthernetServer server(80);
EthernetClient httpClient;

// make httpClient methods available as ordinary functions
int clientAvailable() {
  return httpClient.connected() && httpClient.available();
}
char clientRead() { return httpClient.read(); }
char clientPeek() { return httpClient.peek(); }

void toggleLed(int pin, bool state) {
  digitalWrite(pin, state ? HIGH : LOW);
}

const byte interruptPin = 10;
volatile byte state = LOW;

void sendResponse(const char* status, const String& body) {
  httpClient.println(String("HTTP/1.0 ") + status);
  httpClient.println(F("Content-Type: text/plain"));
  httpClient.print(F("Content-Length: "));
  httpClient.println(body.length());
  httpClient.println();
  httpClient.print(body);
}

void setup() {
  Serial.begin(9600);

  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);

  pinMode(ledPinRed1, OUTPUT);
  pinMode(ledPinRed2, OUTPUT);
  pinMode(ledPinGreen, OUTPUT);
  pinMode(ledPinYellow, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(interruptPin),
                  reset, CHANGE);
}

void loop() {
  httpClient = server.available();
  if (httpClient) {
    Serial.println("new client");

    char buffer[256] = {0};
    int bufferIndex = 0;

    struct stream stream = {clientAvailable, clientRead,
                            clientPeek};

    struct response response = handleRequest(stream);

    if (sensor1BufferFull) {
      toggleLed(ledPinRed1, true);
    } else {
      toggleLed(ledPinRed1, false);
    }

    if (sensor2BufferFull) {
      toggleLed(ledPinRed2, true);
    } else {
      toggleLed(ledPinRed2, false);
    }

    if (recievedRequest) {
      timer1 = lastMillis;
      toggleLed(ledPinGreen, true);
      recievedRequest = false;
    } else if (lastMillis - timer1 > 50) {
      toggleLed(ledPinGreen, false);
    }
    // Serial.println(lastMillis - timer1 > 50);

    if (response.code == INTERNAL_SERVER_ERROR_500) {
      httpClient.println(
          F("HTTP/1.1 500 Internal Server Error"));
      httpClient.println();
    } else {

      double val;
      // Directly sending response based on the status code
      switch (response.code) {
      case OK_200_GET_AVG:
        sendResponse("200 OK", String(response.get_avg));
        val = response.get_avg;
        break;
      case OK_200_GET_STDEV:
        sendResponse("200 OK", String(response.get_stdev));
        val = response.get_stdev;
        break;
      case OK_200_GET_ACTUAL:
        sendResponse("200 OK", String(response.get_actual));
        val = response.get_actual;
        break;
      case CREATED_201_PUT_MODE_ACTIVE:
        sendResponse("201 Created", "");
        break;
      case CREATED_201_PUT_MODE_PASSIVE:
        sendResponse("201", "");
        break;
      case CREATED_201_PUT_CBUFFSIZE:
        sendResponse("201 Created", "");
        break;
      case CREATED_201_POST_MEASUREMENT:
        sendResponse("201 Created", "");
        break;
      case CREATED_201_DELETE_MEASUREMENTS:
        sendResponse("201 Created", "");
        break;
      case BAD_REQUEST_400:
        sendResponse("400 Bad Request", "");
        break;
      case NOT_FOUND_404:
        sendResponse("404 Not Found", "");
        break;
      case INTERNAL_SERVER_ERROR_500:
      default:
        sendResponse(
            "DEFAULT MESSAGE 500 Internal Server Error",
            "");
        break;
      }

      Serial.print("response code: ");
      Serial.println(response.code);
      Serial.println(val);

      delay(1);
      lastMillis++;
      httpClient.stop(); // close connection
      Serial.println("client disconnected");
    }
  }

  readValue();
}

void readValue() {
  switch (currentMode) {
  case 1:
    int randomValue1 = analogRead(A0);
    int randomValue2 = analogRead(A1);
    Serial.print("Random Value from A0: ");
    Serial.println(randomValue1);
    Serial.print("Random Value from A1: ");
    Serial.println(randomValue2);

    // Add the random value to the buffers
    addToBuffer(&sensorBuffer1, randomValue1, 1);
    addToBuffer(&sensorBuffer2, randomValue2, 2);
    break;

  case 2:
    // Do nothing
    // Serial.println("Mode is PASSIVE. No data read.");
    break;

  default:
    // Handle unexpected modes if necessary
    // Serial.println("Unknown mode. No action taken.");
    break;
  }
}
