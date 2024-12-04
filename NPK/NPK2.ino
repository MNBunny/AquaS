#include <Firebase_ESP_Client.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <DHT.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

#define WIFI_SSID "HUAWEI-Zvkm"
#define WIFI_PASSWORD "jKNK4gmG"
#define API_KEY "AIzaSyBdUTGzi9iQ3asge53BP3UfLALtBghNggQ"
#define DATABASE_URL "https://swmscp-9078d-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

// Pins definition
#define RELAY1_PIN 14 // GPIO14 (D5)
#define RELAY2_PIN 12 // GPIO12 (D6)
#define RELAY3_PIN 13 // GPIO13 (D7)
#define RELAY4_PIN 15 // GPIO15 (D8)
#define SOIL_MOISTURE_PIN A0


U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  Serial.begin(9600);
  
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(25, 15, "Initializing...");
  u8g2.sendBuffer();
  delay(3000);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  
  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase initialized.");
    signupOK = true;
  } else {
    Serial.printf("Firebase Sign-up Error: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize relay pins
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(RELAY4_PIN, HIGH);

}

void loop() {
  delay(60000); // Delay 10 minutes between readings

  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 880, 340, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  if (Firebase.ready() && signupOK) {
    // Fetch data from Firebase
    int soilMoisturePercent1 = 0, soilMoisturePercentRealtime = 0;
    Firebase.RTDB.getInt(&fbdo, "SoilMoisture/Percent_1") ? soilMoisturePercent1 = fbdo.intData() : Serial.println("Failed to read Percent_1");
    Firebase.RTDB.setInt(&fbdo, "SoilMoisture/Percent_2", soilMoisturePercent) ? 
      Serial.println("Updated Percent_2 in Firebase") : 
      Serial.println("Failed to update Percent_2");
    int averageSoilMoisture = (soilMoisturePercent1 + soilMoisturePercent) / 2;

    // Display data on OLED
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, "Avg Moisture:");
    u8g2.setCursor(95, 15);
    u8g2.print(averageSoilMoisture);
    u8g2.print(" %");
    u8g2.sendBuffer();

    // Watering logic
    if (averageSoilMoisture <= 55) {
      Serial.println("Low moisture detected, starting irrigation.");

      // Step 1: Pre-watering (Relay 2)
      digitalWrite(RELAY2_PIN, LOW);
      delay(15000);
      digitalWrite(RELAY2_PIN, HIGH);

      // Step 2: Fertilizer check (Relay 3)
      int nitrogen = 0, phosphorus = 0, potassium = 0;
      Firebase.RTDB.getInt(&fbdo, "NPK/Nitrogen") ? nitrogen = fbdo.intData() : Serial.println("Failed to read Nitrogen");
      Firebase.RTDB.getInt(&fbdo, "NPK/Phosphorus") ? phosphorus = fbdo.intData() : Serial.println("Failed to read Phosphorus");
      Firebase.RTDB.getInt(&fbdo, "NPK/Potassium") ? potassium = fbdo.intData() : Serial.println("Failed to read Potassium");

      if (nitrogen >= 31 && nitrogen <= 34 && phosphorus >= 31 && phosphorus <= 34 && potassium >= 31 && potassium <= 34) {
        Serial.println("NPK values in range, activating fertilizer.");
        digitalWrite(RELAY3_PIN, LOW);
        delay(5000);
        digitalWrite(RELAY3_PIN, HIGH);
      }

      // Step 3: Watering cycle (Relay 1)
      for (int cycle = 0; cycle < 3; cycle++) {
        digitalWrite(RELAY1_PIN, LOW);
        delay(15000);
        digitalWrite(RELAY1_PIN, HIGH);
        delay(5000);

        // Update soil moisture
        averageSoilMoisture = (soilMoisturePercent1 + soilMoisturePercent) / 2;
        if (averageSoilMoisture >= 60) {
          Serial.println("Sufficient moisture achieved, stopping watering.");
          break;
        }
      }
    } else {
      Serial.println("Moisture adequate, no watering needed.");
    }
  }
}
