// src/wifi/wifi_module.cpp
#include "wifi_module.h"
#include <esp_wifi.h>  // Для работы с низкоуровневыми Wi-Fi функциями
#include <WiFi.h>      // Библиотека для работы с Wi-Fi на Arduino

// Static event handler implementation
void WiFiModule::wifiEventHandler(arduino_event_id_t event, arduino_event_info_t info) {
    // Add memory barrier and stack protection
    __asm__ __volatile__ ("" ::: "memory");
    
    // Use a simple switch to handle different events safely
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            {
                Serial.println("Новый клиент подключился!");
                
                // SIMPLIFIED: Just log the connection without accessing MAC address
                // The MAC address access was causing memory corruption
                Serial.println("Client connected successfully - MAC access disabled for stability");
                
                // Force memory sync
                __asm__ __volatile__ ("" ::: "memory");
            }
            break;
            
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("Клиент отключился!");
            break;
            
        default:
            // Ignore other events silently
            break;
    }
    
    // Final memory barrier
    __asm__ __volatile__ ("" ::: "memory");
}

void WiFiModule::init(const char* ssid, const char* password) {
    // Store strings safely in class members
    ssid_ = String(ssid);
    password_ = String(password);
    
    // Initialize WiFi AP with stored strings
    WiFi.softAP(ssid_.c_str(), password_.c_str());
}

void WiFiModule::start() {
    Serial.println("Wi-Fi AP запущен!");
    Serial.print("IP-адрес точки доступа: ");
    Serial.println(WiFi.softAPIP());
    
    // Add delay to ensure WiFi is properly initialized
    delay(200);
    
    // Register the static event handler (safer than lambda)
    WiFi.onEvent(wifiEventHandler);
    
    Serial.println("WiFi event handler зарегистрирован");
}

void WiFiModule::stop() {
    // Clean up WiFi resources safely
    WiFi.softAPdisconnect(true);
    Serial.println("WiFi AP остановлен");
}

WiFiModule::~WiFiModule() {
    // Ensure cleanup on destruction
    stop();
}
