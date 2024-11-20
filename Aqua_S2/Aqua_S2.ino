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

#define WIFI_SSID "GlobeAtHome_d7d38_2.4"
#define WIFI_PASSWORD "Jy6YEfHQ"
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
#define DHTPIN D0
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN A0

DHT dht(DHTPIN, DHTTYPE);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  pinMode(DHTPIN, INPUT);
  
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

void loop() {
  delay(1200000); // Delay 20 minutes

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor! Check wiring or sensor.");
    return;
  }

  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 900, 393, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  if (Firebase.ready() && signupOK) {
    int soilMoisturePercentRealtime = 0;

    if (Firebase.RTDB.getInt(&fbdo, "SoilMoisture/Percent_2")) {
      soilMoisturePercentRealtime = fbdo.intData();
    }

    if (Firebase.RTDB.setInt(&fbdo, "SoilMoisture/Percent_2", soilMoisturePercent)) {
      Serial.print("Soil Moisture Sent: ");
      Serial.println(soilMoisturePercent);
    } else {
      Serial.println("Failed to send Soil Moisture reading.");
    }

    Firebase.RTDB.setFloat(&fbdo, "DHT/humidity", h);
    Firebase.RTDB.setFloat(&fbdo, "DHT/temperature", t);

    // Display data on OLED
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(0, 12);
    u8g2.print("Humidity: ");
    u8g2.print(h, 2);
    u8g2.setCursor(0, 24);
    u8g2.print("Temperature: ");
    u8g2.print(t, 2);
    u8g2.setCursor(0, 36);
    u8g2.print("Soil Moisture: ");
    u8g2.print(soilMoisturePercentRealtime);
    u8g2.print(" %");
    u8g2.sendBuffer();

    // Watering logic
    String currentTime = getCurrentTime();
    if (soilMoisturePercentRealtime <= 10) {
      Serial.println("Watering plants immediately due to dryness.");
      digitalWrite(RELAY1_PIN, HIGH);
      delay(5000);
      digitalWrite(RELAY1_PIN, LOW);
    } else if (currentTime.startsWith("05:00") || currentTime.startsWith("17:00")) {
      Serial.println("Scheduled watering.");
      digitalWrite(RELAY1_PIN, HIGH);
      delay(5000);
      digitalWrite(RELAY1_PIN, LOW);
    } else {
      Serial.println("No watering needed.");
    }
  }
}

// Dummy time function
String getCurrentTime() {
  return "12:00"; // Replace with real-time retrieval logic
}
