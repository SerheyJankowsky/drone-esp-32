#include "websocket_server.h"

WebSocketServer::WebSocketServer(uint16_t port) 
    : server_(port), port_(port), running_(false), frameCounter_(0), lastFrameTime_(0) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        wsConnected_[i] = false;
        lastPingTime_[i] = 0;
        frameSkipCount_[i] = 0;
    }
}

String WebSocketServer::calculateWebSocketAccept(const String& key) {
    String combined = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    
    unsigned char hash[20];
    mbedtls_sha1_context ctx;
    mbedtls_sha1_init(&ctx);
    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, (const unsigned char*)combined.c_str(), combined.length());
    mbedtls_sha1_finish(&ctx, hash);
    mbedtls_sha1_free(&ctx);
    
    char encoded[32];
    size_t olen;
    mbedtls_base64_encode((unsigned char*)encoded, sizeof(encoded), &olen, hash, 20);
    
    return String(encoded);
}

void WebSocketServer::sendWebSocketFrame(WiFiClient& client, const String& message) {
    if (!client.connected()) return;
    
    size_t len = message.length();
    uint8_t header[4];
    size_t headerLen = 2;
    
    header[0] = 0x81; // FIN + text frame
    
    if (len < 126) {
        header[1] = len;
        headerLen = 2;
    } else if (len < 65536) {
        header[1] = 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        headerLen = 4;
    }
    
    client.write(header, headerLen);
    client.write((const uint8_t*)message.c_str(), len);
}

void WebSocketServer::sendWebSocketBinaryFrame(WiFiClient& client, const uint8_t* data, size_t len) {
    if (!client.connected()) return;
    
    uint8_t header[10];
    size_t headerLen = 2;
    
    header[0] = 0x82; // FIN + binary frame
    
    if (len < 126) {
        header[1] = len;
        headerLen = 2;
    } else if (len < 65536) {
        header[1] = 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        headerLen = 4;
    } else {
        header[1] = 127;
        header[2] = 0;
        header[3] = 0;
        header[4] = 0;
        header[5] = 0;
        header[6] = (len >> 24) & 0xFF;
        header[7] = (len >> 16) & 0xFF;
        header[8] = (len >> 8) & 0xFF;
        header[9] = len & 0xFF;
        headerLen = 10;
    }
    
    // Send header first
    client.write(header, headerLen);
    
    // Send data in small chunks to prevent overwhelming
    const size_t CHUNK_SIZE = 512;
    size_t offset = 0;
    
    while (offset < len && client.connected()) {
        size_t chunkSize = min(CHUNK_SIZE, len - offset);
        client.write(data + offset, chunkSize);
        offset += chunkSize;
        
        // Small yield between chunks
        if (offset < len) {
            delayMicroseconds(10);
        }
    }
    
    // Don't call flush() - let TCP handle buffering naturally
}

bool WebSocketServer::sendWebSocketBinaryFrameSafe(WiFiClient& client, const uint8_t* data, size_t len) {
    if (!client.connected()) {
        return false;
    }
    
    // Validate data
    if (!data || len == 0) {
        return false;
    }
    
    // NO BUFFER CHECKING - JUST SEND IMMEDIATELY
    // Create WebSocket binary frame header (RFC 6455 compliant)
    uint8_t header[10];
    size_t headerLen = 2;
    
    // First byte: FIN=1 (0x80), RSV=000, Opcode=0x02 (binary)
    header[0] = 0x82;
    
    // Payload length encoding
    if (len < 126) {
        header[1] = (uint8_t)len;
        headerLen = 2;
    } else if (len < 65536) {
        header[1] = 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        headerLen = 4;
    } else {
        header[1] = 127;
        // 64-bit length (big-endian)
        header[2] = 0;
        header[3] = 0;
        header[4] = 0;
        header[5] = 0;
        header[6] = (len >> 24) & 0xFF;
        header[7] = (len >> 16) & 0xFF;
        header[8] = (len >> 8) & 0xFF;
        header[9] = len & 0xFF;
        headerLen = 10;
    }
    
    // Send header immediately - no checking
    client.write(header, headerLen);
    
    // Send all data immediately - no chunking, no checking, no delays
    client.write(data, len);
    
    // Force immediate transmission
    client.flush();
    
    return true; // Always return true - no error checking
}

bool WebSocketServer::start() {
    server_.begin();
    running_ = true;
    Serial.printf("[WS] Native WebSocket server started on port %d\n", port_);
    return true;
}

void WebSocketServer::stop() {
    // Close all WebSocket connections
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (wsConnected_[i]) {
            wsClients_[i].stop();
            wsConnected_[i] = false;
            frameSkipCount_[i] = 0;
        }
    }
    server_.end();
    running_ = false;
    Serial.println("[WS] Native WebSocket server stopped");
}

void WebSocketServer::handleClients() {
    if (!running_) return;
    
    // Handle new connections
    WiFiClient newClient = server_.available();
    if (newClient) {
        Serial.printf("[WS] New client connected from %s\n", newClient.remoteIP().toString().c_str());
        
        // Set client timeout to prevent hanging
        newClient.setTimeout(3000);
        
        // Read HTTP request with timeout
        String request = "";
        unsigned long timeout = millis() + 5000; // Increased timeout
        bool headersComplete = false;
        
        while (newClient.connected() && millis() < timeout && !headersComplete) {
            if (newClient.available()) {
                String line = newClient.readStringUntil('\n');
                request += line + "\n";
                
                // Check for end of headers
                if (line == "\r" || line.length() <= 1) {
                    headersComplete = true;
                    break;
                }
            }
            delay(1); // Small delay to prevent busy waiting
        }
        
        Serial.printf("[WS] Received request:\n%s\n", request.c_str());
        
        // Check if it's a WebSocket upgrade request
        if (request.indexOf("Upgrade: websocket") >= 0 || request.indexOf("Upgrade: WebSocket") >= 0) {
            handleWebSocketUpgrade(newClient, request);
        } else {
            // Send HTML page with WebSocket client
            sendWebPage(newClient);
        }
    }
    
    // Handle existing WebSocket clients and cleanup disconnected ones
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (wsConnected_[i]) {
            if (!wsClients_[i].connected()) {
                Serial.printf("[WS] Client %d disconnected\n", i);
                wsConnected_[i] = false;
                wsClients_[i].stop();
                frameSkipCount_[i] = 0;
            } else {
                // Send periodic ping to keep connection alive
                if (millis() - lastPingTime_[i] > 30000) { // Every 30 seconds
                    sendPing(wsClients_[i]);
                    lastPingTime_[i] = millis();
                }
            }
        }
    }
}

void WebSocketServer::handleWebSocketUpgrade(WiFiClient& client, const String& request) {
    // Find available slot
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!wsConnected_[i]) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        Serial.println("[WS] No available slots for new WebSocket connection");
        client.println("HTTP/1.1 503 Service Unavailable");
        client.println("Connection: close");
        client.println();
        client.stop();
        return;
    }
    
    // Extract WebSocket key with better parsing
    int keyStart = request.indexOf("Sec-WebSocket-Key:");
    if (keyStart == -1) {
        Serial.println("[WS] Missing Sec-WebSocket-Key header");
        client.println("HTTP/1.1 400 Bad Request");
        client.println("Connection: close");
        client.println();
        client.stop();
        return;
    }
    
    keyStart += 18; // Length of "Sec-WebSocket-Key:"
    while (keyStart < request.length() && request.charAt(keyStart) == ' ') keyStart++; // Skip spaces
    int keyEnd = request.indexOf("\r", keyStart);
    if (keyEnd == -1) keyEnd = request.indexOf("\n", keyStart);
    
    String wsKey = request.substring(keyStart, keyEnd);
    wsKey.trim();
    
    if (wsKey.length() == 0) {
        Serial.println("[WS] Empty WebSocket key");
        client.println("HTTP/1.1 400 Bad Request");
        client.println("Connection: close");
        client.println();
        client.stop();
        return;
    }
    
    String wsAccept = calculateWebSocketAccept(wsKey);
    
    Serial.printf("[WS] WebSocket key: %s\n", wsKey.c_str());
    Serial.printf("[WS] WebSocket accept: %s\n", wsAccept.c_str());
    
    // Send WebSocket handshake response
    client.println("HTTP/1.1 101 Switching Protocols");
    client.println("Upgrade: websocket");
    client.println("Connection: Upgrade");
    client.println("Sec-WebSocket-Accept: " + wsAccept);
    client.println();
    client.flush();
    
    // NO OPTIMIZATION - just basic connection setup
    // Remove all timeouts and delays that might interfere with immediate sending
    
    wsClients_[slot] = client;
    wsConnected_[slot] = true;
    lastPingTime_[slot] = millis();
    frameSkipCount_[slot] = 0;
    
    Serial.printf("[WS] WebSocket client %d connected successfully\n", slot);
    
    // Send welcome message
    String welcomeMsg = "ESP32-S3 Camera Ready - MAXIMUM THROUGHPUT MODE - NO FRAME SKIPPING";
    sendWebSocketFrame(wsClients_[slot], welcomeMsg);
}

void WebSocketServer::sendWebPage(WiFiClient& client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    
    client.println("<!DOCTYPE html>");
    client.println("<html><head><title>ESP32-S3 Drone Camera</title>");
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }");
    client.println(".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }");
    client.println("#video-container { text-align: center; margin: 20px 0; }");
    client.println("#video { max-width: 100%; height: auto; border: 3px solid #333; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.2); }");
    client.println("#status { margin: 10px 0; padding: 15px; background: linear-gradient(45deg, #4CAF50, #45a049); color: white; border-radius: 5px; text-align: center; font-weight: bold; }");
    client.println(".info { display: flex; justify-content: space-around; margin: 20px 0; }");
    client.println(".info-box { padding: 10px; background: #e3f2fd; border-radius: 5px; text-align: center; min-width: 100px; }");
    client.println("</style></head>");
    client.println("<body>");
    client.println("<div class='container'>");
    client.println("<h1>🚁 ESP32-S3 Drone Camera - MAXIMUM THROUGHPUT</h1>");
    client.println("<div id='status'>Connecting to WebSocket...</div>");
    client.println("<div class='info'>");
    client.println("<div class='info-box'><strong>Frame:</strong><br><span id='frame-count'>0</span></div>");
    client.println("<div class='info-box'><strong>FPS:</strong><br><span id='fps-display'>0</span></div>");
    client.println("<div class='info-box'><strong>Mode:</strong><br><span id='quality'>NO SKIP</span></div>");
    client.println("</div>");
    client.println("<div id='video-container'>");
    client.println("<img id='video' src='data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7' alt='Video Stream'/>");
    client.println("</div>");
    client.println("</div>");
    client.println("<script>");
    client.println("let ws;");
    client.println("const video = document.getElementById('video');");
    client.println("const status = document.getElementById('status');");
    client.println("const frameCount = document.getElementById('frame-count');");
    client.println("const fpsDisplay = document.getElementById('fps-display');");
    client.println("let frames = 0;");
    client.println("let startTime = Date.now();");
    client.println("let lastFrameTime = Date.now();");
    client.println("");
    client.println("function connectWebSocket() {");
    client.println("  try {");
    client.println("    ws = new WebSocket('ws://192.168.4.1:8080');");
    client.println("    ws.binaryType = 'arraybuffer';");
    client.println("    ");
    client.println("    ws.onopen = function() {");
    client.println("      console.log('WebSocket connected');");
    client.println("      status.innerHTML = '✅ Connected - MAXIMUM THROUGHPUT MODE';");
    client.println("      status.style.background = 'linear-gradient(45deg, #4CAF50, #45a049)';");
    client.println("    };");
    client.println("    ");
    client.println("    ws.onmessage = function(event) {");
    client.println("      if (event.data instanceof ArrayBuffer) {");
    client.println("        const blob = new Blob([event.data], {type: 'image/jpeg'});");
    client.println("        const url = URL.createObjectURL(blob);");
    client.println("        video.onload = () => URL.revokeObjectURL(url);");
    client.println("        video.src = url;");
    client.println("        frames++;");
    client.println("        frameCount.innerHTML = frames;");
    client.println("        const now = Date.now();");
    client.println("        const fps = Math.round(1000 / (now - lastFrameTime));");
    client.println("        fpsDisplay.innerHTML = fps > 0 ? fps : 0;");
    client.println("        lastFrameTime = now;");
    client.println("      } else {");
    client.println("        console.log('Text message:', event.data);");
    client.println("        status.innerHTML = '📝 ' + event.data;");
    client.println("      }");
    client.println("    };");
    client.println("    ");
    client.println("    ws.onclose = function(event) {");
    client.println("      console.log('WebSocket closed:', event.code, event.reason);");
    client.println("      status.innerHTML = '❌ Connection closed (Code: ' + event.code + ')';");
    client.println("      status.style.background = 'linear-gradient(45deg, #f44336, #d32f2f)';");
    client.println("      // Try to reconnect after 3 seconds");
    client.println("      setTimeout(connectWebSocket, 3000);");
    client.println("    };");
    client.println("    ");
    client.println("    ws.onerror = function(error) {");
    client.println("      console.log('WebSocket error:', error);");
    client.println("      status.innerHTML = '⚠️ Connection error - Retrying...';");
    client.println("      status.style.background = 'linear-gradient(45deg, #ff9800, #f57c00)';");
    client.println("    };");
    client.println("  } catch (error) {");
    client.println("    console.error('Failed to create WebSocket:', error);");
    client.println("    status.innerHTML = '❌ Failed to connect';");
    client.println("    setTimeout(connectWebSocket, 5000);");
    client.println("  }");
    client.println("}");
    client.println("");
    client.println("// Start connection");
    client.println("connectWebSocket();");
    client.println("</script>");
    client.println("</body></html>");
    
    client.flush();
    delay(10);
    client.stop();
}

void WebSocketServer::streamVideoFrame(camera_fb_t* fb) {
    if (!running_ || !fb) return;
    
    frameCounter_++;
    
    // NO FRAME RATE LIMITING - SEND IMMEDIATELY
    unsigned long currentTime = millis();
    lastFrameTime_ = currentTime;
    
    // Send frame to ALL connected clients WITHOUT ANY CHECKS
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (wsConnected_[i] && wsClients_[i].connected()) {
            // FORCE SEND - no buffer checking, no error handling, no skipping
            sendWebSocketBinaryFrameSafe(wsClients_[i], fb->buf, fb->len);
            // NO ERROR CHECKING - just send and move on
        }
    }
}

void WebSocketServer::sendPing(WiFiClient& client) {
    if (!client.connected()) return;
    
    // Send WebSocket ping frame
    uint8_t pingFrame[2] = {0x89, 0x00}; // Ping frame with no payload
    client.write(pingFrame, 2);
}

void WebSocketServer::sendWebSocketCloseFrame(WiFiClient& client, uint16_t code, const String& reason) {
    if (!client.connected()) return;
    
    size_t reasonLen = reason.length();
    size_t payloadLen = 2 + reasonLen; // 2 bytes for code + reason
    
    uint8_t header[4];
    size_t headerLen = 2;
    
    // Close frame: FIN=1, Opcode=0x8
    header[0] = 0x88;
    
    if (payloadLen < 126) {
        header[1] = (uint8_t)payloadLen;
        headerLen = 2;
    } else {
        header[1] = 126;
        header[2] = (payloadLen >> 8) & 0xFF;
        header[3] = payloadLen & 0xFF;
        headerLen = 4;
    }
    
    // Send header
    client.write(header, headerLen);
    
    // Send close code (big-endian)
    uint8_t codeBytes[2];
    codeBytes[0] = (code >> 8) & 0xFF;
    codeBytes[1] = code & 0xFF;
    client.write(codeBytes, 2);
    
    // Send reason if provided
    if (reasonLen > 0) {
        client.write((const uint8_t*)reason.c_str(), reasonLen);
    }
    
    client.flush();
    Serial.printf("[WS] Sent close frame with code %d: %s\n", code, reason.c_str());
}

bool WebSocketServer::isRunning() const {
    return running_;
}

int WebSocketServer::getConnectedClients() const {
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (wsConnected_[i]) count++;
    }
    return count;
}

int WebSocketServer::getTotalFrameSkips() const {
    int total = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        total += frameSkipCount_[i];
    }
    return total;
}

void WebSocketServer::sendStatusUpdate() {
    if (!running_) return;
    
    String statusMessage = String("{") +
        "\"type\":\"status\"," +
        "\"frame\":" + frameCounter_ + "," +
        "\"fps\":30," +
        "\"timestamp\":" + millis() + "," +
        "\"heap\":" + ESP.getFreeHeap() + "," +
        "\"clients\":" + getConnectedClients() +
        "}";
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (wsConnected_[i] && wsClients_[i].connected()) {
            sendWebSocketFrame(wsClients_[i], statusMessage);
        }
    }
}
