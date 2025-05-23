#include <Arduino.h>

#define LED_BUILTIN 2 // Define the built-in LED pin for ESP32

// put function declarations here:
//example:
//int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on (HIGH is the voltage level)
  delay(1000); // Wait for a second
  digitalWrite(LED_BUILTIN, LOW); // Turn the LED off (LOW is the voltage level)
  delay(1000); // Wait for a second
  
}

// put function definitions here:
/*example:
int myFunction(int x, int y) {
  return x + y;
}*/