#include <Adafruit_DotStar.h>
#include <WiFi.h>

#define TAPOUT_PIN 27
#define READY_PIN 33

#define NUMPIXELS 30 // Number of LEDs in strip
#define DATA_PIN    12
#define CLOCK_PIN   13
Adafruit_DotStar strip(NUMPIXELS, DATA_PIN, CLOCK_PIN, DOTSTAR_RGB);

void setup() {
  Serial.begin(115200);

  pinMode(TAPOUT_PIN, INPUT_PULLDOWN);
  pinMode(READY_PIN, INPUT_PULLDOWN);

  strip.begin();
  strip.show();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int networkCount = WiFi.scanNetworks();

  for (int i = 0; i < networkCount; ++i) {
    Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
  }
}

void loop() {
  for(int i = 0; i < NUMPIXELS; i++){
    strip.setPixelColor(i, 0xFF0000);
  }
  strip.show();
  delay(20);

}
