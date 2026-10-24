# ESP32 Network Clock & Weather

An embedded C++ project that brings network time and weather onto a TTGO T-Display. Two buttons switch between London and Baku clock screens and current weather for each city.

## Features

- Analogue clock hands, digital time and date on a 240 × 135 display.
- NTP synchronisation; London GMT/BST adjustment and Baku UTC+4 time.
- Current temperature, feels-like temperature, wind and weather icons from Open-Meteo.
- Weather requests in a separate FreeRTOS task so HTTP requests do not block display updates.
- Fifteen-minute weather refresh, one-minute retry after errors, and cached readings with freshness indicators.
- Button edge detection and a 200 ms debounce interval.

## Hardware and dependencies

This sketch targets the original ESP32-based **TTGO T-Display** with an ST7789 screen, buttons on GPIO 0 and GPIO 35, and backlight on GPIO 4. Other boards, including T-Display S3, need configuration changes.

Local build environment: Espressif Arduino ESP32 core **3.3.12**, **TFT_eSPI 2.5.43**, and board target `esp32:esp32:esp32` (ESP32 Dev Module). WiFi, HTTPClient, NetworkClientSecure, cJSON and FreeRTOS are provided by the ESP32 core.

## Setup

1. Install Arduino IDE and the Espressif ESP32 board package using Boards Manager. Install TFT_eSPI through Library Manager.
2. In TFT_eSPI's `User_Setup_Select.h`, enable `#include <User_Setups/Setup25_TTGO_T_Display.h>` and disable other setup includes.
3. Put `esp32-network-clock.ino` inside a folder named `esp32-network-clock`, then open it in Arduino IDE. If downloading GitHub's ZIP, rename the extracted `esp32-network-clock-main` folder first.
4. Replace `YOUR_WIFI_SSID` and `YOUR_WIFI_PASSWORD` in your local sketch with a 2.4 GHz network's credentials. Keep that edited copy private; restore the placeholders before committing changes.
5. Select **ESP32 Dev Module**, select the connected board's serial port, then verify and upload. Open Serial Monitor at **115200 baud** for diagnostics.

The display initially waits for Wi-Fi and network time. Once connected, the left button moves backwards and the right button moves forwards through: London clock → Baku clock → London weather → Baku weather.

## Implementation

`setup()` initialises the display, connects to Wi-Fi, synchronises time, and starts the weather task. `loop()` reads buttons and draws into a TFT sprite before pushing the frame to the screen. The weather worker fetches JSON over HTTPS and updates shared readings inside a critical section; the UI copies those readings before rendering.

## Validation

The published sketch compiled successfully with the environment above: 1,180,228 bytes of flash (90%) and 50,216 bytes of static RAM (15%). This packaging check did not flash or retest a physical board.

## Current limitations

Startup waits for a working Wi-Fi connection and NTP response. After startup the clock can continue locally during an outage, but rebooting offline will remain on the connection screen. The clock's “SYNCED” label reflects Wi-Fi connectivity rather than the age of the last NTP update. Weather requires access to Open-Meteo and is retained only in RAM. No automated hardware test is included.

Weather data: [Open-Meteo](https://open-meteo.com/). Display library: [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI). ESP32 platform: [Arduino-ESP32](https://github.com/espressif/arduino-esp32).
