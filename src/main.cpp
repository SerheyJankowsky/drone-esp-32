// src/main.cpp - Minimized Dual-Core Entry Point
#include <Arduino.h>
#include "system_manager.h"
#include "command_handler.h"

SystemManager systemManager;
CommandHandler commandHandler;

void setup() {
    Serial.begin(115200);
    delay(3000); // Увеличена задержка для стабилизации
    
    Serial.println("\n============================================================");
    Serial.println("🚀 ESP32-S3 DRONE CAMERA SYSTEM - STARTING UP");
    Serial.println("============================================================");
    Serial.printf("💾 Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("🔄 Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("⚡ CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("📊 Free Heap: %lu bytes\n", ESP.getFreeHeap());
    Serial.printf("🔧 SDK Version: %s\n", ESP.getSdkVersion());
    
    // ПРОВЕРКА PSRAM (необязательно, но желательно)
    Serial.println("\n🧠 CHECKING PSRAM AVAILABILITY...");
    bool psramAvailable = psramInit();
    if (!psramAvailable) {
        Serial.println("⚠️  WARNING: PSRAM initialization failed or not available");
        Serial.println("📸 Camera may work with reduced quality/resolution");
        Serial.println("� For best performance, use ESP32-S3 board with PSRAM");
        Serial.printf("� Available heap: %lu bytes\n", ESP.getFreeHeap());
    } else {
        Serial.printf("✅ PSRAM initialized successfully: %lu bytes available\n", ESP.getFreePsram());
        Serial.printf("💾 Free heap after PSRAM init: %lu bytes\n", ESP.getFreeHeap());
    }
    
    Serial.println("\n============================================================");
    Serial.println("🎯 ESP32-S3 Drone Camera System v3.0 - Dual Core");
    Serial.println("============================================================");
    
    // Инициализация системного менеджера с детальными логами
    Serial.println("🔧 Initializing system manager...");
    if (!systemManager.initialize()) {
        Serial.println("❌ CRITICAL ERROR: System initialization FAILED!");
        Serial.println("🔄 System will restart in 5 seconds...");
        delay(5000);
        ESP.restart();
    }
    
    // Подключение обработчика команд
    Serial.println("🔌 Connecting command handler to system manager...");
    commandHandler.setSystemManager(&systemManager);
    
    Serial.println("\n============================================================");
    Serial.println("✅ SYSTEM INITIALIZED SUCCESSFULLY - Dual core operation active");
    Serial.println("📝 Type 'help' for available commands");
    Serial.println("🌐 Connect to WiFi: ESP32-S3_Drone_30fps (password: drone2024)");
    Serial.println("🔗 Web interface: http://192.168.4.1");
    Serial.println("============================================================");
}

void loop() {
    // System manager handles all operations
    systemManager.update();
    
    // Handle serial commands
    commandHandler.processCommands();
    
    // Minimal delay to prevent WDT reset
    delayMicroseconds(100);
}
