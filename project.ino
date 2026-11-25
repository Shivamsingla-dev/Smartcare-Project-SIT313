#include <WiFiNINA.h>
#include <DHT.h>
#include <Servo.h>

// ==================================================
// SMARTCARE - NANO 33 IoT WIRELESS SYSTEM
// Raspberry Pi TCP communication
// ==================================================

// ---------- WIFI SETTINGS ----------
char ssid[] = "Shivam";
char pass[] = "1234567890";

// Raspberry Pi server
IPAddress piIP(10, 26, 228, 231);
const int piPort = 5000;

WiFiClient client;

// ---------- PIN CONFIGURATION ----------
#define DHT_PIN 2
#define DHT_TYPE DHT22

#define PIR_PIN 3
#define TRIG_PIN 5
#define ECHO_PIN 6

#define RELAY_PIN 7
#define BUZZER_PIN 8
#define SERVO_PIN 9
#define SECURITY_BUTTON 10

#define LIGHT1 11
#define LIGHT2 12
#define RED_LED 13

#define LIGHT3 A4
#define LIGHT4 A5

#define LDR_PIN A0
#define GAS_PIN A1
#define WATER_PIN A2
#define SOIL_PIN A3

// ---------- THRESHOLDS ----------
#define DISTANCE_THRESHOLD 50
#define CLEAR_DISTANCE 60

#define LDR_THRESHOLD 500

#define GAS_FAN_THRESHOLD 560
#define GAS_ALARM_THRESHOLD 650

#define WATER_THRESHOLD 100

#define TEMP_FAN_THRESHOLD 28.0

#define GATE_OPEN_TIME 4000UL
#define FALSE_ALARM_TIME 2000UL
#define CLEAR_TIME 1000UL

// ---------- OBJECTS ----------
DHT dht(DHT_PIN, DHT_TYPE);
Servo gateServo;

// ---------- VARIABLES ----------
bool gateLocked = false;
bool gateOpen = false;
bool personLatch = false;

unsigned long gateOpenTime = 0;
unsigned long clearStartTime = 0;
bool clearingPerson = false;

bool falseAlarmProtection = true;
bool intrusionAlarm = false;
bool intrusionLatch = false;

unsigned long detectionStartTime = 0;
bool detectingPerson = false;

String lightMode = "AUTO";
bool roomLightsOn = false;

String fanMode = "AUTO";
bool fanOn = false;

bool manualBuzzer = false;

float temperature = -999;
float humidity = -999;

int gasValue = 0;
int waterValue = 0;
int soilValue = 0;
int lightValue = 0;

float distance = -1;

bool alarmActive = false;
String alarmReason = "NONE";

bool previousButtonState = HIGH;
unsigned long lastButtonTime = 0;

unsigned long lastDHTTime = 0;
unsigned long lastTelemetryTime = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastTCPAttempt = 0;

String incomingCommand = "";

// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(9600);

  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  pinMode(LIGHT1, OUTPUT);
  pinMode(LIGHT2, OUTPUT);
  pinMode(LIGHT3, OUTPUT);
  pinMode(LIGHT4, OUTPUT);

  pinMode(SECURITY_BUTTON, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(RED_LED, LOW);

  noTone(BUZZER_PIN);

  turnLightsOff();

  gateServo.attach(SERVO_PIN);
  gateServo.write(0);

  Serial.println("SMARTCARE STARTING");

  connectWiFi();

}

// ==================================================
// MAIN LOOP
// ==================================================

void loop() {

  maintainWiFi();
  maintainTCP();

  receiveCommands();

  readSensors();

  handleGate();

  handleLighting();

  handleFan();

  handleAlarm();

  handlePhysicalButton();

  sendTelemetry();

}

// ==================================================
// WIFI CONNECTION
// ==================================================

void connectWiFi() {

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 20000) {

    delay(500);
    Serial.print(".");

  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println();
    Serial.println("WiFi connected!");

    Serial.print("Nano IP: ");
    Serial.println(WiFi.localIP());

  } else {

    Serial.println();
    Serial.println("WiFi connection failed.");

  }

}

void maintainWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (millis() - lastWiFiAttempt >= 10000) {

    lastWiFiAttempt = millis();

    Serial.println("Reconnecting WiFi...");

    WiFi.begin(ssid, pass);

  }

}

// ==================================================
// TCP CONNECTION TO PI
// ==================================================

void maintainTCP() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (client.connected()) {
    return;
  }

  client.stop();

  if (millis() - lastTCPAttempt >= 5000) {

    lastTCPAttempt = millis();

    Serial.println("Connecting to Raspberry Pi...");

    if (client.connect(piIP, piPort)) {

      Serial.println("Raspberry Pi connected!");

      client.println("NANO_CONNECTED");

    } else {

      Serial.println("Pi connection failed.");

    }

  }

}

// ==================================================
// SENSOR READING
// ==================================================

void readSensors() {

  if (millis() - lastDHTTime >= 2000) {

    lastDHTTime = millis();

    float newTemperature = dht.readTemperature();
    float newHumidity = dht.readHumidity();

    if (!isnan(newTemperature) &&
        !isnan(newHumidity)) {

      temperature = newTemperature;
      humidity = newHumidity;

    }

  }

  gasValue = analogRead(GAS_PIN);
  waterValue = analogRead(WATER_PIN);
  soilValue = analogRead(SOIL_PIN);
  lightValue = analogRead(LDR_PIN);

  distance = readDistance();

}

// ==================================================
// ULTRASONIC
// ==================================================

float readDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration =
    pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.0343 / 2.0;

}

// ==================================================
// GATE CONTROL
// ==================================================

void handleGate() {

  bool personDetected =
    distance > 0 &&
    distance <= DISTANCE_THRESHOLD;

  bool personCleared =
    distance < 0 ||
    distance > CLEAR_DISTANCE;

  // Rearm after person leaves detection area
  if (personCleared) {

    if (!clearingPerson) {

      clearStartTime = millis();
      clearingPerson = true;

    }

    if (millis() - clearStartTime >= CLEAR_TIME) {

      personLatch = false;
      intrusionLatch = false;
      intrusionAlarm = false;
      detectingPerson = false;

    }

  } else {

    clearingPerson = false;

  }

  // LOCKED MODE
  if (gateLocked) {

    if (gateOpen) {

      gateServo.write(0);
      gateOpen = false;

    }

    if (personDetected && !intrusionLatch) {

      if (!detectingPerson) {

        detectingPerson = true;
        detectionStartTime = millis();

      }

      if (!falseAlarmProtection ||
          millis() - detectionStartTime >=
          FALSE_ALARM_TIME) {

        intrusionAlarm = true;
        intrusionLatch = true;
        detectingPerson = false;

        sendEvent("POTENTIAL_INTRUSION");

      }

    } else if (!personDetected) {

      detectingPerson = false;

    }

    return;

  }

  // NORMAL UNLOCKED MODE
  intrusionAlarm = false;

  if (personDetected &&
      !personLatch &&
      !gateOpen) {

    gateServo.write(90);

    gateOpen = true;
    personLatch = true;

    gateOpenTime = millis();

    sendEvent("GATE_OPENED");

  }

  if (gateOpen &&
      millis() - gateOpenTime >=
      GATE_OPEN_TIME) {

    gateServo.write(0);

    gateOpen = false;

    sendEvent("GATE_CLOSED");

  }

}

// ==================================================
// LIGHTS
// ==================================================

void handleLighting() {

  if (lightMode == "AUTO") {

    if (lightValue > LDR_THRESHOLD) {
      turnLightsOn();
    } else {
      turnLightsOff();
    }

  }

  else if (lightMode == "ON") {
    turnLightsOn();
  }

  else if (lightMode == "OFF") {
    turnLightsOff();
  }

}

void turnLightsOn() {

  digitalWrite(LIGHT1, HIGH);
  digitalWrite(LIGHT2, HIGH);
  digitalWrite(LIGHT3, HIGH);
  digitalWrite(LIGHT4, HIGH);

  roomLightsOn = true;

}

void turnLightsOff() {

  digitalWrite(LIGHT1, LOW);
  digitalWrite(LIGHT2, LOW);
  digitalWrite(LIGHT3, LOW);
  digitalWrite(LIGHT4, LOW);

  roomLightsOn = false;

}

// ==================================================
// FAN
// ==================================================

void handleFan() {

  if (fanMode == "AUTO") {

    bool tempHigh =
      temperature != -999 &&
      temperature >= TEMP_FAN_THRESHOLD;

    bool gasHigh =
      gasValue >= GAS_FAN_THRESHOLD;

    fanOn = tempHigh || gasHigh;

  }

  else if (fanMode == "ON") {
    fanOn = true;
  }

  else if (fanMode == "OFF") {
    fanOn = false;
  }

  // Active LOW relay
  digitalWrite(RELAY_PIN,
               fanOn ? LOW : HIGH);

}

// ==================================================
// ALARM
// ==================================================

void handleAlarm() {

  bool waterAlert =
    waterValue >= WATER_THRESHOLD;

  bool gasAlert =
    gasValue >= GAS_ALARM_THRESHOLD;

  alarmActive =
    intrusionAlarm ||
    waterAlert ||
    gasAlert;

  if (intrusionAlarm) {
    alarmReason = "INTRUSION";
  }

  else if (waterAlert) {
    alarmReason = "WATER";
  }

  else if (gasAlert) {
    alarmReason = "GAS";
  }

  else {
    alarmReason = "NONE";
  }

  digitalWrite(RED_LED,
               alarmActive ? HIGH : LOW);

  if (alarmActive || manualBuzzer) {
    tone(BUZZER_PIN, 1000);
  } else {
    noTone(BUZZER_PIN);
  }

}

// ==================================================
// PHYSICAL BUTTON
// ==================================================

void handlePhysicalButton() {

  bool buttonState =
    digitalRead(SECURITY_BUTTON);

  if (buttonState == LOW &&
      previousButtonState == HIGH &&
      millis() - lastButtonTime > 300) {

    lastButtonTime = millis();

    gateLocked = !gateLocked;

    if (gateLocked) {

      gateServo.write(0);
      gateOpen = false;

      sendEvent("GATE_LOCKED");

    } else {

      intrusionAlarm = false;

      sendEvent("GATE_UNLOCKED");

    }

  }

  previousButtonState = buttonState;

}

// ==================================================
// RECEIVE PI COMMANDS
// ==================================================

void receiveCommands() {

  if (!client.connected()) {
    return;
  }

  while (client.available()) {

    char c = client.read();

    if (c == '\n') {

      incomingCommand.trim();

      processCommand(incomingCommand);

      incomingCommand = "";

    }

    else if (c != '\r') {

      incomingCommand += c;

    }

  }

}

void processCommand(String command) {

  Serial.print("Command received: ");
  Serial.println(command);

  if (command == "LOCK") {

    gateLocked = true;
    gateOpen = false;

    gateServo.write(90);

    sendAck("LOCK");

  }

  else if (command == "UNLOCK") {

    gateLocked = false;
    intrusionAlarm = false;
    detectingPerson = false;

    sendAck("UNLOCK");

  }

  else if (command == "ALARM_RESET") {

    intrusionAlarm = false;

    sendAck("ALARM_RESET");

  }

  else if (command == "FALSE_PROTECTION_ON") {

    falseAlarmProtection = true;

    sendAck("FALSE_PROTECTION_ON");

  }

  else if (command == "FALSE_PROTECTION_OFF") {

    falseAlarmProtection = false;

    sendAck("FALSE_PROTECTION_OFF");

  }

  else if (command == "LIGHT_ON") {

    lightMode = "ON";
    turnLightsOn();

    sendAck("LIGHT_ON");

  }

  else if (command == "LIGHT_OFF") {

    lightMode = "OFF";
    turnLightsOff();

    sendAck("LIGHT_OFF");

  }

  else if (command == "LIGHT_AUTO") {

    lightMode = "AUTO";

    sendAck("LIGHT_AUTO");

  }

  else if (command == "FAN_ON") {

    fanMode = "ON";
    fanOn = true;

    sendAck("FAN_ON");

  }

  else if (command == "FAN_OFF") {

    fanMode = "OFF";
    fanOn = false;

    sendAck("FAN_OFF");

  }

  else if (command == "FAN_AUTO") {

    fanMode = "AUTO";

    sendAck("FAN_AUTO");

  }

  else if (command == "BUZZER_ON") {

    manualBuzzer = true;

    sendAck("BUZZER_ON");

  }

  else if (command == "BUZZER_OFF") {

    manualBuzzer = false;

    sendAck("BUZZER_OFF");

  }

  else if (command == "STATUS") {

    sendTelemetry();

  }

  else {

    sendAck("UNKNOWN_COMMAND");

  }

}

// ==================================================
// TELEMETRY
// ==================================================

void sendTelemetry() {

  if (millis() - lastTelemetryTime < 1000) {
    return;
  }

  lastTelemetryTime = millis();

  String data = "DATA,";

  data += String(temperature, 1) + ",";
  data += String(humidity, 1) + ",";
  data += String(gasValue) + ",";
  data += String(waterValue) + ",";
  data += String(soilValue) + ",";
  data += String(distance, 1) + ",";
  data += String(lightValue) + ",";
  data += fanMode + ",";
  data += String(fanOn) + ",";
  data += lightMode + ",";
  data += String(roomLightsOn) + ",";
  data += String(gateLocked) + ",";
  data += String(gateOpen) + ",";
  data += String(alarmActive) + ",";
  data += alarmReason + ",";
  data += String(falseAlarmProtection);

  if (client.connected()) {
    client.println(data);
  }

  Serial.println(data);

}

// ==================================================
// EVENTS / ACK
// ==================================================

void sendEvent(String eventName) {

  if (client.connected()) {
    client.println("EVENT," + eventName);
  }

  Serial.println("EVENT," + eventName);

}

void sendAck(String command) {

  if (client.connected()) {
    client.println("ACK," + command);
  }

  Serial.println("ACK," + command);

}
