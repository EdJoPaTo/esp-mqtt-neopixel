// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include <Adafruit_NeoPixel.h>
#include <EspMQTTClient.h>

#define CLIENT_NAME "espNeopixel"

EspMQTTClient client(
  "WifiSSID",
  "WifiPassword",
  "192.168.1.100",  // MQTT Broker server ip
  "MQTTUsername",   // Can be omitted if not needed
  "MQTTPassword",   // Can be omitted if not needed
  CLIENT_NAME,     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

const bool mqtt_retained = true;

#define BASIC_TOPIC CLIENT_NAME "/"
#define BASIC_TOPIC_SET BASIC_TOPIC "set/"
#define BASIC_TOPIC_STATUS BASIC_TOPIC "status/"


// Which pin is connected to the NeoPixels?
const int LED_PIN = 13;

// How many NeoPixels are attached?
//const int LED_COUNT = 41;
const int LED_COUNT = (60 * 5) - 41;

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

uint16_t hue = 120; // green
uint8_t sat = 100;
uint8_t bri = 10;
boolean on = true;
int x = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.clear();
  strip.show();            // Turn OFF all pixels ASAP
  //strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  // Optional functionnalities of EspMQTTClient :
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableLastWillMessage(BASIC_TOPIC "connected", "0", mqtt_retained);  // You can activate the retain flag by setting the third parameter to true
}

void onConnectionEstablished() {
  client.subscribe(BASIC_TOPIC_SET "hue", [](const String & payload) {
    int newHue = strtol(payload.c_str(), 0, 10);
    if (hue != newHue) {
      hue = newHue;
      x = 0;
      client.publish(BASIC_TOPIC_STATUS "hue", payload, mqtt_retained);
    }
  });

  client.subscribe(BASIC_TOPIC_SET "sat", [](const String & payload) {
    int newSat = strtol(payload.c_str(), 0, 10);
    if (sat != newSat) {
      sat = newSat;
      x = 0;
      client.publish(BASIC_TOPIC_STATUS "sat", payload, mqtt_retained);
    }
  });

  client.subscribe(BASIC_TOPIC_SET "bri", [](const String & payload) {
    int newBri = strtol(payload.c_str(), 0, 10);
    if (bri != newBri) {
      bri = newBri;
      x = 0;
      client.publish(BASIC_TOPIC_STATUS "bri", payload, mqtt_retained);
    }
  });

  client.subscribe(BASIC_TOPIC_SET "on", [](const String & payload) {
    boolean newOn = payload != "0";
    if (on != newOn) {
      on = newOn;
      x = 0;
      client.publish(BASIC_TOPIC_STATUS "on", payload, mqtt_retained);
    }
  });

  client.publish(BASIC_TOPIC "connected", "2", mqtt_retained);
  client.publish(BASIC_TOPIC_STATUS "hue", String(hue), mqtt_retained);
  client.publish(BASIC_TOPIC_STATUS "sat", String(sat), mqtt_retained);
  client.publish(BASIC_TOPIC_STATUS "bri", String(bri), mqtt_retained);
  client.publish(BASIC_TOPIC_STATUS "on", on ? "1" : "0", mqtt_retained);

  digitalWrite(LED_BUILTIN, HIGH);
}

int lastX = x;
void loop() {
  client.loop();

  //stepInterlaced();
  stepMulti();

  if (x != lastX) {
    Serial.println(x);
    lastX = x;
  }

  strip.show();
  delay(10);
}

void setHsv(int pixel, int hue, int sat, int bri, int on) {
  // move hue from 360 to 2^16
  // move sat from 100 to 256

  // Set pixel's color (in RAM)
  strip.setPixelColor(pixel, strip.ColorHSV(hue * 182, sat * 2.55, bri * on));
}

void stepInterlaced() {
  if (x >= strip.numPixels()) {
    return;
  }

  setHsv(x, hue, sat, bri, on);

  x += 2;
  if (x >= strip.numPixels() && x % 2 == 0) {
    x = 1;
  }
}

void stepMulti() {
  if (x >= 10) {
    return;
  }

  for (int i = x; i < strip.numPixels(); i += 10) {
    setHsv(i, hue, sat, bri, on);
  }

  x++;
}
