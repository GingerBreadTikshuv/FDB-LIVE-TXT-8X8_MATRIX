#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <SPI.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 1
#define CS_PIN 5

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

#define FIREBASE_HOST "https://esp32-dedmo-default-rtdb.firebaseio.com/" 
#define FIREBASE_AUTH "AIzaSyDtL41YZ7JA9QlX1Myvs0474dXZ_PeOAJI" 

#define WIFI_SSID "TP_Link"
#define WIFI_PASSWORD "aris564845"

#define TRIGGER_PIN 26 // HC-SR04 trigger pin
#define ECHO_PIN 25    // HC-SR04 echo pin
#define LDR_PIN 34     // LDR analog pin

FirebaseData fbdo;

unsigned long updateInterval = 5000; // Default delay interval in milliseconds

void setup() {
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  P.begin();

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

bool displayNeedsClearing = true;
bool dataFetched = false;
String valueFromDB;

void loop() {
  static unsigned long lastUpdate = 0;
  static unsigned long lastDelayFetch = 0;
  unsigned long currentMillis = millis();

  // Fetch delay from Firebase every 5 seconds
  if (currentMillis - lastDelayFetch > 5000) {
    if (Firebase.getInt(fbdo, "/test/delay")) { // Fetch the delay from Firebase
      updateInterval = fbdo.intData() * 1000; // Convert to milliseconds if delay is in seconds
      Serial.print("Fetched delay: ");
      Serial.println(updateInterval);
      lastDelayFetch = currentMillis;
    } else {
      Serial.print("Failed to get delay from Firebase. Error: ");
      Serial.println(fbdo.errorReason());
    }
  }

  if (currentMillis - lastUpdate > updateInterval) { // Update every x seconds
    long duration, distance;
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);
    distance = (duration / 2) / 29.1; // calculate the distance

    Firebase.setInt(fbdo,"/test/distance", distance); // Send the distance to Firebase

    lastUpdate = currentMillis;
  }

  int ldrValue = analogRead(LDR_PIN); // read the ldr value
  if (ldrValue > 450) {
    P.setIntensity(0, 15); // Set the brightness of the display to maximum
  } else {
    P.setIntensity(0, 0); // Set the brightness of the display to minimum
  }

  Serial.println(ldrValue);

  if (!dataFetched) {
    if (Firebase.getString(fbdo, "/test/distance")) { // Fetch the distance from Firebase
      valueFromDB = fbdo.stringData();

      Serial.print("Data from Firebase: ");
      Serial.println(valueFromDB);

      if (displayNeedsClearing) {
        P.displayClear();
        P.displayText(valueFromDB.c_str(), PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        displayNeedsClearing = false;
      }
      dataFetched = true;
    } else {
      Serial.print("Failed to get data from Firebase. Error: ");
      Serial.println(fbdo.errorReason());
    }
  }

  if (P.displayAnimate()) {
    displayNeedsClearing = true;
    P.displayReset();
    dataFetched = false;
  }
}
