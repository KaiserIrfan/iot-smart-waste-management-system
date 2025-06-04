// abstract hardware interfaces here

// TODO: params
bool measureFullnessSetup(int trigrPin, int echoPin);
int measureFullness(int trigtPin, int echoPin);

// TODO: params
bool measureTouchSetup(int detectPulsePin);
bool measureTouch(int detectPulsePin);

// TODO: params
int measureWeightSetup();
int measureWeight();

// TODO: params
bool triggerServoSetup();
bool triggerServo();

// TODO: params
bool triggerBuzzerSetup();
bool triggerBuzzer();

// TODO: params
bool triggerDisplaySetup();
bool triggerDisplay();



