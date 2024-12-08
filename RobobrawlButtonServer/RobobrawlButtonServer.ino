#include <Adafruit_DotStar.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>

#define TAPOUT_PIN 10
#define READY_PIN 11
#define NUMPIXELS 30  // Number of LEDs in strip
#define DATA_PIN 12
#define CLOCK_PIN 13
Adafruit_DotStar strip(NUMPIXELS, DATA_PIN, CLOCK_PIN, DOTSTAR_RGB);

const char *ssid = "RobobrawlButton";
const char *password = "doyourobotics";

IPAddress local_ip(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

int readyState;             // the current reading from the input pin
int lastReadyState = LOW;   // the previous reading from the input pin
int tapoutState;            // the current reading from the input pin
int lastTapoutState = LOW;  // the previous reading from the input pin

unsigned long lastReadyDebounceTime = 0;   // the last time the output pin was toggled
unsigned long lastTapoutDebounceTime = 0;  // the last time the output pin was toggled

unsigned long debounceDelay = 50;  // the debounce time; increase if the output flickers

uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

enum Mode { green,
            yellow,
            red,
            orange,
            blue,
            none };

typedef struct struct_message {
  int ready;
  int tapout;
  Mode mode;
} struct_message;

struct_message outgoingValues;
struct_message incomingValues;

int orangeReady = 0;
int orangeTapout = 0;
int blueReady = 0;
int blueTapout = 0;

esp_now_peer_info_t peerInfo;

void setStrip(Mode color) {
  for (int i = 0; i < NUMPIXELS; i++) {
    switch (color) {
      case green:
        strip.setPixelColor(i, 0x0000FF);
        break;
      case yellow:
        strip.setPixelColor(i, 0x880088);
        break;
      case red:
        strip.setPixelColor(i, 0xFF0000);
        break;
      case orange:
        strip.setPixelColor(i, 0xFF0088);
        break;
      case blue:
        strip.setPixelColor(i, 0x00FF00);
        break;
      case none:
        strip.setPixelColor(i, 0x000000);
        break;
    }
  }
  strip.show();
}

void sendESPNOW(int ready, int tapout, Mode mode) {
  outgoingValues.ready = ready;
  outgoingValues.tapout = tapout;
  outgoingValues.mode = mode;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&outgoingValues, sizeof(outgoingValues));

  if (result != ESP_OK) {
    Serial.println("Error sending data");
  }
}
// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// callback when data is recv from Master
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           info->src_addr[0], info->src_addr[1], info->src_addr[2], info->src_addr[3], info->src_addr[4], info->src_addr[5]);
  memcpy(&incomingValues, data, sizeof(incomingValues));
  Serial.print("Last Packet Recv from: ");
  Serial.println(macStr);
  Serial.print("Last Packet Recv Data: ");
  Serial.print(incomingValues.ready);
  Serial.println(incomingValues.tapout);
  Serial.println("");
  if (!blueReady && incomingValues.ready) {
    blueReady = 1;
    sendESPNOW(0, 0, Mode::green);
  }
  if (!blueTapout && !orangeTapout && incomingValues.tapout) {
    blueTapout = 1;
    sendESPNOW(0, 0, Mode::red);
    setStrip(Mode::yellow);
  }

  server.send(200, "text/html", SendHTML());
}


void setup() {
  Serial.begin(115200);

  pinMode(TAPOUT_PIN, INPUT_PULLDOWN);
  pinMode(READY_PIN, INPUT_PULLDOWN);

  strip.begin();
  strip.setBrightness(150);
  strip.show();

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(500);
  Serial.println(WiFi.softAPIP());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  delay(100);
  server.on("/", handle_OnConnect);
  server.on("/identifyteams", handle_IdentifyTeams);
  server.on("/resetmatch", handle_ResetMatch);
  server.on("/orangewin", handle_OrangeWin);
  server.on("/bluewin", handle_BlueWin);
  server.on("/getOrange", handle_OrangeStatus);
  server.on("/getBlue", handle_BlueStatus);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  int readyReading = digitalRead(READY_PIN);
  if (readyReading != lastReadyState) {
    // reset the debouncing timer
    lastReadyDebounceTime = millis();
  }
  if ((millis() - lastReadyDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (readyReading != readyState) {
      readyState = readyReading;

      // only toggle the LED if the new button state is HIGH
      if ((readyState == HIGH) && (!orangeReady)) {
        orangeReady = readyReading;
        setStrip(Mode::green);
      }
    }
  }

  int tapoutReading = digitalRead(TAPOUT_PIN);
  if (tapoutReading != lastTapoutState) {
    // reset the debouncing timer
    lastTapoutDebounceTime = millis();
  }
  if ((millis() - lastTapoutDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (tapoutReading != tapoutState) {
      tapoutState = tapoutReading;

      // only toggle the LED if the new button state is HIGH
      if ((tapoutState == HIGH) && (!orangeTapout)) {
        if (!blueTapout) {
          setStrip(Mode::red);
          sendESPNOW(0, 0, Mode::yellow);
        }
        orangeTapout = tapoutReading;
      }
    }
  }
  // Serial.print("Ready(Button: ");
  // Serial.print(readyReading);
  // Serial.print(", State: ");
  // Serial.print(lastReadyState);
  // Serial.print(") Tapout(Button: ");
  // Serial.print(tapoutReading);
  // Serial.print(", State: ");
  // Serial.print(lastTapoutState);
  // Serial.print(")");
  // Serial.println();
  lastReadyState = readyReading;
  lastTapoutState = tapoutReading;
  Serial.print(orangeReady);
  Serial.print(orangeTapout);
  Serial.print(blueReady);
  Serial.println(blueTapout);
  server.handleClient();
  delay(20);
}
void handle_OnConnect() {
  server.send(200, "text/html", SendHTML());
}
void handle_IdentifyTeams() {
  setStrip(Mode::orange);
  outgoingValues.ready = 0;
  outgoingValues.tapout = 0;
  outgoingValues.mode = Mode::blue;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&outgoingValues, sizeof(outgoingValues));

  if (result != ESP_OK) {
    Serial.println("Error sending data");
  }

  server.send(200, "text/html", SendHTML());
}
void handle_ResetMatch() {
  orangeReady = 0;
  orangeTapout = 0;
  blueReady = 0;
  blueTapout = 0;

  setStrip(Mode::orange);
  sendESPNOW(0, 0, Mode::blue);

  server.send(200, "text/html", SendHTML());
}
void handle_OrangeWin() {
  setStrip(Mode::green);
  sendESPNOW(0, 0, Mode::red);

  server.send(200, "text/html", SendHTML());
}
void handle_BlueWin() {
  setStrip(Mode::red);
 sendESPNOW(0, 0, Mode::green);

  server.send(200, "text/html", SendHTML());
}

void handle_OrangeStatus() {
  String returnVal = "NOT READY";
  if (orangeReady) {
    returnVal = "READY";
  }
  if (orangeTapout) {
    returnVal = "TAPOUT";
  }
  server.send(200, "text/plane", returnVal);
}
void handle_BlueStatus() {
  String returnVal = "NOT READY";
  if (blueReady) {
    returnVal = "READY";
  }
  if (blueTapout) {
    returnVal = "TAPOUT";
  }
  server.send(200, "text/plane", returnVal);
}
void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

String SendHTML() {
  String ptr = R"***(<!DOCTYPE html> <html>
  <head><meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>Robobrawl Ready Button Interface</title>
  <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
  body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}
  .button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}
  .button-on {background-color: #3498db;}
  .button-on:active {background-color: #2980b9;}
  .button-off {background-color: #34495e;}
  .button-off:active {background-color: #2c3e50;}
  p {font-size: 30px;color: #888;margin-bottom: 10px;}
  </style>
  </head>
  <body>
  <h1>Robobrawl Ready Button Interface</h1>
  <a class="button button-off" href="/identifyteams">Identify Teams</a>
  <a class="button button-off" href="/resetmatch">Reset Match</a>
  <a class="button button-off" href="/orangewin">Orange Win</a>
  <a class="button button-off" href="/bluewin">Blue Win</a>

    <h2>
      Orange: <span id="orange">NULL</span>
      Blue: <span id="blue">NULL</span>
    </h2>
  

  <script> setInterval(
    function() {
    getOrangeStatus();
    getBlueStatus();
  }, 500);
  function getOrangeStatus() {
    var statusRequestOrange = new XMLHttpRequest();
    statusRequestOrange.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("orange").innerHTML =
          this.responseText;
      }
    };
    statusRequestOrange.open("GET", "getOrange", true);
    statusRequestOrange.send();
  }
  function getBlueStatus() {
    var statusRequestBlue = new XMLHttpRequest();
    statusRequestBlue.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("blue").innerHTML =
          this.responseText;
      }
    };
    statusRequestBlue.open("GET", "getBlue", true);
    statusRequestBlue.send();
  }
  </script>;
  </body>;
  </html>;)***";
  return ptr;
}
