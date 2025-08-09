// src/http/mjpeg_server.cpp
#include "mjpeg_server.h"

MJPEGServer::MJPEGServer(int port) : server(port), camera(nullptr) {}

void MJPEGServer::start(OV2640Camera* cam) {
    this->camera = cam;
    server.on("/", HTTP_GET, [this]() {
        this->handleRoot();
    });
    server.on("/stream", HTTP_GET, [this]() {
        this->handleStream();
    });
    server.begin();
    Serial.println("MJPEG server started on port 80");
}

void MJPEGServer::handleClients() {
    server.handleClient();
}

void MJPEGServer::handleRoot() {
    server.send(200, "text/html", "<html><head><title>ESP32-S3 MJPEG Stream</title><style>body{margin:0;padding:0;background-color:#000;}img{width:100vw;height:100vh;object-fit:contain;}</style></head><body><img src='/stream'></body></html>");
}

void MJPEGServer::handleStream() {
    WiFiClient client = server.client();
    if (!client) {
        return;
    }

    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: multipart/x-mixed-replace; boundary=--frame\r\n";
    response += "Connection: close\r\n\r\n";
    server.sendContent(response);

    while (client.connected()) {
        camera_fb_t* fb = camera->getFrameBuffer();
        if (!fb) {
            Serial.println("Failed to get frame buffer");
            continue;
        }

        client.write("--frame\r\n");
        client.write("Content-Type: image/jpeg\r\n");
        client.write("Content-Length: ");
        client.print(fb->len);
        client.write("\r\n\r\n");
        client.write((const char*)fb->buf, fb->len);
        client.write("\r\n");

        camera->returnFrameBuffer(fb);

        // Enforce 30 FPS
        vTaskDelay(pdMS_TO_TICKS(33)); // 1000ms / 30fps = 33ms

        if (client.available() > 0) {
            client.readStringUntil('\r');
        }
    }
}
