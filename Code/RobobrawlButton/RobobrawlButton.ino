#include <Adafruit_DotStar.h>
#include <WiFi.h>
#include <esp_now.h>

#define TAPOUT_PIN 10
#define READY_PIN 11
#define NUMPIXELS 30  // Number of LEDs in strip
#define DATA_PIN 12
#define CLOCK_PIN 13
Adafruit_DotStar strip(NUMPIXELS, DATA_PIN, CLOCK_PIN, DOTSTAR_RGB);

int readyState;             // the current reading from the input pin
int lastReadyState = LOW;   // the previous reading from the input pin
int tapoutState;            // the current reading from the input pin
int lastTapoutState = LOW;  // the previous reading from the input pin
int stateChange = 0;
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

esp_now_peer_info_t peerInfo;

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
  Serial.print("\r\nLast Packet Send Status:\t");
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
  Serial.println(incomingValues.mode);
  Serial.println("");
  setStrip(incomingValues.mode);
}
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

void setup() {
  Serial.begin(115200);

  pinMode(TAPOUT_PIN, INPUT_PULLDOWN);
  pinMode(READY_PIN, INPUT_PULLDOWN);

  strip.begin();
  strip.setBrightness(150);
  strip.show();

  WiFi.mode(WIFI_STA);

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
      if (readyState == HIGH) {
        stateChange = 1;
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
      if (tapoutState == HIGH) {
        stateChange = 1;
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

  if (stateChange) {
    sendESPNOW(readyReading, tapoutReading, Mode::none);

    stateChange = 0;
  }

  lastReadyState = readyReading;
  lastTapoutState = tapoutReading;

  delay(20);
}
