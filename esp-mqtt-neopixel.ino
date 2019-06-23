// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const bool mqtt_retained = true;

#define CLIENT_NAME "espNeopixel"

#define BASIC_TOPIC CLIENT_NAME "/"
#define BASIC_TOPIC_SET BASIC_TOPIC "set/"
#define BASIC_TOPIC_STATUS BASIC_TOPIC "status/"


// Which pin is connected to the NeoPixels?
const int LED_PIN = 13;

// How many NeoPixels are attached?
//const int LED_COUNT = 41;
const int LED_COUNT = (60 * 5) - 41;

WiFiClient espClient;
PubSubClient client(espClient);

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

uint16_t hue = 21845; // green
uint8_t sat = 255;
uint8_t bri = 10;
boolean on = true;
int x = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String payloadString = "";
  for (unsigned int i = 0; i < length; i++) {
    payloadString += (char)payload[i];
  }
  Serial.print(payloadString.length());
  Serial.print(" ");
  Serial.print(payloadString);

  if (strcmp(topic, BASIC_TOPIC_SET "hue") == 0) {
    hue = strtol(payloadString.c_str(), 0, 10) * 182; // move 360 to 2^16
    client.publish(BASIC_TOPIC_STATUS "hue", payloadString.c_str(), mqtt_retained);
  } else if (strcmp(topic, BASIC_TOPIC_SET "sat") == 0) {
    sat = strtol(payloadString.c_str(), 0, 10) * 2.55;
    client.publish(BASIC_TOPIC_STATUS "sat", payloadString.c_str(), mqtt_retained);
  } else if (strcmp(topic, BASIC_TOPIC_SET "bri") == 0) {
    bri = strtol(payloadString.c_str(), 0, 10);
    client.publish(BASIC_TOPIC_STATUS "bri", payloadString.c_str(), mqtt_retained);
  } else if (strcmp(topic, BASIC_TOPIC_SET "on") == 0) {
    on = payloadString != "0";
    client.publish(BASIC_TOPIC_STATUS "on", payloadString.c_str(), mqtt_retained);
  } else {
    // what?
  }

  x = 0;

  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientName = CLIENT_NAME;
    clientName += "-";
    clientName += macAsString();

    // Attempt to connect
    if (client.connect(clientName.c_str(), BASIC_TOPIC "connected", 0, mqtt_retained, "0")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(BASIC_TOPIC "connected", "2", mqtt_retained);
      // ... and resubscribe
      client.subscribe(BASIC_TOPIC_SET "hue");
      client.subscribe(BASIC_TOPIC_SET "sat");
      client.subscribe(BASIC_TOPIC_SET "bri");
      client.subscribe(BASIC_TOPIC_SET "on");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

String macAsString() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String result;
  for (int i = 3; i < 6; ++i) {
    result += String(mac[i], 16);
  }
  return result;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.clear();
  strip.show();            // Turn OFF all pixels ASAP
  //strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  WiFi.hostname(CLIENT_NAME);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

int lastX = x;
void loop() {
  if (!client.connected()) {
    digitalWrite(LED_BUILTIN, LOW);
    reconnect();
    digitalWrite(LED_BUILTIN, HIGH);
  }
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

void stepInterlaced() {
  if (x >= strip.numPixels()) {
    return;
  }

  strip.setPixelColor(x, strip.ColorHSV(hue, sat, bri * on)); //  Set pixel's color (in RAM)

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
    strip.setPixelColor(i, strip.ColorHSV(hue, sat, bri * on));
  }

  x++;
}
