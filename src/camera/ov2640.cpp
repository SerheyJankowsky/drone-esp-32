// src/camera/ov2640.cpp
#include "ov2640.h"
#include <algorithm>

static const char* TAG = "OV2640";

OV2640Camera::OV2640Camera() {
    stats_mutex_ = xSemaphoreCreateMutex();
    if (stats_mutex_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create statistics mutex");
    }
    initializeConfig();
    stats_.reset();
}

OV2640Camera::~OV2640Camera() {
    deinitialize();
    if (stats_mutex_ != nullptr) {
        vSemaphoreDelete(stats_mutex_);
    }
}

void OV2640Camera::initializeConfig() {
    // Initialize camera configuration with optimized settings for maximum frame rate
    config_.ledc_channel = LEDC_CHANNEL_0;
    config_.ledc_timer = LEDC_TIMER_0;
    config_.pin_d0 = CameraPins::Y2;
    config_.pin_d1 = CameraPins::Y3;
    config_.pin_d2 = CameraPins::Y4;
    config_.pin_d3 = CameraPins::Y5;
    config_.pin_d4 = CameraPins::Y6;
    config_.pin_d5 = CameraPins::Y7;
    config_.pin_d6 = CameraPins::Y8;
    config_.pin_d7 = CameraPins::Y9;
    config_.pin_xclk = CameraPins::XCLK;
    config_.pin_pclk = CameraPins::PCLK;
    config_.pin_vsync = CameraPins::VSYNC;
    config_.pin_href = CameraPins::HREF;
    config_.pin_sccb_sda = CameraPins::SIOD;
    config_.pin_sccb_scl = CameraPins::SIOC;
    config_.pin_pwdn = CameraPins::PWDN;
    config_.pin_reset = CameraPins::RESET;
    config_.xclk_freq_hz = 20000000; // Optimized clock frequency for stable 20fps
    config_.pixel_format = PIXFORMAT_JPEG;
    
    // Settings optimized for STABLE 20fps delivery
    config_.frame_size = FRAMESIZE_HD;  // 1280x720 - good balance between quality and performance
    config_.jpeg_quality = 25; // Increased compression for stable transmission at 20fps
    config_.fb_count = 3; // Triple buffer for stable processing at 20fps
    config_.fb_location = CAMERA_FB_IN_PSRAM;

    // Memory allocation - prefer PSRAM if available, otherwise use DRAM
    // if (ESP.getFreePsram() > 0) {
    //     config_.fb_location = CAMERA_FB_IN_PSRAM;
    //     ESP_LOGI(TAG, "Using PSRAM for frame buffers");
    // } else {
    //     config_.fb_location = CAMERA_FB_IN_DRAM;
    //     ESP_LOGW(TAG, "Using DRAM for frame buffers (no PSRAM available)");
    // }
    config_.grab_mode = CAMERA_GRAB_WHEN_EMPTY;  // Always get latest frame for real-time
}

bool OV2640Camera::initialize() {
    if (initialized_.load()) {
        ESP_LOGW(TAG, "Camera already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing OV2640 camera for stable 20fps operation...");
    
    if (!checkMemoryConstraints()) {
        last_error_ = CameraError::MEMORY_ALLOCATION_FAILED;
        last_error_message_ = "Insufficient memory for camera initialization";
        return false;
    }

    // Initialize the camera
    esp_err_t err = esp_camera_init(&config_);
    if (err != ESP_OK) {
        last_error_ = CameraError::INIT_FAILED;
        last_error_message_ = "esp_camera_init failed with error: " + std::to_string(err);
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return false;
    }
    
    if (!configureSensor()) {
        esp_camera_deinit();
        return false;
    }
    
    initialized_.store(true);
    stats_.reset();
    last_frame_time_ = millis();
    
    ESP_LOGI(TAG, "Camera initialized successfully for stable 20fps operation!");
    printCameraInfo();
    
    return true;
}

bool OV2640Camera::configureSensor() {
    sensor_t * s = esp_camera_sensor_get();
    if (s == nullptr) {
        last_error_ = CameraError::SENSOR_NOT_FOUND;
        last_error_message_ = "Failed to get sensor";
        ESP_LOGE(TAG, "Failed to get sensor");
        return false;
    }
    // Set sensor settings
    s->set_vflip(s, 1);       // Flip vertically
    s->set_hmirror(s, 0);     // No horizontal mirror
    s->set_brightness(s, 1);  //-2 to 2
    s->set_contrast(s, 1);    //-2 to 2
    s->set_saturation(s, 0);  //-2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - no effect)
    s->set_whitebal(s, 1);    // 0 = disable, 1 = enable
    s->set_awb_gain(s, 1);    // 0 = disable, 1 = enable
    s->set_wb_mode(s, 0);     // 0 to 4 (0 - auto)
    s->set_exposure_ctrl(s, 1); // 0 = disable, 1 = enable
    s->set_aec2(s, 0);        // 0 = disable, 1 = enable
    s->set_ae_level(s, 0);    // -2 to 2
    s->set_aec_value(s, 300); // 0 to 1200
    s->set_gain_ctrl(s, 1);   // 0 = disable, 1 = enable
    s->set_agc_gain(s, 0);    // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0); // 0 to 6
    s->set_bpc(s, 0);         // 0 = disable, 1 = enable
    s->set_wpc(s, 1);         // 0 = disable, 1 = enable
    s->set_raw_gma(s, 1);     // 0 = disable, 1 = enable
    s->set_lenc(s, 1);        // 0 = disable, 1 = enable
    return true;
}

camera_fb_t* OV2640Camera::getFrameBuffer() {
    if (!initialized_.load()) {
        return nullptr;
    }
    return esp_camera_fb_get();
}

void OV2640Camera::returnFrameBuffer(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

void OV2640Camera::deinitialize() {
    if (!initialized_.load()) {
        return;
    }
    
    streaming_.store(false);
    esp_camera_deinit();
    initialized_.store(false);
    
    ESP_LOGI(TAG, "Camera deinitialized");
}

std::unique_ptr<camera_fb_t, std::function<void(camera_fb_t*)>> OV2640Camera::captureFrame() {
    if (!initialized_.load()) {
        last_error_ = CameraError::CAPTURE_FAILED;
        last_error_message_ = "Camera not initialized";
        ESP_LOGW(TAG, "Camera not initialized");
        return nullptr;
    }
    
    frame_start_time_ = millis();
    
    // СТРОГОЕ ОГРАНИЧЕНИЕ 20 FPS на уровне камеры
    const unsigned long CAMERA_FRAME_INTERVAL_MS = 50; // 1000ms / 20fps = 50ms
    
    if (frame_start_time_ - last_frame_time_ < CAMERA_FRAME_INTERVAL_MS) {
        // Слишком рано для следующего кадра - возвращаем nullptr
        return nullptr;
    }
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        last_error_ = CameraError::CAPTURE_FAILED;
        last_error_message_ = "esp_camera_fb_get failed";
        
        if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
            stats_.dropped_frames++;
            xSemaphoreGive(stats_mutex_);
        }
        
        ESP_LOGW(TAG, "Frame capture failed");
        return nullptr;
    }
    
    // ПРОВЕРКА РАЗМЕРА КАДРА для стабильной работы на 20fps
    const size_t MAX_SAFE_FRAME_SIZE = 40960; // 40KB максимум для стабильности на 20fps
    
    if (fb->len > MAX_SAFE_FRAME_SIZE) {
        ESP_LOGW(TAG, "Large frame detected: %zu bytes (max safe: %zu)", fb->len, MAX_SAFE_FRAME_SIZE);
        
        // Возвращаем кадр и пробуем увеличить сжатие
        esp_camera_fb_return(fb);
        
        // Временно увеличиваем качество JPEG (больше сжатие)
        sensor_t* sensor = esp_camera_sensor_get();
        if (sensor) {
            int current_quality = sensor->status.quality;
            if (current_quality < 20) { // Увеличиваем сжатие
                sensor->set_quality(sensor, current_quality + 3);
                ESP_LOGI(TAG, "Increased JPEG compression to quality %d for 20fps stability", current_quality + 3);
            }
        }
        
        // Делаем повторный захват с большим сжатием
        fb = esp_camera_fb_get();
        if (!fb) {
            last_error_ = CameraError::CAPTURE_FAILED;
            last_error_message_ = "Retry frame capture failed";
            return nullptr;
        }
        
        ESP_LOGI(TAG, "Recompressed frame size: %zu bytes", fb->len);
    }
    
    unsigned long capture_time = millis() - frame_start_time_;
    updateStats(fb, capture_time);
    last_frame_time_ = frame_start_time_;
    
    // Return smart pointer with custom deleter
    return std::unique_ptr<camera_fb_t, std::function<void(camera_fb_t*)>>(
        fb, [](camera_fb_t* frame) {
            if (frame) {
                esp_camera_fb_return(frame);
            }
        }
    );
}

bool OV2640Camera::captureFrameAsync(FrameCallback callback) {
    if (!callback) {
        ESP_LOGW(TAG, "Invalid callback provided");
        return false;
    }
    
    auto frame = captureFrame();
    if (frame) {
        callback(frame.get());
        return true;
    }
    
    return false;
}

bool OV2640Camera::setFrameSize(framesize_t size) {
    if (!isValidFrameSize(size)) {
        last_error_ = CameraError::INVALID_CONFIG;
        last_error_message_ = "Invalid frame size";
        return false;
    }
    
    sensor_t* sensor = esp_camera_sensor_get();
    if (!sensor) {
        last_error_ = CameraError::SENSOR_NOT_FOUND;
        return false;
    }
    
    int result = sensor->set_framesize(sensor, size);
    if (result == 0) {
        config_.frame_size = size;
        ESP_LOGI(TAG, "Frame size changed to %d", size);
        return true;
    }
    
    last_error_ = CameraError::INVALID_CONFIG;
    last_error_message_ = "Failed to set frame size";
    return false;
}

bool OV2640Camera::setJpegQuality(uint8_t quality) {
    if (quality > 63) {
        last_error_ = CameraError::INVALID_CONFIG;
        last_error_message_ = "JPEG quality must be 0-63";
        return false;
    }
    
    sensor_t* sensor = esp_camera_sensor_get();
    if (!sensor) {
        last_error_ = CameraError::SENSOR_NOT_FOUND;
        return false;
    }
    
    int result = sensor->set_quality(sensor, quality);
    if (result == 0) {
        config_.jpeg_quality = quality;
        ESP_LOGI(TAG, "JPEG quality changed to %d", quality);
        return true;
    }
    
    last_error_ = CameraError::INVALID_CONFIG;
    last_error_message_ = "Failed to set JPEG quality";
    return false;
}

bool OV2640Camera::setPixelFormat(pixformat_t format) {
    if (format != PIXFORMAT_JPEG && format != PIXFORMAT_RGB565) {
        last_error_ = CameraError::INVALID_CONFIG;
        last_error_message_ = "Unsupported pixel format";
        return false;
    }
    
    sensor_t* sensor = esp_camera_sensor_get();
    if (!sensor) {
        last_error_ = CameraError::SENSOR_NOT_FOUND;
        return false;
    }
    
    int result = sensor->set_pixformat(sensor, format);
    if (result == 0) {
        config_.pixel_format = format;
        ESP_LOGI(TAG, "Pixel format changed");
        return true;
    }
    
    last_error_ = CameraError::INVALID_CONFIG;
    last_error_message_ = "Failed to set pixel format";
    return false;
}

bool OV2640Camera::setGrayscaleMode(bool enable) {
    if (!initialized_.load()) {
        last_error_ = CameraError::CAPTURE_FAILED;
        last_error_message_ = "Camera not initialized";
        ESP_LOGW(TAG, "Camera not initialized");
        return false;
    }
    
    sensor_t* sensor = esp_camera_sensor_get();
    if (!sensor) {
        last_error_ = CameraError::SENSOR_NOT_FOUND;
        last_error_message_ = "Failed to get camera sensor";
        return false;
    }
    
    if (enable) {
        // Включаем черно-белый режим для уменьшения размера файла
        ESP_LOGI(TAG, "🎬 Enabling GRAYSCALE mode for smaller file sizes");
        
        // Убираем насыщенность цветов (делаем изображение серым)
        sensor->set_saturation(sensor, -2);    // Минимальная насыщенность
        sensor->set_special_effect(sensor, 2); // Grayscale special effect
        
        // Дополнительные настройки для лучшего черно-белого изображения
        sensor->set_contrast(sensor, 2);       // Увеличиваем контраст
        sensor->set_brightness(sensor, 0);     // Нормальная яркость
        
        ESP_LOGI(TAG, "✅ Grayscale mode enabled - expect 30-50%% smaller JPEG files");
        
    } else {
        // Возвращаем цветной режим
        ESP_LOGI(TAG, "🌈 Enabling COLOR mode");
        
        sensor->set_saturation(sensor, 0);     // Нормальная насыщенность
        sensor->set_special_effect(sensor, 0); // Без эффектов
        sensor->set_contrast(sensor, 1);       // Нормальный контраст
        sensor->set_brightness(sensor, 0);     // Нормальная яркость
        
        ESP_LOGI(TAG, "✅ Color mode enabled");
    }
    
    return true;
}

FrameStats OV2640Camera::getStatistics() const {
    FrameStats stats_copy;
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        stats_copy = stats_;
        xSemaphoreGive(stats_mutex_);
    }
    return stats_copy;
}

void OV2640Camera::resetStatistics() {
    if (xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        stats_.reset();
        xSemaphoreGive(stats_mutex_);
    }
    ESP_LOGI(TAG, "Statistics reset");
}

void OV2640Camera::logDetailedStats() const {
    FrameStats stats = getStatistics();
    
    ESP_LOGI(TAG, "=== Camera Performance Statistics ===");
    ESP_LOGI(TAG, "Total frames: %lu", stats.total_frames);
    ESP_LOGI(TAG, "Dropped frames: %lu (%.2f%%)", 
             stats.dropped_frames, 
             stats.total_frames > 0 ? (stats.dropped_frames * 100.0f / stats.total_frames) : 0.0f);
    ESP_LOGI(TAG, "Current FPS: %.2f", stats.current_fps);
    ESP_LOGI(TAG, "Average frame size: %lu bytes", stats.avg_frame_size);
    ESP_LOGI(TAG, "Min free heap: %lu bytes", stats.min_heap);
    ESP_LOGI(TAG, "Max frame time: %lu ms", stats.max_frame_time);
    ESP_LOGI(TAG, "Uptime: %lu seconds", (millis() - stats.last_reset_time) / 1000);
    ESP_LOGI(TAG, "=====================================");
}

void OV2640Camera::logFrameInfo(camera_fb_t* fb) const {
    if (!fb) {
        ESP_LOGW(TAG, "Invalid frame buffer");
        return;
    }
    
    FrameStats stats = getStatistics();
    uint32_t free_heap = ESP.getFreeHeap();
    uint32_t free_psram = ESP.getFreePsram();
    
    ESP_LOGI(TAG, "Frame #%lu: %ux%u, %u bytes, %.1f FPS | Heap: %lu, PSRAM: %lu", 
             stats.total_frames,
             fb->width, fb->height, fb->len,
             stats.current_fps,
             free_heap, free_psram);
             
    // Warning if memory is getting low
    if (free_heap < CameraConfig::MIN_FREE_HEAP) {
        logPerformanceWarning("Low heap memory detected");
    }
}

void OV2640Camera::printCameraInfo() const {
    sensor_t* sensor = esp_camera_sensor_get();
    if (!sensor) {
        ESP_LOGW(TAG, "Cannot get sensor information");
        return;
    }
    
    ESP_LOGI(TAG, "=== Camera Configuration ===");
    ESP_LOGI(TAG, "Sensor ID: 0x%02X", sensor->id.PID);
    
    // Get frame size dimensions
    const char* frame_size_names[] = {
        "96x96", "QQVGA", "QCIF", "HQVGA", "240x240", "QVGA", "CIF", "HVGA", "VGA",
        "SVGA", "XGA", "HD", "SXGA", "UXGA", "FHD", "P_HD", "P_3MP", "QXGA", 
        "QHD", "WQXGA", "P_FHD", "QSXGA"
    };
    
    if (config_.frame_size < sizeof(frame_size_names) / sizeof(frame_size_names[0])) {
        ESP_LOGI(TAG, "Frame size: %s", frame_size_names[config_.frame_size]);
    }
    
    ESP_LOGI(TAG, "JPEG quality: %d", config_.jpeg_quality);
    ESP_LOGI(TAG, "Frame buffers: %d", config_.fb_count);
    ESP_LOGI(TAG, "XCLK frequency: %lu Hz", config_.xclk_freq_hz);
    ESP_LOGI(TAG, "Target FPS: %d", CameraConfig::TARGET_FPS);
    ESP_LOGI(TAG, "Frame interval: %lu ms", CameraConfig::FRAME_INTERVAL_MS);
    ESP_LOGI(TAG, "Pixel format: %s", config_.pixel_format == PIXFORMAT_JPEG ? "JPEG" : "RAW");
    ESP_LOGI(TAG, "===========================");
}

void OV2640Camera::printSystemStatus() const {
    ESP_LOGI(TAG, "=== System Status ===");
    ESP_LOGI(TAG, "Camera initialized: %s", initialized_.load() ? "Yes" : "No");
    ESP_LOGI(TAG, "Streaming: %s", streaming_.load() ? "Yes" : "No");
    ESP_LOGI(TAG, "Free heap: %lu bytes", ESP.getFreeHeap());
    ESP_LOGI(TAG, "Free PSRAM: %lu bytes", ESP.getFreePsram());
    ESP_LOGI(TAG, "CPU frequency: %lu MHz", ESP.getCpuFreqMHz());
    ESP_LOGI(TAG, "Last error: %s", errorToString(last_error_));
    if (!last_error_message_.empty()) {
        ESP_LOGI(TAG, "Error message: %s", last_error_message_.c_str());
    }
    ESP_LOGI(TAG, "====================");
}

// Utility and helper methods
const char* OV2640Camera::errorToString(CameraError error) {
    switch (error) {
        case CameraError::NONE: return "No error";
        case CameraError::INIT_FAILED: return "Initialization failed";
        case CameraError::SENSOR_NOT_FOUND: return "Sensor not found";
        case CameraError::CAPTURE_FAILED: return "Capture failed";
        case CameraError::MEMORY_ALLOCATION_FAILED: return "Memory allocation failed";
        case CameraError::INVALID_CONFIG: return "Invalid configuration";
        case CameraError::HARDWARE_ERROR: return "Hardware error";
        default: return "Unknown error";
    }
}

bool OV2640Camera::isValidFrameSize(framesize_t size) {
    return size >= FRAMESIZE_96X96 && size <= FRAMESIZE_QSXGA;
}

// Private helper methods
void OV2640Camera::updateStats(camera_fb_t* fb, unsigned long capture_time) {
    if (!fb || xSemaphoreTake(stats_mutex_, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }
    
    stats_.total_frames++;
    frame_size_accumulator_ += fb->len;
    stats_.avg_frame_size = frame_size_accumulator_ / stats_.total_frames;
    
    // Update min heap tracking
    uint32_t current_heap = ESP.getFreeHeap();
    if (current_heap < stats_.min_heap) {
        stats_.min_heap = current_heap;
    }
    
    // Update max frame time
    if (capture_time > stats_.max_frame_time) {
        stats_.max_frame_time = capture_time;
    }
    
    // Calculate FPS (update every 10 frames for smoother reading)
    if (stats_.total_frames % 10 == 0) {
        unsigned long current_time = millis();
        if (current_time > last_frame_time_ && stats_.total_frames > 0) {
            float elapsed_seconds = (current_time - stats_.last_reset_time) / 1000.0f;
            stats_.current_fps = stats_.total_frames / elapsed_seconds;
        }
    }
    
    xSemaphoreGive(stats_mutex_);
}

bool OV2640Camera::checkMemoryConstraints() const {
    uint32_t free_heap = ESP.getFreeHeap();
    uint32_t free_psram = ESP.getFreePsram();
    
    if (free_heap < CameraConfig::MIN_FREE_HEAP) {
        ESP_LOGE(TAG, "Insufficient heap memory: %lu bytes (minimum: %lu)", 
                 free_heap, CameraConfig::MIN_FREE_HEAP);
        return false;
    }
    
    // PSRAM is preferred but not required
    if (free_psram > 0) {
        ESP_LOGI(TAG, "PSRAM available: %lu bytes - High quality mode enabled", free_psram);
    } else {
        ESP_LOGW(TAG, "No PSRAM detected - Using heap memory (reduced quality)");
        // Check if we have enough heap for basic camera operation
        if (free_heap < 200000) { // 200KB minimum for camera without PSRAM
            ESP_LOGE(TAG, "Insufficient memory for camera operation: %lu bytes", free_heap);
            return false;
        }
    }
    
    return true;
}

void OV2640Camera::logPerformanceWarning(const char* message) const {
    ESP_LOGW(TAG, "Performance Warning: %s", message);
}
