# Cureous Labs – IoT Firmware Engineer Technical Assessment

This repository contains my implementation of the **IoT Firmware Engineer Technical Assessment** for Cureous Labs.

The assessment demonstrates practical experience in:

* Embedded C/C++
* ESP-IDF
* STM32 HAL
* FreeRTOS
* Wi-Fi connectivity
* MQTT
* NVS
* SNTP
* ADC sampling
* UART communication
* GPIO/EXTI interrupts
* FreeRTOS queues
* JSON data formatting
* Modular firmware architecture
* Error handling and debugging

---

## Repository Structure

```text
cureous-iot-firmware-assessment/
│
├── README.md
│
├── Task1_ESP32C6_IoT/
│   ├── CMakeLists.txt
│   ├── sdkconfig.defaults
│   └── main/
│       ├── CMakeLists.txt
│       ├── idf_component.yml
│       ├── main.c
│       ├── wifi_manager.c
│       ├── wifi_manager.h
│       ├── provisioning.c
│       ├── provisioning.h
│       ├── mqtt_manager.c
│       └── mqtt_manager.h
│
├── Task2_STM32_ADC/
│   ├── Core/
│   │   ├── Inc/
│   │   └── Src/
│   ├── Drivers/
│   └── STM32_Task2_ADC.ioc
│
└── Task3_STM32_FreeRTOS/
    ├── Core/
    │   ├── Inc/
    │   └── Src/
    ├── Drivers/
    └── STM32_Task3_FreeRTOS.ioc
```

---

# Task 1 – Native IoT Development using ESP32-C6

## Objective

Implement a native ESP-IDF IoT application running on an **ESP32-C6** with dynamic Wi-Fi provisioning, persistent credential storage, SNTP synchronization, MQTT telemetry, and automatic Wi-Fi reconnection.

## Features Implemented

* ESP-IDF based firmware
* Runtime Wi-Fi credential provisioning
* Wi-Fi credentials stored in NVS
* No hardcoded Wi-Fi credentials
* ESP32-C6 Wi-Fi station mode
* Automatic Wi-Fi reconnection
* SNTP time synchronization
* FreeRTOS heartbeat task
* MQTT connectivity
* JSON heartbeat payload
* Periodic heartbeat every 5 seconds
* Wi-Fi RSSI and uptime information
* Modular Wi-Fi, provisioning, and MQTT components

## Application Flow

```text
              ESP32-C6
                  │
                  ▼
       Check Wi-Fi credentials
                  │
          ┌───────┴───────┐
          │               │
       Found           Not Found
          │               │
          │               ▼
          │        Runtime Provisioning
          │               │
          │               ▼
          │          Save to NVS
          │               │
          └───────┬───────┘
                  ▼
           Connect to Wi-Fi
                  │
          ┌───────┴────────┐
          │                │
       Connected       Connection Lost
          │                │
          ▼                ▼
       SNTP Sync       Auto Reconnect
          │
          ▼
      MQTT Connect
          │
          ▼
   FreeRTOS Heartbeat Task
          │
          ▼
      JSON Payload
          │
          ▼
    MQTT Public Broker
```

## Example Heartbeat Payload

```json
{
    "device": "ESP32-C6",
    "uptime_sec": 25,
    "status": "online",
    "wifi_rssi": -48,
    "timestamp": "2026-10-04T10:30:25Z",
    "firmware": "1.0.0"
}
```

## Build Requirements

* ESP32-C6 development board
* ESP-IDF 6.x
* USB cable
* Windows/Linux development environment
* Python environment configured by ESP-IDF

## Build

Open an ESP-IDF terminal and navigate to the Task 1 directory:

```bash
cd Task1_ESP32C6_IoT
```

Set the ESP32-C6 target:

```bash
idf.py set-target esp32c6
```

Build:

```bash
idf.py build
```

## Flash

Connect the ESP32-C6 board through USB and run:

```bash
idf.py flash
```

## Monitor

```bash
idf.py monitor
```

To build, flash and monitor in one command:

```bash
idf.py flash monitor
```

## Wi-Fi Provisioning

On first boot, the firmware checks NVS for previously stored Wi-Fi credentials.

If credentials are not available, the firmware requests them through the configured runtime console.

Example:

```text
Enter Wi-Fi SSID:
MyWiFi

Enter Wi-Fi Password:
********
```

The credentials are stored in NVS and reused on subsequent boots.

Wi-Fi credentials are intentionally **not hardcoded in the source code**.

---

# Task 2 – STM32 HAL ADC Sensor Sampling & JSON UART Streaming

## Objective

Implement periodic ADC sampling using STM32 HAL, calculate a 10-sample moving average, and transmit the filtered value over UART.

## Features

* STM32 HAL based implementation
* ADC analog input
* Sampling interval: 100 ms
* Sampling frequency: 10 Hz
* 10-sample moving average filter
* UART communication
* UART baud rate: 115200
* Defensive ADC/UART error handling
* Modular filtering and transmission logic

## Application Flow

```text
Analog Sensor
     │
     ▼
   STM32 ADC
     │
     ▼
Sample every 100 ms
     │
     ▼
10-Sample Buffer
     │
     ▼
Moving Average Filter
     │
     ▼
Filtered ADC Value
     │
     ▼
JSON / Readable UART Output
```

## Moving Average

The moving average uses the latest 10 ADC samples:

```text
Average =
(Sample1 + Sample2 + ... + Sample10) / 10
```

For each new sample, the oldest sample is removed and the newest sample is added.

This reduces short-term noise in the sensor signal.

## Example UART Output

```text
ADC Raw: 2048
Moving Average: 2017
```

or:

```json
{"adc_raw":2048,"moving_average":2017}
```

## STM32 Configuration

The STM32 project is configured using STM32CubeMX / STM32CubeIDE.

Required peripherals:

```text
ADC
UART
Timer / periodic timing mechanism
GPIO
```

UART configuration:

```text
Baud Rate : 115200
Data Bits  : 8
Stop Bits  : 1
Parity     : None
```

## Build

Open the project using **STM32CubeIDE**.

Select:

```text
Project → Build Project
```

Connect the STM32 development board and program it using the STM32CubeIDE debugger/programmer.

Open a serial terminal with:

```text
Baud Rate : 115200
Data Bits : 8
Stop Bits : 1
Parity    : None
```

---

# Task 3 – STM32 FreeRTOS Task Synchronization & Logging

## Objective

Implement a producer-consumer architecture using **FreeRTOS queues**.

Two hardware switches generate external interrupts. The interrupt handler captures the button event and sends the event information to a FreeRTOS queue. A separate consumer task receives the event and transmits the information through UART.

## Architecture

```text
             Switch 1
                │
                ▼
              EXTI
                │
                │
             ISR
                │
                │
                ▼
        ┌────────────────┐
        │ FreeRTOS Queue  │
        └────────────────┘
                ▲
                │
             ISR
                │
              EXTI
                ▲
                │
             Switch 2

                │
                ▼
       Consumer / Logger Task
                │
                ▼
             UART
```

## Producer

The GPIO EXTI interrupt identifies the switch that generated the interrupt.

The event contains:

```text
Button ID
Timestamp
```

Example:

```json
{
    "button_id": 1,
    "timestamp": 12500
}
```

The event is then posted to the FreeRTOS queue.

The ISR does not perform blocking operations or UART transmission.

## Consumer

The UART logger task blocks on the FreeRTOS queue:

```text
Queue Receive
      │
      ▼
Event Received
      │
      ▼
Format JSON
      │
      ▼
Transmit UART
```

The task remains blocked when no event is available, avoiding unnecessary CPU usage.

## Queue Safety

The implementation handles:

* Queue full condition
* Queue receive timeout
* Invalid button events
* UART transmission errors
* Interrupt-safe queue operations

The interrupt context uses the appropriate FreeRTOS ISR API.

For example:

```c
xQueueSendFromISR()
```

instead of the normal task API.

## Example Output

```json
{"button_id":1,"timestamp":12500}
{"button_id":2,"timestamp":13842}
{"button_id":1,"timestamp":15201}
```

---

# Error Handling

The firmware implementations include defensive checks for important operations such as:

* Peripheral initialization
* ADC state
* UART transmission
* Wi-Fi connection
* MQTT connection
* NVS operations
* FreeRTOS queue operations
* Memory allocation
* Invalid configuration/input

Errors are logged where appropriate to simplify debugging.

---

# Design Approach

The projects are organized into separate modules instead of keeping the complete application inside a single source file.

For example, Task 1 separates:

```text
main.c
   │
   ├── provisioning.c
   │       └── NVS credential management
   │
   ├── wifi_manager.c
   │       └── Wi-Fi connection/reconnection
   │
   └── mqtt_manager.c
           └── MQTT connection/heartbeat
```

This separation improves:

* Readability
* Maintainability
* Testing
* Debugging
* Reusability

---

# Hardware Used

## Task 1

```text
ESP32-C6 Development Board
```

## Task 2

```text
STM32 Development Board
Analog input / sensor
USB-UART / onboard UART
```

## Task 3

```text
STM32 Development Board
Switch 1
Switch 2
UART terminal
```

---

# Development Tools

```text
ESP-IDF
STM32CubeIDE
STM32CubeMX
FreeRTOS
Git
GitHub
Visual Studio Code / ESP-IDF tools
```

---

# Testing

Each task was developed with a focus on verifying:

* Correct peripheral initialization
* Expected timing behavior
* Correct task synchronization
* Error handling
* UART output
* Network connectivity
* Reconnection behavior
* JSON formatting
* Resource safety

Hardware-specific configuration such as GPIO pins, ADC channels, UART instances, and board settings are documented in the respective project directories.

---

# Important Notes

### Wi-Fi Credentials

Wi-Fi credentials are not stored directly in the source code.

Do not add personal Wi-Fi credentials to GitHub.

### MQTT

Task 1 uses a public MQTT broker for demonstration purposes as permitted by the assessment.

Public brokers may have availability, rate-limit, or connectivity restrictions.

### Security

This project is intended as a technical assessment demonstration. A production deployment should additionally consider:

* MQTT TLS
* Certificate validation
* Secure credential storage / NVS encryption
* Authentication
* Secure OTA
* Watchdog configuration
* Production logging policy

---

# Author

**Thirumoorthi Murugan**

Embedded Software / Firmware Engineer

Skills demonstrated in this assessment:

```text
Embedded C/C++
ESP-IDF
ESP32-C6
STM32 HAL
FreeRTOS
UART
ADC
GPIO
EXTI
SPI
I2C
BLE
Wi-Fi
MQTT
NVS
SNTP
JSON
Git/GitHub
```

---

# Assessment Submission

This repository contains the implementation of the three requested assessment tasks:

```text
Task 1 → ESP32-C6 Native IoT Connectivity
Task 2 → STM32 ADC Sampling & UART Streaming
Task 3 → STM32 FreeRTOS Queue Synchronization & Logging
```

Build and flash instructions for each task are provided in the corresponding project directory.
