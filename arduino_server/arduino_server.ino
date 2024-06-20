#include <Ethernet.h>

extern "C" {
#include "cserver.h"
}

// ethernet shield mac address
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(LOCALHOST);
EthernetServer server(80);
EthernetClient httpClient;

// make httpClient methods available as ordinary functions
int clientAvailable() {
  return httpClient.connected() && httpClient.available();
}
char clientRead() { return httpClient.read(); }
char clientPeek() { return httpClient.peek(); }

void setup() {
  Serial.begin(9600);

  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  // initialize circular buffers
  initializeBuffer(&buffer1);
  initializeBuffer(&buffer2);
}

void loop() {
  // handle sensors
  switch (mode) {
  case MODE_ACTIVE:
    /* code to read sensors */
    break;
  case MODE_PASSIVE:
    /* code to not read sensors */
    break;
  default:
    break;
  }

  // listen for incoming clients
  httpClient = server.available();
  if (httpClient) {
    Serial.println("new client");

    struct stream stream = {clientAvailable, clientPeek,
                            clientRead};
    struct response response = handleRequest(stream);

    if (response.code == INTERNAL_SERVER_ERROR_500) {
      httpClient.println(
          F("HTTP/1.1 500 Internal Server Error"));
      httpClient.println();
    } else if (response.code == NOT_FOUND_404) {
      httpClient.println(F("HTTP/1.1 404 Not Found"));
      httpClient.println();
    } else if (response.code == BAD_REQUEST_400) {
      httpClient.println(F("HTTP/1.1 400 Bad Request"));
      httpClient.println();
    } else if (response.code == OK_200) {
      httpClient.println(F("HTTP/1.1 200 OK"));
      httpClient.println(F("Content-Type: text/html"));
      httpClient.println();
      httpClient.println(response.body);
    } else if (response.code == CREATED_201) {
      httpClient.println(F("HTTP/1.1 201 Created"));
      httpClient.println();
    } else {
      httpClient.println(
          F("HTTP/1.1 500 Internal Server Error"));
      httpClient.println();
    }

    Serial.print("response code: ");
    Serial.println(response.code);

    delay(1);
    httpClient.stop(); // close connection
    Serial.println("client disconnected");
  }
}
