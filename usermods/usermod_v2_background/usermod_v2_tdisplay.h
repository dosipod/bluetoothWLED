/*
REQUIRED SETUP

TFT_eSPI Library Adjustments (board selection)
You need to modify a file in the 'TFT_eSPI' library to select the correct board.
Locate the 'User_Setup_Select.h' file can be found in the '/.pio/libdeps/YOUR-BOARD/TFT_eSPI' folder.

Modify the 'User_Setup_Select.h'
Comment out the following line
//#include <User_Setup.h> // Default setup is root library folder

Uncomment the following line
#include <User_Setups/Setup25_TTGO_T_Display.h> // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
*/

#pragma once

#include "wled.h"
#include "usermod_v2_background.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>

#if defined(USERMOD_BACKGROUND_BLE_SYNC)
#include "usermod_v2_ble_sync.h"
#endif

#define TFT_CH 6

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

class UsermodBackgroundTDisplay : public UsermodBackground
{

private:
#if defined(TFT_BRIGHTNESS)
  uint16_t tftBrightness = TFT_BRIGHTNESS;
#else
  // 0=OFF; 255=MAX
  uint16_t tftBrightness = 50;
#endif

#if defined(TFT_TIMEOUT)
  uint16_t tftTimeout = TFT_TIMEOUT;
#else
  uint16_t tftTimeout = 30; // Seconds
#endif
  bool skipRows = false;
  bool needRedraw = true;
  
  // TTGO T-Display
  String knownSsid = "";
  IPAddress knownIp;
  uint8_t knownBrightness = 0;
  uint8_t knownMode = 0;
  uint8_t knownPalette = 0;
  uint8_t tftcharwidth = 19; // Number of chars that fit on screen with text size set to 2
  unsigned long tftNextTimeout = 0;

  // strings to reduce flash memory usage (used more than twice)
  static const char _strTag[];
  static const char _strBrightness[];
  static const char _strTimeout[];

public:
  // ------------------------------------------------------------
  void setBrightness(uint32_t newBrightness)
  {
    ledcWrite(TFT_CH, newBrightness); // 0-15, 0-255 (with 8 bit resolution);  0=totally dark;255=totally shiny
  }

  // ------------------------------------------------------------
  void initDisplay(int rotation = 3)
  {
    DEBUG_PRINTLN(F("UsermodBackgroundTDisplay :: initDisplay"));
    pinMode(TFT_BL, OUTPUT);
    ledcSetup(TFT_CH, 5000, 8);    // 0-15, 5000, 8
    ledcAttachPin(TFT_BL, TFT_CH); // TFT_BL, 0 - 15

    tft.init();
    tft.setRotation(rotation); // Rotation here is set up for the text to be readable with the port on the left. Use 1 to flip.
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(1, 10);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.print("Init...");

    setBrightness(tftBrightness);
    needRedraw = true;
  }

  // ------------------------------------------------------------
  void updateDisplay()
  {
    // Check if values which are shown on display changed from the last time.
    if (((apActive) ? String(apSSID) : WiFi.SSID()) != knownSsid)
    {
      needRedraw = true;
    }
    else if (knownIp != (apActive ? IPAddress(4, 3, 2, 1) : WiFi.localIP()))
    {
      needRedraw = true;
    }
    else if (knownBrightness != bri)
    {
      needRedraw = true;
    }
    else if (knownMode != strip.getMainSegment().mode)
    {
      needRedraw = true;
    }
    else if (knownPalette != strip.getMainSegment().palette)
    {
      needRedraw = true;
    }

    if (tftNextTimeout < millis())
      setBrightness(0);

    if (!needRedraw)
      return;

    skipRows = false;
    knownSsid = WiFi.SSID();
    knownIp = apActive ? IPAddress(4, 3, 2, 1) : WiFi.localIP();
    knownBrightness = bri;
    knownMode = strip.getMainSegment().mode;
    knownPalette = strip.getMainSegment().palette;

    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);

    // First row
    tft.setCursor(1, 1);
    String network = knownSsid.isEmpty() ? String("AP: "+ String(apSSID)) : knownSsid;
    
    tft.print(network.substring(0, tftcharwidth > 1 ? tftcharwidth - 1 : 0));
    if (network.length() > tftcharwidth)
      tft.print("~");
    
    // Second row
    tft.setTextSize(2);
    tft.setCursor(1, 24);
    tft.setTextColor(TFT_GREENYELLOW);
    #if defined(USERMOD_BACKGROUND_BLE_SYNC)
    if (WLED::instance().wifiDisabled){
      tft.print("Device ID:");
      tft.print(UsermodBackgroundBLESync::instance().getDeviceId());
      skipRows = true;
    }
    #endif

    if(!skipRows){
      tft.print("IP:");
      tft.print(knownIp);
      #if defined(USERMOD_BACKGROUND_BLE_SYNC)
      UsermodBackgroundBLESync::instance().getWIFITimeout();
      #endif
      
      // Third Row
      tft.setCursor(1, 46);
      tft.setTextColor(TFT_GREEN);
      
      if(apActive)
      {
        tft.print("Pass:");
        tft.print(apPass);
      }
      else
      {
        tft.print("Bright: ");
        tft.print(((float(bri) / 255) * 100), 0);
        tft.print("%");
      }
    }

    // Fourth row
    tft.setCursor(1, 68);
    tft.setTextColor(TFT_SKYBLUE);
    char lineBuffer[tftcharwidth + 1];
    extractModeName(knownMode, JSON_mode_names, lineBuffer, tftcharwidth);
    Serial.println(currentPreset);
    tft.print(currentPreset);
    tft.print(" ");
    tft.print(lineBuffer);

    // Fifth row
    tft.setCursor(1, 90);
    tft.setTextColor(TFT_BLUE);
    extractModeName(knownPalette, JSON_palette_names, lineBuffer, tftcharwidth);
    tft.print(lineBuffer);

    // Sixth row
    tft.setCursor(1, 112);
    tft.setTextColor(TFT_VIOLET);
    #if defined(TFT_BATTERY_PIN)
    float voltage =  (analogReadMilliVolts(TFT_BATTERY_PIN) / 1000.0f) * 2.0f  + 0;
    tft.print("Batt~ " + String(voltage));
    #else
    tft.print("mA ~ "+ String(strip.currentMilliamps));
    #endif

    needRedraw = false;
    tftNextTimeout =  millis() + (tftTimeout * 1000);
    setBrightness(tftBrightness);
  }

  // ------------------------------------------------------------
  void setup()
  {
    #if defined(TFT_BG_REFRESH_RATE_MS)
      backgroundRefresh = TFT_BG_REFRESH_RATE_MS;
    #endif

    #if defined(TFT_BG_STACK_SIZE)
      backgroundStackSize = TFT_BG_STACK_SIZE;
    #endif

    UsermodBackground::setup();
    DEBUG_PRINTLN(F("UsermodBackgroundTDisplay :: setup"));
    
    initDisplay();
  }

  // ------------------------------------------------------------
  void backgroundLoop()
  {
    if(needRedraw)
    {
      updateDisplay();
    }
  }

  // ------------------------------------------------------------
  void loop(){}

  // Fired when WiFi is (re-)connected.
  // Initialize own network interfaces here
  // ------------------------------------------------------------
  void connected()
  {
    needRedraw = true;
  }


  // fired upon WLED state change
  // ------------------------------------------------------------
  void onStateChange(uint8_t mode)
  {
    needRedraw = true;
  }

  // Add JSON entries that go to cfg.json
  // ------------------------------------------------------------
  void addToConfig(JsonObject &root)
  {
    JsonObject top = root.createNestedObject(FPSTR(_strTag));
    top[FPSTR(_strBrightness)] = tftBrightness;
    top[FPSTR(_strTimeout)] = tftTimeout;
  }

  // Read JSON entries that go to cfg.json
  // ------------------------------------------------------------
  bool readFromConfig(JsonObject &root)
  {
    JsonObject top = root[FPSTR(_strTag)];

    if (top.isNull())
    {
      DEBUG_PRINTLN(FPSTR(_strTag));
      return false;
    }

    getJsonValue(top[FPSTR(_strBrightness)], tftBrightness, tftBrightness);
    getJsonValue(top[FPSTR(_strTimeout)], tftTimeout, tftTimeout);

    return true;
  }

  // Allows you to optionally give your V2 usermod an unique ID.
  // ------------------------------------------------------------
  uint16_t getId()
  {
    return USERMOD_ID_TDISPLAY;
  }
};

// strings to reduce flash memory usage (used more than twice)
const char UsermodBackgroundTDisplay::_strTag[] PROGMEM = "TDisp";
const char UsermodBackgroundTDisplay::_strBrightness[] PROGMEM = "Brightness 0-255";
const char UsermodBackgroundTDisplay::_strTimeout[] PROGMEM = "Timeout in Seconds";