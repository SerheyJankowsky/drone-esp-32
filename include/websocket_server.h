#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>
#include "esp_camera.h"

class WebSocketServer {
private:
    static const int MAX_CLIENTS = 3;
    
    WiFiServer server_;
    uint16_t port_;
    bool running_;
    WiFiClient wsClients_[MAX_CLIENTS];
    bool wsConnected_[MAX_CLIENTS];
    uint32_t frameCounter_;
    unsigned long lastFrameTime_;
    unsigned long lastPingTime_[MAX_CLIENTS];
    int frameSkipCount_[MAX_CLIENTS];
    
    String calculateWebSocketAccept(const String& key);
    void sendWebSocketFrame(WiFiClient& client, const String& message);
    void sendWebSocketBinaryFrame(WiFiClient& client, const uint8_t* data, size_t len);
    bool sendWebSocketBinaryFrameSafe(WiFiClient& client, const uint8_t* data, size_t len);
    void sendWebSocketCloseFrame(WiFiClient& client, uint16_t code = 1000, const String& reason = "");
    void handleWebSocketUpgrade(WiFiClient& client, const String& request);
    void sendWebPage(WiFiClient& client);
    void sendPing(WiFiClient& client);
    
public:
    WebSocketServer(uint16_t port = 8080);
    
    bool start();
    void stop();
    void handleClients();
    void streamVideoFrame(camera_fb_t* fb);
    void sendStatusUpdate();
    
    bool isRunning() const;
    int getConnectedClients() const;
    int getTotalFrameSkips() const; // New method to get total skip count
};

#endif
