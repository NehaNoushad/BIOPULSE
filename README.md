# BioPulse – Smart Biogas Monitoring System

## Overview

BioPulse is an ESP32-based biogas monitoring system designed to monitor gas sensor readings, temperature, and slurry fill level in a biogas tank. It uses an MQ-6 gas sensor, a DHT11 temperature sensor, and an HC-SR04 ultrasonic sensor to collect data and display it on a real-time web dashboard.

The ESP32 creates its own Wi-Fi access point, allowing users to access the dashboard through a smartphone or computer without requiring an internet connection.

## Components Used

- ESP32 development board
- MQ-6 gas sensor
- DHT11 temperature and humidity sensor
- HC-SR04 ultrasonic distance sensor
- Jumper wires
- Breadboard (if used)

## Features

- Real-time gas sensor readings
- Temperature monitoring
- Ultrasonic slurry-level estimation
- Threshold-based gas alerts
- High slurry-level warning
- Live web dashboard
- Wi-Fi access point hosted by the ESP32
- Offline monitoring without an internet connection
- Sensor readings displayed through a browser

## Working Principle

1. The MQ-6 sensor provides readings based on changes in the surrounding gas concentration.
2. The DHT11 measures the surrounding temperature.
3. The HC-SR04 measures the distance between the sensor and the slurry surface.
4. The ESP32 processes the sensor readings and estimates the slurry fill percentage using the configured tank height.
5. The ESP32 hosts a web dashboard that displays the readings and status indicators.
6. Users connect to the ESP32's Wi-Fi network and open the dashboard in a web browser to monitor the system.

## Software and Technologies

- Arduino IDE
- C++ (Arduino framework)
- HTML
- CSS
- JavaScript
- ESP32 Wi-Fi and WebServer libraries

## How to Use

1. Connect the sensors to the ESP32 according to the project wiring.
2. Open the Arduino code in the Arduino IDE.
3. Install the required DHT sensor libraries.
4. Upload the code to the ESP32.
5. Connect your smartphone or computer to the Wi-Fi network:

   - **SSID:** `Biogas_Monitor`
   - **Password:** `BioGas123`

6. Open a browser and visit:

   `http://192.168.4.1`

7. View the live sensor readings and system status on the BioPulse dashboard.

## Configuration

The project includes configurable settings for:

- Gas sensor alert threshold
- Tank height
- High slurry-level threshold
- Sensor reading interval

These settings should be adjusted and tested for the actual hardware and tank dimensions.

## Future Improvements

- Add an audible alarm for gas threshold alerts.
- Use a suitable sensor for more accurate methane measurement.
- Add data logging and historical charts.
- Improve sensor calibration and fault detection.
- Add remote monitoring and notifications.
- Use a waterproof temperature probe to measure slurry temperature directly.

## Safety and Limitations

- The MQ-6 is primarily designed for LPG detection and is not a certified methane or biogas safety detector.
- Gas sensor readings are raw ADC values and do not represent calibrated gas concentrations in ppm.
- The DHT11 measures ambient temperature and may not represent the temperature of the slurry inside the tank.
- The slurry fill percentage is an estimate based on the configured tank height and ultrasonic distance reading.
- Use appropriate voltage dividers to protect ESP32 GPIO pins from 5 V sensor outputs.

## Author

Elevator Pitchers - HACKVERSE'26
