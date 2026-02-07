
// Dependencies
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

bool sessionActive = false;
bool calibrated = false;

// Time Duration for each Pushing session
unsigned long startTime = 0;
unsigned long lastPushTime = 0;
float durations = 0;
int pushupCount = 0;
float totalCalories = 0;
float userWeight = 70.0; // By default

// Functions and variable for MPU6050 Sensor
float minZ = 10, maxZ = -10;
float downThreshold, upThreshold;
bool goingDown = false;

// BLE Characteristics
BLEServer* pServer = NULL;
BLEService* pService = NULL;
BLECharacteristic* pPushupCountCharacteristic = NULL;
BLECharacteristic* pDurationCharacteristic = NULL;
BLECharacteristic* pCaloriesCharacteristic = NULL;
BLECharacteristic* pWeightCharacteristic = NULL;
BLECharacteristic* pCalibrateCharacteristic = NULL;
BLECharacteristic* pStartCharacteristic = NULL;
BLECharacteristic* pStopCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// UUID Settings
#define SERVICE_UUID                      "a7eadddc-909c-405b-a6d6-ae6a66446979"
#define PUSHUP_COUNT_CHARACTERISTIC_UUID  "9cb0dd18-9e30-4bc1-aa93-d5ab3f5851ee"
#define DURATION_CHARACTERISTIC_UUID      "cf62e221-fa63-478f-a2cb-23a3da3cf4af"
#define CALORIES_CHARACTERISTIC_UUID      "15de5b9f-1c31-482a-b25d-e2cb3d76cc84"
#define WEIGHT_CHARACTERISTIC_UUID        "275da945-1b8f-475a-ad26-c439816b728d"
#define CALIBRATE_CHARACTERISTIC_UUID     "abd4150e-b409-4c46-bce8-7c8bf00efbc2"
#define START_CHARACTERISTIC_UUID         "a48f6b0c-b7e2-4c85-9649-245858766b0e"
#define STOP_CHARACTERISTIC_UUID          "a7ec0213-d3ca-41af-887d-90cb6e66b46e"

// Sensor Functions
void calibrateSensor() {
    Serial.println("\nCalibrating... move slowly up and down once or twice");
    minZ = 10; maxZ = -10;
    calibrated = false;

    unsigned long t0 = millis();
    while (millis() - t0 < 5000) {
        int16_t ax, ay, az;
        mpu.getAcceleration(&ax, &ay, &az);
        float z = az / 16384.0;
        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
        delay(10);
    }

    downThreshold = minZ + (maxZ - minZ) * 0.3;
    upThreshold   = minZ + (maxZ - minZ) * 0.7;
    calibrated = true;

    Serial.print("Calibration complete! Thresholds - Down: ");
    Serial.print(downThreshold);
    Serial.print(", Up: ");
    Serial.println(upThreshold);
}

void startSession() {
    if (!calibrated) {
        Serial.println("Please calibrate first!");
        return;
    }
    sessionActive = true;
    pushupCount = 0;
    totalCalories = 0;
    startTime = millis();
    lastPushTime = startTime;
    durations = 0;
    
    // Reset BLE values
    if (pPushupCountCharacteristic) {
        pPushupCountCharacteristic->setValue("0");
        pPushupCountCharacteristic->notify();
    }
    if (pDurationCharacteristic) {
        pDurationCharacteristic->setValue("0.0");
        pDurationCharacteristic->notify();
    }
    if (pCaloriesCharacteristic) {
        pCaloriesCharacteristic->setValue("0.00");
        pCaloriesCharacteristic->notify();
    }
    
    Serial.println("Session started!");
}

void stopSession() {    
    sessionActive = false;
    
    if (pDurationCharacteristic) {
        String durationStr = String(durations, 1);
        pDurationCharacteristic->setValue(durationStr.c_str());
        pDurationCharacteristic->notify();
    }
    
    if (pCaloriesCharacteristic) {
        String caloriesStr = String(totalCalories, 2);
        pCaloriesCharacteristic->setValue(caloriesStr.c_str());
        pCaloriesCharacteristic->notify();
    }
    
    Serial.println("Session stopped!");
}

float calcMET(float pushTime) {
    if (pushTime <= 1.0) return 10.0;
    if (pushTime <= 2.0) return 8.0;  
    if (pushTime <= 3.0) return 7.0;
    return 6.0;
}

float calcCalories(float weight, float pushTime) {
    float MET = calcMET(pushTime);
    return (MET * weight * (pushTime / 3600.0));
}

void detectPushup() {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    float z = az / 16384.0;

    if (z < downThreshold && !goingDown) {
        goingDown = true;
    }
    else if (z > upThreshold && goingDown) {
        pushupCount++;
        unsigned long now = millis();
        float pushTime = (now - lastPushTime) / 1000.0;
        lastPushTime = now;
        durations += pushTime;
        goingDown = false;

        float calories = calcCalories(userWeight, pushTime);
        totalCalories += calories;

        if (pPushupCountCharacteristic) {
            String countStr = String(pushupCount);
            pPushupCountCharacteristic->setValue(countStr.c_str());
            pPushupCountCharacteristic->notify();
        }
        
        if (pDurationCharacteristic) {
            String durationStr = String(durations, 1);
            pDurationCharacteristic->setValue(durationStr.c_str());
            pDurationCharacteristic->notify();
        }
        
        if (pCaloriesCharacteristic) {
            String caloriesStr = String(totalCalories, 2);
            pCaloriesCharacteristic->setValue(caloriesStr.c_str());
            pCaloriesCharacteristic->notify();
        }

        Serial.printf("Pushup #%d | Time: %.2fs | Calories: %.2fkCal\n", 
                      pushupCount, pushTime, calories);
    }
    delay(50);
}

// Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks { 
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Device Connected!");
    };
    
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device Disconnected!");
        if (sessionActive) {
            sessionActive = false;
            Serial.println("Session stopped due to disconnect");
        }
    };
};

// Weight Callback - FIXED: Use Arduino String
class WeightCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        // Get value as Arduino String
        String rxValue = pCharacteristic->getValue().c_str();
        
        if (rxValue.length() > 0) {
            Serial.print("Weight Characteristic event, written: ");
            Serial.println(rxValue);
            
            userWeight = rxValue.toFloat();
            Serial.print("New weight set: ");
            Serial.print(userWeight);
            Serial.println(" kg");
            
            // Update characteristic value
            pCharacteristic->setValue(rxValue.c_str());
        }
    }
};

// Calibrate Callback - FIXED: Use Arduino String
class CalibrateCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String rxValue = pCharacteristic->getValue().c_str();
        
        if (rxValue.length() > 0) {
            Serial.print("Calibrate Characteristic event, written: ");
            
            // Check first character (should be "1" for calibrate)
            if (rxValue[0] == '1') {
                calibrateSensor();
                Serial.println("Calibration triggered via BLE");
            }
        }
    }
};

// Start Callback - FIXED: Use Arduino String
class StartCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String rxValue = pCharacteristic->getValue().c_str();
        
        if (rxValue.length() > 0) {
            Serial.print("Start Characteristic event, written: ");
            Serial.println(rxValue);
            
            if (rxValue[0] == '1') {
                startSession();
                Serial.println("Session started via BLE");
            }
        }
    }
};

// Stop Callback - FIXED: Use Arduino String
class StopCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String rxValue = pCharacteristic->getValue().c_str();
        
        if (rxValue.length() > 0) {
            Serial.print("Stop Characteristic event, written: ");
            Serial.println(rxValue);
            
            if (rxValue[0] == '1') {
                stopSession();
                Serial.println("Session stopped via BLE");
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);

    // Initialize MPU
    mpu.initialize();
    Serial.println("MPU6050 Found!");
    
    // Create the BLE Device
    BLEDevice::init("Pushup Device v2");
    
    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create the BLE Service
    pService = pServer->createService(SERVICE_UUID);

    Serial.println("Creating characteristics...");
    
    // ==== CREATE ALL CHARACTERISTICS PROPERLY ====
    // Pushup Count Characteristic
    Serial.println("Creating PUSHUP_COUNT characteristic...");
    pPushupCountCharacteristic = pService->createCharacteristic(
        PUSHUP_COUNT_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pPushupCountCharacteristic->addDescriptor(new BLE2902());
    pPushupCountCharacteristic->setValue("0");
    Serial.println("✓ PUSHUP_COUNT created");

    // Duration Characteristic
    Serial.println("Creating DURATION characteristic...");
    pDurationCharacteristic = pService->createCharacteristic(
        DURATION_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pDurationCharacteristic->addDescriptor(new BLE2902());
    pDurationCharacteristic->setValue("0.0");
    Serial.println("✓ DURATION created");

    // Calories Characteristic - THIS WAS MISSING
    Serial.println("Creating CALORIES characteristic...");
    pCaloriesCharacteristic = pService->createCharacteristic(
        CALORIES_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCaloriesCharacteristic->addDescriptor(new BLE2902());
    pCaloriesCharacteristic->setValue("0.00");
    Serial.println("✓ CALORIES created");

    // Weight Characteristic
    Serial.println("Creating WEIGHT characteristic...");
    pWeightCharacteristic = pService->createCharacteristic(
        WEIGHT_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_WRITE
    );
    pWeightCharacteristic->setCallbacks(new WeightCharacteristicCallbacks());
    pWeightCharacteristic->setValue("70.0");
    Serial.println("✓ WEIGHT created");

    // Calibrate Characteristic
    Serial.println("Creating CALIBRATE characteristic...");
    pCalibrateCharacteristic = pService->createCharacteristic(
        CALIBRATE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCalibrateCharacteristic->setCallbacks(new CalibrateCharacteristicCallbacks());
    Serial.println("✓ CALIBRATE created");

    // Start Characteristic
    Serial.println("Creating START characteristic...");
    pStartCharacteristic = pService->createCharacteristic(
        START_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pStartCharacteristic->setCallbacks(new StartCharacteristicCallbacks());
    Serial.println("✓ START created");

    // Stop Characteristic
    Serial.println("Creating STOP characteristic...");
    pStopCharacteristic = pService->createCharacteristic(
        STOP_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pStopCharacteristic->setCallbacks(new StopCharacteristicCallbacks());
    Serial.println("✓ STOP created");

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();
    
    Serial.println("\n=== Pushup Device Started ===");
    Serial.println("Device Name: Pushup Device v2");
    Serial.println("Service UUID: " SERVICE_UUID);
    Serial.print("Characteristics created: ");
    Serial.println("PUSHUP_COUNT, DURATION, CALORIES, WEIGHT, CALIBRATE, START, STOP");
    Serial.println("Waiting for BLE connection...");
}

void loop() {
    if (deviceConnected && sessionActive) {
        detectPushup();
    }
    
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("Advertising restarted");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}