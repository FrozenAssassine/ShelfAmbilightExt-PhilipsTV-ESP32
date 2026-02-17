#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "secrets.h"

#define LED_PIN 16
#define NUM_LEDS_TOTAL 38                 // total leds on the single strip
#define NUM_LEDS_LEFT 19                  // number of leds for left side
#define NUM_LEDS_RIGHT 19                 // number of leds for right side
#define NUM_BOTTOM_LEFT 8                 // number of left leds affected by bottom glow
#define NUM_BOTTOM_RIGHT 8                // number of right leds affected by bottom glow
const float alpha = 0.8;                  // smoothing the smaller the slower
const unsigned long updateInterval = 200; // ms do not put too high, may crash the tv

Adafruit_NeoPixel strip(NUM_LEDS_TOTAL, LED_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastUpdate = 0;

struct RGB
{
  uint8_t r, g, b;
};

RGB jsonToRGB(const JsonVariant &var)
{
  RGB c;
  c.r = var["r"];
  c.g = var["g"];
  c.b = var["b"];
  return c;
}

RGB interpolate(RGB a, RGB b, float t)
{
  RGB c;
  c.r = a.r + (b.r - a.r) * t;
  c.g = a.g + (b.g - a.g) * t;
  c.b = a.b + (b.b - a.b) * t;
  return c;
}

RGB blend(RGB oldColor, RGB newColor, float alpha)
{
  RGB c;
  c.r = oldColor.r + (newColor.r - oldColor.r) * alpha;
  c.g = oldColor.g + (newColor.g - oldColor.g) * alpha;
  c.b = oldColor.b + (newColor.b - oldColor.b) * alpha;
  return c;
}

RGB currentColors[NUM_LEDS_TOTAL];

void setup()
{
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PW);
  Serial.println("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  strip.begin();
  strip.show();

  // wait until the tv booted and is ready. Otherwise it may crash :O
  for (int i = 0; i < NUM_LEDS_TOTAL; i++)
  {
    strip.setPixelColor(i, 10, 70, 80);
    strip.show();
    delay(1000);
  }

  for (int i = 0; i < NUM_LEDS_TOTAL; i++)
    currentColors[i] = {0, 0, 0};
}

void updateLeds(StaticJsonDocument<4096> doc)
{
  JsonObject layer1 = doc["layer1"];
  JsonObject left = layer1["left"];
  JsonObject right = layer1["right"];

  // RIGHT SIDE (physical first strip, normal order)
  for (int i = 0; i < NUM_LEDS_RIGHT; i++)
  {
    int idx = i * (right.size() - 1) / (NUM_LEDS_RIGHT - 1);
    RGB newColor = jsonToRGB(right[String(idx)]);
    int ledIndex = i;
    currentColors[ledIndex] = blend(currentColors[ledIndex], newColor, alpha);
    strip.setPixelColor(ledIndex, strip.Color(currentColors[ledIndex].r, currentColors[ledIndex].g, currentColors[ledIndex].b));
  }

  // LEFT SIDE (physical second strip, reversed order)
  for (int i = 0; i < NUM_LEDS_LEFT; i++)
  {
    int idx = i * (left.size() - 1) / (NUM_LEDS_LEFT - 1);
    RGB newColor = jsonToRGB(left[String(idx)]);
    int ledIndex = NUM_LEDS_TOTAL - 1 - i;
    currentColors[ledIndex] = blend(currentColors[ledIndex], newColor, alpha);
    strip.setPixelColor(ledIndex, strip.Color(currentColors[ledIndex].r, currentColors[ledIndex].g, currentColors[ledIndex].b));
  }

  int startRightBottom = (NUM_LEDS_RIGHT - NUM_BOTTOM_RIGHT) / 2;
  int startLeftBottom = NUM_LEDS_TOTAL - NUM_LEDS_LEFT + (NUM_LEDS_LEFT - NUM_BOTTOM_LEFT) / 2;

  // BOTTOM GLOW (synthetic)
  RGB bottomLeft = jsonToRGB(left[String(left.size() - 1)]);
  RGB bottomRight = jsonToRGB(right[String(right.size() - 1)]);

  // Right side bottom glow
  for (int i = 0; i < NUM_BOTTOM_RIGHT; i++)
  {
    float t = i / float(NUM_BOTTOM_RIGHT - 1);
    RGB blended = interpolate(bottomLeft, bottomRight, t);
    int ledIndex = startRightBottom + i;
    currentColors[ledIndex] = blend(currentColors[ledIndex], blended, alpha);
    strip.setPixelColor(ledIndex, strip.Color(currentColors[ledIndex].r, currentColors[ledIndex].g, currentColors[ledIndex].b));
  }

  // Left side bottom glow (reversed)
  for (int i = 0; i < NUM_BOTTOM_LEFT; i++)
  {
    float t = i / float(NUM_BOTTOM_LEFT - 1);
    RGB blended = interpolate(bottomLeft, bottomRight, t);
    int ledIndex = startLeftBottom + i;
    currentColors[ledIndex] = blend(currentColors[ledIndex], blended, alpha);
    strip.setPixelColor(ledIndex, strip.Color(currentColors[ledIndex].r, currentColors[ledIndex].g, currentColors[ledIndex].b));
  }

  strip.show();
}

void loop()
{
  if (millis() - lastUpdate >= updateInterval)
  {
    lastUpdate = millis();

    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;
      http.begin(TV_URL);
      http.setReuse(true);
      int httpCode = http.GET();

      if (httpCode != 200)
      {
        Serial.printf("HTTP GET failed: %d\n", httpCode);
        delay(1000);
        return;
      }

      String payload = http.getString();
      StaticJsonDocument<4096> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error)
      {
        Serial.println("Failed to parse JSON");
        return;
      }
      updateLeds(doc);

      http.end();
    }
  }
}