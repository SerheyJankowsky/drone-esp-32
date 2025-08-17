// include/flight_controller.h
#pragma once

#include <Arduino.h>

class FlightController {
public:
    FlightController();
    void initialize();
    void update();
    void testConnection();

private:
    HardwareSerial& fcSerial;
};
