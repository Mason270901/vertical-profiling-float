/*
  Vertical Profiling Float — web server with AJAX endpoints.
  Files are served from a LittleFS image flashed to the spiffs partition.

  Steps:
  1. Put files in varduino/data/
  2. Run: make fs-flash   (builds and flashes the LittleFS image)
  3. Run: make && make program  (compile and flash the sketch)
  4. Connect to the access point "yourAP"
  5. Browse to http://192.168.4.1/

  AJAX endpoints (return JSON, no redirect):
    GET /H     — turn LED on  → {"led":"on"}
    GET /L     — turn LED off → {"led":"off"}
    GET /data  — 44-element uint32 array → {"data":[...]}
*/

#include <WiFi.h>
#include <WiFiAP.h>
#include <LittleFS.h>

#include "syringe.hpp"

#define MOTOR_UP   48
#define MOTOR_DOWN 47
// motor driver board is labeled
// in 1, in 2, in 3, in 4
// nc  , nc  , blue, orange

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif


// how many seconds does it take to home the linear actuator
const int home_seconds = 38;


const char *ssid     = "RN02verticalprofile";
const char *password = "password1!";

WiFiServer server(80);

// ---------------------------------------------------------------------------
// Pre-populated 44-element array of 32-bit sensor readings
// Simulated depth/pressure profile (values in millibars × 10)
// ---------------------------------------------------------------------------
#define DATA_LEN 44
uint32_t sensorData[DATA_LEN];

typedef void (*Runnable)(unsigned long);

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

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

// Send a JSON response with CORS headers
void sendJson(WiFiClient &client, int code, const String &body) {
  String status = (code == 200) ? "200 OK" : "404 Not Found";
  client.println("HTTP/1.1 " + status);
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.print("Content-Length: ");
  client.println(body.length());
  client.println("Connection: close");
  client.println();
  client.print(body);
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

  syringeSetup(MOTOR_UP, MOTOR_DOWN);

  // turn LED on
  digitalWrite(LED_BUILTIN, HIGH);

  // record when we are homing the axis
  unsigned long start_home = micros();

  // Empty the syringe at startup so position is known
  waterOut();


  // Pre-populate sensor data array with a simulated depth/pressure profile
  for (int i = 0; i < DATA_LEN; i++) {
    // Simulate a dive-and-resurface profile: ramp down then back up
    // Values represent pressure in millibars × 10 (i.e. 10130 = 1013.0 mbar)
    uint32_t depth = (i < DATA_LEN / 2)
      ? (10130 + (uint32_t)i * 480)          // descending
      : (10130 + (uint32_t)(DATA_LEN - 1 - i) * 480); // ascending
    sensorData[i] = depth;
  }

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed — did you run `make program-data` ?");
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

  // wait until 38 seconds after homging
  while( (micros() - start_home) < (home_seconds * 1000000) ) {
    // do nothing
  }

  waterStop();

  // turn LED off
  digitalWrite(LED_BUILTIN, LOW);

  // Zero all tracking state — syringe is now at a known empty position
  syringeReset();
}

// ---------------------------------------------------------------------------
// wifi_loop — non-blocking Runnable that handles one HTTP request at a time.
// ---------------------------------------------------------------------------
#define WIFI_STATE_IDLE    0
#define WIFI_STATE_READING 1

static int        wifiState       = WIFI_STATE_IDLE;
static WiFiClient wifiClient;
static String     wifiRequestLine;
static String     wifiCurrentLine;
static bool       wifiFirstLine;

void wifi_loop(unsigned long now) {
  switch (wifiState) {

  case WIFI_STATE_IDLE: {
    WiFiClient c = server.available();
    if (!c) return;
    wifiClient      = c;
    wifiRequestLine = "";
    wifiCurrentLine = "";
    wifiFirstLine   = true;
    Serial.println("New Client.");
    wifiState = WIFI_STATE_READING;
    break;
  }

  case WIFI_STATE_READING: {
    if (!wifiClient.connected()) {
      wifiClient.stop();
      Serial.println("Client Disconnected.");
      wifiState = WIFI_STATE_IDLE;
      return;
    }
    // Drain only the bytes that are already available — never block.
    while (wifiClient.available()) {
      char c = wifiClient.read();
      if (c == '\n') {
        if (wifiFirstLine) {
          wifiRequestLine = wifiCurrentLine;  // e.g. "GET /index.html HTTP/1.1"
          wifiFirstLine   = false;
        }
        if (wifiCurrentLine.length() == 0) {
          // Blank line = end of headers, dispatch response.

          // Parse path from request line: "GET /path HTTP/1.1"
          String path = "/";
          int pathStart = wifiRequestLine.indexOf(' ');
          int pathEnd   = wifiRequestLine.indexOf(' ', pathStart + 1);
          if (pathStart >= 0 && pathEnd > pathStart) {
            path = wifiRequestLine.substring(pathStart + 1, pathEnd);
          }
          Serial.print("Request: ");
          Serial.println(path);

          // AJAX endpoint: LED on
          if (path == "/H") {
            digitalWrite(LED_BUILTIN, HIGH);
            sendJson(wifiClient, 200, "{\"led\":\"on\"}");

          // AJAX endpoint: LED off
          } else if (path == "/L") {
            digitalWrite(LED_BUILTIN, LOW);
            sendJson(wifiClient, 200, "{\"led\":\"off\"}");

          // AJAX endpoint: set syringe setpoint  GET /setpoint?v=NNN
          } else if (path.startsWith("/setpoint")) {
            int paramIdx = path.indexOf("v=");
            if (paramIdx >= 0) {
              int val = path.substring(paramIdx + 2).toInt();
              if (val < 0)   val = 0;
              if (val > 250) val = 250;
              syringeSetpoint(val);
              sendJson(wifiClient, 200, "{\"setpoint\":" + String(val) + "}");
            } else {
              sendJson(wifiClient, 200, "{\"error\":\"missing v param\"}");
            }

          // AJAX endpoint: return the 44-element data array as JSON
          } else if (path == "/data") {
            String json = "{\"data\":[";
            for (int i = 0; i < DATA_LEN; i++) {
              json += String(sensorData[i]);
              if (i < DATA_LEN - 1) json += ",";
            }
            json += "]}";
            sendJson(wifiClient, 200, json);
          } else {
            if (!serveFile(wifiClient, path)) {
              send404(wifiClient);
            }
          }

          wifiClient.stop();
          Serial.println("Client Disconnected.");
          wifiState = WIFI_STATE_IDLE;
          return;
        }
        wifiCurrentLine = "";
      } else if (c != '\r') {
        wifiCurrentLine += c;
      }
    }
    break;
  }

  }
}

// ---------------------------------------------------------------------------
// led_blink — non-blocking Runnable that toggles the LED every 2 seconds.
// Called at the rate defined by period[]; no internal timing needed.
// ---------------------------------------------------------------------------
static bool ledOn = false;

void led_blink(unsigned long now) {
  ledOn = !ledOn;
  digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
}

// ---------------------------------------------------------------------------
// syringe
// ---------------------------------------------------------------------------

void run_syringe(unsigned long now) {
  syringeLoop();
}

// ---------------------------------------------------------------------------
// Runnable table + main loop
// ---------------------------------------------------------------------------
Runnable runnables[] = { wifi_loop, led_blink, run_syringe };

const int num_runnable = ARRAY_SIZE(runnables);

// Last time each Runnable was called
unsigned long last_run[] = {0, 0, 0};

// Call period in us for each Runnable (0 = every loop iteration)
unsigned long period[] = {0, 2000000, 1000};

void loop() {
  const unsigned long now = micros();
  for (int i = 0; i < num_runnable; i++) {
    if (now - last_run[i] >= period[i]) {
      runnables[i](now);
      last_run[i] = now;
    }
  }
}
