# ESP32-S3 FPV Drone Camera

This project transforms an ESP32-S3 with an OV2640 camera into a high-performance FPV (First-Person View) drone camera, streaming smooth, full-screen video over Wi-Fi. It features a stable 30 FPS MJPEG stream, a robust Wi-Fi access point, and a comprehensive serial command interface for real-time diagnostics and control.

## 🚀 Key Features

- **High-Quality 30 FPS Video Stream:** Delivers a smooth and stable video feed, optimized for FPV applications.
- **MJPEG Streaming Server:** Utilizes an efficient MJPEG server to stream video directly to any web browser.
- **Full-Screen Display:** The HTML interface is designed to display the video stream in full-screen, dynamically adjusting to any screen size for an immersive experience.
- **Wi-Fi Access Point:** Creates its own Wi-Fi network, allowing for a direct and reliable connection without external routers.
- **Dual-Core Architecture:** Leverages both cores of the ESP32-S3 for maximum performance, dedicating one core to video processing and the other to network operations.
- **Interactive Serial Console:** Provides a rich set of commands for real-time debugging, status monitoring, and configuration changes.

## 🛠️ Hardware & Software

### Hardware

- **Board:** 4D Systems GEN4-ESP32 (ESP32-S3R8N16) or a compatible ESP32-S3 board with PSRAM.
- **Camera:** OV2640 Camera Module.

### Software

- **Framework:** Arduino
- **IDE:** PlatformIO
- **Libraries:**
  - `espressif/esp32-camera`: For camera interfacing.
  - `WebServer`: For the MJPEG streaming server.
  - `WiFi`: For creating the access point.

## 🏁 Getting Started

### Prerequisites

- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO IDE Extension](https://platformio.org/platformio-ide)

### Installation & Upload

1.  Clone this repository to your local machine.
2.  Open the project folder in Visual Studio Code with PlatformIO installed.
3.  Connect your ESP32-S3 board to your computer.
4.  Build and upload the firmware using the PlatformIO "Upload" task.

## 🛰️ Usage

1.  **Power On:** After uploading the firmware, power on your ESP32-S3 board.
2.  **Connect to Wi-Fi:** On your computer or mobile device, connect to the Wi-Fi network with the following credentials:
    - **SSID:** `ESP32-S3_Drone_30fps`
    - **Password:** `drone2024`
3.  **View Stream:** Open a web browser and navigate to the following address:
    - **URL:** `http://192.168.4.1`

The video stream will appear, filling the entire browser window.

## 💻 Serial Commands

Connect to the ESP32-S3's serial port using a terminal emulator (like the one in PlatformIO) at a baud rate of `115200` to access the command console. Type `help` to see a full list of available commands.

### System Commands

- `help`: Shows the help menu.
- `status`: Prints a detailed system status report.
- `restart`: Reboots the ESP32-S3.
- `memory`: Shows current memory usage.
- `uptime`: Displays the system uptime.

### Camera Commands

- `start`: Starts the video stream.
- `stop`: Stops the video stream.
- `reset`: Resets the camera module.
- `stats`: Shows detailed camera performance statistics.
- `quality <0-63>`: Sets the JPEG quality.
- `grayscale`: Switches to grayscale mode.
- `color`: Switches back to color mode.

### Wi-Fi Commands

- `wifi`: Shows the current Wi-Fi status.
- `wifireset`: Restarts the Wi-Fi module.
- `wificlients`: Shows a list of connected clients.
