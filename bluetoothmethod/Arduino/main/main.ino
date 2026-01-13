
// // Dependencies
// #include <BLEDevice.h>
// #include <BLEServer.h>
// #include <BLEUtils.h>
// #include <BLE2902.h>
// #include <Wire.h>
// #include <MPU6050.h>

// MPU6050 mpu;

// bool sessionActive = false;
// bool calibrated = false;

// // Time Duration for each Pushing session to be initilizaed here to avoid latency delay and avoid compatibility issue with BLE
// unsigned long startTime = 0;
// unsigned long lastPushTime = 0;
// float durations = 0;
// int pushupCount = 0;
// float totalCalories = 0;
// float userWeight = 70.0; // By default

// // Functions and variable for MMPU6050 Sensor
// float minZ = 10, maxZ = -10;
// float downThreshold, upThreshold;
// bool goingDown = false;

// void calibrateSensor(){
//     Serial.println("\nCalibrating... move slowly up and down once or twice")
//     minZ = 10; maxZ = -10;

//     unsigned long t0 = millis();
//     while(millis() - t0 < 5000){
//         int16_t ax, ay, az;
//         mpu.getAcceleration(&ax, &ay, &az);
//         float z = az / 16384.0;
//         if (z < minZ) minZ = z;
//         if (z > maxZ) maxZ = z;
//         delay(10);
//     }

//     downThreshold = minZ + (maxZ - minZ) * 0.3;
//     upThreshold   = minZ + (maxZ - minZ) * 0.7;
//     calibrated = true
// }

// void startSession(){
//     if (!calibrated){
//         Serial.println("Please calibrate first!");
//         return;
//     }
//     sessionActive = true;
//     pushupCount = 0;
//     startTime = millis();
//     lastPushTime = startTime;
//     durations = 0;
// }

// void stopSession(){    
//     sessionActive  false;
//     DURATION_CHARACTERISTICS_UUID->setValue(String(durations).c_str());
//     DURATION_CHARACTERISTICS_UUID->notify();
//     CALORIES_CHARACTERISTICS_UUID->setValue(String(totalCalories).c_str());
//     CALORIES_CHARACTERISTICS_UUID->notify();

//     duration = 0;
//     totalCalories = 0;
//     pushupCount = 0;

// }

// float calcMET(float pushTime) {
//   if (pushTime <= 1.0) return 10.0;
//   if (pushTime <= 2.0) return 8.0;  
//   if (pushTime <= 3.0) return 7.0;
//   return 6.0;
// }

// float calcCalories(float userWeight, float pushTime){
//     float MET = calcMET(pushTime);
//     return (MET * userWeight * (pushTime / 3600.0));
// }

// void detectPushup(){
//     int16_t ax, ay, az;
//     mpu.getAcceleration(&az, &ay, &az);
//     float z = az / 16384.0;

//     if (z < downThreshold && !goingDown){
//         goingDown = true;
//     }
//     else if ( z > upThreshold && goingDown){
//         pushupCount++;
//         unsigned long now = millis();
//         float pushTime = (now - lastPushTime) / 1000.0;
//         lastPushTime = now;
//         durations += pushTime;
//         goingDown = false;

//         float Calories = calcCalories(userWeight, pushTime);
//         totalCalories += calories;

//         PUSHUP_COUNT_CHARACTERISTICS_UUID->setValue(String(pushupCount).c_str());
//         PUSHUP_COUNT_CHARACTERISTICS_UUID->notify();


//         Serial.printf("Pushup #%d | Time: %.2fs | Calories: %.2fkCal ", pushupCount, goingDown, calories);
//     }
//     delay(50);
// }

// // Functions and variable for BLE Bluetooth 
// BLEServer* pServer = NULL;
// BLECharacteristic* pPushupCountCharacteristics = NULL;
// BLECharacteristic* pDurationCharacteristics = NULL;
// BLECharacteristic* pCaloriesCharacteristics = NULL;
// BLECharacteristic* pWeightCharacteristics = NULL;
// BLECharacteristic* pCalibrateCharacteristics  = NULL;
// BLECharacteristic* pStartCharacteristics = NULL;
// BLECharacteristic* pStopCharacteristics = NULL; 

// bool deviceConnected = false;
// bool oldDeviceConnected = false;
// uint32_t value = 0;

// // UUID Settings to establish connection between device and Web App over BLE
// #define SERVICE_UUID                      "19b10000-e8f2-537e-4f6c-d104768a1214"
// #define PUSHUP_COUNT_CHARACTERISTICS_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"
// #define DURATION_CHARACTERISTICS_UUID     "19b10002-e8f2-537e-4f6c-d104768a1214"
// #define CALORIES_CHARACTERISTICS_UUID     "19b10003-e8f2-537e-4f6c-d104768a1214"
// #define WEIGHT_CHARACTERISTICS_UUID       "19b10004-e8f2-537e-4f6c-d104768a1214"
// #define CALIBRATE_CHARACTERISTICS_UUID    "19b10005-e8f2-537e-4f6c-d104768a1214"
// #define START_CHARACTERISTICS_UUID        "19b10006-e8f2-537e-4f6c-d104768a1214"
// #define STOP_CHARACTERISTICS_UUID         "19b10007-e8f2-537e-4f6c-d104768a1214"

// class MyServerCallbacks: public BLEServerCallbacks { 
//     void onConnect(BLEServer* pServer){
//         deviceConnected = true;
//     };
//     void onDisconnect(BLEServer* pServer){
//         deviceConnected = false;
//         if(sessionActive){  // Stop session if diconnect
//             sessionActive = false;
//         }
//     };
// };

// class calibrateCharacteristicsCallbacks : public BLECharacteristicCallbacks{
//     void onWrite(BLECharacteristic* pCalibrateCharacteristics){
//         uint32_t value = 0;
//         String value = pCalibrateCharacteristics->getValue();
//         if(value.length() > 0){
//             Serial.print("Calibrate Characteristic event, write")
//             Serial.println(static_cast<int>(value[0]));

//             int receivedValue = static_cast<int>(value[0]);
//             if(receivedValue == 1){
//                 calibrateSensor();
//                 value = "0";
//             }
//         }
//     }
// }
// class startCharacteristicCallbacks : public BLECharacteristicCallbacks{
//     void onWrite(BLECharacteristic* START_CHARACTERISTICS_UUID){
//         uint32_t value = 0;
//         String value = START_CHARACTERISTICS_UUID->getValue();
//         if(value.length > 0){
//             Serial.print("Start Characteristic event, write")
//             Serial.println(static_cast<int>(value[0]));

//             int receivedValue = static_cast<int>(value[0]);
//             if(receivedValue == 1){
//                 startSession();
//                 value = NULL;
//             }
//         }
//     }
// }

// class stopCharacteristicCallbacks : public BLECharacteristicCallbacks{
//     void onWrite(BLECharacteristic* STOP_CHARACTERISTICS_UUID){
//         uint32_t value = 0;
//         String value = STOP_CHARACTERISTICS_UUID->getValue();
//         if(value.length > 0){
//             Serial.print("Stop Characteristics event, write")
//             Serial.println(static_cast<int>(value[0]));

//             int receivedValue = static_cast<int>(value[0]);
//             if(receivedValue == 1){
//                 stopSession();

//                 value = NULL;
//             }
//         }
//     }
// }

// class weightCharacteristicCallbacks : public BLECharacteristicCallbacks{
//     void onWrite(BLECharacteristic* WEIGHT_CHARACTERISTICS_UUID){
//         uint32_t value = 0;
//         String value = STOP_CHARACTERISTICS_UUID->getValue();
//         if(value.length > 0){
//             Serial.print("Stop Characteristics event, write")
//             Serial.println(static_cast<int>(value[0]));

//             userWeight = static_cast<int>(value[0]);
//         }
//     }
// }
// void setup(){
//     Serial.begin(115200);
//     Wire.begin(21,22);

//     // Initializae MPU
//     mpu.initialize();

//     // Create the BLE Server
//     BLEDevice::init("Pushup Device")
//     pServer = BLEDevice::createServer();
//     pServer->setCallbacks(new MyServerCallbacks());
    
//     // Create the BLE Service
//     BLEService *pService = pServer->createService(SERVICE_UUID);

//     // Create the BLE Characteristics 
//     pPushupCountCharacteristics = pService->createCharacteristic(
//         PUSHUP_COUNT_CHARACTERISTICS_UUID,
//         BLECharacteristic::PROPERTY_READ     |
//         BLECharacteristic::PROPERTY_WRITE    |
//         BLECharacteristic::PROPERTY_NOTIFY   |
//         BLECharacteristic::PROPERTY_INDICATE 
//     );

//     pDurationCharacteristics = pService->createCharacteristic(
//         PUSHUP_COUNT_CHARACTERISTICS_UUID,
//         BLECharacteristic::PROPERTY_READ     |
//         BLECharacteristic::PROPERTY_WRITE    |
//         BLECharacteristic::PROPERTY_NOTIFY   |
//         BLECharacteristic::PROPERTY_INDICATE 
//     );

    
//     pCaloriesCharacteristics = pService->createCharacteristic(
//         CALORIES_CHARACTERISTICS_UUID,
//         BLECharacteristic::PROPERTY_READ     |
//         BLECharacteristic::PROPERTY_WRITE    |
//         BLECharacteristic::PROPERTY_NOTIFY   |
//         BLECharacteristic::PROPERTY_INDICATE 
//     )

//     pWeightCharacteristics = pService->createCharacteristic(
//         WEIGHT_CHARACTERISTICS_UUID,
//         BLECharacteristic::PROPERTY_READ     |
//         BLECharacteristic::PROPERTY_WRITE    |
//         BLECharacteristic::PROPERTY_NOTIFY   |
//         BLECharacteristic::PROPERTY_INDICATE 
//     )

//     pCalibrateCharacteristics = pService->createCharacteristic(
//         CALIBRATE_CHARACTERISTICS_UUID,
//         BLECharacteristic::PROPERTY_WRITE
//     )

//     pStartCharacteristics = pService->createCharacteristic(
//         START_CHARACTERISTICS_UUID,
//         BLECharacteristic::PROPERTY_WRITE
//     )

//     pStopCharacteristics = pService->createCharacteristic(
//         STOP_CHARACTERISTICS_UUID,
//         BLECharacteristic::PROPERTY_WRITE
//     )
    

//     pWeightCharacteristics->setCallbacks(new weightCharacteristicCallbacks());
//     pCalibrateCharacteristics->setCallbacks(new calibrateCharacteristicsCallbacks());
//     pStartCharacteristics->setCallbacks(new startCharacteristicCallbacks())
//     pStopCharacteristics->setCallbacks(new stopCharacteristicCallbacks())

//     pCalibrateCharacteristics->addDescriptor(new BLE2902());
//     pStartCharacteristics->addDescriptor(new BLE2902());
//     pStopCharacteristics->addDescriptor(new BLE2902());
//     pPushupCountCharacteristics->addDescriptor(new BLE2902());
//     pDurationCharacteristics->addDescriptor(new BLE2902());
//     pCaloriesCharacteristics->addDescriptor(new BLE2902());
//     pWeightCharacteristics->addDescriptor(new BLE2902());

//     pService->start();

//     BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
//     pAdvertising->addServiceUUID(SERVICE_UUID);
//     pAdvertising->setScanResponse(false);
//     pAdvertising->setMinPreferred(0x0);
//     BLEDevice::startrAdvertising();

//     Serial.println("\nInitializing Pushup Device")
//     int n = 3;
//     while(n < 3){
//         Serial.print(".")
//         n++;
//     }
//     Serial.println("\nPushup Device has been Started");

//     Serial.println("Waiting a client connection to notify...");
// }

// void loop(){

//     if (deviceConnected){
//         if (sessionActive) detectPushup();
//     }
//     if(!deviceConnected && oldDeviceConnected){
//         Serial.println("Device disconnected.")
//         pServer->startAdvertising();
//         Serial.println("Start advertising.")
//         oldDeviceConnected = deviceConnected;
//     }
//     if(deviceConnected && !old){
//         oldDeviceConnected = deviceConnected;
//         Serial.println("Device Connected");
//     }
// }


// Dependencies
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

bool sessionActive = false;
bool calibrated = false;  // Missing declaration

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
BLECharacteristic* pPushupCountCharacteristics = NULL;  // Fixed naming
BLECharacteristic* pDurationCharacteristics = NULL;      // Fixed naming
BLECharacteristic* pCaloriesCharacteristics = NULL;      // Fixed naming
BLECharacteristic* pWeightCharacteristics = NULL;        // Fixed naming
BLECharacteristic* pCalibrateCharacteristics = NULL;     // Fixed naming
BLECharacteristic* pStartCharacteristics = NULL;         // Fixed naming
BLECharacteristic* pStopCharacteristics = NULL;          // Fixed naming

bool deviceConnected = false;
bool oldDeviceConnected = false;

// UUID Settings
#define SERVICE_UUID                      "19b10000-e8f2-537e-4f6c-d104768a1214"
#define PUSHUP_COUNT_CHARACTERISTICS_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"
#define DURATION_CHARACTERISTICS_UUID     "19b10002-e8f2-537e-4f6c-d104768a1214"
#define CALORIES_CHARACTERISTICS_UUID     "19b10003-e8f2-537e-4f6c-d104768a1214"
#define WEIGHT_CHARACTERISTICS_UUID       "19b10004-e8f2-537e-4f6c-d104768a1214"
#define CALIBRATE_CHARACTERISTICS_UUID    "19b10005-e8f2-537e-4f6c-d104768a1214"
#define START_CHARACTERISTICS_UUID        "19b10006-e8f2-537e-4f6c-d104768a1214"
#define STOP_CHARACTERISTICS_UUID         "19b10007-e8f2-537e-4f6c-d104768a1214"

// Weight Callback
class WeightCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String rxValue = pCharacteristic->getValue();
        
        if (rxValue.length() > 0) {
            Serial.print("Weight Characteristic event, written: ");
            
            // Convert string to float for weight
            String weightStr = "";
            for (int i = 0; i < rxValue.length(); i++) {
                weightStr += (char)rxValue[i];
            }
            
            userWeight = weightStr.toFloat();
            Serial.print("New weight set: ");
            Serial.print(userWeight);
            Serial.println(" kg");
            
            // Update characteristic value
            pCharacteristic->setValue(weightStr.c_str());
            pCharacteristic->notify();
        }
    }
};

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
    
    Serial.println("Session started!");
}

void stopSession() {    
    sessionActive = false;
    
    // Update all characteristics before stopping
    if (pDurationCharacteristics) {
        String durationStr = String(durations, 1);
        pDurationCharacteristics->setValue(durationStr.c_str());
        pDurationCharacteristics->notify();
    }
    
    if (pCaloriesCharacteristics) {
        String caloriesStr = String(totalCalories, 2);
        pCaloriesCharacteristics->setValue(caloriesStr.c_str());
        pCaloriesCharacteristics->notify();
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

        // Update BLE characteristics
        if (pPushupCountCharacteristics) {
            String countStr = String(pushupCount);
            pPushupCountCharacteristics->setValue(countStr.c_str());
            pPushupCountCharacteristics->notify();
        }
        
        if (pDurationCharacteristics) {
            String durationStr = String(durations, 1);
            pDurationCharacteristics->setValue(durationStr.c_str());
            pDurationCharacteristics->notify();
        }
        
        if (pCaloriesCharacteristics) {
            String caloriesStr = String(totalCalories, 2);
            pCaloriesCharacteristics->setValue(caloriesStr.c_str());
            pCaloriesCharacteristics->notify();
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
        // Stop session if active
        if (sessionActive) {
            sessionActive = false;
            Serial.println("Session stopped due to disconnect");
        }
    };
};

// Calibrate Callback
class CalibrateCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String rxValue = pCharacteristic->getValue();
        
        if (rxValue.length() > 0) {
            Serial.print("Calibrate Characteristic event, written: ");
            Serial.println((int)rxValue[0]);

            if (rxValue[0] == 0x01) {  // Using byte comparison
                calibrateSensor();
                // Clear the value after processing
                pCharacteristic->setValue("0");
                Serial.println("Calibration complete via BLE");
            }
        }
    }
};

// Start Callback
class StartCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String rxValue = pCharacteristic->getValue();
        
        if (rxValue.length() > 0) {
            Serial.print("Start Characteristic event, written: ");
            Serial.println((int)rxValue[0]);

            if (rxValue[0] == 0x01) {  // Using byte comparison
                startSession();
                // Clear the value after processing
                pCharacteristic->setValue("0");
                Serial.println("Session started via BLE");
            }
        }
    }
};

// Stop Callback
class StopCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String rxValue = pCharacteristic->getValue();
        
        if (rxValue.length() > 0) {
            Serial.print("Stop Characteristic event, written: ");
            Serial.println((int)rxValue[0]);

            if (rxValue[0] == 0x01) {  // Using byte comparison
                stopSession();
                // Clear the value after processing
                pCharacteristic->setValue("0");
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
    
    // Create the BLE Server
    BLEDevice::init("Pushup Device");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create the BLE Characteristics 
    pPushupCountCharacteristics = pService->createCharacteristic(
        PUSHUP_COUNT_CHARACTERISTICS_UUID,
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pDurationCharacteristics = pService->createCharacteristic(
        DURATION_CHARACTERISTICS_UUID,
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pCaloriesCharacteristics = pService->createCharacteristic(
        CALORIES_CHARACTERISTICS_UUID,
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pWeightCharacteristics = pService->createCharacteristic(
        WEIGHT_CHARACTERISTICS_UUID,
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_WRITE | 
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pCalibrateCharacteristics = pService->createCharacteristic(
        CALIBRATE_CHARACTERISTICS_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );

    pStartCharacteristics = pService->createCharacteristic(
        START_CHARACTERISTICS_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );

    pStopCharacteristics = pService->createCharacteristic(
        STOP_CHARACTERISTICS_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );

    // Set callbacks
    pWeightCharacteristics->setCallbacks(new WeightCharacteristicCallbacks());
    pCalibrateCharacteristics->setCallbacks(new CalibrateCharacteristicCallbacks());
    pStartCharacteristics->setCallbacks(new StartCharacteristicCallbacks());
    pStopCharacteristics->setCallbacks(new StopCharacteristicCallbacks());

    // Add descriptors for notifications
    pPushupCountCharacteristics->addDescriptor(new BLE2902());
    pDurationCharacteristics->addDescriptor(new BLE2902());
    pCaloriesCharacteristics->addDescriptor(new BLE2902());
    pWeightCharacteristics->addDescriptor(new BLE2902());

    // Start service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();
    
    Serial.println("\nPushup Device Started");
    Serial.println("Waiting for BLE connection...");
    Serial.println("Device Name: Pushup Device");
}

void loop() {
    if (deviceConnected && sessionActive) {
        detectPushup();
    }
    
    // Handle disconnection
    if (!deviceConnected && oldDeviceConnected) {
        Serial.println("Device disconnected. Restarting advertising...");
        delay(500);
        pServer->startAdvertising();
        oldDeviceConnected = deviceConnected;
        Serial.println("Advertising restarted");
    }
    
    // Handle connection
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println("Device Connected!");
    }
}