// include/task_manager.h
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "ov2640.h"
#include "wifi_module.h"

class TaskManager {
private:
    // Task handles
    TaskHandle_t videoStreamTaskHandle;
    
    // Component references
    OV2640Camera* camera;
    WiFiModule* wifi;
    
    // Task control flags
    bool tasks_running;
    bool video_streaming_enabled;
    bool verbose_logging;
    
    // Task functions (static for FreeRTOS)
    static void videoStreamTask(void* parameter);
    
    // Task parameters structure
    struct TaskParams {
        TaskManager* manager;
        OV2640Camera* camera;
        WiFiModule* wifi;
    };
    
    TaskParams taskParams;

public:
    TaskManager();
    ~TaskManager();
    
    // Task lifecycle
    bool initialize(OV2640Camera* cam, WiFiModule* wf);
    void update();
    void stop();
    
    // Control functions
    void enableVideoStreaming() { video_streaming_enabled = true; }
    void disableVideoStreaming() { video_streaming_enabled = false; }
    void toggleVerboseLogging() { verbose_logging = !verbose_logging; }
    
    // Status functions
    bool isRunning() const { return tasks_running; }
    bool isVideoStreamingEnabled() const { return video_streaming_enabled; }
    bool isVerboseLogging() const { return verbose_logging; }
};