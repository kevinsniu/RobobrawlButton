#include <Adafruit_DotStar.h>

#define TAPOUT_PIN 27
#define READY_PIN 33

#define NUMPIXELS 30 // Number of LEDs in strip
#define DATA_PIN    12
#define CLOCK_PIN   13
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

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

  for (int i = 0; i < n; ++i) {
    

    Serial.printf("%-32.32s", WiFi.SSID(i).c_str());

  }
}

void loop() {
  strip.setPixelColor(head, color); // 'On' pixel at head
  strip.show();                     // Refresh strip
  delay(20);  

}
