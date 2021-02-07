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
#include <ArduinoOTA.h>
#include <credentials.h>
#include <EspMQTTClient.h>

#include "lamp.h"

#define CLIENT_NAME "espNeopixel"
const bool MQTT_RETAINED = true;

EspMQTTClient client(
    WIFI_SSID,
    WIFI_PASSWORD,
    MQTT_SERVER, // MQTT Broker server ip
    CLIENT_NAME, // Client name that uniquely identify your device
    1883         // The MQTT port, default to 1883. this line can be omitted
);

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

Lamp stateStart;
Lamp stateEnd;
uint8_t somethingSet = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);

  stateStart.hue = 240; // blue
  stateStart.saturation = 100;
  stateStart.brightness = 10;
  stateStart.on = true;

  stateEnd.hue = 120; // green
  stateEnd.saturation = 100;
  stateEnd.brightness = 10;
  stateEnd.on = true;

  strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.clear();
  //strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  ArduinoOTA.setHostname(CLIENT_NAME);

  // EspMQTTClient
  client.enableDebuggingMessages();
  client.enableHTTPWebUpdater();
  client.enableLastWillMessage(BASIC_TOPIC "connected", "0", MQTT_RETAINED);
}

void onConnectionEstablished() {
  client.subscribe(BASIC_TOPIC_SET "start/hue", [](const String &payload) {
    int value = strtol(payload.c_str(), 0, 10);
    stateStart.hue = value;
    somethingSet |= 1 << 0;
    client.publish(BASIC_TOPIC_STATUS "start/hue", payload, MQTT_RETAINED);
  });

  client.subscribe(BASIC_TOPIC_SET "start/sat", [](const String &payload) {
    int value = strtol(payload.c_str(), 0, 10);
    stateStart.saturation = value;
    somethingSet |= 1 << 1;
    client.publish(BASIC_TOPIC_STATUS "start/sat", payload, MQTT_RETAINED);
  });

  client.subscribe(BASIC_TOPIC_SET "start/bri", [](const String &payload) {
    int value = strtol(payload.c_str(), 0, 10);
    stateStart.brightness = value;
    somethingSet |= 1 << 2;
    client.publish(BASIC_TOPIC_STATUS "start/bri", payload, MQTT_RETAINED);
  });

  client.subscribe(BASIC_TOPIC_SET "start/on", [](const String &payload) {
    boolean value = payload != "0";
    stateStart.on = value;
    somethingSet |= 1 << 3;
    client.publish(BASIC_TOPIC_STATUS "start/on", payload, MQTT_RETAINED);
  });

  client.subscribe(BASIC_TOPIC_SET "end/hue", [](const String &payload) {
    int value = strtol(payload.c_str(), 0, 10);
    stateEnd.hue = value;
    somethingSet |= 1 << 4;
    client.publish(BASIC_TOPIC_STATUS "end/hue", payload, MQTT_RETAINED);
  });

  client.subscribe(BASIC_TOPIC_SET "end/sat", [](const String &payload) {
    int value = strtol(payload.c_str(), 0, 10);
    stateEnd.saturation = value;
    somethingSet |= 1 << 5;
    client.publish(BASIC_TOPIC_STATUS "end/sat", payload, MQTT_RETAINED);
  });

  client.subscribe(BASIC_TOPIC_SET "end/bri", [](const String &payload) {
    int value = strtol(payload.c_str(), 0, 10);
    stateEnd.brightness = value;
    somethingSet |= 1 << 6;
    client.publish(BASIC_TOPIC_STATUS "end/bri", payload, MQTT_RETAINED);
  });

  client.subscribe(BASIC_TOPIC_SET "end/on", [](const String &payload) {
    boolean value = payload != "0";
    stateEnd.on = value;
    somethingSet |= 1 << 7;
    client.publish(BASIC_TOPIC_STATUS "end/on", payload, MQTT_RETAINED);
  });

  ArduinoOTA.begin();

  client.publish(BASIC_TOPIC "connected", "2", MQTT_RETAINED);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  client.loop();
  ArduinoOTA.handle();

  // Only update colors when all 8 topics were set
  if (somethingSet == pow(2, 8) - 1) {
    for (int i = 0; i < strip.numPixels(); i++) {
      double position = (1.0 * i) / strip.numPixels();

      int sat, hue;
      if (stateStart.on && stateEnd.on) {
        hue = interpolateHue(stateStart.hue, stateEnd.hue, position);
        sat = interpolateLinear(stateStart.saturation, stateEnd.saturation, position);
      } else if (stateStart.on) {
        hue = stateStart.hue;
        sat = stateStart.saturation;
      } else {
        hue = stateEnd.hue;
        sat = stateEnd.saturation;
      }

      int bri = interpolateLinear(stateStart.brightness * stateStart.on, stateEnd.brightness * stateEnd.on, position);
      setHsv(i, hue, sat, bri);
    }

    strip.show();
  }

  delay(10);
}

void setState(int pixel, Lamp state) {
  setHsv(pixel, state.hue, state.saturation, state.brightness * state.on);
}

void setHsv(int pixel, int hue, int sat, int bri) {
  // move hue from 360 to 2^16
  // move sat from 100 to 2^8
  // move bri from 100 to 2^8

  // Set pixel's color (in RAM)
  strip.setPixelColor(pixel, strip.ColorHSV(hue * 182, sat * 2.55, bri * 2.55));
}
