#include <Ethernet.h>

extern "C"
{
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
int clientAvailable()
{
  return httpClient.connected() && httpClient.available();
}
char clientRead() { return httpClient.read(); }
char clientPeek() { return httpClient.peek(); }

void setup()
{
  Serial.begin(9600);

  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

void loop()
{
  httpClient = server.available();
  if (httpClient)
  {
    Serial.println("new client");

    struct stream stream = {clientAvailable, clientPeek,
                            clientRead};
    struct response response = handleRequest(stream);

    if (sensor1BufferFull)
    {
      toggleLed(ledPinRed1, true);
    }
    else
    {
      toggleLed(ledPinRed1, false);
    }

    if (sensor2BufferFull)
    {
      toggleLed(ledPinRed2, true);
    }
    else
    {
      toggleLed(ledPinRed2, false);
    }

    if (recievedRequest)
    {
      timer1 = lastMillis;
      toggleLed(ledPinGreen, true);
      recievedRequest = false;
    } else if (lastMillis - timer1 > 500)
    {
      toggleLed(ledPinGreen, false);
    }

    if (response.code == INTERNAL_SERVER_ERROR_500)
    {
      httpClient.println(
          F("HTTP/1.1 500 Internal Server Error"));
      httpClient.println();
    }
    else
    {
      httpClient.println(
          F("HTTP/1.1 500 Internal Server Error"));
      httpClient.println();
    }

    Serial.print("response code: ");
    Serial.println(response.code);

    delay(1);
    lastMillis++;
    httpClient.stop(); // close connection
    Serial.println("client disconnected");
  }
}

void toggleLed(int pin, bool state)
{
  digitalWrite(pin, state ? HIGH : LOW);
}
