// ================== PINS ==================//
// Stepper pins//

//Shoulder Stepper
#define STEP_X 17
#define DIR_X 16
#define LIMIT_X 19

//Elbow Stepper
#define STEP_Y 26
#define DIR_Y 25
#define LIMIT_Y 18

//Base Stepper
#define STEP_Z 14
#define DIR_Z 27
#define LIMIT_Z 23

//Rail Stepper
#define STEP_A 33
#define DIR_A 32
#define LIMIT_A 35

// Servo pins
#define WRIST_SERVO_PIN 2
#define GRIPPER_SERVO_PIN 15

// Conveyor + IR
#define IR_SENSOR 34
#define IN1 4
#define IN2 5
#define ENA 13

bool conveyorRunning = false;

#include <AccelStepper.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SDA_PIN 21
#define SCL_PIN 22

String lastUID = "";
unsigned long lastScanTime = 0;
const unsigned long scanCooldown = 500; // 3 seconds

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

const char* ssid = "Aero";
const char* password = "AeroKing2026";

// Shelf positions (Stepper A)
long shelfPosition = 2000;  // default

String currentUID = "";
String currentProduct = "";
String currentLocation = "";

// ================== OBJECTS ==================
Servo wristServo;
Servo gripperServo;

AccelStepper stepperX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper stepperY(AccelStepper::DRIVER, STEP_Y, DIR_Y);
AccelStepper stepperZ(AccelStepper::DRIVER, STEP_Z, DIR_Z);
AccelStepper stepperA(AccelStepper::DRIVER, STEP_A, DIR_A);

// ================== SETTINGS ==================
#define MAX_SPEED 4000
#define ACCELERATION 2500

bool homingDone = false;

// ================== FUNCTIONS ==================

void runAllSteppers() {
  stepperX.run();
  stepperY.run();
  stepperZ.run();
  stepperA.run();
}

bool isAnyMotorRunning() {
  return (stepperX.distanceToGo() != 0 ||
          stepperY.distanceToGo() != 0 ||
          stepperZ.distanceToGo() != 0 ||
          stepperA.distanceToGo() != 0);
}

void moveToPosition(long x, long y, long z, long a) {
  stepperX.moveTo(x);
  stepperY.moveTo(y);
  stepperZ.moveTo(z);
  stepperA.moveTo(a);

  while (isAnyMotorRunning()) {
    runAllSteppers();

    // Continuously monitor IR during every move
    bool objectPresent = (digitalRead(IR_SENSOR) == LOW);

    if (objectPresent && conveyorRunning) {
      stopConveyor();
      conveyorRunning = false;
    }

    if (!objectPresent && !conveyorRunning) {
      startConveyor();
      conveyorRunning = true;
    }
  }
}

//==============================PN532===================================//
bool initPN532() {
  for (int i = 0; i < 5; i++) {
    if (nfc.getFirmwareVersion()) {
      nfc.SAMConfig();
      Serial.println("PN532 Ready");
      return true;
    }
    Serial.println("Retrying PN532...");
    delay(500);
  }
  return false;
}

// ================= WIFI =================
void connectWiFi() {
  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ================= SEND DATA =================
void sendToServer(String uid, String product, String location) {

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin("http://192.168.100.55:3000/api/package");
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"uid\":\"" + uid + "\",";
    json += "\"product\":\"" + product + "\",";
    json += "\"location\":\"" + location + "\",";
    json += "\"status\":\"sorted\"";
    json += "}";

    http.setTimeout(2000); 
    int code = http.POST(json);

    Serial.print("HTTP Response: ");
    Serial.println(code);

    http.end();
  }
}
// ================= UID CONVERTER =================
String getUIDString(uint8_t *uid, uint8_t length) {
  String uidStr = "";
  for (uint8_t i = 0; i < length; i++) {
    if (uid[i] < 0x10) uidStr += "0";
    uidStr += String(uid[i], HEX);
  }
  uidStr.toUpperCase();
  return uidStr;
}

bool checkNFC(unsigned long timeoutMs) {

  uint8_t uid[7];
  uint8_t uidLength;

  unsigned long start = millis();

  while (millis() - start < timeoutMs) {

    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 200)) {

      String uidString = getUIDString(uid, uidLength);

      if (uidString == lastUID && (millis() - lastScanTime < scanCooldown)) {
      continue;
      delay(10);
  }

      Serial.print("UID: ");
      Serial.println(uidString);

      lastUID = uidString;
      lastScanTime = millis();

      String product;
      String location;

      if (uidString == "5393A7E7520001") {
        product = "Coca-Cola"; location = "Shelf 1"; shelfPosition = 2000;
      } 
      else if (uidString == "53D2B0E7520001") {
        product = "Fanta";     location = "Shelf 2"; shelfPosition = 8000;
      } 
      else if (uidString == "539CB6E7520001") {
        product = "Sprite";    location = "Shelf 3"; shelfPosition = 14000;
      } 
      else if (uidString == "5325BFE7520001") {
        product = "Pepsi";     location = "Shelf 4"; shelfPosition = 20000;
      } 
      else {
        Serial.println("→ Unknown package");
        return false;
      }

      Serial.print("→ ");
      Serial.println(location);

      sendToServer(uidString, product, location);

      currentUID = uidString;
      currentProduct = product;
      currentLocation = location;

      return true; // valid tag found
    }
  }

  return false; // timeout, no tag
}

// ===== Conveyor Control =====
void startConveyor() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  ledcWrite(ENA, 200);
  delay(100);
  ledcWrite(ENA, 168); // speed
  conveyorRunning = true;
}

void stopConveyor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  ledcWrite(ENA, 0);
  conveyorRunning = false;
}

// ===== Homing =====
void homeAxis(AccelStepper &stepper, int limitPin, int direction) {

  stepper.setMaxSpeed(2000);
  stepper.setAcceleration(1000);

  stepper.moveTo(100000 * direction);

  while (digitalRead(limitPin) == LOW) {
    stepper.run();
  }

  stepper.stop();
  stepper.setCurrentPosition(0);

  stepper.moveTo(-600 * direction);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  stepper.setMaxSpeed(300);
  stepper.moveTo(10000 * direction);

  while (digitalRead(limitPin) == LOW) {
    stepper.run();
  }

  stepper.stop();
  stepper.setCurrentPosition(0);

  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);
}

// ===== Robot Sequence =====
void runRobotCycle() {

  Serial.println("Object detected → Starting pick & place");

  wristServo.write(180);

  // Pick
  moveToPosition(1600, 1300, 0, 10000);
  gripperServo.write(96);

  delay(600);

  moveToPosition(0, 0, 0, 10000);
  wristServo.write(90);
 
 //Drop
  moveToPosition(0, 0, 7200, 10000);
  moveToPosition(0, 0, 7200, shelfPosition);
  moveToPosition(1100, 950, 7200, shelfPosition);

  gripperServo.write(70);
  delay(600);

  moveToPosition(0, 0, 7200, shelfPosition);
  moveToPosition(0, 0, 0, shelfPosition);
  moveToPosition(0, 0, 0, 10000);
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  delay(5000);

  Wire.begin(21, 22);
  Wire.setClock(50000);
  Wire.setTimeOut(1000);

  // Servo initial
  wristServo.attach(WRIST_SERVO_PIN, 500, 2400);
  gripperServo.attach(GRIPPER_SERVO_PIN, 500, 2400);

  // Limit switches
  pinMode(LIMIT_X, INPUT_PULLUP);
  pinMode(LIMIT_Y, INPUT_PULLUP);
  pinMode(LIMIT_Z, INPUT_PULLUP);
  pinMode(LIMIT_A, INPUT);

  // Conveyor
  pinMode(IR_SENSOR, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  ledcAttach(ENA, 1000, 8);

  Serial.println("Starting Homing...");

  // Homing
  wristServo.write(90);
  gripperServo.write(70);
  homeAxis(stepperX, LIMIT_X, -1);
  homeAxis(stepperY, LIMIT_Y, -1);
  homeAxis(stepperZ, LIMIT_Z, -1);
  homeAxis(stepperA, LIMIT_A, -1);

  Serial.println("Homing complete!");

   // Stepper settings
  stepperX.setMaxSpeed(MAX_SPEED);
  stepperX.setAcceleration(ACCELERATION);

  stepperY.setMaxSpeed(MAX_SPEED);
  stepperY.setAcceleration(ACCELERATION);

  stepperZ.setMaxSpeed(MAX_SPEED);
  stepperZ.setAcceleration(ACCELERATION);

  stepperA.setMaxSpeed(MAX_SPEED);
  stepperA.setAcceleration(ACCELERATION);

  homingDone = true;

  nfc.begin();

  if (!initPN532()) {
    Serial.println("PN532 failed to initialize!");
  }

  connectWiFi(); 

  stepperA.moveTo(10000);

  while (stepperA.distanceToGo() != 0) {
    stepperA.run();
  }
  delay(1000);

  // Start conveyor AFTER reaching position
  startConveyor();
}

// ================== LOOP ==================
void loop() {
  if (!homingDone) return;

  if (digitalRead(IR_SENSOR) == LOW) {

    stopConveyor();
    Serial.println("IR triggered — scanning NFC...");

    bool validTagFound = checkNFC(4000);  //wait up to 4s

    if (!validTagFound) {
      Serial.println("No tag found after 4s — resuming conveyor");

      startConveyor();

      // Prevent retrigger
      while (digitalRead(IR_SENSOR) == LOW) {
        delay(100);
      }

      return;
    }

    Serial.println("Valid tag detected → running robot");
    delay(200);
    runRobotCycle();
  }
}