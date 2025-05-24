#include <Arduino.h>
#include <WiFi.h> // Include the WiFi library for ESP32
#include <WiFiManager.h> // Include the WiFi library for ESP32

#define TriggerPin 23 // Define the trigger pin
#define LED_BUILTIN 2 // Define the built-in LED pin for ESP32

// put function declarations here:
//example:
//int myFunction(int, int);
void checkWiFiTrigger(); // Function to check the trigger pin and start WiFi configuration

void setup() {
  // put your setup code here, to run once:
  WiFi.mode(WIFI_STA); // Set the WiFi mode to Station
  WiFiManager wm;
  Serial.begin(115200);

  //pinMode:
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  pinMode(TriggerPin, INPUT_PULLUP); // Initialize the TriggerPin as an input

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

void loop() {
  // put your main code here, to run repeatedly:
  if(!WiFi.isConnected()) { // Check if WiFi is not connected
    Serial.println("WiFi is not connected");
    digitalWrite(LED_BUILTIN, LOW); // LED indicates WiFi is not connected
  }
  checkWiFiTrigger(); // Check if user want to configure WiFi
}

// put function definitions here:
void checkWiFiTrigger() {
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
