// src/flight_controller/flight_controller.cpp
#include "flight_controller.h"

FlightController::FlightController() : fcSerial(Serial1) {}

void FlightController::initialize() {
    // Initialize serial communication with the flight controller
    // Common baud rates for flight controllers are 115200 or 57600
    Serial1.begin(57600); // RX on GPIO 1, TX on GPIO 2
    Serial.println("Flight Controller serial initialized.");
}

void FlightController::update() {
    // Read data from the flight controller and process it
     while (Serial1.available()) {
      uint8_t c = Serial1.read();
      Serial.print("0x");
      Serial.print(c, HEX);
      Serial.print(" ");
    }
}

void FlightController::testConnection() {
   Serial.println("=== Отправка MSP запроса ===");
    
    uint8_t msp_request[] = {'$', 'M', '<', 0, 101, 101};
    Serial1.write(msp_request, sizeof(msp_request));
    
    Serial.print("Отправлено: ");
    for(int i = 0; i < sizeof(msp_request); i++) {
        Serial.print("0x");
        if(msp_request[i] < 16) Serial.print("0");
        Serial.print(msp_request[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    Serial.println("Ожидание ответа FC (3 сек)...");
    
    // Ждем ответ 3 секунды
    unsigned long start = millis();
    bool got_response = false;
    
    while(millis() - start < 3000) {
        if(Serial1.available()) {
            if(!got_response) {
                Serial.print("Ответ FC: ");
                got_response = true;
            }
            uint8_t byte = Serial1.read();
            Serial.print("0x");
            if(byte < 16) Serial.print("0");
            Serial.print(byte, HEX);
            Serial.print(" ");
        }
        delay(1);
    }
    
    if(got_response) {
        Serial.println(" <- СВЯЗЬ РАБОТАЕТ! ✅");
    } else {
        Serial.println("НЕТ ОТВЕТА - проверьте подключение ❌");
    }
    Serial.println("========================");
}
