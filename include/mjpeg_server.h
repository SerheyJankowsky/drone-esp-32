// include/mjpeg_server.h
#pragma once

#include <WebServer.h>
#include "ov2640.h"

class MJPEGServer {
public:
    MJPEGServer(int port = 80);
    void start(OV2640Camera* cam);
    void handleClients();

private:
    void handleRoot();
    void handleStream();

    WebServer server;
    OV2640Camera* camera;
};
