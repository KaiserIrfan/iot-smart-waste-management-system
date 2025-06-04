// Library to abstract microcontroller <-> sensor/actuator communication

#include <hardwareAPI.h>

namespace hardwareAPI {

    // ------------------
    // --- SENSOR API ---
    // ------------------

    bool measureFullnessSetup(uint8_t UVtrigrPin, uint8_t UVechoPin) {
        // Initialize pin to send ultrasonic burst
        pinMode(UVtrigrPin, OUTPUT);
        // Initialize pin to receive ultrasonic burst
        pinMode(UVechoPin, INPUT);
    }
    int measureFullness(uint8_t UVtrigtPin, uint8_t UVechoPin, float distanceMin, float distanceMax) {
        
        // --- Send the ultrasonic bust 
        
        // Make sure the pin to send out the ultrasonic burst is low first
        digitalWrite(UVtrigtPin, LOW);
        delayMicroseconds(2);
        // Send a 10 microsecond ultrasonic burst
        digitalWrite(UVtrigtPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(UVtrigtPin, LOW);

        // --- Receive ultrasonic burst and calculate distance

        // How long to receive ultrasonic burst again
        float durationToReceive = pulseIn(UVechoPin, HIGH);
        // Convert duration to distance
        // TODO: Check formula for correctness
        float measuredDistance = (durationToReceive * 0.0343) / 2;
        
        // --- Standardize distance ---
        float relativeDistance = measuredDistance - distanceMin;
        float distanceRange = distanceMax - distanceMin;
        float standardizedDistance = (relativeDistance/distanceRange) * 100;

        return standardizedDistance;
    }

    bool measureTouchSetup(int capacitiveDetectPulsePin) {

    }
    bool measureTouch(int detectPulsePin) {

    }

    bool measureWeightSetup(int scaleDataPin, int scaleClockPin, float scaleCalibrationFactor) {

    }
    int measureWeight(int scaleDataPin, int scaleClockPin, float scaleCalibrationFactor) {

    }

    // --------------------
    // --- ACTUATOR API ---
    // --------------------

    // TODO: params
    bool triggerServoSetup() {

    }
    bool triggerServo() {

    }

    // TODO: params
    bool triggerBuzzerSetup() {

    }
    bool triggerBuzzer();

    // TODO: params
    bool triggerDisplaySetup() {

    }
    bool triggerDisplay() {

    }

}