// -----------------
// --- Libraries ---
// -----------------
#include <Arduino.h>
#include <WiFi.h> // Include the WiFi library for ESP32
#include <WiFiManager.h> // Include the WiFi library for ESP32
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <HX711.h> // Include the HX711 library for weight sensor
#include <ESP32Servo.h> // Include the Servo library for servo actuator
#include <LiquidCrystal_I2C.h>

// -------------------------------
// --- Behaviour configuration ---
// -------------------------------
#define LidCloseAfterMillis 10000 // After how many milliseconds the lid will be closed again
#define LidBlockAtFullness 85 // Control at which fullness level lid will refuse to open
#define CompressionRatioThreshold 150 // Control at which (fullness/weight) ratio the trash is considered to be compressable
#define buzzingFrequency 500 // time in milliseconds between beeps

// ------------------------------
// --- Hardware configuration ---
// ------------------------------

// Control wifi
#define WiFiTriggerPin 23 // Define the wifi trigger pin

// Status LED
#define LED_BUILTIN 2 // Define the built-in LED pin for ESP32

// Weight sensor
#define LoadcellDoutPin NULL;
#define LoadcellSckPiN NULL;

// UV sensors
// TODO: replace pins with actual pins
#define UVtrigrPin NULL // Define the pin to send ultrasonic burst
#define UVechoPin NULL // Define the pin to receive ultrasonic burst

// Capacitive touch sensor
#define CapacitiveSignalPin NULL // Define the pin to read the capacitive touch sensor

// Servo 
#define ServoControlPin NULL

// Buzzer
#define BuzzerControlPin NULL

// LCD
#define lcdAddress NULL // TODO: Run scanner sketch to get LCD address

// --- Firebase config ---

#define API_KEY "AIzaSyDN0bocHyMBXdRX7nLLn9TRyZ6pghbHqfI" // Firebase API key
#define DATABASE_URL "https://smart-waste-management-1b537-default-rtdb.asia-southeast1.firebasedatabase.app/"

// -------------------
// --- Calibration ---
// -------------------

// Calibrate weight sensor
// --> Divider: The value to divide the raw weight reading by
#define LoadcellDivider 1 // TODO: replace with actual value

// Calibrate UV sensor for bin fullness)
// --> Min: Distance measured when bin is full
// --> Max: Distance measured when bin is empty
// TODO: replace with actual values
#define UVdistanceMin NULL
#define UVdistanceMax NULL

// Calibrate servo
#define ServoMinPulse 1000
#define ServoMaxPulse 2000

// ------------------------
// --- Global variables ---
// ------------------------

// --- Firebase data ---
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// --- Status vars ---
bool firebaseConnected = false; // Variable to check if Firebase is connected
bool lid_open = false; // Variable to store whether lid is open or not
ulong lidLastOpened = 0; // Variable to store when lid was last opened
ulong lastBuzzed = 0;
bool buzzerOn = false;

// Scale object for weight sensor
HX711 scale;

// Servo object for servo actuator
Servo myServo;

// LCD object for LCD
LiquidCrystal_I2C lcd(lcdAddress, 16, 2);

// -------------------------
// --- Custom data types ---
// -------------------------

struct SensorData {
  float weight;
  float fullness;
  bool touch;
};

// -----------------------------
// --- Function declarations ---
// -----------------------------


// --- Setup functions ----

void pinsSetup();
void wifiSetup();
void firebaseSetup();

// --- Loop functions ---

// Helper functions
bool updateNeeded(ulong frequency, ulong *lastUpdate);

// IoT function
void wifiCheckTrigger(); // Function to check the trigger pin and start WiFi configuration
void wifiFirebaseConnectionCheck();
bool firebaseReadLidTrigger();
bool firebaseSend(String key, int value); // Function to send data to Firebase

// Hardware functions
void sensorReadWeightSetup(); // Function to setup the weight sensor
void sensorReadFullnessSetup(); // Function to setup the UV sensor
void sensorReadTouchSetup(); // Function to setup the touch sensor

float sensorReadWeight(); // Function to read the weight sensor
float sensorReadFullness(); // Function to read the UV sensor
bool sensorReadTouch(); // Function to read the touch sensor
SensorData sensorRead(); // Function to read the sensors and send data to Firebase

void actuatorServoSetup(); // Function to setup the servo actuator
void actuatorDisplaySetup(); // Function to setup the display
void actuatorBuzzerSetup(); // Function to setup the buzzer

void actuatorServoOpenLid(); // Function to open the lid using the servo actuator
void actuatorServoCloseLid(); // Function to close the lid using the servo actuator
void actuatorDisplayMessage(String messageLine1, String messageLine2); // Function to display a message on the display
void actuatorDisplayResetMessage(); // Function to reset the display message
void actuatorBuzzerBuzz(); // Function to buzz the buzzer
void actuatorBuzzerNoBuzz(); // Function to stop buzzing


// -------------
// --- Main ----
// -------------

// --- Setup, run once at boot ---
void setup() {
  Serial.begin(115200);
  pinsSetup();
  wifiSetup();
  firebaseSetup();
}

// --- Loop, run repeatedly until shutdown ---
void loop() {

  // --- Perform IoT connection check ---
  // --> Check if user wants to enter WiFi setup mode
  // --> Check if connected to WiFi
  // --> Check if connected to Firebase
  wifiCheckTrigger(); // Check if user want to configure WiFi
  wifiFirebaseConnectionCheck(); // Check if WiFi and Firebase is connected

  // --- Read data ---
  bool firebaseTriggerLid = firebaseReadLidTrigger();
  SensorData mySensorData = sensorRead(); // Read the sensors

  // --- Control the lid ---

  // Default state
  // --> Closed lid, no override, ready for user
  if(!mySensorData.touch && !lid_open && !firebaseTriggerLid) {
    actuatorDisplayMessage("Hello, stranger!", "");
  }
  // User requests opening
  // --> Bin has enough space: Adhere user request 
  else if(mySensorData.touch && !lid_open && mySensorData.fullness < LidBlockAtFullness) {
    actuatorServoOpenLid();
    actuatorDisplayMessage("Ready to dump!", "");
  }
  // --> Bin is too full: Refuse user request
  else if(mySensorData.touch && !lid_open && mySensorData.fullness >= LidBlockAtFullness) {
    actuatorDisplayMessage("Bin full, next", "around corner!");
    actuatorBuzzerBuzz(); // Buzz the buzzer to indicate bin is full
  }
  // City employee requests opening
  else if(firebaseTriggerLid && !lid_open) {
    actuatorServoOpenLid();
    actuatorDisplayMessage("Close lid", "when done!");
    actuatorBuzzerBuzz(); // Buzz the buzzer to indicate lid is open
  }
  
  // Request compression when needed
  if(lid_open && !firebaseTriggerLid) {
    float fullnessWeightRatio = mySensorData.fullness / mySensorData.weight; 
    if(fullnessWeightRatio > CompressionRatioThreshold) {
      actuatorDisplayMessage("Please compress", "the trash!");
      actuatorBuzzerBuzz(); 
  }
  
  // Close lid when conditions fullfilled
  if(lid_open && updateNeeded(LidCloseAfterMillis, &lidLastOpened) && !firebaseTriggerLid) {
    actuatorServoCloseLid();
    actuatorDisplayResetMessage();
  }

  // Stop buzzing when frequency exceeds -> no condition requests buzzing anymore
  // TODO: buzzing logic needs to be tested, whether really generates beep-beep-beep-...
  if(updateNeeded(buzzingFrequency*2, &lastBuzzed) && buzzerOn) {
    actuatorBuzzerNobuzz();
  }
}


// ------------------------
// --- Helper functions ---
// ------------------------

// --- Setup functions ---

void pinsSetup() {
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  pinMode(WiFiTriggerPin, INPUT_PULLUP); // Initialize the WiFi TriggerPin as an input
}

void wifiSetup() {
  WiFi.mode(WIFI_STA); // Set the WiFi mode to Station
  WiFiManager wm;

  if (!wm.autoConnect("ESP32_WiFi_Config")) {
    Serial.println("Failed to connect to WiFi. Starting Config Portal...");
  }
  
  if (WiFi.isConnected()) { // Check if WiFi is already connected
    Serial.println("WiFi is connected.");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID()); // Print the connected SSID
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); // Print the local IP address
    digitalWrite(LED_BUILTIN, HIGH); // LED indicates WiFi is connected
  }
}

void firebaseSetup() {
  config.api_key = API_KEY; // Set the Firebase API key
  config.database_url = DATABASE_URL; // Set the Firebase database URL
  if(WiFi.isConnected()) {
    auth.user.email = "esp32@google.com";
    auth.user.password = "esp-32";
    Firebase.begin(&config, &auth); // Initialize Firebase with the config and auth
    if (Firebase.ready()) {
      Serial.println("Firebase is ready.");
      Serial.print("Firebase Host: ");
      Serial.println(config.database_url.c_str()); // Print the Firebase database URL
      firebaseConnected = true; // Set firebaseConnected to true
    }
    else
    {
      Serial.println("Failed to initialize Firebase.");
    }
  }
}

// --- Loop functions ---

bool updateNeeded(ulong frequency, ulong *lastUpdate) {
  ulong currentTime = millis();
  ulong passedTime = currentTime - *lastUpdate;
  if(passedTime > frequency) {
    *lastUpdate = currentTime; // Update the last update time
    return true;
  }
  return false;
}

void wifiCheckTrigger() {
  // This function checks the trigger pin
  if (digitalRead(WiFiTriggerPin) == LOW) { // If the trigger pin is LOW
    Serial.println("WiFi setting activated!"); 
    WiFiManager wm; // Create a WiFiManager object
    wm.setConfigPortalTimeout(120); // Set a timeout for the config portal
    Serial.println("Timeout in 120 seconds.");
    if (wm.startConfigPortal("ESP32_WiFi_Config")) { // Start the config portal with a custom SSID
      Serial.println("WiFi Config Portal started successfully!");
      Serial.println("WiFi is connected.");
      Serial.print("SSID: ");
      Serial.println(WiFi.SSID()); // Print the connected SSID
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP()); // Print the local IP address
    } else {
      Serial.println("Failed to start WiFi Config Portal.");
    }
  } 
}

void wifiFirebaseConnectionCheck() {
    if(!WiFi.isConnected()) { // Check if WiFi is not connected
    Serial.println("WiFi is not connected");
    digitalWrite(LED_BUILTIN, LOW); // LED indicates WiFi is not connected
  } else {
    digitalWrite(LED_BUILTIN, HIGH); // LED indicates WiFi is connected
    if(auth.token.uid != ""){
      firebaseConnected = true; // Set firebaseConnected to true if UID is not empty
    } else {firebaseConnected = false;}
  }
}

bool firebaseReadLidTrigger() {
  static ulong lastUpdated = 0;
  if(Firebase.ready() && firebaseConnected == true && updateNeeded(5000, &lastUpdated)){
    if (Firebase.RTDB.getBool(&fbdo, "/open_lid")) {
      Serial.print("Open Lid: ");
      if (fbdo.boolData()) {
        Serial.println("true"); // Print true if the lid is open
        return fbdo.boolData();
      }
      Serial.println("false"); // Print false if the lid is closed
    }
  }
  return false;
}

bool firebaseSend(String key, int value) {
  if(Firebase.ready() && firebaseConnected == true) {
    if (Firebase.RTDB.setInt(&fbdo, key, value)) {
      Serial.print("Data sent to Firebase: ");
      Serial.print(key);
      Serial.print(" = ");
      Serial.println(value);
      return true; // Return true if data is sent successfully
    } else {
      Serial.print("Failed to send data to Firebase: ");
      Serial.println(fbdo.errorReason()); // Print the error reason if sending fails
    }
  } else {
    Serial.println("Firebase is not ready or not connected.");
  }
  return false;
}

void sensorReadWeightSetup() {
  scale.begin(LoadcellDoutPin, LoadcellSckPiN);
  scale.set_scale(LoadcellDivider);
  scale.tare();
}

float sensorReadWeight() {
  float weight = scale.get_units();
  return weight;
}

void sensorReadFullnessSetup() {
  // Initialize pin to send ultrasonic burst
  pinMode(UVtrigrPin, OUTPUT);
  // Initialize pin to receive ultrasonic burst
  pinMode(UVechoPin, INPUT);  
}

float sensorReadFullness() {
  
  // --- Send the ultrasonic bust 
  
  // Make sure the pin to send out the ultrasonic burst is low first
  digitalWrite(UVtrigrPin, LOW);
  delayMicroseconds(2);
  // Send a 10 microsecond ultrasonic burst
  digitalWrite(UVtrigrPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(UVtrigrPin, LOW);

  // --- Receive ultrasonic burst and calculate distance

  // How long to receive ultrasonic burst again
  float durationToReceive = pulseIn(UVechoPin, HIGH);
  // Convert duration to distance
  // TODO: Check formula for correctness
  float measuredDistance = (durationToReceive * 0.0343) / 2;
  
  // --- Standardize distance ---
  float relativeDistance = measuredDistance - UVdistanceMin;
  float distanceRange = UVdistanceMax - UVdistanceMin;
  float standardizedDistance = (relativeDistance/distanceRange) * 100;

  return standardizedDistance;
}

void sensorReadTouchSetup() {
  // Initialize the pin to read the capacitive touch sensor
  pinMode(CapacitiveSignalPin, INPUT);
}

bool sensorReadTouch() {
  return digitalRead(CapacitiveSignalPin); // Read the capacitive touch sensor pin
}

SensorData sensorRead() {
  static ulong lastUpdate = 0;
  if(updateNeeded(500, &lastUpdate)) {
    SensorData mySensorData = {
      .weight = sensorReadWeight(), // Read the weight sensor
      .fullness = sensorReadFullness(), // Read the UV sensor
      .touch = sensorReadTouch() // Read the touch sensor
    };
    Serial.println("Sensor data read:");
    Serial.print("Weight: ");
    Serial.println(mySensorData.weight);
    Serial.print("Fullness: ");
    Serial.println(mySensorData.fullness);
    Serial.print("Touch: ");
    Serial.println(mySensorData.touch);
    // Send the sensor data to Firebase
    firebaseSend("/Readings/weight", mySensorData.weight);
    firebaseSend("/Readings/fullness", mySensorData.fullness);
    firebaseSend("/Readings/touch", mySensorData.touch);
    return mySensorData;;
  }
}

void actuatorServoSetup() {
  myServo.attach(ServoControlPin, ServoMinPulse, ServoMaxPulse); // Attach the servo to the control pin with min and max pulse width
}

void actuatorServoOpenLid() {
  myServo.write(90); // Write 0 degrees to the servo to open the lid
  lid_open = true; 
  lidLastOpened= millis();
  Serial.println("Lid opened.");
  firebaseSend("Readings/lid_open", true); // Send the lid status to Firebase
}

void actuatorServoCloseLid() {
  myServo.write(0); // Write 0 degrees to the servo to close the lid
  lid_open = false;
  Serial.println("Lid closed.");
  firebaseSend("Readings/lid_open", false); // Send the lid status to Firebase
}

void actuatorDisplaySetup() {

}

void actuatorDisplayMessage(String messageLine1, String messageLine2) {
  lcd.clear();
  lcd.setCursor(0, 0); // Set the cursor to the first row
  lcd.print(messageLine1); // Print the first message row
  lcd.setCursor(0, 1); // Set the cursor to the second row
  lcd.print(messageLine2); // Print the second message row
}

void actuatorDisplayResetMessage() {
  lcd.clear(); // Clear the display
}

void actuatorBuzzerSetup() {
  pinMode(BuzzerControlPin, OUTPUT); // Initialize the buzzer control pin as an output
  digitalWrite(BuzzerControlPin, LOW); // Set the buzzer to LOW initially
}

void actuatorBuzzerBuzz() {
  // Turn on buzzer when it 
  // --> previously was off
  // --> and the buzzingFrequency time has passed
  if(!buzzerOn && updateNeeded(buzzingFrequency, &lastBuzzed)) {
    tone(BuzzerControlPin, 750); // Generate a tone at 750 Hz on the buzzer control pin
    buzzerOn = true;
    lastBuzzed = millis();
  }
  // Turn off buzzer
  // --> when it previously was on
  // --> and the buzzingFrequency time has passed
  else if(buzzerOn && updateNeeded(buzzingFrequency, &lastBuzzed)){
    noTone(BuzzerControlPin);
    buzzerOn = false;
  }
}

void actuatorBuzzerNobuzz() {
  noTone(BuzzerControlPin);
}