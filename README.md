# ASSIF System: IoT Bio-monitoring and Automated Water Purification 🐟💧

## Overview
The ASSIF system is an embedded IoT device designed to standardize the rearing conditions of biological models (such as *Danio rerio*) for ecotoxicology research. This automated system ensures stable water quality, eliminating experimental biases caused by environmental fluctuations.

## Features
* **Real-Time Monitoring:** Continuous tracking of temperature (°C) and Electrical Conductivity (µS/cm) with dynamic thermal compensation.
* **Automated Sterilization:** Daily automated water purification cycle (UV light + DC pump circulation) synchronized via an NTP server.
* **Cloud Telemetry:** Wi-Fi connectivity to a Blynk IoT dashboard for remote monitoring and manual override capabilities.

## Hardware Components & Wiring Guide
* **Microcontroller:** ESP32-C3 mini (3.3V Logic)
* **Temperature Probe:** DS18B20 (Waterproof)
* **Water Quality:** Analog TDS/EC Sensor
* **Display:** I2C OLED (128x64)
* **Actuators:** 5V Relay (Active LOW) for UV Lamp, 2N2222 NPN Transistor circuit for DC Mini Pump.

### Pinout Configuration
| Component | ESP32-C3 Pin | Notes / Additional Connections |
| :--- | :--- | :--- |
| **DS18B20 Temp Probe (Data)** | GPIO 5 | Requires a 4.7kΩ pull-up resistor between Data and 3.3V. |
| **Analog TDS/EC Sensor (Signal)**| GPIO 3 | Analog input (ADC). Powered by 3.3V. |
| **OLED Display (SDA)** | GPIO 8 | I2C Data line. |
| **OLED Display (SCL)** | GPIO 9 | I2C Clock line. |
| **UV Lamp Relay (IN/Signal)** | GPIO 10 | 5V Relay (Active LOW). Powered by 5V pin. |
| **DC Mini Pump (via 2N2222)** | GPIO 7 | Connected to the transistor's Base via a 100Ω resistor. Flyback diode (1N4007) across pump. |

## Software Architecture
Written in C++ (Arduino Framework), the code utilizes an asynchronous (non-blocking) architecture using `BlynkTimer`, allowing seamless multitasking between sensor polling, display updates, and cloud communication.


---
*Project developed as part of a Master's degree in Marine Environment and Bioresources Management.*
