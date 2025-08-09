// include/command_handler.h - Расширенные команды для диагностики
#pragma once

#include <Arduino.h>

class SystemManager; // Forward declaration

class CommandHandler {
private:
    SystemManager* systemManager;
    
    // Command processing
    void processCommand(const String& command);
    void showHelp();
    
    // System commands
    void handleSystemCommands(const String& command);
    void showMemoryInfo();
    void showUptimeInfo();
    
    // Camera commands
    void handleCameraCommands(const String& command);
    
    // WiFi commands
    void handleWiFiCommands(const String& command);
    void handleMJPEGCommands(const String& command);

public:
    CommandHandler();
    
    void setSystemManager(SystemManager* manager) { systemManager = manager; }
    void processCommands();
};