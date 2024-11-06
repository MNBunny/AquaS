#include "DHT.h"
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Arduino.h>

#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Pin definitions
#define RE D4
#define DE D3
#define DHTPIN D1
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN A0

// WiFi and Firebase credentials
#define WIFI_SSID "HUAWEI-Zvkm"
#define WIFI_PASSWORD "jKNK4gmG"
#define API_KEY "AIzaSyBdUTGzi9iQ3asge53BP3UfLALtBghNggQ"
#define DATABASE_URL "https://swmscp-9078d-default-rtdb.firebaseio.com/"

// RS485 commands for NPK sensor
const byte nitro[] = {0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xB5, 0xCC};
const byte phos[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xE4, 0x0C};
const byte pota[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xC0};

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial mod(D7, D6);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
byte values[11];

void setup() {
  Serial.begin(9600);
  mod.begin(9600);
  dht.begin();
  
  pinMode(DHTPIN, INPUT);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);

  // Connect to WiFi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  // Initialize Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase setup OK");
    signupOK = true;
  } else {
    Serial.printf("Firebase signup error: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  delay(6000); // Delay between readings

  byte val1, val2, val3;
  val1 = nitrogen();
  delay(250);
  val2 = phosphorous();
  delay(250);
  val3 = potassium();
  delay(250);

  Serial.print("Nitrogen: ");
  Serial.print(val1);
  Serial.println(" mg/kg");
  
  Serial.print("Phosphorous: ");
  Serial.print(val2);
  Serial.println(" mg/kg");

  Serial.print("Potassium: ");
  Serial.print(val3);
  Serial.println(" mg/kg");
  delay(2000);

  // Read humidity and temperature from DHT sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor! Check wiring or sensor.");
    return;
  }
  
  // Read soil moisture
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 900, 393, 0, 100);

  // Send data to Firebase
  if (Firebase.ready() && signupOK) {
    sendFirebaseData("DHT/humidity", h, "Humidity: ", " %");
    sendFirebaseData("DHT/temperature", t, "Temperature: ", " °C");
    sendFirebaseData("SoilMoisture/Percent_1", soilMoisturePercent, "Soil Moisture: ", " %");

    // Send NPK data
    sendFirebaseData("NPK/Nitrogen", val1, "Nitrogen: ", " mg/kg");
    sendFirebaseData("NPK/Phosphorus", val2, "Phosphorus: ", " mg/kg");
    sendFirebaseData("NPK/Potassium", val3, "Potassium: ", " mg/kg");
  }

  Serial.println("______________________________");
}

void sendFirebaseData(String path, float data, String label, String unit) {
  if (Firebase.RTDB.setFloat(&fbdo, path, data)) {
    Serial.print(label);
    Serial.print(data);
    Serial.println(unit);
  } else {
    Serial.println("FAILED to send " + label + fbdo.errorReason());
  }
}

byte nitrogen() {
  return readNPK(nitro);
}

byte phosphorous() {
  return readNPK(phos);
}

byte potassium() {
  return readNPK(pota);
}

byte readNPK(const byte command[]) {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
  byte response = 0;
  
  if (mod.write(command, sizeof(command)) == 4) {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    
    for (byte i = 0; i < 7; i++) {
      values[i] = mod.read();
      Serial.print(values[i], HEX);
    }
    Serial.println();
    response = values[4];
  }
  
  return response;
}
