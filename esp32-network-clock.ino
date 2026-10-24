#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <cJSON.h>

#include <WiFi.h>
#include <time.h>
#include <TFT_eSPI.h>
#include <math.h>
#include <esp_system.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite screen = TFT_eSprite(&tft);

// =====================================================
// YOUR HOTSPOT
// =====================================================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// London automatically handles GMT/BST
const char* londonTZ =
  "GMT0BST,M3.5.0/1,M10.5.0";

const char* ntp1 = "pool.ntp.org";
const char* ntp2 = "time.cloudflare.com";

// =====================================================
// TTGO BUTTONS
// =====================================================
#define BUTTON_LEFT  0
#define BUTTON_RIGHT 35

// =====================================================
// DISPLAY
// =====================================================
const int SCREEN_W = 240;
const int SCREEN_H = 135;

const int CLOCK_X = 68;
const int CLOCK_Y = 70;
const int CLOCK_R = 55;

// 0 = London
// 1 = Baku
// 2 = London weather, 3 = Baku weather
int mode = 0;

bool lastLeftState = HIGH;
bool lastRightState = HIGH;

unsigned long lastButtonTime = 0;

// =====================================================
// DRAW ANALOG CLOCK FACE
// =====================================================

void drawClockFace() {

  screen.drawCircle(
    CLOCK_X,
    CLOCK_Y,
    CLOCK_R,
    TFT_WHITE
  );

  screen.drawCircle(
    CLOCK_X,
    CLOCK_Y,
    CLOCK_R - 1,
    TFT_DARKGREY
  );

  // Minute/hour marks
  for (int i = 0; i < 60; i++) {

    float angle =
      (i * 6 - 90) * DEG_TO_RAD;

    int outerX =
      CLOCK_X +
      cos(angle) * (CLOCK_R - 3);

    int outerY =
      CLOCK_Y +
      sin(angle) * (CLOCK_R - 3);

    int innerRadius;

    if (i % 5 == 0) {
      innerRadius = CLOCK_R - 10;
    } else {
      innerRadius = CLOCK_R - 6;
    }

    int innerX =
      CLOCK_X +
      cos(angle) * innerRadius;

    int innerY =
      CLOCK_Y +
      sin(angle) * innerRadius;

    screen.drawLine(
      innerX,
      innerY,
      outerX,
      outerY,
      i % 5 == 0
        ? TFT_WHITE
        : TFT_DARKGREY
    );
  }

  // Clock numbers
  screen.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  screen.setTextSize(1);

  screen.setCursor(
    CLOCK_X - 5,
    CLOCK_Y - 45
  );
  screen.print("12");

  screen.setCursor(
    CLOCK_X + 40,
    CLOCK_Y - 3
  );
  screen.print("3");

  screen.setCursor(
    CLOCK_X - 2,
    CLOCK_Y + 39
  );
  screen.print("6");

  screen.setCursor(
    CLOCK_X - 45,
    CLOCK_Y - 3
  );
  screen.print("9");
}

// =====================================================
// DRAW CLOCK HANDS
// =====================================================

void drawHands(
  int hour,
  int minute,
  int second
) {

  float hourAngle =
    (((hour % 12)
      + minute / 60.0)
      * 30.0 - 90.0)
    * DEG_TO_RAD;

  float minuteAngle =
    ((minute
      + second / 60.0)
      * 6.0 - 90.0)
    * DEG_TO_RAD;

  float secondAngle =
    (second * 6.0 - 90.0)
    * DEG_TO_RAD;

  // Hour
  int hx =
    CLOCK_X +
    cos(hourAngle) * 27;

  int hy =
    CLOCK_Y +
    sin(hourAngle) * 27;

  screen.drawLine(
    CLOCK_X,
    CLOCK_Y,
    hx,
    hy,
    TFT_WHITE
  );

  screen.drawLine(
    CLOCK_X + 1,
    CLOCK_Y,
    hx + 1,
    hy,
    TFT_WHITE
  );

  // Minute
  int mx =
    CLOCK_X +
    cos(minuteAngle) * 39;

  int my =
    CLOCK_Y +
    sin(minuteAngle) * 39;

  screen.drawLine(
    CLOCK_X,
    CLOCK_Y,
    mx,
    my,
    TFT_CYAN
  );

  // Second
  int sx =
    CLOCK_X +
    cos(secondAngle) * 47;

  int sy =
    CLOCK_Y +
    sin(secondAngle) * 47;

  screen.drawLine(
    CLOCK_X,
    CLOCK_Y,
    sx,
    sy,
    TFT_RED
  );

  screen.fillCircle(
    CLOCK_X,
    CLOCK_Y,
    3,
    TFT_WHITE
  );
}

// =====================================================
// GET LONDON TIME
// =====================================================

void getLondonTime(
  struct tm &londonTime
) {

  time_t now = time(nullptr);

  localtime_r(
    &now,
    &londonTime
  );
}

// =====================================================
// GET BAKU TIME
// =====================================================

void getBakuTime(
  struct tm &bakuTime
) {

  time_t now = time(nullptr);

  // Azerbaijan = UTC + 4
  now += 4 * 3600;

  gmtime_r(
    &now,
    &bakuTime
  );
}

// =====================================================
// MODE 1 / MODE 2
// ANALOG + DIGITAL
// =====================================================

void drawSingleClock(
  struct tm timeData,
  const char* city,
  uint16_t cityColor
) {

  char digitalTime[10];
  char dateText[20];

  strftime(
    digitalTime,
    sizeof(digitalTime),
    "%H:%M:%S",
    &timeData
  );

  strftime(
    dateText,
    sizeof(dateText),
    "%d %b %Y",
    &timeData
  );

  drawClockFace();

  drawHands(
    timeData.tm_hour,
    timeData.tm_min,
    timeData.tm_sec
  );

  // City
  screen.setTextColor(
    cityColor,
    TFT_BLACK
  );

  screen.setTextSize(1);

  screen.setCursor(155, 20);
  screen.print(city);

  // Digital time
  screen.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  screen.setTextSize(2);

  screen.setCursor(135, 38);
  screen.print(digitalTime);

  // Date
  screen.setTextSize(1);

  screen.setCursor(148, 68);
  screen.print(dateText);

  // Network status
  if (
    WiFi.status()
    == WL_CONNECTED
  ) {

    screen.fillCircle(
      148,
      96,
      3,
      TFT_GREEN
    );

    screen.setTextColor(
      TFT_GREEN,
      TFT_BLACK
    );

    screen.setCursor(157, 93);
    screen.print("SYNCED");

  } else {

    screen.fillCircle(
      148,
      96,
      3,
      TFT_RED
    );

    screen.setTextColor(
      TFT_RED,
      TFT_BLACK
    );

    screen.setCursor(157, 93);
    screen.print("OFFLINE");
  }

  // Mode hint
  screen.setTextColor(
    TFT_DARKGREY,
    TFT_BLACK
  );

  screen.setCursor(140, 118);
  screen.print("<  MODE  >");
}

// =====================================================
// ONLINE WEATHER - Open-Meteo, refreshed every 15 minutes
// =====================================================
struct WeatherData {
  float temperature;
  float feelsLike;
  float wind;
  int code;
  int isDay;
  bool valid;
  bool failed;
  unsigned long fetchedAt;
};
WeatherData weather[2] = {};
portMUX_TYPE weatherMux = portMUX_INITIALIZER_UNLOCKED;
bool weatherTaskReady = false;
const char* cityNames[2] = {"LONDON", "BAKU"};
const char* cityCoordinates[2] = {
  "latitude=51.5074&longitude=-0.1278",
  "latitude=40.4093&longitude=49.8671"
};

const char* weatherDescription(int code) {
  switch (code) {
    case 0: return "Clear sky";
    case 1: return "Mainly clear";
    case 2: return "Partly cloudy";
    case 3: return "Overcast";
    case 45: case 48: return "Fog";
    case 51: case 53: case 55: return "Drizzle";
    case 56: case 57: return "Freezing drizzle";
    case 61: return "Light rain";
    case 63: return "Rain";
    case 65: return "Heavy rain";
    case 66: case 67: return "Freezing rain";
    case 71: case 73: case 75: case 77: return "Snow";
    case 80: case 81: case 82: return "Rain showers";
    case 85: case 86: return "Snow showers";
    case 95: case 96: case 99: return "Thunderstorm";
    default: return "Weather unavailable";
  }
}

bool fetchWeather(int city) {
  NetworkClientSecure client;
  client.useBuiltinCACertBundle();
  client.setHandshakeTimeout(10);
  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(10000);
  String url = "https://api.open-meteo.com/v1/forecast?";
  url += cityCoordinates[city];
  url += "&current=temperature_2m,apparent_temperature,weather_code,wind_speed_10m,is_day&forecast_days=1";
  bool ok = false;
  int status = -1;
  WeatherData next = {};
  if (http.begin(client, url)) {
    status = http.GET();
    if (status == HTTP_CODE_OK) {
      String body = http.getString();
      cJSON* root = cJSON_Parse(body.c_str());
      cJSON* current = cJSON_GetObjectItemCaseSensitive(root, "current");
      cJSON* temp = cJSON_GetObjectItemCaseSensitive(current, "temperature_2m");
      cJSON* feels = cJSON_GetObjectItemCaseSensitive(current, "apparent_temperature");
      cJSON* wind = cJSON_GetObjectItemCaseSensitive(current, "wind_speed_10m");
      cJSON* code = cJSON_GetObjectItemCaseSensitive(current, "weather_code");
      cJSON* day = cJSON_GetObjectItemCaseSensitive(current, "is_day");
      if (cJSON_IsNumber(temp) && cJSON_IsNumber(feels) &&
          cJSON_IsNumber(wind) && cJSON_IsNumber(code) && cJSON_IsNumber(day)) {
        next.temperature = temp->valuedouble;
        next.feelsLike = feels->valuedouble;
        next.wind = wind->valuedouble;
        next.code = code->valueint;
        next.isDay = day->valueint;
        next.valid = true;
        next.fetchedAt = millis();
        ok = true;
      }
      cJSON_Delete(root);
    }
    http.end();
  }
  portENTER_CRITICAL(&weatherMux);
  if (ok) weather[city] = next;
  else weather[city].failed = true; // Preserve last good reading on failure.
  portEXIT_CRITICAL(&weatherMux);
  if (ok) {
    Serial.printf("Weather %s: %.1f C, feels %.1f C, %s, wind %.1f km/h\n",
      cityNames[city], next.temperature, next.feelsLike,
      weatherDescription(next.code), next.wind);
  } else {
    Serial.printf("Weather %s fetch failed: HTTP %d\n", cityNames[city], status);
  }
  return ok;
}

// Network calls run in a separate task; screen and buttons never wait for HTTP.
void weatherWorker(void* unused) {
  unsigned long attemptedAt[2] = {};
  bool attempted[2] = {};
  bool succeeded[2] = {};
  for (;;) {
    if (WiFi.status() == WL_CONNECTED && time(nullptr) > 1700000000) {
      for (int city = 0; city < 2; ++city) {
        unsigned long interval = succeeded[city] ? 15UL * 60UL * 1000UL : 60000UL;
        if (!attempted[city] || millis() - attemptedAt[city] >= interval) {
          succeeded[city] = fetchWeather(city);
          attemptedAt[city] = millis();
          attempted[city] = true;
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void drawWeatherIcon(int code, int day) {
  int x = 191, y = 48;
  if (code <= 2) {
    if (day) {
      screen.fillCircle(x, y, 13, TFT_YELLOW);
      for (int i = 0; i < 8; ++i) {
        float a = i * PI / 4;
        screen.drawLine(x + cos(a)*17, y + sin(a)*17,
                        x + cos(a)*22, y + sin(a)*22, TFT_YELLOW);
      }
    } else {
      screen.fillCircle(x, y, 15, TFT_WHITE);
      screen.fillCircle(x + 7, y - 5, 13, TFT_BLACK);
    }
    if (code < 2) return;
  }
  screen.fillCircle(x - 12, y + 6, 10, TFT_LIGHTGREY);
  screen.fillCircle(x, y, 14, TFT_LIGHTGREY);
  screen.fillCircle(x + 14, y + 7, 9, TFT_LIGHTGREY);
  screen.fillRect(x - 12, y + 6, 27, 11, TFT_LIGHTGREY);
  if (code >= 51 && code <= 99) {
    bool snow = (code >= 71 && code <= 77) || code == 85 || code == 86;
    for (int i = -1; i <= 1; ++i) {
      if (snow) screen.fillCircle(x + i * 12, y + 24, 2, TFT_WHITE);
      else screen.drawLine(x + i*12, y + 21, x + i*12 - 4, y + 27, TFT_CYAN);
    }
  }
}

void drawWeather(int city) {
  WeatherData data;
  portENTER_CRITICAL(&weatherMux);
  data = weather[city];
  portEXIT_CRITICAL(&weatherMux);
  uint16_t accent = city == 0 ? TFT_CYAN : TFT_YELLOW;
  bool online = WiFi.status() == WL_CONNECTED;
  screen.setTextWrap(false);
  screen.setTextColor(accent, TFT_BLACK);
  screen.setTextSize(2);
  screen.setCursor(12, 8);
  screen.print(cityNames[city]);
  screen.setTextSize(1);
  screen.setCursor(160, 13);
  screen.print("WEATHER");
  screen.drawFastHLine(12, 28, 216, TFT_DARKGREY);
  if (data.valid) {
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.setTextSize(4);
    screen.setCursor(12, 36);
    screen.printf("%.0f", data.temperature);
    screen.setTextSize(2);
    screen.print(" C");
    drawWeatherIcon(data.code, data.isDay);
    screen.setTextSize(1);
    screen.setCursor(12, 74);
    screen.print(weatherDescription(data.code));
    screen.setCursor(12, 89);
    screen.printf("Feels %.0f C", data.feelsLike);
    screen.setCursor(121, 89);
    screen.printf("Wind %.0f km/h", data.wind);
    unsigned long minutes = (millis() - data.fetchedAt) / 60000UL;
    bool stale = !online || data.failed || minutes >= 30;
    screen.setTextColor(stale ? TFT_ORANGE : TFT_GREEN, TFT_BLACK);
    screen.setCursor(12, 104);
    if (!online) screen.print("OFFLINE | ");
    else if (stale) screen.print("CACHED | ");
    else screen.print("ONLINE | ");
    screen.printf("Fetched %lu min ago", minutes);
  } else {
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.setTextSize(2);
    screen.setCursor(12, 45);
    screen.print(!online ? "WiFi offline" : data.failed ? "Retrying..." : "Loading...");
    screen.setTextSize(1);
    screen.setCursor(12, 81);
    screen.print(!weatherTaskReady ? "Weather task unavailable" : "Weather refreshes automatically");
  }
  screen.setTextSize(1);
  screen.setTextColor(TFT_DARKGREY, TFT_BLACK);
  screen.setCursor(12, 123);
  screen.print("Open-Meteo.com");
  screen.setCursor(160, 123);
  screen.print("< MODE >");
}

// Both buttons now navigate all four clock and weather screens.
void checkButtons() {
  unsigned long now = millis();
  bool leftState = digitalRead(BUTTON_LEFT);
  bool rightState = digitalRead(BUTTON_RIGHT);
  bool leftPressed = leftState == LOW && lastLeftState == HIGH;
  bool rightPressed = rightState == LOW && lastRightState == HIGH;
  if (now - lastButtonTime > 200) {
    if (leftPressed) {
      mode = (mode + 3) % 4;
      lastButtonTime = now;
    } else if (rightPressed) {
      mode = (mode + 1) % 4;
      lastButtonTime = now;
    }
  }
  lastLeftState = leftState;
  lastRightState = rightState;
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // Backlight
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  // Buttons
  pinMode(
    BUTTON_LEFT,
    INPUT_PULLUP
  );

  // GPIO 35 has no internal
  // pull-up on ESP32
  pinMode(
    BUTTON_RIGHT,
    INPUT
  );

  // Display
  tft.init();
  tft.setRotation(1);

  screen.createSprite(
    SCREEN_W,
    SCREEN_H
  );

  screen.fillSprite(
    TFT_BLACK
  );

  screen.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  screen.setTextSize(2);

  screen.setCursor(25, 45);
  screen.print("Connecting...");

  screen.setTextSize(1);

  screen.setCursor(25, 75);
  screen.print("Waiting for WiFi");

  screen.pushSprite(0, 0);

  // WiFi
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      Serial.printf("WiFi disconnected; reason=%u\n", info.wifi_sta_disconnected.reason);
    }
  });
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  Serial.printf("Looking for hotspot: [%s]\n", ssid);
  int networks = WiFi.scanNetworks();
  bool found = false;
  for (int i = 0; i < networks; ++i) {
    if (WiFi.SSID(i) == ssid) {
      found = true;
      Serial.printf("Hotspot found: channel=%d signal=%d dBm\n", WiFi.channel(i), WiFi.RSSI(i));
    }
  }
  Serial.println(found ? "Hotspot visible; connecting" : "Hotspot NOT visible on 2.4 GHz; check name and compatibility");
  WiFi.scanDelete();
  WiFi.begin(ssid, password);
  unsigned long attemptStarted = millis();
  unsigned long lastReport = 0;
  while (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastReport >= 2000) {
      Serial.printf("Waiting for WiFi; status=%d\n", (int)WiFi.status());
      lastReport = now;
    }
    if (now - attemptStarted >= 20000) {
      Serial.println("Retrying WiFi connection...");
      WiFi.disconnect();
      delay(100);
      WiFi.begin(ssid, password);
      attemptStarted = millis();
    }
    delay(100);
  }
  Serial.print("WiFi connected; IP: ");
  Serial.println(WiFi.localIP());

  // Network time
  configTzTime(
    londonTZ,
    ntp1,
    ntp2
  );

  screen.fillSprite(
    TFT_BLACK
  );

  screen.setTextSize(2);

  screen.setCursor(35, 55);
  screen.print(
    "Syncing time..."
  );

  screen.pushSprite(0, 0);

  struct tm testTime;
  while (!getLocalTime(&testTime)) delay(500);

  weatherTaskReady = xTaskCreatePinnedToCore(
    weatherWorker, "weather", 12288, nullptr, 1, nullptr, 0
  ) == pdPASS;
  if (!weatherTaskReady) Serial.println("Could not start weather task");
}

void loop() {
  checkButtons();
  screen.fillSprite(TFT_BLACK);
  if (mode == 0) {
    struct tm london;
    getLondonTime(london);
    drawSingleClock(london, "LONDON", TFT_CYAN);
  } else if (mode == 1) {
    struct tm baku;
    getBakuTime(baku);
    drawSingleClock(baku, "BAKU", TFT_YELLOW);
  } else {
    drawWeather(mode - 2);
  }
  screen.pushSprite(0, 0);
    static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    struct tm t;
    getLondonTime(t);
    Serial.printf("London time: %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);
  }
  delay(30);
}
