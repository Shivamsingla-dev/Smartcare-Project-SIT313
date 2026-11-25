# SmartCare – Intelligent Assisted-Living Safety System

SmartCare is an embedded systems project designed to support assisted living through environmental monitoring, home automation, and remote device control.

The project combines an **Arduino Nano 33 IoT** and a **Raspberry Pi 4** to monitor environmental conditions, control connected devices, and display real-time information through a Python-based dashboard.

## Project Objectives

- Monitor room temperature and humidity.
- Monitor air-quality-related readings.
- Detect water using a water sensor.
- Monitor soil moisture.
- Automatically control room lighting based on surrounding brightness.
- Control a ventilation fan using a relay module.
- Control a gate mechanism using a servo motor.
- Provide audible alerts through a buzzer.
- Display sensor readings and device status on a central dashboard.
- Enable wireless communication between the Arduino and Raspberry Pi.

## Hardware Components

| Component | Purpose |
|---|---|
| Raspberry Pi 4 | Runs the Python server and dashboard |
| Arduino Nano 33 IoT | Sensor processing and hardware control |
| DHT22 | Temperature and humidity monitoring |
| MQ135 | Air-quality-related readings |
| Water sensor | Water detection |
| Soil moisture sensor | Soil moisture monitoring |
| LDR | Light detection |
| HC-SR04 | Distance measurement |
| 2-channel relay module | Fan control |
| DC fan | Ventilation |
| Servo motor | Gate mechanism |
| Buzzer | Audible alerts |
| LEDs | Room lighting and warnings |
| Push button | Physical security control |

## Software and Technologies

- Arduino IDE
- Arduino programming (C/C++)
- Python 3
- Tkinter
- TCP socket communication
- HTTP API
- Raspberry Pi OS
- Wi-Fi

## System Architecture

SmartCare uses two main processing devices:

**Arduino Nano 33 IoT**
- Reads sensor values.
- Processes sensor information.
- Controls connected hardware.
- Sends sensor data to the Raspberry Pi.
- Receives commands from the Raspberry Pi.

**Raspberry Pi 4**
- Runs the Python server.
- Receives data from the Arduino over Wi-Fi.
- Provides an HTTP API for the dashboard.
- Displays sensor readings and device status.
- Sends control commands to the Arduino.

### Communication Flow

```text
Sensors and Actuators
        |
        v
Arduino Nano 33 IoT
        |
        | Wi-Fi / TCP (Port 5000)
        v
Raspberry Pi 4
        |
        v
Python Server and HTTP API
        |
        | HTTP (Port 5001)
        v
Tkinter Dashboard
```

## Main Features

### 1. Environmental Monitoring
The system collects temperature, humidity, air-quality-related readings, water sensor values, and soil moisture readings.

### 2. Automatic Lighting
The LDR detects surrounding brightness. When the environment becomes dark, the Arduino automatically switches on the room LEDs.

### 3. Fan Control
The ventilation fan is connected through a relay module. It supports automatic and manual control.

### 4. Gate Control
A servo motor operates the gate mechanism. Users can lock and unlock the gate through the dashboard.

### 5. Audible Alerts
A buzzer provides audible alerts when required by the system.

### 6. Central Dashboard
The Python dashboard displays sensor readings and allows users to control selected devices from one interface.

## Repository Structure

```text
SmartCare/
│
├── Arduino/
│   └── smartcare.ino
│
├── RaspberryPi/
│   ├── server.py
│   └── dashboard.py
│
├── Documentation/
│   ├── architecture.png
│   └── project-report.pdf
│
└── README.md
```

*Update the folder and file names above to match the actual structure of your GitHub repository.*

## Setup and Installation

### Requirements

- Raspberry Pi 4 with Raspberry Pi OS
- Arduino Nano 33 IoT
- Python 3
- Arduino IDE
- Wi-Fi connection
- Required sensors and output components

### Step 1: Upload Arduino Code

1. Open the Arduino IDE.
2. Open the SmartCare Arduino sketch.
3. Select Arduino Nano 33 IoT as the board.
4. Configure the Wi-Fi and Raspberry Pi server details.
5. Upload the sketch to the Arduino.

### Step 2: Prepare the Raspberry Pi

Open the Raspberry Pi terminal and install Tkinter if it is not already installed:

```bash
sudo apt update
sudo apt install python3 python3-tk
```

### Step 3: Start the Python Server

Navigate to the project directory:

```bash
cd ~/smartcare-dashboard
```

Start the server:

```bash
python3 server.py
```

The server listens for Arduino data through TCP port `5000` and provides the dashboard API through port `5001`.

### Step 4: Start the Dashboard

Open another Raspberry Pi terminal:

```bash
cd ~/smartcare-dashboard
python3 dashboard.py
```

The SmartCare dashboard should now open and display incoming sensor information.

## Dashboard Controls

| Control | Description |
|---|---|
| Lock Gate | Locks the gate using the servo motor |
| Unlock Gate | Unlocks the gate using the servo motor |
| Reset Alarm | Resets the alarm |
| Lights ON | Manually switches room lights on |
| Lights OFF | Manually switches room lights off |
| Lights AUTO | Enables automatic lighting |
| Fan ON | Switches the fan on |
| Fan OFF | Switches the fan off |
| Fan AUTO | Enables automatic fan control |
| False Protection ON/OFF | Controls false-alarm protection |

## Testing and Evaluation

The following areas were tested during development:

- Sensor readings from the DHT22.
- MQ135 sensor readings.
- Water and soil moisture sensor readings.
- Automatic lighting using the LDR.
- Wireless communication between the Arduino and Raspberry Pi.
- Sensor data display on the Python dashboard.
- Gate servo lock and unlock operation.

The ultrasonic sensor produced invalid readings during some tests and requires further troubleshooting. The fan's physical operation should also be verified during final evaluation.

## Fault Tolerance and Limitations

SmartCare considers reliability through:

- Separating sensor processing from dashboard functionality.
- Monitoring the Arduino-to-Raspberry Pi connection.
- Identifying invalid sensor readings.
- Providing manual controls for selected devices.
- Including false-alarm protection controls.

The current prototype has limitations, including the need for further ultrasonic sensor testing, improved communication recovery, and longer-duration system testing.

## Future Improvements

- Add automatic reconnection after Wi-Fi interruptions.
- Improve invalid sensor detection and error handling.
- Store sensor readings for historical analysis.
- Add notifications for important environmental events.
- Improve the dashboard design.
- Perform longer reliability and performance tests.


## Author

**Shivam Singla**

SIT210 – Embedded Systems Development

## Acknowledgement

This project was developed as part of the SIT210 Embedded Systems Development unit to explore the integration of microcontrollers, sensors, actuators, wireless communication, and Python-based interfaces in a practical assisted-living application.
