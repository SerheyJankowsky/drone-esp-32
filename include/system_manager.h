// include/system_manager.h
#pragma once

#include <Arduino.h>
#include "task_manager.h"
#include "wifi_module.h"
#include "ov2640.h"
#include "mjpeg_server.h" // Replaced WebSocket with MJPEG

class SystemManager {
private:
    TaskManager taskManager;
    WiFiModule wifi;
    OV2640Camera camera;
    MJPEGServer mjpegServer; // Replaced wsServer
    
    bool system_initialized;
    unsigned long last_stats_log;
    
    static const unsigned long STATS_LOG_INTERVAL = 5000; // 5 seconds

public:
    SystemManager();
    ~SystemManager();
    
    // System lifecycle
    bool initialize();
    void update();
    void shutdown();
    
    // Status and monitoring
    bool isInitialized() const { return system_initialized; }
    void printSystemStatus();
    
    // Component access
    WiFiModule& getWiFi() { return wifi; }
    OV2640Camera& getCamera() { return camera; }
    MJPEGServer& getMJPEGServer() { return mjpegServer; } // New getter
    TaskManager& getTaskManager() { return taskManager; }
};