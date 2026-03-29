/*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.
  Files are served from a LittleFS image flashed to the spiffs partition.

  Steps:
  1. Put files in WiFiAccessPoint/data/
  2. Run: make fs-flash   (builds and flashes the LittleFS image)
  3. Run: make && make program  (compile and flash the sketch)
  4. Connect to the access point "yourAP"
  5. Browse to http://192.168.4.1/

  Special control endpoints (no file needed):
    GET /H  — turn LED on
    GET /L  — turn LED off

  Source: https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/tree/master/libraries/WiFi/examples/WiFiAccessPoint
*/

#include <WiFi.h>
#include <WiFiAP.h>
#include <LittleFS.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

const char *ssid     = "yourAP";
const char *password = "yourPassword";

WiFiServer server(80);

// Return a MIME type string based on file extension
String mimeType(const String &path) {
  if (path.endsWith(".html") || path.endsWith(".htm")) return "text/html";
  if (path.endsWith(".css"))  return "text/css";
  if (path.endsWith(".js"))   return "application/javascript";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".png"))  return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".ico"))  return "image/x-icon";
  if (path.endsWith(".svg"))  return "image/svg+xml";
  if (path.endsWith(".txt"))  return "text/plain";
  return "application/octet-stream";
}

// Send a file from LittleFS to the client, returns true on success
bool serveFile(WiFiClient &client, const String &path) {
  String filePath = (path == "/") ? "/index.html" : path;
  if (!LittleFS.exists(filePath)) {
    return false;
  }
  File file = LittleFS.open(filePath, "r");
  if (!file) {
    return false;
  }
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: ");
  client.println(mimeType(filePath));
  client.print("Content-Length: ");
  client.println(file.size());
  client.println("Connection: close");
  client.println();
  // Stream file in chunks
  uint8_t buf[512];
  while (file.available()) {
    int len = file.read(buf, sizeof(buf));
    client.write(buf, len);
  }
  file.close();
  return true;
}

void send404(WiFiClient &client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("404 Not Found");
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println();

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed — did you run make fs-flash?");
    while (1);
  }
  Serial.println("LittleFS mounted.");

  Serial.println("Configuring access point...");
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while (1);
  }
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
  Serial.println("Server started");
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("New Client.");
  String requestLine = "";
  String currentLine = "";
  bool firstLine = true;

  while (client.connected()) {
    if (!client.available()) continue;

    char c = client.read();
    if (c == '\n') {
      if (firstLine) {
        requestLine = currentLine;  // e.g. "GET /index.html HTTP/1.1"
        firstLine = false;
      }
      if (currentLine.length() == 0) {
        // Blank line = end of request headers, time to respond

        // Parse path from request line: "GET /path HTTP/1.1"
        String path = "/";
        int pathStart = requestLine.indexOf(' ');
        int pathEnd   = requestLine.indexOf(' ', pathStart + 1);
        if (pathStart >= 0 && pathEnd > pathStart) {
          path = requestLine.substring(pathStart + 1, pathEnd);
        }
        Serial.print("Request: ");
        Serial.println(path);

        // LED control endpoints
        if (path == "/H") {
          digitalWrite(LED_BUILTIN, HIGH);
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("LED ON");
        } else if (path == "/L") {
          digitalWrite(LED_BUILTIN, LOW);
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("LED OFF");
        } else {
          if (!serveFile(client, path)) {
            send404(client);
          }
        }
        break;
      }
      currentLine = "";
    } else if (c != '\r') {
      currentLine += c;
    }
  }

  client.stop();
  Serial.println("Client Disconnected.");
}
