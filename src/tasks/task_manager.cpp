// src/tasks/task_manager.cpp
#include "task_manager.h"

TaskManager::TaskManager() 
    : videoStreamTaskHandle(nullptr), webSocketTaskHandle(nullptr),
      camera(nullptr), wsServer(nullptr), wifi(nullptr),
      tasks_running(false), video_streaming_enabled(true), verbose_logging(false) {
}

TaskManager::~TaskManager() {
    stop();
}

bool TaskManager::initialize(OV2640Camera* cam, WebSocketServer* ws, WiFiModule* wf) {
    if (!cam || !ws || !wf) {
        Serial.println("[TASK] Error: Invalid component pointers");
        return false;
    }
    
    camera = cam;
    wsServer = ws;
    wifi = wf;
    
    // Setup task parameters
    taskParams.manager = this;
    taskParams.camera = camera;
    taskParams.wsServer = wsServer;
    taskParams.wifi = wifi;
    
    Serial.println("[TASK] Creating dual-core tasks...");
    
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
    
    // Create WebSocket task on Core 0 (network operations)
    BaseType_t result2 = xTaskCreatePinnedToCore(
        webSocketTask,            // Task function
        "WebSocketTask",          // Task name
        6144,                     // Stack size (6KB)
        &taskParams,              // Task parameters
        1,                        // Priority (normal)
        &webSocketTaskHandle,     // Task handle
        0                         // Core 0 (network and WiFi)
    );
    
    if (result1 != pdPASS || result2 != pdPASS) {
        Serial.println("[ERROR] Failed to create dual-core tasks");
        tasks_running = false; // Reset flag on failure
        stop();
        return false;
    }
    Serial.println("[SUCCESS] Dual-core tasks created:");
    Serial.println("  - Core 1: Video streaming (high priority)");
    Serial.println("  - Core 0: WebSocket server (normal priority)");
    
    return true;
}

void TaskManager::update() {
    // Task manager itself doesn't need frequent updates
    // Tasks run independently on their assigned cores
    
    // Periodic status check
    static unsigned long last_status_check = 0;
    if (millis() - last_status_check >= 10000) { // Every 10 seconds
        if (tasks_running) {
            Serial.printf("[TASK] Tasks running - Video: Core 1, WebSocket: Core 0\n");
            Serial.printf("[TASK] Video streaming: %s, Verbose: %s\n", 
                         video_streaming_enabled ? "ON" : "OFF",
                         verbose_logging ? "ON" : "OFF");
        }
        last_status_check = millis();
    }
}

void TaskManager::stop() {
    if (!tasks_running) return;
    
    Serial.println("[TASK] Stopping dual-core tasks...");
    
    tasks_running = false;
    
    // Delete tasks
    if (videoStreamTaskHandle) {
        vTaskDelete(videoStreamTaskHandle);
        videoStreamTaskHandle = nullptr;
    }
    
    if (webSocketTaskHandle) {
        vTaskDelete(webSocketTaskHandle);
        webSocketTaskHandle = nullptr;
    }
    
    Serial.println("[TASK] All tasks stopped");
}

// Static task function for video streaming (Core 1)
void TaskManager::videoStreamTask(void* parameter) {
    TaskParams* params = static_cast<TaskParams*>(parameter);
    TaskManager* manager = params->manager;
    OV2640Camera* camera = params->camera;
    WebSocketServer* wsServer = params->wsServer;
    
    Serial.println("[VIDEO_TASK] Video streaming task started on Core 1");
    
    // Wait for system to fully initialize
    delay(1000);
    
    // 20fps = 50ms между кадрами для стабильности
    const unsigned long TARGET_FRAME_INTERVAL = 50; // 20fps (50ms)
    unsigned long last_frame_time = 0;
    unsigned long frame_count = 0;
    unsigned long fps_start_time = millis();
    
    Serial.println("[VIDEO_TASK] Starting 20fps video capture loop...");
    
    while (manager->tasks_running) {
        if (!manager->video_streaming_enabled) {
            delay(100);
            continue;
        }
        
        unsigned long current_time = millis();
        
        // Точный контроль частоты кадров - 20fps
        if (current_time - last_frame_time >= TARGET_FRAME_INTERVAL) {
            auto frame = camera->captureFrame();
            
            if (frame) {
                // Отправляем кадр только если есть подключенные клиенты
                int connected_clients = wsServer->getConnectedClients();
                if (connected_clients > 0) {
                    wsServer->streamVideoFrame(frame.get());
                    frame_count++;
                }
                
                // Verbose logging
                if (manager->verbose_logging) {
                    camera->logFrameInfo(frame.get());
                }
                
                last_frame_time = current_time;
            }
        }
        
        // Статистика FPS каждые 10 секунд
        if (current_time - fps_start_time >= 10000) {
            float actual_fps = (float)frame_count / 10.0f;
            Serial.printf("[VIDEO_TASK] Actual FPS: %.1f, Target: 20.0, Frames sent: %lu\n", 
                         actual_fps, frame_count);
            
            frame_count = 0;
            fps_start_time = current_time;
        }
        
        // Минимальная задержка для предотвращения watchdog
        delayMicroseconds(500);
    }
    
    Serial.println("[VIDEO_TASK] Video streaming task ended");
    vTaskDelete(nullptr);
}

// Static task function for WebSocket server (Core 0)
void TaskManager::webSocketTask(void* parameter) {
    TaskParams* params = static_cast<TaskParams*>(parameter);
    TaskManager* manager = params->manager;
    WebSocketServer* wsServer = params->wsServer;
    WiFiModule* wifi = params->wifi;
    
    Serial.println("[WS_TASK] WebSocket server task started on Core 0");
    
    // Wait for system to fully initialize
    delay(500);
    
    unsigned long last_stats_log = 0;
    const unsigned long STATS_LOG_INTERVAL = 5000; // 5 seconds
    
    Serial.println("[WS_TASK] Starting WebSocket handling loop...");
    
    while (manager->tasks_running) {
        // Handle WebSocket clients
        wsServer->handleClients();
        
        // Check WiFi stability
        wifi->checkStability();
        
        // Periodic statistics
        if (millis() - last_stats_log >= STATS_LOG_INTERVAL) {
            if (manager->verbose_logging) {
                Serial.printf("[WS_TASK] WebSocket clients: %d\n", wsServer->getConnectedClients());
            }
            last_stats_log = millis();
        }
        
        // Small delay for network operations
        delay(10);
    }
    
    Serial.println("[WS_TASK] WebSocket server task ended");
    vTaskDelete(nullptr);
}
