// include/wifi_module.h
#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <WiFi.h>
#include <String.h>

class WiFiModule {
private:
    String ssid_;
    String password_;
    unsigned long lastStabilityCheck_;
    
    // Static event handler to avoid lambda capture issues
    static void wifiEventHandler(arduino_event_id_t event, arduino_event_info_t info);
    
public:
    WiFiModule() : lastStabilityCheck_(0) {}
    ~WiFiModule();

    void init(const char* ssid, const char* password);
    void start();
    void stop();  // Add stop method for cleanup
    void checkStability(); // Проверка стабильности WiFi
    bool isStable(); // Проверка что WiFi стабилен
    void showConnectedClients(); // Показать подключенных клиентов
    void showStatus(); // Показать статус точки доступа
};

#endif
