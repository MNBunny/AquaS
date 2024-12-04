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
  delay(600000); // Delay 1 minute between readings

  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 880, 340, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  if (Firebase.ready() && signupOK) {
    // Fetch data from Firebase for soil moisture and nutrients
    int soilMoisturePercent1 = 0, soilMoisturePercentRealtime = 0;
    Firebase.RTDB.getInt(&fbdo, "SoilMoisture/Percent_1") ? soilMoisturePercent1 = fbdo.intData() : Serial.println("Failed to read Percent_1");
    Firebase.RTDB.setInt(&fbdo, "SoilMoisture/Percent_2", soilMoisturePercent) ? 
      Serial.println("Updated Percent_2 in Firebase") : 
      Serial.println("Failed to update Percent_2");
    int averageSoilMoisture = (soilMoisturePercent1 + soilMoisturePercent) / 2;

    // Fetch NPK data (Nutrients)
    int nitrogen = 0, phosphorus = 0, potassium = 0;
    Firebase.RTDB.getInt(&fbdo, "NPK/Nitrogen") ? nitrogen = fbdo.intData() : Serial.println("Failed to read Nitrogen");
    Firebase.RTDB.getInt(&fbdo, "NPK/Phosphorus") ? phosphorus = fbdo.intData() : Serial.println("Failed to read Phosphorus");
    Firebase.RTDB.getInt(&fbdo, "NPK/Potassium") ? potassium = fbdo.intData() : Serial.println("Failed to read Potassium");

    // Check if moisture is needed
    bool moistureNeeded = averageSoilMoisture <= 55;
    
    // Check if nutrients are needed
    bool nutrientsNeeded = (nitrogen < 31 || nitrogen > 34 || phosphorus < 31 || phosphorus > 34 || potassium < 31 || potassium > 34);

    // Display average soil moisture on OLED screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, "Avg Moisture:");
    u8g2.setCursor(95, 15);
    u8g2.print(averageSoilMoisture);
    u8g2.print(" %");

    if (moistureNeeded && nutrientsNeeded) {
      // Both are needed (Watering + Fertilizer)
      u8g2.setCursor(0, 30);
      u8g2.print("Watering + Fertilizer");
    } else if (moistureNeeded) {
      // Only watering is needed
      u8g2.setCursor(0, 30);
      u8g2.print("Watering only");
    } else if (nutrientsNeeded) {
      // Only fertilizer is needed
      u8g2.setCursor(0, 30);
      u8g2.print("Fertilizer only");
    } else {
      // Conditions are fine, no action needed
      u8g2.setCursor(0, 30);
      u8g2.print("Conditions OK");
    }

    u8g2.sendBuffer();  // Update OLED with the current status

    // Perform operations based on needs
    if (moistureNeeded && nutrientsNeeded) {
      // Step 1: Fertilizer if nutrients are needed (Relay 3)
      digitalWrite(RELAY2_PIN, LOW);  // Open fertilizer relay
      delay(2000);  // 2 seconds for fertilizer dispensing
      digitalWrite(RELAY2_PIN, HIGH);  // Close fertilizer relay

      // Step 2: Pre-watering if moisture is low (Relay 2)
      digitalWrite(RELAY3_PIN, LOW);  // Open pre-watering relay
      delay(14000);  // 15 seconds pre-watering
      digitalWrite(RELAY3_PIN, HIGH);  // Close pre-watering relay

      // Step 3: Watering cycle if moisture is low (Relay 1)
      for (int cycle = 0; cycle < 3; cycle++) {
        digitalWrite(RELAY1_PIN, LOW);  // Open watering relay
        delay(8000);  // Watering duration
        digitalWrite(RELAY1_PIN, HIGH);  // Close watering relay
        delay(5000);  // Pause before next cycle

        // Update soil moisture after watering cycle
        averageSoilMoisture = (soilMoisturePercent1 + soilMoisturePercent) / 2;
        if (averageSoilMoisture >= 60) {
          break;  // Stop watering if moisture is sufficient
        }
      }

    } else if (moistureNeeded) {
      // Only moisture-related operations
      digitalWrite(RELAY3_PIN, LOW);  // Open pre-watering relay
      delay(14000);  // 15 seconds pre-watering
      digitalWrite(RELAY3_PIN, HIGH);  // Close pre-watering relay

      // Watering cycle
      for (int cycle = 0; cycle < 3; cycle++) {
        digitalWrite(RELAY1_PIN, LOW);  // Open watering relay
        delay(8000);  // Watering duration
        digitalWrite(RELAY1_PIN, HIGH);  // Close watering relay
        delay(5000);  // Pause before next cycle

        // Update soil moisture after watering cycle
        averageSoilMoisture = (soilMoisturePercent1 + soilMoisturePercent) / 2;
        if (averageSoilMoisture >= 60) {
          break;  // Stop watering if moisture is sufficient
        }
      }

    } else if (nutrientsNeeded) {
      // Only nutrients-related operations (Relay 3, 1, 2)
      // Step 1: Fertilizer if nutrients are needed (Relay 3)
      digitalWrite(RELAY2_PIN, LOW);  // Open fertilizer relay
      delay(2000);  // 2 seconds for fertilizer dispensing
      digitalWrite(RELAY2_PIN, HIGH);  // Close fertilizer relay

      // Step 2: Pre-watering if moisture is low (Relay 2)
      digitalWrite(RELAY3_PIN, LOW);  // Open pre-watering relay
      delay(14000);  // 15 seconds pre-watering
      digitalWrite(RELAY3_PIN, HIGH);  // Close pre-watering relay

      // Step 3: Watering cycle if moisture is low (Relay 1)
      for (int cycle = 0; cycle < 3; cycle++) {
        digitalWrite(RELAY1_PIN, LOW);  // Open watering relay
        delay(8000);  // Watering duration
        digitalWrite(RELAY1_PIN, HIGH);  // Close watering relay
        delay(5000);  // Pause before next cycle

        // Update soil moisture after watering cycle
        averageSoilMoisture = (soilMoisturePercent1 + soilMoisturePercent) / 2;
        if (averageSoilMoisture >= 60) {
          break;  // Stop watering if moisture is sufficient
        }
      }
    }
  }
}
