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
#include <Servo.h> // Include the Servo library for servo actuator
#include <Adafruit_SSD1306.h>

// -------------------------------
// --- Behaviour configuration ---
// -------------------------------
#define LidCloseAfterMillis 10000 // After how many milliseconds the lid will be closed again
#define LidBlockAtFullness 85 // Control at which fullness level lid will refuse to open
#define CompressionRatioThreshold 150 // Control at which (fullness/weight) ratio the trash is considered to be compressable
#define buzzingFrequency 500 // time in milliseconds between beeps

// Define pins for the ESP32
#define WiFiTriggerPin 23 // Define the trigger pin
#define LED_BUILTIN 2 // Define the built-in LED pin for ESP32

// US sensors
// TODO: replace pins with actual pins
#define UStrigrPin 5 // Define the pin to send ultrasonic burst
#define USechoPin 18 // Define the pin to receive ultrasonic burst

// Weight sensor
#define LoadcellDoutPin 16
#define LoadcellSckPin 4

// Capacitive touch sensor
#define CapacitiveSignalPin 14 // Define the pin to read the capacitive touch sensor

// Servo 
#define ServoControlPin 13

// Buzzer
#define BuzzerControlPin 19

// OLED
#define oledAddress 0x3C // Usually 0x3 or 0x27

// --- Firebase config ---
#define API_KEY "AIzaSyDN0bocHyMBXdRX7nLLn9TRyZ6pghbHqfI" // Firebase API key
#define DATABASE_URL "https://smart-waste-management-1b537-default-rtdb.asia-southeast1.firebasedatabase.app/"

// -------------------
// --- Calibration ---
// -------------------

// Calibrate weight sensor
// --> Divider: The value to divide the raw weight reading by
#define LoadcellDivider 1 // TODO: replace with actual value

// Calibrate US sensor for bin fullness)
// --> Min: Distance measured when bin is full
// --> Max: Distance measured when bin is empty
// TODO: replace with actual values
#define USdistanceMin NULL
#define USdistanceMax NULL

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
unsigned long previousMillis = 0; // Variable to store the last data send/read
unsigned long previousdisplay = 0;
bool lid_open = false; // Variable to store whether lid is open or not
ulong lidLastOpened = 0; // Variable to store when lid was last opened
ulong lastBuzzed = 0;
bool buzzerOn = false;
bool loopstart = true;
unsigned long lastTouchTime  = 0;
bool last_touch = false;

// Scale object for weight sensor
HX711 scale;

// Servo object for servo actuator
Servo myServo;

// OLED object
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// -------------------------
// --- Custom data types ---
// -------------------------

struct SensorData {
  float weight;
  float fullness;
  bool touch;
};
String touchmsg, fullmsg, weightmsg, lidmsg = "";

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
int firebaseReadLidTrigger();
bool firebaseSend(String key, int value); // Function to send data to Firebase

// Hardware functions
void sensorReadWeightSetup(); // Function to setup the weight sensor
void sensorReadFullnessSetup(); // Function to setup the US sensor
void sensorReadTouchSetup(); // Function to setup the touch sensor

float sensorReadWeight(); // Function to read the weight sensor
float sensorReadFullness(); // Function to read the US sensor
bool sensorReadTouch(); // Function to read the touch sensor
SensorData sensorRead(); // Function to read the sensors and send data to Firebase

void actuatorServoSetup(); // Function to setup the servo actuator
void actuatorDisplaySetup(); // Function to setup the display
void actuatorBuzzerSetup(); // Function to setup the buzzer

void actuatorServoOpenLid(); // Function to open the lid using the servo actuator
void actuatorServoCloseLid(); // Function to close the lid using the servo actuator
void actuatorDisplayMessage(String messageLine); // Function to display a message on the display
void actuatorDisplayResetMessage(); // Function to reset the display message
void actuatorBuzzerBuzz(); // Function to buzz the buzzer
void actuatorBuzzerNobuzz(); // Function to stop buzzing



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Start");
  pinsSetup();
  wifiSetup();
  firebaseSetup();

  // Setup sensors
  sensorReadFullnessSetup(); // Setup the US sensor
  sensorReadTouchSetup(); // Setup the touch sensor

  // Setup actuators
  actuatorDisplaySetup(); // Setup the display
  actuatorDisplayMessage("Hello");
  actuatorServoSetup();

}

void loop() {
  if(loopstart){
    Serial.println("Loop Start");
    loopstart = false;
  }
  // --- Perform IoT connection check ---
  // --> Check if user wants to enter WiFi setup mode
  // --> Check if connected to WiFi
  // --> Check if connected to Firebase
  wifiCheckTrigger(); // Check if user want to configure WiFi
  wifiFirebaseConnectionCheck(); // Check if WiFi and Firebase is connected


  // --- Read data ---
  static ulong lastPrint = 0;
  bool firebaseTriggerLid = bool(firebaseReadLidTrigger());
  

  // --- Print data ---
  if(updateNeeded(1000, &lastPrint)) {
    SensorData mySensorData = sensorRead(); // Read the sensors
    Serial.print("Firebase open: ");
    Serial.println(firebaseTriggerLid);
    Serial.print("Touch: ");
    Serial.println((mySensorData.touch ? "true" : "false"));
    touchmsg = String("Touch:") + (mySensorData.touch ? "true" : "false");
    Serial.print("Fullness: ");
    Serial.println(mySensorData.fullness);
    fullmsg = "Fullness:" + String(mySensorData.fullness);
    Serial.print("Weight: ");
    Serial.println(mySensorData.weight);
    weightmsg = "Weight:" + String(mySensorData.weight);
    lidmsg = String("lid:") + (lid_open ? "Open" : "Close");
    actuatorDisplayResetMessage();
    actuatorDisplayMessage(touchmsg);
    actuatorDisplayMessage(fullmsg);
    actuatorDisplayMessage(weightmsg);
    actuatorDisplayMessage(lidmsg);

    if (mySensorData.touch) {
      last_touch = true;
      lastTouchTime = millis();
    }
  
    // Maintain last_touch for 10 seconds after the last actual touch
    if (millis() - lastTouchTime > 10000) {
      last_touch = false;
    }

    if (firebaseTriggerLid || last_touch)
    {
      actuatorServoOpenLid();
    }else{
      actuatorServoCloseLid();
    }
    
  }
}

void pinsSetup() {
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  pinMode(WiFiTriggerPin, INPUT_PULLUP); // Initialize the WiFi TriggerPin as an input
  Serial.println("Pin Setup Completed");
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
  Serial.println("Setting up Firebase");
  IPAddress ip;
  if (!WiFi.hostByName("www.googleapis.com", ip)) {
    Serial.println("❌ DNS lookup failed. Firebase domain unreachable.");
    return;
  }
  config.api_key = API_KEY; // Set the Firebase API key
  config.database_url = DATABASE_URL; // Set the Firebase database URL
  config.time_zone = 8; // Malaysia GMT+8
  config.cert.data = nullptr; // Use built-in root cert
  config.token_status_callback = tokenStatusCallback; // Enables token readiness feedback
  config.timeout.serverResponse = 10000; // 10 seconds
  Firebase.reconnectWiFi(true);
  Serial.setDebugOutput(true);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) < 100000) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\n✅ Time synced");

  if(WiFi.isConnected()) {
    auth.user.email = "esp32@google.com";
    auth.user.password = "esp-32";
    auth.token.uid.clear(); // Optional: clear cached UID to force new token

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

bool updateNeeded(ulong frequency, ulong *lastUpdate) {
  ulong currentTime = millis();
  ulong passedTime = currentTime - *lastUpdate;
  if(passedTime > frequency) {
    *lastUpdate = currentTime; // Update the last update time
    return true;
  }
  return false;
}

// put function definitions here:
void wifiCheckTrigger() {
  // This function checks the trigger pin
  static unsigned long lastTrigger = 0;
  if (millis() - lastTrigger > 2000 && digitalRead(WiFiTriggerPin) == LOW) { // If the trigger pin is LOW
    lastTrigger = millis();
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
    digitalWrite(LED_BUILTIN, LOW); // LED indicates WiFi is not connected
    Serial.println("WiFi is not connected");
  } else {
    digitalWrite(LED_BUILTIN, HIGH); // LED indicates WiFi is connected
    if(auth.token.uid != ""){
      firebaseConnected = true; // Set firebaseConnected to true if UID is not empty
    } 
    else {
      firebaseConnected = false;
    }
  }
  // Check if Firebase is ready
  if (WiFi.isConnected() && !Firebase.ready() && auth.token.uid == "") {
    Serial.println("Retrying Firebase initialization...");
    Firebase.begin(&config, &auth);
  }

}

int firebaseReadLidTrigger() {
  if(Firebase.ready() && firebaseConnected == true){
    if (Firebase.RTDB.getInt(&fbdo, "/open_lid")) {
      return fbdo.intData();
    }
  }
  return lid_open;
}

bool firebaseSend(String key, int value) {
  if(Firebase.ready() && firebaseConnected == true) {
    if (Firebase.RTDB.setInt(&fbdo, key, value)) {
      return true; // Return true if data is sent successfully
    } 
    else {
      Serial.print("Failed to send data to Firebase: ");
      Serial.println(fbdo.errorReason()); // Print the error reason if sending fails
    }
  } 
  else {
    Serial.println("Firebase is not ready or not connected.");
  }
  return false;
}

void sensorReadWeightSetup() {
  scale.begin(LoadcellDoutPin, LoadcellSckPin);
  scale.set_scale(LoadcellDivider);
  scale.tare();
}

float sensorReadWeight() {
  //float weight = scale.get_units();
  //Serial.println(weight);
  //return weight;
  return 99;
}

void sensorReadFullnessSetup() {
  // Initialize pin to send ultrasonic burst
  pinMode(UStrigrPin, OUTPUT);
  // Initialize pin to receive ultrasonic burst
  pinMode(USechoPin, INPUT);  
}

float sensorReadFullness() {
  // --- Send the ultrasonic bust ---
  // Make sure the pin to send out the ultrasonic burst is low first
  digitalWrite(UStrigrPin, LOW);
  delayMicroseconds(2);
  // Send a 10 microsecond ultrasonic burst
  digitalWrite(UStrigrPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(UStrigrPin, LOW);

  // --- Receive ultrasonic burst and calculate distance
  // How long to receive ultrasonic burst again
  float durationToReceive = pulseIn(USechoPin, HIGH, 30000);
  // Convert duration to distance
  // TODO: Check formula for correctness
  float measuredDistance = (durationToReceive * 0.0343) / 2;
  
  if (durationToReceive == 0) {
    Serial.println("⚠️ No echo detected (timeout)");
  }

  /*
  // --- Standardize distance ---
  float relativeDistance = measuredDistance - USdistanceMin;
  float distanceRange = USdistanceMax - USdistanceMin;
  float standardizedDistance = (relativeDistance/distanceRange) * 100;
  */
  //return standardizedDistance;
  return measuredDistance;
}

void sensorReadTouchSetup() {
  // Initialize the pin to read the capacitive touch sensor
  pinMode(CapacitiveSignalPin, INPUT);
}

bool sensorReadTouch() {
  if (touchRead(CapacitiveSignalPin) < 40) {  // Adjust threshold if needed
    return 1;
  }
  return 0;
}

SensorData sensorRead() {
  SensorData mySensorData = {
    .weight = sensorReadWeight(), // Read the weight sensor
    .fullness = sensorReadFullness(), // Read the US sensor
    .touch = sensorReadTouch() // Read the touch sensor
  };

  
  // Send the sensor data to Firebase
  firebaseSend("/Readings/weight", mySensorData.weight);
  firebaseSend("/Readings/fullness", mySensorData.fullness);
  firebaseSend("/Readings/touch", mySensorData.touch);
  Serial.println("Loop complete: Read sensors");
  return mySensorData;

}

void actuatorServoSetup() {
  myServo.attach(ServoControlPin); // Attach the servo to the control pin with min and max pulse width
  myServo.write(180); // Set servo to position 0 at initially
}

void actuatorServoOpenLid() {
  myServo.write(90); // Write 0 degrees to the servo to open the lid
  lid_open = true; 
  lidLastOpened= millis();
  Serial.println("Lid opened.");
  firebaseSend("Readings/lid_open", true); // Send the lid status to Firebase
}

void actuatorServoCloseLid() {
  myServo.write(180); // Write 0 degrees to the servo to close the lid
  lid_open = false;
  Serial.println("Lid closed.");
  firebaseSend("Readings/lid_open", false); // Send the lid status to Firebase
}

void actuatorDisplaySetup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, oledAddress)) {
    Serial.println(F("❌ SSD1306 allocation failed"));
  }
  display.setTextSize(2);  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
}

void actuatorDisplayMessage(String messageLine) {
  display.setTextSize(1);
  display.println(messageLine);
  display.display();
}

void actuatorDisplayResetMessage() {
  display.clearDisplay();
  display.setCursor(0, 0); 
  display.display();
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