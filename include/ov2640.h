// include/ov2640.h
#ifndef OV2640_H
#define OV2640_H

#include <Arduino.h>
#include <memory>
#include <functional>
#include <atomic>
#include <string>
#include "esp_camera.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Camera pin definitions for ESP32-S3
namespace CameraPins {
    constexpr int PWDN = -1;
    constexpr int RESET = -1;
    constexpr int XCLK = 15;
    constexpr int SIOD = 4;
    constexpr int SIOC = 5;
    constexpr int Y9 = 16;
    constexpr int Y8 = 17;
    constexpr int Y7 = 18;
    constexpr int Y6 = 12;
    constexpr int Y5 = 10;
    constexpr int Y4 = 8;
    constexpr int Y3 = 9;
    constexpr int Y2 = 11;
    constexpr int VSYNC = 6;
    constexpr int HREF = 7;
    constexpr int PCLK = 13;
}

// Camera configuration constants
namespace CameraConfig {
    constexpr uint32_t XCLK_FREQ_HZ = 20000000;  // Optimized for stable 20fps
    constexpr uint8_t TARGET_FPS = 20;
    constexpr uint32_t FRAME_INTERVAL_MS = 1000 / TARGET_FPS;  // 50ms
    constexpr uint8_t JPEG_QUALITY = 12;  // Balanced quality for 20fps stability
    constexpr uint8_t FRAME_BUFFER_COUNT = 2;  // Double buffering for 20fps
    constexpr size_t MIN_FREE_HEAP = 50000;  // Minimum heap threshold
}

// Frame statistics structure
struct FrameStats {
    uint32_t total_frames{0};
    uint32_t dropped_frames{0};
    uint32_t avg_frame_size{0};
    float current_fps{0.0f};
    uint32_t min_heap{UINT32_MAX};
    uint32_t max_frame_time{0};
    unsigned long last_reset_time{0};
    
    void reset() {
        total_frames = 0;
        dropped_frames = 0;
        avg_frame_size = 0;
        current_fps = 0.0f;
        min_heap = UINT32_MAX;
        max_frame_time = 0;
        last_reset_time = millis();
    }
};

// Camera error codes
enum class CameraError {
    NONE = 0,
    INIT_FAILED,
    SENSOR_NOT_FOUND,
    CAPTURE_FAILED,
    MEMORY_ALLOCATION_FAILED,
    INVALID_CONFIG,
    HARDWARE_ERROR
};

// Callback function type for frame processing
using FrameCallback = std::function<void(camera_fb_t*)>;

class OV2640Camera {
public:
    // Public methods for frame access
    camera_fb_t* getFrameBuffer();
    void returnFrameBuffer(camera_fb_t* fb);

private:
    camera_config_t config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> streaming_{false};
    FrameStats stats_;
    SemaphoreHandle_t stats_mutex_;
    
    // Performance tracking
    unsigned long last_frame_time_{0};
    unsigned long frame_start_time_{0};
    uint32_t frame_size_accumulator_{0};
    
    // Error handling
    CameraError last_error_{CameraError::NONE};
    std::string last_error_message_;
    
    // Private methods
    void initializeConfig();
    bool configureSensor();
    void updateStats(camera_fb_t* fb, unsigned long capture_time);
    bool checkMemoryConstraints() const;
    void logPerformanceWarning(const char* message) const;

public:
    explicit OV2640Camera();
    ~OV2640Camera();
    
    // Core functionality
    bool initialize();
    void deinitialize();
    
    // Frame operations
    std::unique_ptr<camera_fb_t, std::function<void(camera_fb_t*)>> captureFrame();
    bool captureFrameAsync(FrameCallback callback);
    
    // Configuration
    bool setFrameSize(framesize_t size);
    bool setJpegQuality(uint8_t quality);
    bool setPixelFormat(pixformat_t format);
    bool setGrayscaleMode(bool enable);  // НОВЫЙ МЕТОД: черно-белый режим
    
    // Status and diagnostics
    bool isInitialized() const noexcept { return initialized_.load(); }
    bool isStreaming() const noexcept { return streaming_.load(); }
    CameraError getLastError() const noexcept { return last_error_; }
    const std::string& getLastErrorMessage() const noexcept { return last_error_message_; }
    
    // Statistics
    FrameStats getStatistics() const;
    void resetStatistics();
    void logDetailedStats() const;
    void logFrameInfo(camera_fb_t* fb) const;
    
    // System information
    void printCameraInfo() const;
    void printSystemStatus() const;
    
    // Utility
    static const char* errorToString(CameraError error);
    static bool isValidFrameSize(framesize_t size);
    
    // Delete copy constructor and assignment operator
    OV2640Camera(const OV2640Camera&) = delete;
    OV2640Camera& operator=(const OV2640Camera&) = delete;
    
    // Move constructor and assignment operator
    OV2640Camera(OV2640Camera&&) = delete;
    OV2640Camera& operator=(OV2640Camera&&) = delete;
};

#endif // OV2640_H
