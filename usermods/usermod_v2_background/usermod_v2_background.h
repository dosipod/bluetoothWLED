#pragma once

#include "wled.h"

class UsermodBackground : public Usermod {
  private:
    
  public:
    #if defined(BG_REFRESH_RATE_MS)
      uint16_t backgroundRefresh = BG_REFRESH_RATE_MS;
    #else
      uint16_t backgroundRefresh = 5000;
    #endif

    #if defined(BG_TASK_STACK_SIZE)
      uint16_t backgroundStackSize = BG_TASK_STACK_SIZE;
    #else
      uint16_t backgroundStackSize = 3000;
    #endif

    bool enabled = false;
    bool initDone = false;
    bool disableBackgroundProcessing = false;
    TaskHandle_t BackgroundTaskHandle = nullptr;

    // ------------------------------------------------------------
    static void backgroundTask(void *parameter){
      TickType_t xLastWakeTime = xTaskGetTickCount();
      UsermodBackground *instance = static_cast<UsermodBackground *>(parameter);
      const TickType_t xBGFrequency = instance->backgroundRefresh * portTICK_PERIOD_MS / 2;  
      
      for(;;) {
        delay(1);
        vTaskDelayUntil(&xLastWakeTime, xBGFrequency);
        
        if (instance->disableBackgroundProcessing) {
          continue;
        }
        instance->backgroundLoop();
      }
    }

    // ------------------------------------------------------------
    void onUpdateBegin(bool init)
    {
      DEBUG_PRINTF("UsermodBackground::onUpdateBegin - [%d]\n", init);
      disableBackgroundProcessing = true;

      if (init ) {
        if(BackgroundTaskHandle) {
          vTaskSuspend(BackgroundTaskHandle);   
        }
      } 
      else {
        if (BackgroundTaskHandle) {
          vTaskResume(BackgroundTaskHandle);
        } 
        else {    
          xTaskCreatePinnedToCore(
            backgroundTask,         /* Task function. */
            "BGT",                  /* name of task. */
            backgroundStackSize,    /* Stack size of task */
            this,                   /* parameter of the task */
            1,                      /* priority of the task */
            &BackgroundTaskHandle,  /* Task handle to keep track of created task */
            xPortGetCoreID() == 0 ? 1 : 0); /* pin task to other core */      
        }
      }
      
      if (enabled) disableBackgroundProcessing = false;
    }

    // ------------------------------------------------------------
    void setup(){
      enabled = true;
      disableBackgroundProcessing = true;
      if (enabled) onUpdateBegin(false);                    // create background task
      if (BackgroundTaskHandle == nullptr) enabled = false; // background task creation failed
      if (enabled) disableBackgroundProcessing = false;     // all good - enable background processing
      initDone = true;
    }

    // ------------------------------------------------------------
    virtual void backgroundLoop(){
      DEBUG_PRINTLN(F("UsermodBackground::backgroundLoop"));
    }

    // ------------------------------------------------------------
    void loop() {
    }
};