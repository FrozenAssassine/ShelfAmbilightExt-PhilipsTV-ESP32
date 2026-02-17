# Shelf Ambilight Extension (Philips TV)

Small ESP32-based ambilight that reads color data from a Philips TV and drives a single WS2812/NeoPixel strip (38 LEDs).

### Overview

- ESP32 requests color layers from the TV and renders them on a 38-LED NeoPixel strip.
- LEDs are split into left and right segments and a synthetic bottom glow is generated from edge colors.

### Hardware

- ESP32 (or compatible)
- WS2812 / NeoPixel strip (NUM_LEDS_TOTAL = 38)
- 5V power supply for LEDs (common ground with ESP32)
- Data pin: GPIO 16 (LED_PIN in code)

## Usage

- Configure WiFi and TV URL in secrets.h (see below).
- Build and upload with PlatformIO.

### secrets.example.h

- Template file included to show required defines.
- Copy this file to `secrets.h` and fill in values.

### Wiring diagram

- DIN of the LED strip -> ESP32 GPIO 16
- 5V and GND to the strip power
- See image below for proper led layout

### Image (LED connection)

The black is the cable, if you use a different wirering the order might not work with this code.

![LED layout](images/led_layout.png)
