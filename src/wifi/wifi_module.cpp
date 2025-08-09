#include "wifi_module.h"
#include <Arduino.h>

WiFiModule::WiFiModule() : lastStabilityCheck_(0) {}

void WiFiModule::init(const char* ssid, const char* password) {
    ssid_ = String(ssid);
    password_ = String(password);

    // Жесткий сброс WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(2000);
    
    // Инициализация в режиме AP
    if (!WiFi.mode(WIFI_AP)) {
        Serial.println("[WiFi] ❌ Не удалось установить режим AP");
        return;
    }

    // Конфигурация IP
    IPAddress ap_ip(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    if (!WiFi.softAPConfig(ap_ip, ap_ip, subnet)) {
        Serial.println("[WiFi] ❌ Ошибка конфигурации AP");
        return;
    }

    // Установка обработчика событий
    WiFi.onEvent(wifiEventHandler);
}

void WiFiModule::start() {
    Serial.println("[WiFi] Запуск точки доступа...");
    
    // Остановка предыдущего соединения
    WiFi.softAPdisconnect(true);
    delay(1000);
    
    // Запуск AP с оптимизированными параметрами
    if (!WiFi.softAP(ssid_.c_str(), NULL, 1, false, 4)) {
        Serial.println("[WiFi] ❌ Ошибка запуска AP!");
        return;
    }

    // Оптимизация для FPV
    optimizeForFPV();
    
    Serial.printf("[WiFi] Точка доступа запущена: %s\n", ssid_.c_str());
    Serial.printf("[WiFi] IP адрес: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("[WiFi] MAC адрес: %s\n", WiFi.softAPmacAddress().c_str());
}

void WiFiModule::optimizeForFPV() {
    // Минимальная задержка
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);
    
    // Фиксированный канал 1
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    
    // Allow 802.11b/g/n for better compatibility
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    
    // Максимальная мощность
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    esp_wifi_set_max_tx_power(84); // 21dBm
    
    // Ширина канала 20MHz
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
    
    Serial.println("[WiFi] Оптимизировано для FPV: минимальная задержка");
}

void WiFiModule::stop() {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("[WiFi] Точка доступа остановлена");
}

bool WiFiModule::isConnected() const {
    return WiFi.softAPgetStationNum() > 0;
}

void WiFiModule::checkStability() {
    if (millis() - lastStabilityCheck_ < 5000) return;
    lastStabilityCheck_ = millis();

    if (WiFi.getMode() != WIFI_AP) {
        Serial.println("[WiFi] Восстановление точки доступа...");
        start();
    }
}

void WiFiModule::scanNetworks() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    Serial.println("[WiFi] Сканирование сетей...");
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        Serial.println("[WiFi] Сети не найдены");
    } else {
        Serial.printf("[WiFi] Найдено %d сетей:\n", n);
        for (int i = 0; i < n; i++) {
            Serial.printf("%d: %s (Канал %d) %ddBm\n",
                i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
        }
    }
    
    WiFi.mode(WIFI_AP);
}

void WiFiModule::showStatus() const {
    Serial.println("\n=== WiFi Status ===");
    Serial.printf("SSID: %s\n", WiFi.softAPSSID().c_str());
    Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("MAC: %s\n", WiFi.softAPmacAddress().c_str());
    Serial.printf("Clients: %d\n", WiFi.softAPgetStationNum());
    Serial.printf("Channel: %d\n", WiFi.channel());
    Serial.println("==================\n");
}

void WiFiModule::wifiEventHandler(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("[WiFi] Точка доступа запущена");
            break;
            
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.printf("[WiFi] Клиент подключен: %02X:%02X:%02X:%02X:%02X:%02X\n",
                info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
                info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
                info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
            break;
            
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.printf("[WiFi] Клиент отключен: %02X:%02X:%02X:%02X:%02X:%02X\n",
                info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],
                info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],
                info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5]);
            break;
            
        default:
            break;
    }
}

void WiFiModule::showConnectedClients() const {
    wifi_sta_list_t clients;
    esp_wifi_ap_get_sta_list(&clients);
    
    Serial.printf("Подключено клиентов: %d\n", clients.num);
    for (int i = 0; i < clients.num; i++) {
        wifi_sta_info_t client = clients.sta[i];
        Serial.printf("Client %d: MAC %02X:%02X:%02X:%02X:%02X:%02X, RSSI: %ddBm\n",
            i+1, client.mac[0], client.mac[1], client.mac[2],
            client.mac[3], client.mac[4], client.mac[5], client.rssi);
    }
}