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
#define RELAY1_PIN D5
#define RELAY2_PIN D6
#define RELAY3_PIN D7
#define RELAY4_PIN D8
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

  // Initialize all relays to OFF
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);
}

bool relay2Activated = false; // Flag to track if Relay 2 has been activated
bool relay3Activated = false; // Flag to track if Relay 3 has been activated

void loop() {
  delay(600000); // Delay 10 minutes between readings

  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 880, 340, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  if (Firebase.ready() && signupOK) {
    int soilMoisturePercentRealtime = 0;
    int soilMoisturePercent1 = 0;

    // Read the soil moisture percentage for Percent_2
    if (Firebase.RTDB.getInt(&fbdo, "SoilMoisture/Percent_2")) {
      soilMoisturePercentRealtime = fbdo.intData();
    }

    // Read the soil moisture percentage for Percent_1
    if (Firebase.RTDB.getInt(&fbdo, "SoilMoisture/Percent_1")) {
      soilMoisturePercent1 = fbdo.intData();
    }

    // Send the current soil moisture data to Firebase
    if (Firebase.RTDB.setInt(&fbdo, "SoilMoisture/Percent_2", soilMoisturePercent)) {
      Serial.print("Soil Moisture Sent: ");
      Serial.println(soilMoisturePercent);
    } else {
      Serial.println("Failed to send Soil Moisture reading.");
    }

    // Calculate the average of Percent_1 and Percent_2
    int averageSoilMoisture = (soilMoisturePercent1 + soilMoisturePercentRealtime) / 2;

    // Read NPK values from Firebase
    int nitrogen = 0;
    int phosphorus = 0;
    int potassium = 0;

    if (Firebase.RTDB.getInt(&fbdo, "NPK/Nitrogen")) {
      nitrogen = fbdo.intData();
    }

    if (Firebase.RTDB.getInt(&fbdo, "NPK/Phosphorus")) {
      phosphorus = fbdo.intData();
    }

    if (Firebase.RTDB.getInt(&fbdo, "NPK/Potassium")) {
      potassium = fbdo.intData();
    }

    // Clear the OLED and display the data
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);

    u8g2.drawStr(0, 15, "Avg Moisture:");
    u8g2.setCursor(95, 15);
    u8g2.print(averageSoilMoisture);
    u8g2.print(" %");

    // Display NPK values
    u8g2.setCursor(0, 30);
    u8g2.print("N: ");
    u8g2.print(nitrogen);
    u8g2.setCursor(0, 45);
    u8g2.print("P: ");
    u8g2.print(phosphorus);
    u8g2.setCursor(0, 60);
    u8g2.print("K: ");
    u8g2.print(potassium);

    u8g2.sendBuffer();

    // Watering logic based on the average soil moisture value
    if (averageSoilMoisture <= 55) {
      Serial.println("Watering plants immediately due to dryness.");
      
      // If Relay 2 has not been activated yet, turn it on for 15 seconds
      if (!relay2Activated) {
        digitalWrite(RELAY2_PIN, HIGH); // Turn on Relay 2 (first step)
        delay(15000); // Keep Relay 2 on for 15 seconds
        digitalWrite(RELAY2_PIN, LOW); // Turn off Relay 2
        relay2Activated = true; // Set the flag to indicate Relay 2 has been activated
        delay(5000); // Pause for 5 seconds before starting the next step
      }

      // Check NPK value for Relay 3 operation
      if (nitrogen >= 31 && nitrogen <= 34 && phosphorus >= 31 && phosphorus <= 34 && potassium >= 31 && potassium <= 34 && !relay3Activated) {
        Serial.println("NPK values are in range. Activating Relay 3.");
        digitalWrite(RELAY3_PIN, HIGH); // Turn on Relay 3 (for 5 seconds)
        delay(5000); // Relay 3 runs for 5 seconds
        digitalWrite(RELAY3_PIN, LOW); // Turn off Relay 3
        relay3Activated = true; // Set the flag to indicate Relay 3 has been activated
        delay(5000); // Pause for 5 seconds before starting the next step
      }

      // Cycle process for Relay 1 (water pump)
      for (int cycle = 0; cycle < 3; cycle++) {
        digitalWrite(RELAY1_PIN, HIGH); // Turn the pump ON
        delay(15000); // Keep the pump ON for 15 seconds
        digitalWrite(RELAY1_PIN, LOW); // Turn the pump OFF
        delay(5000); // Pause for 5 seconds
        
        // Recalculate the average soil moisture
        averageSoilMoisture = (soilMoisturePercent1 + soilMoisturePercentRealtime) / 2;

        // Check if soil moisture has reached the threshold
        if (averageSoilMoisture >= 60) {
          Serial.println("Soil moisture level has reached 55%. Stopping watering.");
          break; // Exit the loop if moisture is sufficient
        }
      }
    } else {
      // If soil moisture is above 45%, reset relay2Activated flag
      relay2Activated = false;
      relay3Activated = false; // Reset Relay 3 activation flag
      Serial.println("No watering needed.");
    }
  }
}
