#pragma once

#include "wled.h"
#include "usermod_v2_background.h"
#include <NimBLEDevice.h>

const char *bleServiceUUIDBase = "CAFEBABE-1234-5678-ABCD-12345678FFAA"; // 36
// const char *bleServiceUUIDBase = "CAFEBABE-1234-5678-ABCD"; // 36

class UsermodBackgroundBLESync : public UsermodBackground {
  private:
    // WLED_ENABLE_WIFI_SWITCH - Required for BLE
    #if defined(WLED_ENABLE_WIFI_SWITCH)
    bool bleEnabled = true;
    #else
    bool bleEnabled = false;
    #endif

    bool bleInitialized = false; 
    bool wifiEnabled = true; 

    NimBLEScan* _pScan;
    NimBLEAdvertising* _pAdvertising;
    NimBLEUUID _pBleServiceUUID = NimBLEUUID(bleServiceUUIDBase);

    uint8_t deviceId = 0x7F; 
    uint16_t wifiTimeout = 5;
    std::string bleName = "BLE32";
    unsigned long wifiTimeoutTime = 0;
    
    // strings to reduce flash memory usage (used more than twice)
    static const char _strTag[];
    static const char _strDevice[];
    static const char _strEnabled[];
    static const char _strName[];
    static const char _strSUUID[];
    static const char _strTimeout[];
    
    class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
      public:
      void onResult(NimBLEAdvertisedDevice* advertisedDevice) override {
          if (!advertisedDevice->isAdvertisingService(UsermodBackgroundBLESync::instance().getServiceUUID())) {
            return;
          }
          
          std::string manufData = advertisedDevice->getManufacturerData();
          if (!manufData.empty()) {
             UsermodBackgroundBLESync::instance().decodeData(manufData);   
          }
      }
    };

  public:
    
    static UsermodBackgroundBLESync& instance()
    {
      static UsermodBackgroundBLESync instance;
      return instance;
    }
      
    // ------------------------------------------------------------
    void setDeviceId(uint8_t id) {
      deviceId = id;
      updateAdvertising();
    }

    // ------------------------------------------------------------
    int getDeviceId() {
      return deviceId;
    }

    // ------------------------------------------------------------
    String getBLEName() {
      return bleName.c_str();
    }

    // ------------------------------------------------------------
    void setServiceUUID(NimBLEUUID uuid) {
      _pBleServiceUUID = uuid;
      updateAdvertising();
    }

    // ------------------------------------------------------------
    NimBLEUUID getServiceUUID() {
      return _pBleServiceUUID;
    }

    // ------------------------------------------------------------
    String getWIFITimeout(){
      return String((wifiTimeoutTime - millis()) / 60000, 2);
    }

    // ------------------------------------------------------------
    void decodeData(std::string manufData) {
      // Convert the hex data into decimal values
      uint8_t deviceState = currentPreset;
      uint8_t advDeviceId = static_cast<int>(manufData[0]);
      uint8_t advPresetId = static_cast<int>(manufData[1]);

      DEBUG_PRINTLN(F("UsermodBackgroundBLESync :: decodeData"));
      
      if(advDeviceId >= deviceId){
        return;
      }

      if(advPresetId != deviceState){
        DEBUG_PRINTLN(F("UsermodBackgroundBLESync :: applyingPreset"));
        applyPresetWithFallback(advPresetId, CALL_MODE_DIRECT_CHANGE);
        updateAdvertising();
      }
    }

    // ------------------------------------------------------------
    void updateAdvertising() {
      uint8_t deviceState = currentPreset;
      const uint8_t manufData[] = {deviceId, deviceState}; 
      DEBUG_PRINTLN(F("UsermodBackgroundBLESync :: updateAdvertising"));
      
      if(_pAdvertising == nullptr){
        return;
      }

      _pAdvertising->stop();
      _pAdvertising->setManufacturerData(std::string((char*)manufData, sizeof(manufData)));
      _pAdvertising->start();
    }

    // ------------------------------------------------------------
    void startScan() {
      _pScan->setActiveScan(true); // Active scanning sends scan request to the advertiser to get more details
      _pScan->setInterval(100);    // Time between scan windows in milliseconds
      _pScan->setWindow(99);       // Length of each scan window in milliseconds
      _pScan->start(3, false);     // Start scan for 3 seconds
    }
    
    // ------------------------------------------------------------
    void bleInit(bool fromSetup){
      DEBUG_PRINTLN(F("Initializing BLE"));
      WLED::instance().disableWiFi();
      NimBLEDevice::init(bleName);
      _pAdvertising = NimBLEDevice::getAdvertising();
      _pAdvertising->addServiceUUID(_pBleServiceUUID);      
      updateAdvertising();
      
      _pScan = NimBLEDevice::getScan(); // Create a new scan
      _pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
   
      wifiEnabled = false;
      bleInitialized = true;
      stateUpdated(CALL_MODE_NOTIFICATION);
    }

    // WLED State Change
    // ------------------------------------------------------------
    void onStateChange(uint8_t mode)
    {
      DEBUG_PRINTLN(F("UsermodBackgroundBLESync :: onStateChange"));
      updateAdvertising();
    }

    // ------------------------------------------------------------
    void onUpdateBegin(bool init)
    {
      DEBUG_PRINTLN(F("UsermodBackground :: onUpdateBegin"));
      UsermodBackground::onUpdateBegin(init);
      
      if(!enabled){
        bleEnabled = false;
        _pAdvertising->stop();
        _pScan->stop();
      }
    }

    // ------------------------------------------------------------
    void setup() {
      UsermodBackground::setup();
      DEBUG_PRINTLN(F("UsermodBackgroundBLESync :: setup")); 

      #if defined(BLE_BG_REFRESH_RATE_MS)
      backgroundRefresh = BLE_BG_REFRESH_RATE_MS;
      #endif

      #if defined(BLE_BG_STACK_SIZE)
      backgroundStackSize = BLE_BG_STACK_SIZE;
      #endif
    }

    // ------------------------------------------------------------
    void backgroundLoop(){

      // TODO: Disable WiFi - Will require reboot to enable BLE again
      if(bleEnabled && !bleInitialized && wifiTimeoutTime == 0){
        wifiTimeoutTime = millis() + (wifiTimeout * 60000);
      }

      // Disable WiFi - Start BLE Functionality
      if(wifiTimeoutTime != 0 && wifiTimeoutTime < millis()){
        DEBUG_PRINTLN(F("UsermodBackground :: Wifi Timeout Starting BLE"));
        wifiTimeoutTime = 0;
        bleInit(true);
        return;
      }

      // BLE Scan for other devices
      if(bleEnabled && bleInitialized){
        DEBUG_PRINTLN(F("UsermodBackground :: BLE Scan"));
        startScan();
      }    
    }


    // ------------------------------------------------------------
    void loop(){}

    
    // Add JSON entries that go to cfg.json
    // ------------------------------------------------------------
    void addToConfig(JsonObject &root)
    {
      JsonObject top = root.createNestedObject(FPSTR(_strTag));
      top[FPSTR(_strEnabled)] = bleEnabled;
      top[FPSTR(_strName)] = bleName;
      top[FPSTR(_strSUUID)] = _pBleServiceUUID.toString();
      top[FPSTR(_strDevice)] = deviceId;
      top[FPSTR(_strEnabled)] = bleEnabled;
      top[FPSTR(_strTimeout)] = wifiTimeout;
    }

    // Read JSON entries that go to cfg.json
    // ------------------------------------------------------------
    bool readFromConfig(JsonObject &root)
    {
      JsonObject top = root[FPSTR(_strTag)];

      if(top.isNull()) {
        DEBUG_PRINTLN(FPSTR(_strTag));
        return false;
      }

      std::string serviceId;
      getJsonValue(top[FPSTR(_strEnabled)], bleEnabled, true);
      getJsonValue(top[FPSTR(_strName)], bleName, "BLE32");
      getJsonValue(top[FPSTR(_strSUUID)], serviceId, bleServiceUUIDBase);
      _pBleServiceUUID = NimBLEUUID(serviceId);
      
      deviceId = top[FPSTR(_strDevice)] | deviceId;
      deviceId = (uint8_t) max(0,min(255,(int)deviceId));
      
      wifiTimeout = top[FPSTR(_strTimeout)] | wifiTimeout;
      wifiTimeout = (uint8_t) max(1,min(255,(int)wifiTimeout));
      
      updateAdvertising();
      return true;
    }

  // Allows you to optionally give your V2 usermod an unique ID.
  // ------------------------------------------------------------
  uint16_t getId() {
    return USERMOD_ID_BLE_SYNC;
  }
};

// strings to reduce flash memory usage (used more than twice)
const char UsermodBackgroundBLESync::_strTag[] PROGMEM = "BSync";
const char UsermodBackgroundBLESync::_strEnabled[] PROGMEM = "BLE Enabled";
const char UsermodBackgroundBLESync::_strName[] PROGMEM = "BLE Name";
const char UsermodBackgroundBLESync::_strDevice[] PROGMEM = "Device ID lower gets priority";
const char UsermodBackgroundBLESync::_strSUUID[] PROGMEM =  "Service UUID";
const char UsermodBackgroundBLESync::_strTimeout[] PROGMEM = "WiFi Timeout Minutes";
