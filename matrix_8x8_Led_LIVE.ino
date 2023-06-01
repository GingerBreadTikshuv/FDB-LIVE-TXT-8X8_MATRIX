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

FirebaseData fbdo;

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
}

bool displayNeedsClearing = true; // Flag to track if the display needs to be cleared
bool dataFetched = false;
String valueFromDB;

void loop() {
  if (!dataFetched){
    if (Firebase.getString(fbdo, "/test/int")){
      valueFromDB = fbdo.stringData();
    
      // Debug: Check if data is fetched from Firebase
      Serial.print("Data from Firebase: ");
      Serial.println(valueFromDB);

      if (displayNeedsClearing) {
        P.displayClear(); // Clear the display only when necessary
        P.displayText(valueFromDB.c_str(), PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        displayNeedsClearing = false; // Reset the flag
      }
      dataFetched = true;
    } else {
      // Debug: Check for errors in fetching data from Firebase
      Serial.print("Failed to get data from Firebase. Error: ");
      Serial.println(fbdo.errorReason());
    }
  }
  
  if (P.displayAnimate()) {
    displayNeedsClearing = true; // Set the flag to indicate that the display needs to be cleared before displaying new content
    P.displayReset();
    dataFetched = false;
  }
}
