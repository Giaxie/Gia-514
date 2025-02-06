#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Client Code
#include "BLEDevice.h"

static BLEUUID serviceUUID("277f5112-6656-451d-bbbb-716358bec63e");
static BLEUUID charUUID("ff355124-3c94-42c2-b4b7-9e5dbea6c0e1");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// Added global variables
float maxDistance = -INFINITY;
float minDistance = INFINITY;
int dataCount = 0;

// Data aggregation function
void updateDistanceStats(float distance) {
    if (distance > maxDistance) {
        maxDistance = distance;
    }
    if (distance < minDistance) {
        minDistance = distance;
    }
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    // Convert received data to float
    float currentDistance = atof((char*)pData);
    
    // Update statistics
    updateDistanceStats(currentDistance);
    dataCount++;

    // Print required information
    Serial.print("Current Distance: ");
    Serial.print(currentDistance);
    Serial.print(" | Max: ");
    Serial.print(maxDistance);
    Serial.print(" | Min: ");
    Serial.println(minDistance);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);
    pClient->setMTU(517);

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        pClient->disconnect();
        return false;
    }

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        pClient->disconnect();
        return false;
    }

    if(pRemoteCharacteristic->canNotify())
        pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Client...");
  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Connected to BLE Server");
    } else {
      Serial.println("Connection failed");
    }
    doConnect = false;
  }

  if (connected) {
    // Removed the writeValue operation
    // Add delay to prevent spamming
    delay(100);
  } else if(doScan) {
    BLEDevice::getScan()->start(0);
  }
}