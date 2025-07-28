#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <WiFi.h>
#include <esp_wifi.h>

class WiFiModule {
public:
    WiFiModule();
    void init(const char* ssid, const char* password);
    void start();
    void stop();
    bool isConnected() const;
    void checkStability();
    void optimizeForFPV();
    void scanNetworks();
    void showStatus() const;

private:
    String ssid_;
    String password_;
    unsigned long lastStabilityCheck_;
    
    static void wifiEventHandler(arduino_event_id_t event, arduino_event_info_t info);
    void showConnectedClients() const;
};

#endif