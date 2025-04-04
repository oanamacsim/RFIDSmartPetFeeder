# ESP32 Firmware for RFID Smart Pet Feeder

This repository contains the firmware for the **RFID Smart Pet Feeder**, a smart IoT device designed to automate pet feeding using RFID-based access control, weight monitoring, and remote control via a mobile app. The firmware is built for the ESP32 microcontroller and integrates multiple hardware components to deliver a seamless and reliable pet-feeding solution.

---

## Project Overview

The **RFID Smart Pet Feeder** is an IoT-enabled device that ensures controlled and automated feeding for pets. It uses RFID tags to identify authorized pets, dispenses food based on predefined schedules or manual commands, and monitors food levels using a weight sensor. The device connects to a home WiFi network, enabling remote control and monitoring via an iOS app.

This project demonstrates expertise in:
- **Embedded Systems Development** (ESP32, Arduino framework)
- **IoT Integration** (WiFi, HTTP APIs, remote control)
- **Hardware Interfacing** (RFID, motors, sensors)
- **Real-Time System Design** (scheduling, event handling)
- **Firmware Optimization** (memory management, power efficiency)

---

## Features

### Core Functionality
- **RFID Tag Scanning**: Detects and verifies RFID tags to allow or deny access to food.
- **Motor Control**: Manages a stepper motor for the trap door and a DC motor for food dispensing.
- **Weight Monitoring**: Tracks food levels in the bowl using an HX711 weight sensor.
- **WiFi Connectivity**: Connects to a home network for remote control and data synchronization via an iOS app.
- **Time-Based Feeding**: Dispenses food at scheduled times.

### Advanced Features
- **Over-the-Air Configuration**: Users can configure feeding schedules and RFID tags via an IOS mobile application.
- **Data Collection**: Feeding events, gate activity, and food weight are saved into a cloud MySQL database.
- **Fault Tolerance**: Handles WiFi disconnections gracefully by retrying connections and falling back to an access point mode to reconfigure the WiFi network address.

---

## Hardware Requirements

- **Microcontroller**: ESP32
- **RFID Reader**: RDM6300
- **Motors**:
  - Stepper motor (for trap door control)
  - DC motor with gearbox (for food dispensing)
- **Sensors**:
  - HX711 weight sensor (for food level monitoring)
- **Power Supply**: 9V battery or adapter
- **Relay Module**: For motor control

---

## Software Architecture

The firmware is modular and follows an object-oriented design. Key components include:

### Controllers
- **FeederController**: Manages the feeding process, including scheduling and dispensing.
- **GateController**: Controls the trap door using a stepper motor.
- **RFIDController**: Handles RFID tag scanning and validation.
- **WeightController**: Monitors food levels using the HX711 sensor.
- **WifiController**: Manages WiFi connectivity and communication with the remote server.
- **MemoryController**: Handles non-volatile storage for configuration data.

### Key Design Patterns
- **Modularity**: Each hardware component is managed by a dedicated controller class.
- **Asynchronous Communication**: Leverages `AsyncTCP` and `ESPAsyncWebServer` for non-blocking network operations.

---

## Setup Instructions

### Prerequisites
1. **Arduino IDE**: Download and install the [Arduino IDE](https://www.arduino.cc/).
2. **ESP32 Board Support**: Add ESP32 support to the Arduino IDE by following [this guide](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide/).
3. **Required Libraries**:
   - `ESPAsyncWebServer`
   - `AsyncTCP`
   - `HX711`
   - `RDM6300`

### Flashing the Firmware
1. Connect the ESP32 to your computer via USB.
2. Open the `FeederESP32Firmware.ino` file in the Arduino IDE.
3. Select the correct board and port from the `Tools` menu.
4. Click the `Upload` button to flash the firmware.

### Configuration
1. **WiFi Setup**: On first boot, the ESP32 creates a hotspot. Connect to it and configure your WiFi credentials via the web interface.
2. **RFID Tag Registration**: Use the iOS app to register RFID tags for your pets.
3. **Feeding Schedule**: Configure feeding times and quantities via the app or web interface.

---

## Technical Details

### Key Algorithms
- **Time Synchronization**: Uses NTP to synchronize the ESP32's internal clock with an external time server.
- **Weight Calibration**: Implements a calibration routine for the HX711 sensor to ensure accurate weight measurements.
- **RFID Validation**: Compares scanned RFID tags against a list of registered tags stored in non-volatile memory.

### Data Flow
1. **RFID Scanning**: The RFID reader scans tags and sends the data to the `RFIDController`.
2. **Tag Validation**: The `RFIDController` checks the tag against registered tags and triggers the `GateController` to open or close the trap door.
3. **Feeding Process**: The `FeederController` manages food dispensing based on schedules or manual commands.
4. **Data Logging**: Feeding events, weight changes, and gate activity are logged to a remote server via HTTP POST requests.

---

Feel free to explore the code and reach out with any questions or feedback!
