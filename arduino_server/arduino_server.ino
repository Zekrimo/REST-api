#include <Ethernet.h>

extern "C" {
#include "cserver.h"
extern bool sensor1BufferFull;
extern bool sensor2BufferFull;
extern bool recievedRequest;
extern bool bufferInitError;
}

const int ledPinRed1 = 13;
const int ledPinRed2 = 11;
const int ledPinGreen = 12;
const int ledPinYellow = 10;

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

void sendResponse(const char* status, const String& body) {
  httpClient.println(String("HTTP/1.0 ") + status);
  httpClient.println(F("Content-Type: text/plain"));
  httpClient.print(F("Content-Length: "));
  httpClient.println(body.length());
  httpClient.println();
  httpClient.print(body);
}

// allow to log msg in C code to print on serial monitor
void serialLog(const char* msg) { Serial.print(msg); }

void setup() {
  Serial.begin(9600);

  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  initLogger(serialLog);
}

void loop() {
  httpClient = server.available();
  if (httpClient) {
    Serial.println("new client");

    char buffer[256] = {0};
    int bufferIndex = 0;

    struct stream stream = {clientAvailable, clientRead,
                            clientPeek};

    // while (clientRead > 0 && bufferIndex < 255) {
    //   buffer[bufferIndex++] = stream.read();
    // }
    // buffer[bufferIndex] = '\0';
    // Serial.println(buffer);
    // Serial.println();

    struct response response = handleRequest(stream);

    // if (sensor1BufferFull) {
    //   toggleLed(ledPinRed1, true);
    // } else {
    //   toggleLed(ledPinRed1, false);
    // }

    // if (sensor2BufferFull) {
    //   toggleLed(ledPinRed2, true);
    // } else {
    //   toggleLed(ledPinRed2, false);
    // }

    // if (recievedRequest) {
    //   timer1 = lastMillis;
    //   toggleLed(ledPinGreen, true);
    //   recievedRequest = false;
    // } else if (lastMillis - timer1 > 500) {
    //   toggleLed(ledPinGreen, false);
    // }

    if (response.code == INTERNAL_SERVER_ERROR_500) {
      httpClient.println(
          F("HTTP/1.1 500 Internal Server Error"));
      httpClient.println();
    } else {

      // Directly sending response based on the status code
      switch (response.code) {
      case OK_200_GET_AVG:
        sendResponse("200 OK", String(response.get_avg));
        Serial.println(response.get_avg);
        break;
      case OK_200_GET_STDEV:
        sendResponse("200 OK", String(response.get_stdev));
        Serial.println(response.get_stdev);
        break;
      case OK_200_GET_ACTUAL:
        sendResponse("200 OK", String(response.get_actual));
         Serial.println(response.get_actual);
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

      delay(1);
      lastMillis++;
      httpClient.stop(); // close connection
      Serial.println("client disconnected");
    }
  }
}
