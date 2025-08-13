// src/commands/command_handler.cpp - Расширенные команды для диагностики
#include "command_handler.h"
#include "system_manager.h"

CommandHandler::CommandHandler() : systemManager(nullptr) {
}

void CommandHandler::processCommands() {
    if (!Serial.available()) return;
    
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command.isEmpty()) return;
    
    Serial.printf("[CMD] Выполняется команда: '%s'\n", command.c_str());
    processCommand(command);
}

void CommandHandler::processCommand(const String& command) {
    if (!systemManager) {
        Serial.println("[ERROR] ❌ SystemManager не инициализирован!");
        return;
    }
    
    // === СПРАВОЧНЫЕ КОМАНДЫ ===
    if (command == "help" || command == "?") {
        showHelp();
        return;
    }
    
    // === СИСТЕМНЫЕ КОМАНДЫ ===
    if (command == "status" || command == "info") {
        handleSystemCommands(command);
        return;
    }
    
    if (command == "restart" || command == "reboot") {
        Serial.println("[CMD] 🔄 Перезагрузка системы через 3 секунды...");
        delay(3000);
        ESP.restart();
        return;
    }
    
    if (command == "memory" || command == "mem") {
        showMemoryInfo();
        return;
    }
    
    if (command == "uptime") {
        showUptimeInfo();
        return;
    }
    
    // === КАМЕРА КОМАНДЫ ===
    if (command.startsWith("cam") || command == "start" || command == "stop" || 
        command == "reset" || command == "quality" || command == "stats" || 
        command == "verbose" || command == "clear" || command == "fps") {
        handleCameraCommands(command);
        return;
    }
    
    // === WiFi КОМАНДЫ ===
    if (command.startsWith("wifi") || command == "clients") {
        handleWiFiCommands(command);
        return;
    }
    
    // MJPEG commands
    if (command.startsWith("mjpeg")) {
        handleMJPEGCommands(command);
        return;
    }
    
    Serial.printf("[ERROR] Unknown command: '%s'. Type 'help' for available commands.\n", 
                 command.c_str());
}

void CommandHandler::handleCameraCommands(const String& command) {
    auto& camera = systemManager->getCamera();
    auto& taskManager = systemManager->getTaskManager();
    
    if (command == "start") {
        taskManager.enableVideoStreaming();
        Serial.println("[CMD] Video streaming started");
    }
    else if (command == "stop") {
        taskManager.disableVideoStreaming();
        Serial.println("[CMD] Video streaming stopped");
    }
    else if (command == "reset") {
        Serial.println("[CMD] Resetting camera...");
        camera.deinitialize();
        delay(1000);
        if (camera.initialize()) {
            Serial.println("[SUCCESS] Camera reset successful");
        } else {
            Serial.printf("[ERROR] Camera reset failed: %s\n", 
                         camera.getLastErrorMessage().c_str());
        }
    }
    else if (command == "stats") {
        camera.logDetailedStats();
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
    else if (command == "verbose") {
        taskManager.toggleVerboseLogging();
        Serial.printf("[CMD] Verbose logging: %s\n", 
                     taskManager.isVerboseLogging() ? "ON" : "OFF");
    }
    else if (command == "grayscale" || command == "bw") {
        Serial.println("[CMD] 🎬 Switching to GRAYSCALE mode...");
        if (camera.setGrayscaleMode(true)) {
            Serial.println("[SUCCESS] ✅ Grayscale mode enabled");
            Serial.println("[INFO] 📊 JPEG files will be 30-50% smaller");
            Serial.println("[INFO] 🚀 Better WebSocket stability with large scenes");
        } else {
            Serial.printf("[ERROR] ❌ Failed to enable grayscale: %s\n", 
                         camera.getLastErrorMessage().c_str());
        }
    }
    else if (command == "color" || command == "rgb") {
        Serial.println("[CMD] 🌈 Switching to COLOR mode...");
        if (camera.setGrayscaleMode(false)) {
            Serial.println("[SUCCESS] ✅ Color mode enabled");
            Serial.println("[INFO] 🎨 Full color video streaming");
            Serial.println("[WARNING] ⚠️  Larger JPEG files - may cause disconnects");
        } else {
            Serial.printf("[ERROR] ❌ Failed to enable color: %s\n", 
                         camera.getLastErrorMessage().c_str());
        }
    }
}

void CommandHandler::handleWiFiCommands(const String& command) {
    auto& wifi = systemManager->getWiFi();
    
    if (command == "wifi") {
        Serial.printf("[WiFi] 📡 SSID: ESP32-S3_Drone_30fps\n");
        Serial.printf("[WiFi] 🌐 IP: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("[WiFi] 👥 Connected clients: %d\n", WiFi.softAPgetStationNum());
        Serial.printf("[WiFi] 📶 Channel: %d\n", WiFi.channel());
        Serial.printf("[WiFi] 📊 Mode: %s\n", (WiFi.getMode() & WIFI_AP) ? "AP active" : "AP inactive");
        Serial.printf("[WiFi] 🔋 Power: %d dBm\n", WiFi.getTxPower());
        Serial.printf("[WiFi] ✅ Stability: %s\n", wifi.isConnected() ? "STABLE" : "ISSUES");
    }
    else if (command == "wifireset") {
        Serial.println("[WiFi] 🔄 Full WiFi configuration reset...");
        wifi.stop();
        delay(1000);
        wifi.init("ESP32-S3_Drone_30fps", "drone2024");
        delay(500);
        wifi.start();
        Serial.println("[WiFi] ✅ WiFi restarted. Try connecting again.");
    }
    else if (command == "wificlients") {
        wifi.checkStability();
    }
}

void CommandHandler::handleMJPEGCommands(const String& command) {
    if (command == "mjpegstatus") {
        Serial.println("[MJPEG] MJPEG server is RUNNING on port 80");
        Serial.println("[MJPEG] Stream URL: http://192.168.4.1/stream");
    }
}

void CommandHandler::handleSystemCommands(const String& command) {
    if (command == "status" || command == "info") {
        systemManager->printSystemStatus();
    }
}

void CommandHandler::showHelp() {
    Serial.println("\n🚁 ===== ESP32-S3 FPV DRONE CAMERA КОМАНДЫ =====");
    Serial.println();
    Serial.println("📷 УПРАВЛЕНИЕ КАМЕРОЙ:");
    Serial.println("  start         - ▶️  Запустить видео стриминг");
    Serial.println("  stop          - ⏹️  Остановить видео стриминг");  
    Serial.println("  reset         - 🔄 Перезапустить модуль камеры");
    Serial.println("  fps           - 📊 Показать текущий FPS");
    Serial.println("  quality       - 🎨 Установить качество JPEG (0-63)");
    Serial.println("  grayscale/bw  - 🎬 Черно-белый режим (меньше размер)");
    Serial.println("  color/rgb     - 🌈 Цветной режим (больше размер)");
    Serial.println("  stats         - 📈 Статистика камеры");
    Serial.println();
    Serial.println("🌐 СЕТЬ И ПОДКЛЮЧЕНИЯ:");
    Serial.println("  wifi          - 📶 Статус WiFi точки доступа"); 
    Serial.println("  clients       - 👥 Список подключенных клиентов");
    Serial.println("  ws            - 🔌 Статус WebSocket сервера");
    Serial.println();
    Serial.println("🖥️  СИСТЕМА И ДИАГНОСТИКА:");
    Serial.println("  status        - ℹ️  Полный статус системы");
    Serial.println("  memory        - 💾 Использование памяти");
    Serial.println("  uptime        - ⏱️  Время работы системы");
    Serial.println("  restart       - 🔄 Перезагрузка ESP32-S3");
    Serial.println();
    Serial.println("🛠️  ОТЛАДКА:");
    Serial.println("  verbose       - 🔍 Переключить подробные логи");
    Serial.println("  clear         - 🧹 Очистить экран");
    Serial.println("  help          - ❓ Показать эту справку");
    Serial.println();
    Serial.println("💡 ПРИМЕРЫ:");
    Serial.println("  > status      # Показать статус всех модулей");  
    Serial.println("  > fps         # Узнать текущую частоту кадров");
    Serial.println("  > grayscale   # Включить ч/б режим (стабильнее)");
    Serial.println("  > color       # Включить цветной режим");
    Serial.println("  > clients     # Сколько устройств подключено");
    Serial.println("  > memory      # Проверить свободную память");
    Serial.println();
    Serial.println("🌐 ПОДКЛЮЧЕНИЕ К ДРОНУ:");
    Serial.println("  WiFi:    ESP32-S3_Drone_30fps");
    Serial.println("  Пароль:  drone2024");
    Serial.println("  Браузер: http://192.168.4.1:8080");
    Serial.println("==================================================");
}

void CommandHandler::showMemoryInfo() {
    Serial.println("\n💾 ===== ИНФОРМАЦИЯ О ПАМЯТИ =====");
    
    // Heap память
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedHeap = totalHeap - freeHeap;
    float heapUsage = (float)usedHeap / totalHeap * 100;
    
    Serial.printf("📊 HEAP память:\n");
    Serial.printf("   Свободно: %lu KB (%lu bytes)\n", freeHeap / 1024, freeHeap);
    Serial.printf("   Занято:   %lu KB (%lu bytes)\n", usedHeap / 1024, usedHeap);
    Serial.printf("   Всего:    %lu KB (%lu bytes)\n", totalHeap / 1024, totalHeap);
    Serial.printf("   Загрузка: %.1f%%\n", heapUsage);
    
    // PSRAM память
    uint32_t freePsram = ESP.getFreePsram();
    uint32_t totalPsram = ESP.getPsramSize();
    uint32_t usedPsram = totalPsram - freePsram;
    float psramUsage = totalPsram > 0 ? (float)usedPsram / totalPsram * 100 : 0;
    
    Serial.printf("\n📊 PSRAM память:\n");
    if (totalPsram > 0) {
        Serial.printf("   Свободно: %lu MB (%lu KB)\n", freePsram / 1024 / 1024, freePsram / 1024);
        Serial.printf("   Занято:   %lu MB (%lu KB)\n", usedPsram / 1024 / 1024, usedPsram / 1024);
        Serial.printf("   Всего:    %lu MB (%lu KB)\n", totalPsram / 1024 / 1024, totalPsram / 1024);
        Serial.printf("   Загрузка: %.1f%%\n", psramUsage);
    } else {
        Serial.println("   ❌ PSRAM не обнаружена или не инициализирована");
    }
    
    // Flash память
    uint32_t flashSize = ESP.getFlashChipSize();
    Serial.printf("\n📊 FLASH память:\n");
    Serial.printf("   Размер:   %lu MB (%lu KB)\n", flashSize / 1024 / 1024, flashSize / 1024);
    
    // Рекомендации
    Serial.println("\n💡 РЕКОМЕНДАЦИИ:");
    if (heapUsage > 80) {
        Serial.println("   ⚠️  Высокое использование HEAP! Возможны сбои.");
    } else if (heapUsage > 60) {
        Serial.println("   ⚡ Умеренное использование HEAP.");
    } else {
        Serial.println("   ✅ Оптимальное использование HEAP.");
    }
    
    if (totalPsram > 0 && psramUsage > 80) {
        Serial.println("   ⚠️  Высокое использование PSRAM!");
    }
    
    Serial.println("=====================================");
}

void CommandHandler::showUptimeInfo() {
    unsigned long uptimeMs = millis();
    unsigned long seconds = uptimeMs / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    Serial.println("\n⏱️  ===== ВРЕМЯ РАБОТЫ СИСТЕМЫ =====");
    Serial.printf("🚀 Система работает: ");
    
    if (days > 0) {
        Serial.printf("%lu дн. ", days);
    }
    if (hours > 0) {
        Serial.printf("%lu ч. ", hours);
    }
    if (minutes > 0) {
        Serial.printf("%lu мин. ", minutes);
    }
    Serial.printf("%lu сек.\n", seconds);
    
    Serial.printf("📊 Всего миллисекунд: %lu\n", uptimeMs);
    Serial.printf("🔄 Частота CPU: %lu MHz\n", ESP.getCpuFreqMHz());
    
    // Статистика перезагрузок
    esp_reset_reason_t resetReason = esp_reset_reason();
    Serial.printf("🔄 Причина последней перезагрузки: ");
    switch (resetReason) {
        case ESP_RST_POWERON:
            Serial.println("Включение питания");
            break;
        case ESP_RST_EXT:
            Serial.println("Внешний сброс");
            break;
        case ESP_RST_SW:
            Serial.println("Программный сброс");
            break;
        case ESP_RST_PANIC:
            Serial.println("Паника/Exception");
            break;
        case ESP_RST_INT_WDT:
            Serial.println("Watchdog таймер");
            break;
        case ESP_RST_TASK_WDT:
            Serial.println("Task Watchdog");
            break;
        case ESP_RST_WDT:
            Serial.println("Другой Watchdog");
            break;
        case ESP_RST_DEEPSLEEP:
            Serial.println("Выход из Deep Sleep");
            break;
        case ESP_RST_BROWNOUT:
            Serial.println("Просадка питания");
            break;
        case ESP_RST_SDIO:
            Serial.println("SDIO сброс");
            break;
        default:
            Serial.printf("Неизвестная причина (%d)", resetReason);
            break;
    }
    
    Serial.println("=====================================");
}
