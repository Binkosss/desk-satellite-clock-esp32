/*
  Project: Desk Satellite Clock (ESP32)
  Author: Binkosss
  Created: 2024-11-14
  Purpose: Run a custom desk clock on ESP32 with MAX7219 time/date display,
           DHT11 temperature/humidity, and satellite-style LED animation.
           Hardware uses a custom PCB milled on a CNC 3018.
           Two bottom NeoPixels simulate engine glow, and one top LED
           blinks like a satellite antenna beacon.
           Display is a vintage lens-style 9-digit 7-segment module from old calculator.
           the last digit is intentionally unused because 8 digits are enough to display time, date, or temperature/humidity.
*/

#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <MAX7219.h>
#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

#ifndef TIMEZONE_POSIX
#define TIMEZONE_POSIX "UTC0"
#endif

// NeoPixel settings
#define LED_PIN 5
#define LED_COUNT 2
#define LED_PIN_SINGLE 18 // Single LED pin
#define DISPLAY_DIGITS_USED 8 // 9-digit module in hardware, last digit intentionally unused
#define LED_ACTIVE_HOUR_START 7
#define LED_ACTIVE_HOUR_END 21 // LEDs are off from 21:00 to 06:59
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// LED animation state
uint32_t currentColor = 0x000000;
uint32_t targetColor = 0x0000FF;
int colorStep = 1;
int pixelBrightness = 30;

unsigned long lastColorUpdateMs = 0;  // Last color update time
unsigned long colorTransitionDelayMs = 10; // Delay between color steps (ms)
unsigned long lastNtpSyncMs = 0;
unsigned long ntpSyncIntervalMs = 1000;
unsigned long lastWifiReconnectAttemptMs = 0;
unsigned long wifiReconnectIntervalMs = 10000;

// DHT11 settings
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Wi-Fi settings
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// NTP settings
const char* timeServer = "pool.ntp.org";
const char* timezonePosix = TIMEZONE_POSIX;
WiFiUDP udp;
NTPClient timeClient(udp, timeServer, 0, 60000);

MAX7219 max7219;

// Rotating display mode
int displayMode = 0;

// Button pin
#define BUTTON_PIN 13
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
unsigned long lastButtonChangeMs = 0;
unsigned long debounceDelayMs = 30;

TaskHandle_t displayTaskHandle;
TaskHandle_t ledTaskHandle;

// Function declarations
void taskDisplay(void *parameter);
void taskLedAnimation(void *parameter);
void displayTime(int hours, int minutes, int seconds);
void displayDate(int year, int month, int day);
void displayTemperatureHumidity(float temperature, float humidity);
void smoothColorTransition(void);
bool getLocalDateTime(tm &localDateTime);
bool shouldRunLedAnimation(const tm &localDateTime);
void turnOffAllLeds();

///////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Wi-Fi connected.");

  // Apply timezone (POSIX TZ format) for automatic standard/DST handling
  setenv("TZ", timezonePosix, 1);
  tzset();
  Serial.print("Timezone set to: ");
  Serial.println(timezonePosix);

  // Init NTP (UTC source)
  timeClient.begin();

  // Init MAX7219, DHT, NeoPixel and pins
  max7219.Begin();
  dht.begin();
  strip.begin();
  strip.show();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  currentButtonState = digitalRead(BUTTON_PIN);
  lastButtonState = currentButtonState;
  displayMode = 0; // Always start with time view after reset/power loss
  pinMode(LED_PIN_SINGLE, OUTPUT);  // Configure single LED pin
  digitalWrite(LED_PIN_SINGLE, LOW); 
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);    // Turn built-in LED off

  // Create tasks on both cores
  xTaskCreatePinnedToCore(taskDisplay, "Display Task", 4096, NULL, 1, &displayTaskHandle, 1); // Core 1
  xTaskCreatePinnedToCore(taskLedAnimation, "LED Animation Task", 2048, NULL, 1, &ledTaskHandle, 0); // Core 0
}

///////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  // Empty loop, tasks are handled by FreeRTOS
}

///////////////////////////////////////////////////////////////////////////////////////////

void taskDisplay(void *parameter) {
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      unsigned long currentMs = millis();
      if (currentMs - lastWifiReconnectAttemptMs >= wifiReconnectIntervalMs) {
        lastWifiReconnectAttemptMs = currentMs;
        WiFi.reconnect();
      }
    }

    currentButtonState = digitalRead(BUTTON_PIN);

    // Check if button was pressed
    if (currentButtonState == LOW && lastButtonState == HIGH) {
      lastButtonChangeMs = millis();  // Reset debounce timer
      displayMode = (displayMode + 1) % 3;  // Cycle modes: 0, 1, 2
    }

    lastButtonState = currentButtonState;

    tm localDateTime = {};
    bool hasLocalTime = getLocalDateTime(localDateTime);

    // Show data based on current mode
    switch(displayMode) {
      case 0:  // Show time
        if (hasLocalTime) {
          displayTime(localDateTime.tm_hour, localDateTime.tm_min, localDateTime.tm_sec);
        }
        break;
      
      case 1:  // Show date
        if (hasLocalTime) {
          displayDate(localDateTime.tm_year + 1900, localDateTime.tm_mon + 1, localDateTime.tm_mday);
        }
        break;

      case 2:  // Show temperature and humidity
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();
        if (!isnan(temperature) && !isnan(humidity)) {
          displayTemperatureHumidity(temperature, humidity);
        }
        break;
    }

    delay(10);  // Keep CPU time for other tasks
  }
}

bool getLocalDateTime(tm &localDateTime) {
  unsigned long currentMs = millis();
  if (currentMs - lastNtpSyncMs >= ntpSyncIntervalMs) {
    lastNtpSyncMs = currentMs;
    if (WiFi.status() == WL_CONNECTED) {
      timeClient.update();
    }
  }

  time_t epochUtc = (time_t)timeClient.getEpochTime();
  if (epochUtc <= 0) {
    return false;
  }

  localtime_r(&epochUtc, &localDateTime);
  return true;
}

void displayTime(int hours, int minutes, int seconds) {
  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d-%02d-%02d", hours, minutes, seconds);
  max7219.Clear();
  for (int i = 0; i < DISPLAY_DIGITS_USED; i++) {
    max7219.DisplayChar(i, timeStr[i], 0);
  }
}

void displayDate(int year, int month, int day) {
  char dateStr[9];
  snprintf(dateStr, sizeof(dateStr), "%02d-%02d-%02d", day, month, year % 100);
  max7219.Clear();
  for (int i = 0; i < DISPLAY_DIGITS_USED; i++) {
    max7219.DisplayChar(i, dateStr[i], 0);
  }
}

void displayTemperatureHumidity(float temperature, float humidity) {
  char tempHumidityStr[9];
  snprintf(tempHumidityStr, sizeof(tempHumidityStr), "%02dC  %02dH", (int)temperature, (int)humidity);
  max7219.Clear();
  for (int i = 0; i < DISPLAY_DIGITS_USED; i++) {
    max7219.DisplayChar(i, tempHumidityStr[i], 0);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////

void taskLedAnimation(void *parameter) {
  bool ledState = LOW;  // LED state
  unsigned long previousMillis = 0;
  const int shortBlinkInterval = 100;  // Short ON/OFF blink time (ms)
  const int longPauseInterval = 5000;   // Long pause after two blinks (ms)
  int blinkStep = 0;  // Animation step

  for (;;) {
    unsigned long currentMillis = millis();
    tm localDateTime = {};
    bool hasLocalTime = getLocalDateTime(localDateTime);

    if (hasLocalTime && !shouldRunLedAnimation(localDateTime)) {
      turnOffAllLeds();
      blinkStep = 0;
      previousMillis = currentMillis;
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }
    
    switch (blinkStep) {
      case 0:  // First blink
          ledState = HIGH;  // Turn LED on
          digitalWrite(LED_PIN_SINGLE, ledState);
          digitalWrite(LED_BUILTIN, !ledState);
        if (currentMillis - previousMillis >= shortBlinkInterval) {
          previousMillis = currentMillis;
          blinkStep = 1;
        }
        break;
      
        case 1:  // Short off after first blink
          ledState = LOW;  // Turn LED off
          digitalWrite(LED_PIN_SINGLE, ledState);
          digitalWrite(LED_BUILTIN, !ledState);
        if (currentMillis - previousMillis >= shortBlinkInterval) {
          previousMillis = currentMillis;
          blinkStep = 2;
        }
        break;

        case 2:  // Second blink
          ledState = HIGH;  // Turn LED on
          digitalWrite(LED_PIN_SINGLE, ledState);
          digitalWrite(LED_BUILTIN, !ledState);
        if (currentMillis - previousMillis >= shortBlinkInterval) {
          previousMillis = currentMillis;
          blinkStep = 3;
        }
        break;

      case 3:  // Long pause after two blinks
          ledState = LOW;  // Turn LED off
          digitalWrite(LED_PIN_SINGLE, ledState);
          digitalWrite(LED_BUILTIN, !ledState);
        if (currentMillis - previousMillis >= longPauseInterval) {
          previousMillis = currentMillis;
          blinkStep = 0;  // Reset cycle
        }
        break;
    }

    smoothColorTransition();  // NeoPixel animation
    vTaskDelay(pdMS_TO_TICKS(1));  // Yield CPU for stable long-run timing
  }
}

void smoothColorTransition() {
  unsigned long currentTime = millis();
  
  // Run only after transition delay
  if (currentTime - lastColorUpdateMs >= colorTransitionDelayMs) {
    lastColorUpdateMs = currentTime; // Save update timestamp

    int r1 = (currentColor >> 16) & 0xFF;
    int g1 = (currentColor >> 8) & 0xFF;
    int b1 = currentColor & 0xFF;
    int r2 = (targetColor >> 16) & 0xFF;
    int g2 = (targetColor >> 8) & 0xFF;
    int b2 = targetColor & 0xFF;

    // If target reached, pick a new random color
    if (r1 == r2 && g1 == g2 && b1 == b2) {
      targetColor = strip.Color(random(0, 255), random(0, 255), random(0, 255));
    } else {
      // Move current color toward target
      r1 = r1 < r2 ? r1 + min(colorStep, r2 - r1) : r1 - min(colorStep, r1 - r2);
      g1 = g1 < g2 ? g1 + min(colorStep, g2 - g1) : g1 - min(colorStep, g1 - g2);
      b1 = b1 < b2 ? b1 + min(colorStep, b2 - b1) : b1 - min(colorStep, b1 - b2);

      currentColor = strip.Color(r1, g1, b1);

      // Apply color to all NeoPixels
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, r1 * pixelBrightness / 255, g1 * pixelBrightness / 255, b1 * pixelBrightness / 255);
      }
      strip.show();
    }
  }
}

bool shouldRunLedAnimation(const tm &localDateTime) {
  return localDateTime.tm_hour >= LED_ACTIVE_HOUR_START && localDateTime.tm_hour < LED_ACTIVE_HOUR_END;
}

void turnOffAllLeds() {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
  digitalWrite(LED_PIN_SINGLE, LOW);
  digitalWrite(LED_BUILTIN, LOW);
}

///////////////////////////////////////////////////////////////////////////////////////////