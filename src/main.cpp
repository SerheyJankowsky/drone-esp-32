// src/main.cpp
#include <Arduino.h>
#include "wifi_module.h"
#include "ov2640.h"
#include "websocket_server.h"

// Function declarations
void processVideoFrame(camera_fb_t* fb);
void handleSerialCommands();
void printSystemStatus();

// System objects
WiFiModule wifi;
OV2640Camera camera;
WebSocketServer wsServer(8080);

// Video streaming configuration
unsigned long last_frame_time = 0;
unsigned long last_stats_log = 0;
bool streaming_enabled = true;
bool verbose_logging = false;
bool web_streaming_enabled = true;

// Performance monitoring
const unsigned long STATS_LOG_INTERVAL = 5000; // 5 seconds
const unsigned long FRAME_INTERVAL = 50; // ~20fps (50ms between frames)

void setup() {
    Serial.begin(115200);
    
    // Wait for serial monitor
    delay(2000);
    Serial.println("\n==================================================");
    Serial.println("ESP32-S3 Drone Camera System v2.2");
    Serial.println("Optimized for Stable 20fps WebSocket Streaming");
    Serial.println("==================================================");
    
    // Initial memory check
    Serial.printf("[MEMORY] Initial free heap: %lu KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("[MEMORY] Initial free PSRAM: %lu KB\n", ESP.getFreePsram() / 1024);

    // Initialize camera first (critical for system stability)
    Serial.println("\n[INIT] Initializing OV2640 camera...");
    if (!camera.initialize()) {
        Serial.printf("[ERROR] Camera initialization failed: %s\n", 
                     camera.getLastErrorMessage().c_str());
        Serial.println("[SYSTEM] Entering safe mode - camera disabled");
        streaming_enabled = false;
    } else {
        Serial.println("[SUCCESS] Camera initialized successfully");
        camera.printSystemStatus();
    }

    // Initialize WiFi
    Serial.println("\n[INIT] Initializing WiFi...");
    
    // Add safety delay before WiFi initialization
    delay(500);
    
    wifi.init("ESP32-S3_Drone_30fps", "drone2024");
    
    // Add delay after init before starting
    delay(200);
    
    wifi.start();
    
    // Wait for WiFi to stabilize
    delay(1000);
    
    // Check memory after WiFi init
    Serial.printf("[MEMORY] After WiFi init - Free heap: %lu KB\n", ESP.getFreeHeap() / 1024);
    
    // Initialize Native WebSocket Server (bypassing AsyncWebServer issues)
    Serial.println("\n[INIT] Initializing native WebSocket server...");
    
    delay(500);
    uint32_t heap_before_tcp = ESP.getFreeHeap();
    Serial.printf("[MEMORY] Before WebSocket server init - Free heap: %lu KB\n", heap_before_tcp / 1024);
    
    if (heap_before_tcp < 50000) { // Less than 50KB
        Serial.printf("[WARNING] Low memory before TCP init: %lu bytes\n", heap_before_tcp);
        Serial.println("[SYSTEM] Skipping WebSocket server due to low memory");
        web_streaming_enabled = false;
    } else {
        String wifi_ip = WiFi.softAPIP().toString();
        Serial.printf("[DEBUG] Starting WebSocket server on IP: %s\n", wifi_ip.c_str());
        
        if (wsServer.start()) {
            Serial.printf("[SUCCESS] Native WebSocket server running at http://%s:8080/\n", wifi_ip.c_str());
            web_streaming_enabled = true;
            
            // Check memory after WebSocket server start
            uint32_t heap_after_ws = ESP.getFreeHeap();
            Serial.printf("[MEMORY] After WebSocket server init - Free heap: %lu KB\n", heap_after_ws / 1024);
        } else {
            Serial.println("[ERROR] Failed to start WebSocket server");
            web_streaming_enabled = false;
        }
    }
    
    Serial.println("\n[SYSTEM] Initialization complete!");
    Serial.println("[INFO] Type 'help' for available commands");
    
    if (streaming_enabled) {
        Serial.println("[STREAM] Starting 20fps video stream...");
    }
    
    if (web_streaming_enabled) {
        Serial.println("[WEB] Native WebSocket streaming ready for clients");
        String current_ip = WiFi.softAPIP().toString();
        Serial.printf("[WEB] Connect to: http://%s:8080/\n", current_ip.c_str());
    }
    
    last_frame_time = millis();
    last_stats_log = millis();
}

void loop() {
    // Memory safety check
    static unsigned long last_memory_check = 0;
    if (millis() - last_memory_check >= 30000) { // Check every 30 seconds
        uint32_t free_heap = ESP.getFreeHeap();
        if (free_heap < 50000) { // Less than 50KB
            Serial.printf("[WARNING] Low memory detected: %lu bytes\n", free_heap);
        }
        last_memory_check = millis();
    }
    
    // High-frequency camera capture for 20fps with lag optimization
    if (streaming_enabled && web_streaming_enabled && (millis() - last_frame_time >= FRAME_INTERVAL)) {
        auto frame = camera.captureFrame();
        
        if (frame) {
            // Send frame to WebSocket clients
            if (web_streaming_enabled) {
                wsServer.streamVideoFrame(frame.get());
            }
            
            // Process the frame for additional functionality
            processVideoFrame(frame.get());
            
            // Log frame info if verbose mode is enabled
            if (verbose_logging) {
                camera.logFrameInfo(frame.get());
            }
            
            last_frame_time = millis();
        } else {
            // Debug: Log when frame capture fails
            static unsigned long last_fail_log = 0;
            if (millis() - last_fail_log > 5000) { // Log every 5 seconds
                Serial.println("[DEBUG] Frame capture failed");
                last_fail_log = millis();
            }
        }
        // Note: frame is automatically released when unique_ptr goes out of scope
    }
    
    // TESTING: Reduced delay for smoother streaming
    delayMicroseconds(50); // Reduced from 1ms to 50μs
    
    // Periodic statistics logging
    if (millis() - last_stats_log >= STATS_LOG_INTERVAL) {
        if (streaming_enabled) {
            camera.logDetailedStats();
        }
        if (web_streaming_enabled) {
            Serial.printf("[WS] WebSocket server: RUNNING - %d clients connected\n", wsServer.getConnectedClients());
        }
        last_stats_log = millis();
    }
    
    // Handle WebSocket server clients
    if (web_streaming_enabled) {
        wsServer.handleClients();
    }
    
    // Handle serial commands
    handleSerialCommands();
    
    // Feed the watchdog to prevent resets
    yield();
    
    // Minimal delay to prevent WDT reset
    delayMicroseconds(100);
}

void processVideoFrame(camera_fb_t* fb) {
    if (!fb) return;
    
    // Send frame to WebSocket clients (moved to main loop for optimization)
    // This is now handled directly in the main loop before processVideoFrame
    
    // Advanced frame processing for 20fps stream
    // This is where you can implement:
    // - Motion detection algorithms
    // - Object detection and tracking
    // - Image enhancement
    // - Data compression
    // - Local storage
    
    static unsigned long last_detailed_log = 0;
    unsigned long current_time = millis();
    
    // Detailed logging every 10 seconds
    if (current_time - last_detailed_log >= 10000) {
        FrameStats stats = camera.getStatistics();
        
        Serial.println("\n=== 20fps Video Stream Status ===");
        Serial.printf("Camera - Total frames: %lu\n", stats.total_frames);
        Serial.printf("Camera - Dropped frames: %lu (%.2f%%)\n", 
                     stats.dropped_frames, 
                     stats.total_frames > 0 ? (stats.dropped_frames * 100.0f / stats.total_frames) : 0.0f);
        Serial.printf("Camera - Current FPS: %.2f\n", stats.current_fps);
        Serial.printf("Camera - Average frame size: %lu KB\n", stats.avg_frame_size / 1024);
        Serial.printf("Camera - Current frame: %ux%u, %lu bytes\n", fb->width, fb->height, fb->len);
        Serial.printf("Camera - Memory - Min heap: %lu KB, Current: %lu KB\n", 
                     stats.min_heap / 1024, ESP.getFreeHeap() / 1024);
        
        if (web_streaming_enabled) {
            Serial.printf("[WS] WebSocket clients: %d, streaming video frames\n", wsServer.getConnectedClients());
        }
        
        Serial.println("==================================\n");
        
        last_detailed_log = current_time;
    }
}

void handleSerialCommands() {
    if (!Serial.available()) return;
    
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "start") {
        streaming_enabled = true;
        Serial.println("[CMD] 10fps video streaming started");
    }
    else if (command == "stop") {
        streaming_enabled = false;
        Serial.println("[CMD] Video streaming stopped");
    }
    else if (command == "info") {
        camera.printCameraInfo();
        camera.printSystemStatus();
        
        FrameStats stats = camera.getStatistics();
        Serial.printf("[INFO] Streaming: %s\n", streaming_enabled ? "ON" : "OFF");
        Serial.printf("[INFO] Total frames: %lu\n", stats.total_frames);
        Serial.printf("[INFO] Current FPS: %.2f\n", stats.current_fps);
        Serial.printf("[INFO] Verbose logging: %s\n", verbose_logging ? "ON" : "OFF");
    }
    else if (command == "reset") {
        Serial.println("[CMD] Resetting camera...");
        camera.deinitialize();
        delay(1000);
        if (camera.initialize()) {
            Serial.println("[SUCCESS] Camera reset successful");
            streaming_enabled = true;
        } else {
            Serial.printf("[ERROR] Camera reset failed: %s\n", 
                         camera.getLastErrorMessage().c_str());
            streaming_enabled = false;
        }
    }
    else if (command == "stats") {
        camera.logDetailedStats();
    }
    else if (command == "verbose") {
        verbose_logging = !verbose_logging;
        Serial.printf("[CMD] Verbose logging: %s\n", verbose_logging ? "ON" : "OFF");
    }
    else if (command == "clear") {
        camera.resetStatistics();
        Serial.println("[CMD] Statistics cleared");
    }
    else if (command == "quality") {
        Serial.println("[CMD] Enter JPEG quality (0-63, lower=better): ");
        while (!Serial.available()) delay(10);
        int quality = Serial.parseInt();
        if (camera.setJpegQuality(quality)) {
            Serial.printf("[SUCCESS] JPEG quality set to %d\n", quality);
        } else {
            Serial.printf("[ERROR] Failed to set quality: %s\n", 
                         camera.getLastErrorMessage().c_str());
        }
    }
    else if (command == "status") {
        printSystemStatus();
    }
    else if (command == "webstats") {
        if (web_streaming_enabled) {
            Serial.printf("[WS] WebSocket server: RUNNING on port 8080\n");
            Serial.printf("[WS] Connected clients: %d/3\n", wsServer.getConnectedClients());
            Serial.println("[WS] Sending 20fps video frames to connected clients");
        } else {
            Serial.println("[ERROR] Web streaming not enabled");
        }
    }
    else if (command == "clients") {
        if (web_streaming_enabled) {
            Serial.printf("[WS] Active WebSocket clients: %d/3\n", wsServer.getConnectedClients());
        } else {
            Serial.println("[ERROR] Web streaming not enabled");
        }
    }
    else if (command == "webstart") {
        if (!web_streaming_enabled) {
            String wifi_ip = WiFi.softAPIP().toString();
            if (wsServer.start()) {
                web_streaming_enabled = true;
                Serial.printf("[SUCCESS] Native WebSocket server started at http://%s:8080/\n", wifi_ip.c_str());
            } else {
                Serial.println("[ERROR] Failed to start WebSocket server");
            }
        } else {
            Serial.println("[INFO] Web streaming already active");
        }
    }
    else if (command == "webstop") {
        if (web_streaming_enabled) {
            wsServer.stop();
            web_streaming_enabled = false;
            Serial.println("[SUCCESS] WebSocket server stopped");
        } else {
            Serial.println("[INFO] Web streaming already stopped");
        }
    }
    else if (command == "wstest") {
        if (web_streaming_enabled) {
            wsServer.sendStatusUpdate();
            Serial.println("[TEST] Sent test message to all WebSocket clients");
        } else {
            Serial.println("[ERROR] WebSocket server not running");
        }
    }
    else if (command == "help") {
        Serial.println("\n=== Available Commands ===");
        Serial.println("Camera Controls:");
        Serial.println("  start    - Start 20fps video streaming");
        Serial.println("  stop     - Stop video streaming");
        Serial.println("  reset    - Reset camera module");
        Serial.println("  quality  - Set JPEG quality (0-63)");
        Serial.println("");
        Serial.println("Information & Monitoring:");
        Serial.println("  info     - Show camera and system info");
        Serial.println("  stats    - Show detailed camera statistics");
        Serial.println("  status   - Show complete system status");
        Serial.println("  verbose  - Toggle verbose frame logging");
        Serial.println("  clear    - Clear statistics");
        Serial.println("");
        Serial.println("Web Streaming:");
        Serial.println("  webstats - Show web streaming statistics");
        Serial.println("  clients  - List connected web clients");
        Serial.println("  webstart - Start web streaming server");
        Serial.println("  webstop  - Stop web streaming server");
        Serial.println("  wstest   - Send test message to WebSocket clients");
        Serial.println("");
        Serial.println("  help     - Show this help");
        Serial.println("==========================\n");
    }
    else {
        Serial.printf("[ERROR] Unknown command: '%s'. Type 'help' for available commands.\n", 
                     command.c_str());
    }
}

void printSystemStatus() {
    Serial.println("\n=== System Status ===");
    Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
    Serial.printf("CPU Frequency: %lu MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size: %lu MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("Free Heap: %lu KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("Free PSRAM: %lu KB\n", ESP.getFreePsram() / 1024);
    Serial.printf("Camera Status: %s\n", camera.isInitialized() ? "Initialized" : "Not initialized");
    Serial.printf("Streaming: %s\n", streaming_enabled ? "Active" : "Inactive");
    Serial.printf("Target FPS: 20\n");
    Serial.println("=====================\n");
}
