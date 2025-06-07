// -----------------
// --- Libraries ---
// -----------------
#include <Arduino.h>
#include <WiFi.h> // Include the WiFi library for ESP32
#include <WiFiManager.h> // Include the WiFi library for ESP32
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ---------------------
// --- Configuration ---
// ---------------------

// --- Pins ---
#define TriggerPin 23 // Define the trigger pin
#define LED_BUILTIN 2 // Define the built-in LED pin for ESP32

// --- Firebase config ---
#define API_KEY "AIzaSyDN0bocHyMBXdRX7nLLn9TRyZ6pghbHqfI" // Firebase API key
#define DATABASE_URL "https://smart-waste-management-1b537-default-rtdb.asia-southeast1.firebasedatabase.app/"

// ------------------------
// --- Global variables ---
// ------------------------

// --- Firebase data ---
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// --- Firebase status ---
bool firebaseConnected = false; // Variable to check if Firebase is connected

// --- Timing ---
unsigned long previousTime = 0; // Variable to store the last data send/read

// --- Custom data types ---

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
bool updateNeeded();

// Hardware functions

// IoT function
void wifiCheckTrigger(); // Function to check the trigger pin and start WiFi configuration
void wifiFirebaseConnectionCheck();
bool firebaseReadLidTrigger();
bool firebaseSend(String key, int value); // Function to send data to Firebase
float sensorReadWeight(); // Function to read the weight sensor
float sensorReadFullness(); // Function to read the UV sensor
bool sensorReadTouch(); // Function to read the touch sensor
SensorData sensorRead(); // Function to read the sensors and send data to Firebase


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

  // Perform IoT connection check
  // --> Check if user wants to enter WiFi setup mode
  // --> Check if connected to WiFi
  // --> Check if connected to Firebase
  wifiCheckTrigger(); // Check if user want to configure WiFi
  wifiFirebaseConnectionCheck(); // Check if WiFi and Firebase is connected

  // Read data
  bool firebaseTriggerLid = firebaseReadLidTrigger();
  SensorData mySensorData = sensorRead(); // Read the sensors
}


// ------------------------
// --- Helper functions ---
// ------------------------

// --- Setup functions ---

void pinsSetup() {
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  pinMode(TriggerPin, INPUT_PULLUP); // Initialize the TriggerPin as an input
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
  if (digitalRead(TriggerPin) == LOW) { // If the trigger pin is LOW
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

float sensorReadWeight() {
  // This function should read the weight sensor and return the weight
  // For now, we will return a dummy value
  return 42.0; // Dummy value for weight
}

float sensorReadFullness() {
  // This function should read the UV sensor and return the fullness
  // For now, we will return a dummy value
  return 100.0; // Dummy value for fullness
}

bool sensorReadTouch() {
  // This function should read the touch sensor and return the touch value
  // For now, we will return a dummy value
  return false; // Dummy value for touch
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