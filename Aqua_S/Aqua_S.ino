#include "DHT.h"
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <ModbusMaster.h>

// Pin definitions for RS485 communication
#define RE D4  // Modified RE pin since D4 is used for DHT11
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

// Create an instance of the ModbusMaster object
ModbusMaster node;

void preTransmission() {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
}

void postTransmission() {
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);
}

void setup() {
  Serial.begin(9600);
  
  dht.begin();
  pinMode(DHTPIN, INPUT);

  // RS485 communication pin modes
  pinMode(DE, OUTPUT);
  pinMode(RE, OUTPUT);

  // Initialize Modbus communication
  node.begin(1, Serial);  // 1 is the Modbus slave ID
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

  // Get NPK sensor values via RS485
  uint8_t result;
  uint16_t data[3];  // Assuming the NPK sensor returns 3 values (N, P, K)
  
  result = node.readInputRegisters(0x0000, 3);  // Address and number of registers to read
  if (result == node.ku8MBSuccess) {
    data[0] = node.getResponseBuffer(0x00);  // Nitrogen value
    data[1] = node.getResponseBuffer(0x01);  // Phosphorus value
    data[2] = node.getResponseBuffer(0x02);  // Potassium value
  } else {
    Serial.println("Failed to read from NPK sensor! Check wiring or sensor.");
  }

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
    if (result == node.ku8MBSuccess) {
      if (Firebase.RTDB.setInt(&fbdo, "NPK/Nitrogen", data[0])) {
        Serial.print("Nitrogen: ");
        Serial.println(data[0]);
      } else {
        Serial.println("FAILED to send Nitrogen: " + fbdo.errorReason());
      }

      if (Firebase.RTDB.setInt(&fbdo, "NPK/Phosphorus", data[1])) {
        Serial.print("Phosphorus: ");
        Serial.println(data[1]);
      } else {
        Serial.println("FAILED to send Phosphorus: " + fbdo.errorReason());
      }

      if (Firebase.RTDB.setInt(&fbdo, "NPK/Potassium", data[2])) {
        Serial.print("Potassium: ");
        Serial.println(data[2]);
      } else {
        Serial.println("FAILED to send Potassium: " + fbdo.errorReason());
      }
    }
  }

  Serial.println("______________________________");
}
