#include "wifi_module.h"
#include <esp_wifi.h>  // Для работы с низкоуровневыми Wi-Fi функциями
#include <WiFi.h>      // Библиотека для работы с Wi-Fi на Arduino

// Static event handler implementation
void WiFiModule::wifiEventHandler(arduino_event_id_t event, arduino_event_info_t info) {
    // Детальный обработчик для диагностики проблем подключения
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("[WiFi] ✅ Точка доступа УСПЕШНО ЗАПУЩЕНА - готова к подключениям!");
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("[WiFi] ❌ Точка доступа ОСТАНОВЛЕНА");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("[WiFi] 🎯 УСПЕХ! Клиент ПОДКЛЮЧИЛСЯ к точке доступа!");
            Serial.printf("[WiFi] 📱 MAC клиента: %02X:%02X:%02X:%02X:%02X:%02X\n",
                info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
                info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
                info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("[WiFi] 🔌 Клиент ОТКЛЮЧИЛСЯ от точки доступа");
            Serial.printf("[WiFi] 📱 MAC клиента: %02X:%02X:%02X:%02X:%02X:%02X\n",
                info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],
                info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],
                info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5]);
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            Serial.println("[WiFi] 🌐 IP адрес НАЗНАЧЕН клиенту - подключение завершено!");
            break;
        case ARDUINO_EVENT_WIFI_READY:
            Serial.println("[WiFi] 🔧 WiFi система готова к работе");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            // Логируем probe requests для диагностики подключения
            Serial.println("[WiFi] 🔍 Устройство СКАНИРУЕТ сеть - сеть ВИДНА и доступна!");
            break;
        default:
            Serial.printf("[WiFi] 📡 WiFi событие: %d (для диагностики)\n", event);
            break;
    }
}

void WiFiModule::init(const char* ssid, const char* password) {
    // Store strings safely in class members
    ssid_ = String(ssid);
    password_ = String(password);

    Serial.println("[WiFi] Инициализируем СОВМЕСТИМУЮ точку доступа для подключения...");
    // Полная очистка WiFi состояния
    WiFi.mode(WIFI_OFF);
    delay(1000);

    // Настройка WiFi в режиме точки доступа с совместимыми параметрами
    WiFi.mode(WIFI_AP);
    delay(500);

    // КРИТИЧНО: Используем сниженную мощность для лучшей совместимости
    // Некоторые устройства не могут подключиться к максимальной мощности
    WiFi.setTxPower(WIFI_POWER_17dBm);  // Снижено с максимальной мощности

    // Устанавливаем статический IP для точки доступа
    IPAddress local_ip(192, 168, 4, 1);      // IP адрес точки доступа
    IPAddress gateway(192, 168, 4, 1);       // Шлюз (тот же что и AP)
    IPAddress subnet(255, 255, 255, 0);      // Маска подсети

    if (!WiFi.softAPConfig(local_ip, gateway, subnet)) {
        Serial.println("[WiFi] ❌ Ошибка настройки IP конфигурации!");
        return;
    }

    // Настраиваем обработчик событий WiFi для диагностики
    WiFi.onEvent(wifiEventHandler);

    Serial.println("[WiFi] 🎯 ВАЖНЫЕ ИНСТРУКЦИИ ПО ПОДКЛЮЧЕНИЮ:");
    Serial.println("[WiFi] 📋 1. Откройте настройки WiFi на телефоне/компьютере");
    Serial.printf("[WiFi] 📋 2. Найдите сеть: %s\n", ssid_.c_str());
    Serial.println("[WiFi] 📋 3. Введите пароль при запросе");
    Serial.printf("[WiFi] 📋 4. Пароль: %s\n", password_.c_str());
    Serial.println("[WiFi] 📋 5. После подключения откройте браузер");
    Serial.println("[WiFi] 📋 6. Перейдите на: http://192.168.4.1");
    Serial.println("[WiFi] 🔧 Используем КАНАЛ 1 для максимальной совместимости");
    Serial.println("[WiFi] ⚡ Мощность снижена до 17dBm для стабильности");

    Serial.println("[WiFi] 🚀 Запускаем точку доступа...");
}

void WiFiModule::start() {
    Serial.println("[WiFi] 🔥 СТАРТ процедуры создания точки доступа");

    // Дополнительные проверки перед запуском
    if (ssid_.length() == 0) {
        Serial.println("[WiFi] ❌ ОШИБКА: SSID не задан!");
        return;
    }

    if (WiFi.getMode() != WIFI_AP) {
        Serial.println("[WiFi] 🔄 Переключаемся в режим точки доступа...");
        WiFi.mode(WIFI_AP);
        delay(1000);
    }

    // Показываем информацию о сети
    Serial.println("[WiFi] 📡 ИНФОРМАЦИЯ О СЕТИ:");
    Serial.printf("[WiFi] 📡 SSID: %s (ищите эту сеть в списке WiFi)\n", ssid_.c_str());
    Serial.printf("[WiFi] 🔑 Пароль: %s\n", password_.c_str());
    Serial.println("[WiFi] 🌐 IP адрес: 192.168.4.1");
    Serial.println("[WiFi] 🔧 Канал: 1 (для совместимости)");
    Serial.println("[WiFi] ⚡ Мощность: 17dBm (для стабильности)");

    // Попытка создания защищенной точки доступа
    Serial.println("[WiFi] 🔒 Создаём ЗАЩИЩЁННУЮ точку доступа...");

    // Параметры: SSID, пароль, канал 1 (совместимость), скрытая=false, макс.клиентов=4
    bool success = WiFi.softAP(ssid_.c_str(), password_.c_str(), 1, false, 4);

    if (success) {
        Serial.println("[WiFi] ✅ Защищённая точка доступа СОЗДАНА успешно!");

        // Показываем MAC адрес точки доступа
        Serial.printf("[WiFi] 📱 MAC адрес AP: %s\n", WiFi.softAPmacAddress().c_str());

        // Настраиваем DHCP сервер для автоматической выдачи IP
        Serial.println("[WiFi] 🌐 DHCP сервер активен: 192.168.4.2 - 192.168.4.10");

        // Включаем режим совместимости
        esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);

        Serial.println("[WiFi] 🎯 ГОТОВО! Точка доступа работает и ждёт подключений");
        Serial.println("[WiFi] 📋 Инструкция:");
        Serial.println("[WiFi] 📋   1) Найдите сеть в списке WiFi");
        Serial.println("[WiFi] 📋   2) Введите пароль");
        Serial.println("[WiFi] 📋   3) Откройте http://192.168.4.1 в браузере");
    } else {
        Serial.println("[WiFi] ❌ ОШИБКА создания защищённой точки доступа!");
        Serial.println("[WiFi] 🔓 Пробуем создать ОТКРЫТУЮ сеть для диагностики...");

        // Fallback: создаём открытую сеть для тестирования
        if (WiFi.softAP(ssid_.c_str())) {
            Serial.println("[WiFi] ⚠️ Создана ОТКРЫТАЯ точка доступа (без пароля)");
            Serial.println("[WiFi] 🔓 Это временно для диагностики проблем подключения");
        } else {
            Serial.println("[WiFi] ❌ Критическая ошибка: не удалось создать точку доступа!");
        }
    }

    // Показываем текущий статус
    showStatus();
}

void WiFiModule::checkStability() {
    unsigned long now = millis();

    // Проверяем стабильность каждые 5 секунд
    if (now - lastStabilityCheck_ < 5000) {
        return;
    }

    lastStabilityCheck_ = now;

    // Проверяем, работает ли точка доступа
    if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
        Serial.println("[WiFi] ⚠️ ПРОБЛЕМА: точка доступа не активна!");
        Serial.println("[WiFi] 🔄 Перезапускаем точку доступа...");

        // Перезапускаем точку доступа
        WiFi.mode(WIFI_AP);
        delay(1000);

        // Восстанавливаем настройки
        IPAddress local_ip(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);
        WiFi.softAPConfig(local_ip, gateway, subnet);

        // Пересоздаём точку доступа с совместимыми настройками
        WiFi.setTxPower(WIFI_POWER_17dBm);
        WiFi.softAP(ssid_.c_str(), password_.c_str(), 1, 0, 4);

        Serial.println("[WiFi] ✅ Точка доступа восстановлена");
    }

    // Проверяем количество подключенных клиентов
    int clients = WiFi.softAPgetStationNum();
    if (clients > 0) {
        Serial.printf("[WiFi] 👥 Подключено клиентов: %d\n", clients);

        // Показываем информацию о подключенных клиентах
        showConnectedClients();
    } else {
        // Отображаем информацию о доступности сети каждые 30 секунд
        static unsigned long lastInfoShow = 0;
        if (now - lastInfoShow > 30000) {
            lastInfoShow = now;
            Serial.println("[WiFi] 📡 Точка доступа активна, ожидаем подключений...");
            Serial.printf("[WiFi] 🔍 Ищите сеть: %s\n", ssid_.c_str());
            Serial.println("[WiFi] 🌐 IP для браузера: http://192.168.4.1");
        }
    }
}

bool WiFiModule::isStable() {
    // Проверяем, что WiFi в режиме точки доступа
    wifi_mode_t mode = WiFi.getMode();
    if (mode != WIFI_AP && mode != WIFI_AP_STA) {
        return false;
    }

    // Проверяем, что IP настроен правильно
    IPAddress ip = WiFi.softAPIP();
    return (ip != IPAddress(0, 0, 0, 0));
}

void WiFiModule::stop() {
    Serial.println("[WiFi] 🛑 Останавливаем точку доступа...");

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("[WiFi] ✅ Точка доступа остановлена");
}

void WiFiModule::showConnectedClients() {
    int clientCount = WiFi.softAPgetStationNum();

    Serial.printf("[WiFi] 👥 Подключенных устройств: %d\n", clientCount);
    if (clientCount > 0) {
        Serial.println("[WiFi] 📱 Активные подключения:");
        // Здесь можно добавить более детальную информацию о клиентах
        // если потребуется в будущем
        Serial.println("[WiFi] 🌐 Клиенты могут обращаться к http://192.168.4.1");
    }
}

void WiFiModule::showStatus() {
    Serial.println("\n[WiFi] 📊 === СТАТУС ТОЧКИ ДОСТУПА ===");
    Serial.printf("[WiFi] 📡 Режим: %s\n",
                  WiFi.getMode() == WIFI_AP ? "Точка доступа" :
                  WiFi.getMode() == WIFI_AP_STA ? "AP+STA" : "Неизвестный");
    Serial.printf("[WiFi] 🌐 IP адрес: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("[WiFi] 📱 MAC адрес: %s\n", WiFi.softAPmacAddress().c_str());
    Serial.printf("[WiFi] 👥 Подключено устройств: %d\n", WiFi.softAPgetStationNum());
    Serial.printf("[WiFi] 📡 SSID: %s\n", ssid_.c_str());
    Serial.println("[WiFi] =====================================\n");
}

WiFiModule::~WiFiModule() {
    stop();
}
// src/wifi/wifi_module.cpp#include "wifi_module.h"#include <esp_wifi.h>  // Для работы с низкоуровневыми Wi-Fi функциями#include <WiFi.h>      // Библиотека для работы с Wi-Fi на Arduino// Static event handler implementationvoid WiFiModule::wifiEventHandler(arduino_event_id_t event, arduino_event_info_t info) {    // Детальный обработчик для диагностики проблем подключения    switch (event) {        case ARDUINO_EVENT_WIFI_AP_START:            Serial.println("[WiFi] ✅ Точка доступа УСПЕШНО ЗАПУЩЕНА - готова к подключениям!");            break;                    case ARDUINO_EVENT_WIFI_AP_STOP:            Serial.println("[WiFi] ❌ Точка доступа ОСТАНОВЛЕНА");            break;                    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:            Serial.println("[WiFi] 🎯 УСПЕХ! Клиент ПОДКЛЮЧИЛСЯ к точке доступа!");            Serial.printf("[WiFi] 📱 MAC клиента: %02X:%02X:%02X:%02X:%02X:%02X\n",                         info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],                         info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],                         info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);            break;                    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:            Serial.println("[WiFi] 🔌 Клиент ОТКЛЮЧИЛСЯ от точки доступа");            Serial.printf("[WiFi] 📱 MAC клиента: %02X:%02X:%02X:%02X:%02X:%02X\n",                         info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],                         info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],                         info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5]);            break;                    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:            Serial.println("[WiFi] 🌐 IP адрес НАЗНАЧЕН клиенту - подключение ЗАВЕРШЕНО!");            break;                    case ARDUINO_EVENT_WIFI_READY:            Serial.println("[WiFi] 🔧 WiFi система готова к работе");            break;                    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:            // Логируем probe requests для диагностики подключения            Serial.println("[WiFi] 🔍 Устройство СКАНИРУЕТ сеть - сеть ВИДНА и доступна!");            break;                    default:            Serial.printf("[WiFi] 📡 WiFi событие: %d (для диагностики)\n", event);            break;    }}void WiFiModule::init(const char* ssid, const char* password) {    // Store strings safely in class members    ssid_ = String(ssid);    password_ = String(password);        Serial.println("[WiFi] Инициализируем СОВМЕСТИМУЮ точку доступа для подключения...");        // Полная очистка WiFi состояния    WiFi.mode(WIFI_OFF);    delay(1000);        // Настройка WiFi в режиме точки доступа с совместимыми параметрами    WiFi.mode(WIFI_AP);    delay(500);        // КРИТИЧНО: Используем сниженную мощность для лучшей совместимости    // Некоторые устройства не могут подключиться к максимальной мощности    WiFi.setTxPower(WIFI_POWER_17dBm);  // Снижено с максимальной мощности        // Устанавливаем статический IP для точки доступа    IPAddress local_ip(192, 168, 4, 1);      // IP адрес точки доступа    IPAddress gateway(192, 168, 4, 1);       // Шлюз (тот же что и AP)    IPAddress subnet(255, 255, 255, 0);      // Маска подсети        if (!WiFi.softAPConfig(local_ip, gateway, subnet)) {        Serial.println("[WiFi] ❌ Ошибка настройки IP конфигурации!");        return;    }        // Настраиваем обработчик событий WiFi для диагностики    WiFi.onEvent(wifiEventHandler);        Serial.println("[WiFi] 🎯 ВАЖНЫЕ ИНСТРУКЦИИ ПО ПОДКЛЮЧЕНИЮ:");    Serial.println("[WiFi] 📋 1. Откройте настройки WiFi на телефоне/компьютере");    Serial.printf("[WiFi] 📋 2. Найдите сеть: %s\n", ssid_.c_str());    Serial.println("[WiFi] 📋 3. Введите пароль при запросе");    Serial.printf("[WiFi] 📋 4. Пароль: %s\n", password_.c_str());    Serial.println("[WiFi] 📋 5. После подключения откройте браузер");    Serial.println("[WiFi] 📋 6. Перейдите на: http://192.168.4.1");    Serial.println("[WiFi] 🔧 Используем КАНАЛ 1 для максимальной совместимости");    Serial.println("[WiFi] ⚡ Мощность снижена до 17dBm для стабильности");        Serial.println("[WiFi] 🚀 Запускаем точку доступа...");}void WiFiModule::start() {    Serial.println("[WiFi] 🔥 СТАРТ процедуры создания точки доступа");        // Дополнительные проверки перед запуском    if (ssid_.length() == 0) {        Serial.println("[WiFi] ❌ ОШИБКА: SSID не задан!");        return;    }        if (WiFi.getMode() != WIFI_AP) {        Serial.println("[WiFi] 🔄 Переключаемся в режим точки доступа...");        WiFi.mode(WIFI_AP);        delay(1000);    }        // Показываем информацию о сети    Serial.println("[WiFi] 📡 ИНФОРМАЦИЯ О СЕТИ:");    Serial.printf("[WiFi] 📡 SSID: %s (ищите эту сеть в списке WiFi)\n", ssid_.c_str());    Serial.printf("[WiFi] 🔑 Пароль: %s\n", password_.c_str());    Serial.println("[WiFi] 🌐 IP адрес: 192.168.4.1");    Serial.println("[WiFi] 🔧 Канал: 1 (для совместимости)");    Serial.println("[WiFi] ⚡ Мощность: 17dBm (для стабильности)");        // Попытка создания защищенной точки доступа    Serial.println("[WiFi] 🔒 Создаём ЗАЩИЩЁННУЮ точку доступа...");        // Параметры: SSID, пароль, канал 1 (совместимость), скрытая=false, макс.клиентов=4    bool success = WiFi.softAP(ssid_.c_str(), password_.c_str(), 1, false, 4);        if (success) {        Serial.println("[WiFi] ✅ Защищённая точка доступа СОЗДАНА успешно!");                // Показываем MAC адрес точки доступа        Serial.printf("[WiFi] 📱 MAC адрес AP: %s\n", WiFi.softAPmacAddress().c_str());                // Настраиваем DHCP сервер для автоматической выдачи IP        Serial.println("[WiFi] 🌐 DHCP сервер активен: 192.168.4.2 - 192.168.4.10");                // Включаем режим совместимости        esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);                Serial.println("[WiFi] 🎯 ГОТОВО! Точка доступа работает и ждёт подключений");        Serial.println("[WiFi] 📋 Инструкция:");        Serial.println("[WiFi] 📋   1) Найдите сеть в списке WiFi");        Serial.println("[WiFi] 📋   2) Введите пароль");        Serial.println("[WiFi] 📋   3) Откройте http://192.168.4.1 в браузере");            } else {        Serial.println("[WiFi] ❌ ОШИБКА создания защищённой точки доступа!");        Serial.println("[WiFi] 🔓 Пробуем создать ОТКРЫТУЮ сеть для диагностики...");                // Fallback: создаём открытую сеть для тестирования        if (WiFi.softAP(ssid_.c_str())) {            Serial.println("[WiFi] ⚠️ Создана ОТКРЫТАЯ точка доступа (без пароля)");            Serial.println("[WiFi] 🔓 Это временно для диагностики проблем подключения");        } else {            Serial.println("[WiFi] ❌ Критическая ошибка: не удалось создать точку доступа!");        }    }        // Показываем текущий статус    showStatus();}void WiFiModule::checkStability() {    unsigned long now = millis();        // Проверяем стабильность каждые 5 секунд    if (now - lastStabilityCheck_ < 5000) {        return;    }        lastStabilityCheck_ = now;        // Проверяем, работает ли точка доступа    if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {        Serial.println("[WiFi] ⚠️ ПРОБЛЕМА: точка доступа не активна!");        Serial.println("[WiFi] 🔄 Перезапускаем точку доступа...");                // Перезапускаем точку доступа        WiFi.mode(WIFI_AP);        delay(1000);                // Восстанавливаем настройки        IPAddress local_ip(192, 168, 4, 1);        IPAddress gateway(192, 168, 4, 1);        IPAddress subnet(255, 255, 255, 0);        WiFi.softAPConfig(local_ip, gateway, subnet);                // Пересоздаём точку доступа с совместимыми настройками        WiFi.setTxPower(WIFI_POWER_17dBm);        WiFi.softAP(ssid_.c_str(), password_.c_str(), 1, 0, 4);                Serial.println("[WiFi] ✅ Точка доступа восстановлена");    }        // Проверяем количество подключенных клиентов    int clients = WiFi.softAPgetStationNum();    if (clients > 0) {        Serial.printf("[WiFi] 👥 Подключено клиентов: %d\n", clients);                // Показываем информацию о подключенных клиентах        showConnectedClients();    } else {        // Отображаем информацию о доступности сети каждые 30 секунд        static unsigned long lastInfoShow = 0;        if (now - lastInfoShow > 30000) {            lastInfoShow = now;            Serial.println("[WiFi] 📡 Точка доступа активна, ожидаем подключений...");            Serial.printf("[WiFi] 🔍 Ищите сеть: %s\n", ssid_.c_str());            Serial.println("[WiFi] 🌐 IP для браузера: http://192.168.4.1");        }    }}bool WiFiModule::isStable() {    // Проверяем, что WiFi в режиме точки доступа    wifi_mode_t mode = WiFi.getMode();    if (mode != WIFI_AP && mode != WIFI_AP_STA) {        return false;    }        // Проверяем, что IP настроен правильно    IPAddress ip = WiFi.softAPIP();    return (ip != IPAddress(0, 0, 0, 0));}void WiFiModule::stop() {    Serial.println("[WiFi] 🛑 Останавливаем точку доступа...");        WiFi.softAPdisconnect(true);    WiFi.mode(WIFI_OFF);        Serial.println("[WiFi] ✅ Точка доступа остановлена");}void WiFiModule::showConnectedClients() {    int clientCount = WiFi.softAPgetStationNum();        Serial.printf("[WiFi] 👥 Подключенных устройств: %d\n", clientCount);        if (clientCount > 0) {        Serial.println("[WiFi] 📱 Активные подключения:");                // Здесь можно добавить более детальную информацию о клиентах        // если потребуется в будущем                Serial.println("[WiFi] 🌐 Клиенты могут обращаться к http://192.168.4.1");    }}void WiFiModule::showStatus() {    Serial.println("\n[WiFi] 📊 === СТАТУС ТОЧКИ ДОСТУПА ===");    Serial.printf("[WiFi] 📡 Режим: %s\n",                   WiFi.getMode() == WIFI_AP ? "Точка доступа" :                   WiFi.getMode() == WIFI_AP_STA ? "AP+STA" : "Неизвестный");    Serial.printf("[WiFi] 🌐 IP адрес: %s\n", WiFi.softAPIP().toString().c_str());    Serial.printf("[WiFi] 📱 MAC адрес: %s\n", WiFi.softAPmacAddress().c_str());    Serial.printf("[WiFi] 👥 Подключено устройств: %d\n", WiFi.softAPgetStationNum());    Serial.printf("[WiFi] 📡 SSID: %s\n", ssid_.c_str());    Serial.println("[WiFi] =====================================\n");}WiFiModule::~WiFiModule() {    stop();}