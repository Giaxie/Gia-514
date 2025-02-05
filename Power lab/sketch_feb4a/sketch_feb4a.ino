#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

// WiFi & Firebase Credentials
#define WIFI_SSID "UW MPSK"
#define WIFI_PASSWORD "*(eEtdNa6Y"
#define DATABASE_URL "https://battery-assignment-e7597-default-rtdb.firebaseio.com"
#define DATABASE_SECRET "AIzaSyDDnLXXJfRUfmmAa8xEuykGIvmzPBcsIBQ"

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

// HC-SR04 Sensor Pins
const int trigPin = 0;
const int echoPin = 4;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

// Power-saving parameters
const int deepSleepDuration = 30; // 30 seconds deep sleep
const int measurementInterval = 5000; // Measure every 5 seconds
const int uploadInterval = 60000; // Upload every 60 seconds
float lastDistance = -1;
bool objectDetected = false;

void setup() {
    Serial.begin(115200);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    
    // Measure distance
    float currentDistance = measureDistance();

    // Check for movement
    if (currentDistance <= 50) {
        objectDetected = true;
    } else {
        objectDetected = false;
    }

    // Upload to Firebase if an object is detected
    if (objectDetected) {
        connectToWiFi();
        initFirebase();
        sendDataToFirebase(currentDistance);
        WiFi.disconnect();
    }

    // Enter deep sleep mode if no object is detected for 30s
    Serial.println("Entering deep sleep...");
    esp_sleep_enable_timer_wakeup(deepSleepDuration * 1000000);
    esp_deep_sleep_start();
}

void loop() {
    // Never reached
}

// Function to measure distance
float measureDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float distance = duration * soundSpeed / 2;

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    return distance;
}

// Connect to WiFi
void connectToWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    Serial.println("\nConnected to WiFi");
}

// Initialize Firebase
void initFirebase() {
    ssl.setInsecure();
    initializeApp(client, app, getAuth(dbSecret));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    client.setAsyncResult(result);
}

// Send data to Firebase
void sendDataToFirebase(float distance) {
    Serial.print("Uploading distance: ");
    Serial.println(distance);
    Database.push<number_t>(client, "/distance", number_t(distance));
}
