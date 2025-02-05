#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "UW MPSK"
#define WIFI_PASSWORD "*(eEtdNa6Y" // Replace with your network password

#define DATABASE_SECRET "AIzaSyDDnLXXJfRUfmmAa8xEuykGIvmzPBcsIBQ" // Replace with your API key
#define DATABASE_URL "https://battery-assignment-e7597-default-rtdb.firebaseio.com" // Replace with your database URL

#define MAX_WIFI_RETRIES 5  // Maximum WiFi connection retries
#define DEEP_SLEEP_TIME 30   // Deep sleep duration (30 seconds)
#define OBJECT_DETECTION_THRESHOLD 70  // Distance threshold for detecting an object (70cm)
#define CHECK_INTERVAL 5000    // Check distance every 5 seconds when object is detected
#define NO_OBJECT_TIMEOUT 10000  // If no object is detected for 10 seconds, enter deep sleep

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));

FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

// Ultrasonic sensor pins
const int trigPin = 0;
const int echoPin = 4;
const float soundSpeed = 0.034; // Speed of sound

// Function declarations
float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  connectToWiFi();
  initFirebase();
}

void loop() {
  unsigned long startTime = millis();
  bool objectDetected = false;

  Serial.println("Starting detection...");

  // 改成 10 秒未检测到物体就进入深度睡眠
  while (millis() - startTime < NO_OBJECT_TIMEOUT) {
    float distance = measureDistance();

    if (distance <= OBJECT_DETECTION_THRESHOLD) {
      objectDetected = true;
      Serial.println("Object detected! Keeping ESP32 active...");
      break;
    }

    delay(2000);  // **降低检测频率，每 2 秒检测一次**
  }

  if (!objectDetected) {
    Serial.println("No object detected for 10 seconds. Entering deep sleep...");
    WiFi.disconnect();
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME * 1000000);  // **设置 30 秒深度睡眠**
    esp_deep_sleep_start();
  }

  while (objectDetected) {
    float distance = measureDistance();
    sendDataToFirebase(distance);
    delay(CHECK_INTERVAL);
  }
}

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

void connectToWiFi() {
  Serial.println(WiFi.macAddress());
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi...");
  int wifiCnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiCnt++;
    if (wifiCnt > MAX_WIFI_RETRIES) {
      Serial.println("WiFi connection failed");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initFirebase() {
  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  ssl.setInsecure();
#if defined(ESP8266)
  ssl.setBufferSizes(1024, 1024);
#endif

  Serial.println("Initializing Firebase...");
  initializeApp(client, app, getAuth(dbSecret));
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  client.setAsyncResult(result);
}

void sendDataToFirebase(float distance) {
  Serial.print("Uploading distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  String name = Database.push<number_t>(client, "/test/distance", number_t(distance));
  if (client.lastError().code() == 0) {
    Serial.println("Data uploaded successfully");
  } else {
    Serial.print("Upload failed: ");
    Serial.println(client.lastError().message().c_str());
  }
}
