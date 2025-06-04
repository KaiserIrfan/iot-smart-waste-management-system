// Library to abstract microcontroller <-> sensor/actuator communication

#include <hardwareAPI.h>

namespace hardwareAPI {

    // ------------------
    // --- SENSOR API ---
    // ------------------

    bool measureFullnessSetup(int UVtrigrPin, int UVechoPin) {

    }
    int measureFullness(int UVtrigtPin, int UVechoPin) {

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
    bool triggerServoSetup();
    bool triggerServo();

    // TODO: params
    bool triggerBuzzerSetup();
    bool triggerBuzzer();

    // TODO: params
    bool triggerDisplaySetup();
    bool triggerDisplay();

}