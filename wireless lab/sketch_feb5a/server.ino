#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// TODO: add new global variables for your sensor readings and processed data
float distance = 0.0;
float processedDistance = 0.0;

// float filteredValue = 0.0;

#define TRIG_PIN 2                                                          // 触发信号引脚
#define ECHO_PIN 21                                                         // 回声信号引脚

// TODO: Change the UUID to your own (any specific one works, but make sure they're different from others'). You can generate one here: https://www.uuidgenerator.net/
#define SERVICE_UUID        "277f5112-6656-451d-bbbb-716358bec63e"
#define CHARACTERISTIC_UUID "ff355124-3c94-42c2-b4b7-9e5dbea6c0e1"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

// TODO: add DSP algorithm functions here
// #define WINDOW_SIZE 10
// float movingAverage(float newValue, float values[], int& index) {
//     values[index] = newValue;
//     index = (index + 1) % WINDOW_SIZE;
    
//     float sum = 0;
//     for (int i = 0; i < WINDOW_SIZE; i++) {
//         sum += values[i];
//     }
//     return sum / WINDOW_SIZE;
// }
// Simple low-pass filter (exponential smoothing)
float lowPassFilter(float newValue, float prevValue, float alpha) {
    return alpha * newValue + (1.0 - alpha) * prevValue;
}


void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // TODO: add codes for handling your sensor setup (pinMode, etc.)
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // TODO: name your device to avoid conflictions
    BLEDevice::init("ESP32_Gia_Xie");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();
    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // TODO: add codes for handling your sensor readings (analogRead, etc.)
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * 0.034 / 2;

    // TODO: use your defined DSP algorithm to process the readings
    processedDistance = lowPassFilter(distance, processedDistance,0.6);
    Serial.print("Raw Distance: ");
    Serial.println(distance);
    Serial.print("Processed Distance: ");
    Serial.println(processedDistance);
    
    if (deviceConnected) {
        // Send new readings to database
        // TODO: change the following code to send your own readings and processed data
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval and processedDistance <= 30) {
          String distanceStr = String(processedDistance, 2); 
          pCharacteristic->setValue(distanceStr.c_str());
          pCharacteristic->notify();
          // Serial.println("Notify value: Hello World");
          Serial.print("Notified value: ");
          Serial.println(distanceStr);
          Serial.print("Raw Distance: ");
          Serial.println(distance);
          Serial.print("Processed Distance: ");
          Serial.println(processedDistance);
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}


// 277f5112-6656-451d-bbbb-716358bec63e