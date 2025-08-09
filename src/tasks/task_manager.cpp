// src/tasks/task_manager.cpp
#include "task_manager.h"

TaskManager::TaskManager() 
    : videoStreamTaskHandle(nullptr),
      camera(nullptr), wifi(nullptr),
      tasks_running(false), video_streaming_enabled(true), verbose_logging(false) {
}

TaskManager::~TaskManager() {
    stop();
}

bool TaskManager::initialize(OV2640Camera* cam, WiFiModule* wf) {
    if (!cam || !wf) {
        Serial.println("[TASK] Error: Invalid component pointers");
        return false;
    }
    
    camera = cam;
    wifi = wf;
    
    // Setup task parameters
    taskParams.manager = this;
    taskParams.camera = camera;
    taskParams.wifi = wifi;
    
    Serial.println("[TASK] Creating video stream task...");
    
    // Set tasks_running to true BEFORE creating tasks
    tasks_running = true;
    
    // Create video streaming task on Core 1 (high priority for camera)
    BaseType_t result1 = xTaskCreatePinnedToCore(
        videoStreamTask,           // Task function
        "VideoStreamTask",         // Task name
        8192,                     // Stack size (8KB)
        &taskParams,              // Task parameters
        2,                        // Priority (high)
        &videoStreamTaskHandle,   // Task handle
        1                         // Core 1 (dedicated to video)
    );
    
    if (result1 != pdPASS) {
        Serial.println("[ERROR] Failed to create video stream task");
        tasks_running = false; // Reset flag on failure
        stop();
        return false;
    }
    Serial.println("[SUCCESS] Video stream task created on Core 1");
    
    return true;
}

void TaskManager::update() {
    // Task manager itself doesn't need frequent updates
    // Tasks run independently on their assigned cores
    
    // Periodic status check
    static unsigned long last_status_check = 0;
    if (millis() - last_status_check >= 10000) { // Every 10 seconds
        if (tasks_running) {
            Serial.printf("[TASK] Video streaming task running on Core 1\n");
            Serial.printf("[TASK] Video streaming: %s, Verbose: %s\n", 
                         video_streaming_enabled ? "ON" : "OFF",
                         verbose_logging ? "ON" : "OFF");
        }
        last_status_check = millis();
    }
}

void TaskManager::stop() {
    if (!tasks_running) return;
    
    Serial.println("[TASK] Stopping video stream task...");
    
    tasks_running = false;
    
    // Delete tasks
    if (videoStreamTaskHandle) {
        vTaskDelete(videoStreamTaskHandle);
        videoStreamTaskHandle = nullptr;
    }
}

// --- Private Task Functions ---

void TaskManager::videoStreamTask(void* parameter) {
    TaskParams* params = (TaskParams*)parameter;
    TaskManager* manager = params->manager;

    // This task is now idle. The MJPEG server handles frame grabbing on client connect.
    while (manager->tasks_running) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Idle loop
    }
    vTaskDelete(nullptr);
}
