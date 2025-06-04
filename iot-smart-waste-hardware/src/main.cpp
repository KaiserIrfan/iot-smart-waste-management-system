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

bool updateNeeded(ulong frequency) {
  ulong currentTime = millis();
  ulong passedTime = currentTime - previousTime;
  if(passedTime > frequency) {
    previousTime = currentTime;
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
  if(Firebase.ready() && firebaseConnected == true && updateNeeded(5000)){
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
