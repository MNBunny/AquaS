#include "DHT.h"
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>

// Pin definitions for RS485 communication
#define RE D4
#define DE D3

#define DHTPIN D1
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN A0

DHT dht(DHTPIN, DHTTYPE);

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "HUAWEI-Zvkm"
#define WIFI_PASSWORD "jKNK4gmG"
#define API_KEY "AIzaSyBdUTGzi9iQ3asge53BP3UfLALtBghNggQ"
#define DATABASE_URL "https://swmscp-9078d-default-rtdb.firebaseio.com/" 

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

// Modbus and RS485 communication
ModbusMaster node;
SoftwareSerial mod(2, 3); // RX, TX for NPK sensor

const byte nitro[] = {0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};
byte values[11];

void preTransmission() {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
}

void postTransmission() {
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);
}

byte readNPK(const byte *command) {
  mod.write(command, 8);
  delay(1000);
  if (mod.available() == 11) {
    for (int i = 0; i < 11; i++) {
      values[i] = mod.read();
    }
    return values[4];  // Return the byte that contains the sensor value
  }
  return 0;
}

void setup() {
  Serial.begin(9600);
  
  dht.begin();
  pinMode(DHTPIN, INPUT);

  // RS485 communication pin modes
  pinMode(DE, OUTPUT);
  pinMode(RE, OUTPUT);

  // Initialize Modbus communication
  mod.begin(9600);
  node.begin(2, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("Firebase setup OK");
    signupOK = true;
  }
  else {
    Serial.printf("Firebase signup error: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  delay(6000); // Delay between readings

  // Read humidity and temperature from the DHT sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor! Check wiring or sensor.");
    return;  // Exit loop if there's an issue reading DHT sensor
  }
  
  // Read soil moisture
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 900, 393, 0, 100);

  // Read NPK values
  byte nitrogenValue = readNPK(nitro);
  byte phosphorousValue = readNPK(phos);
  byte potassiumValue = readNPK(pota);

  // Send data to Firebase
  if (Firebase.ready() && signupOK) {
    // Send DHT data
    if (Firebase.RTDB.setFloat(&fbdo, "DHT/humidity", h)) {
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.println(" %");
    } else {
      Serial.println("FAILED to send humidity: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "DHT/temperature", t)) {
      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.println(" °C");
    } else {
      Serial.println("FAILED to send temperature: " + fbdo.errorReason());
    }

    // Send Soil Moisture data
    if (Firebase.RTDB.setInt(&fbdo, "SoilMoisture/Percent_1", soilMoisturePercent)) {
      Serial.print("Soil Moisture: ");
      Serial.print(soilMoisturePercent);
      Serial.println(" %");
    } else {
      Serial.println("FAILED to send soil moisture: " + fbdo.errorReason());
    }

    // Send NPK data
    if (Firebase.RTDB.setInt(&fbdo, "NPK/Nitrogen", nitrogenValue)) {
      Serial.print("Nitrogen: ");
      Serial.println(nitrogenValue);
    } else {
      Serial.println("FAILED to send Nitrogen: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "NPK/Phosphorous", phosphorousValue)) {
      Serial.print("Phosphorous: ");
      Serial.println(phosphorousValue);
    } else {
      Serial.println("FAILED to send Phosphorous: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "NPK/Potassium", potassiumValue)) {
      Serial.print("Potassium: ");
      Serial.println(potassiumValue);
    } else {
      Serial.println("FAILED to send Potassium: " + fbdo.errorReason());
    }
  }

  Serial.println("______________________________");
}
