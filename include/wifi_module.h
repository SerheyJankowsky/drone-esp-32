// include/wifi_module.h
#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <WiFi.h>
#include <String.h>

class WiFiModule {
private:
    String ssid_;
    String password_;
    
    // Static event handler to avoid lambda capture issues
    static void wifiEventHandler(arduino_event_id_t event, arduino_event_info_t info);
    
public:
    WiFiModule() = default;
    ~WiFiModule();
    
    void init(const char* ssid, const char* password);
    void start();
    void stop();  // Add stop method for cleanup
};

#endif
