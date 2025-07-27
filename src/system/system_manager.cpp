// src/system/system_manager.cpp
#include "system_manager.h"

SystemManager::SystemManager() 
    : wsServer(8080), system_initialized(false), last_stats_log(0) {
}

SystemManager::~SystemManager() {
    shutdown();
}

bool SystemManager::initialize() {
    Serial.println("🔧 [SYSTEM] Starting system component initialization...");
    
    // Initial memory check
    Serial.printf("📊 [MEMORY] Initial free heap: %lu KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("🧠 [MEMORY] Initial free PSRAM: %lu KB\n", ESP.getFreePsram() / 1024);

    // Initialize camera first (critical for system stability)
    Serial.println("📷 [INIT] Step 1/4: Initializing OV2640 camera...");
    delay(500); // Дополнительная задержка для стабильности
    
    if (!camera.initialize()) {
        Serial.printf("❌ [ERROR] Camera initialization FAILED: %s\n", 
                     camera.getLastErrorMessage().c_str());
        Serial.println("🔧 [DEBUG] Check camera connections and PSRAM availability");
        return false;
    }
    Serial.println("✅ [SUCCESS] Camera initialized successfully");
    Serial.printf("📊 [MEMORY] After camera init - Free heap: %lu KB\n", ESP.getFreeHeap() / 1024);

    // Initialize WiFi
    Serial.println("📡 [INIT] Step 2/4: Initializing WiFi Access Point...");
    delay(500);
    wifi.init("ESP32-S3_Drone_30fps", "drone2024");
    delay(200);
    wifi.start();
    delay(1000);
    Serial.printf("📊 [MEMORY] After WiFi init - Free heap: %lu KB\n", ESP.getFreeHeap() / 1024);

    // Initialize WebSocket Server
    Serial.println("🌐 [INIT] Step 3/4: Initializing WebSocket server...");
    delay(500);
    uint32_t heap_before = ESP.getFreeHeap();
    
    if (heap_before < 50000) {
        Serial.printf("⚠️  [WARNING] Low memory before WebSocket init: %lu bytes\n", heap_before);
        Serial.println("🔧 [SYSTEM] Skipping WebSocket server due to low memory");
        return false;
    } else {
        String wifi_ip = WiFi.softAPIP().toString();
        if (wsServer.start()) {
            Serial.printf("✅ [SUCCESS] WebSocket server running at http://%s:8080/\n", wifi_ip.c_str());
        } else {
            Serial.println("❌ [ERROR] Failed to start WebSocket server");
            return false;
        }
    }

    // Initialize dual-core task manager
    Serial.println("⚙️  [INIT] Step 4/4: Starting dual-core task manager...");
    delay(500);
    if (!taskManager.initialize(&camera, &wsServer, &wifi)) {
        Serial.println("❌ [ERROR] Failed to initialize task manager");
        return false;
    }

    system_initialized = true;
    last_stats_log = millis();
    
    Serial.println("🎉 [SUCCESS] ALL system components initialized successfully!");
    Serial.printf("📊 [FINAL] Free heap: %lu KB, Free PSRAM: %lu KB\n", 
                  ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);
    return true;
}

void SystemManager::update() {
    if (!system_initialized) return;
    
    // Update task manager (handles dual-core operations)
    taskManager.update();
    
    // Handle WebSocket clients
    wsServer.handleClients();
    
    // Check WiFi stability
    wifi.checkStability();
    
    // Periodic statistics logging
    if (millis() - last_stats_log >= STATS_LOG_INTERVAL) {
        Serial.printf("[SYSTEM] Uptime: %lu seconds\n", millis() / 1000);
        Serial.printf("[WS] WebSocket clients: %d\n", wsServer.getConnectedClients());
        
        FrameStats stats = camera.getStatistics();
        Serial.printf("[CAMERA] FPS: %.2f, Frames: %lu\n", stats.current_fps, stats.total_frames);
        
        last_stats_log = millis();
    }
    
    // Feed watchdog
    yield();
}

void SystemManager::shutdown() {
    if (!system_initialized) return;
    
    Serial.println("[SYSTEM] Shutting down system components...");
    
    taskManager.stop();
    wsServer.stop();
    wifi.stop();
    camera.deinitialize();
    
    system_initialized = false;
    Serial.println("[SYSTEM] Shutdown complete");
}

void SystemManager::printSystemStatus() {
    Serial.println("\n=== System Status ===");
    Serial.printf("System Initialized: %s\n", system_initialized ? "YES" : "NO");
    Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
    Serial.printf("CPU Frequency: %lu MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size: %lu MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("Free Heap: %lu KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("Free PSRAM: %lu KB\n", ESP.getFreePsram() / 1024);
    
    // Camera status
    Serial.printf("Camera: %s\n", camera.isInitialized() ? "Initialized" : "Not initialized");
    
    // WiFi status
    Serial.printf("WiFi AP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("WiFi Clients: %d\n", WiFi.softAPgetStationNum());
    
    // WebSocket status
    Serial.printf("WebSocket Clients: %d\n", wsServer.getConnectedClients());
    
    // Task manager status
    Serial.printf("Dual-Core Tasks: %s\n", taskManager.isRunning() ? "Running" : "Stopped");
    
    Serial.println("=====================\n");
}
