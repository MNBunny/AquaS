//#include <Wire.h>
//#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

#define DHTPIN D4
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN A0
#define RELAY_PIN D3

DHT dht(DHTPIN, DHTTYPE);
//LiquidCrystal_I2C lcd(0x27, 16, 2);

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

bool relayOn = false;
unsigned long relayStartMillis = 0;
const unsigned long relayOnDuration = 2 * 60 * 1000; // 2 minutes in milliseconds

#define WIFI_SSID "GlobeAtHome_d7d38_2.4"
#define WIFI_PASSWORD "Jy6YEfHQ"
#define API_KEY "AIzaSyBdUTGzi9iQ3asge53BP3UfLALtBghNggQ"
#define DATABASE_URL "https://swmscp-9078d-default-rtdb.firebaseio.com/" 

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

void setup() {
  Serial.begin(9600);
  //lcd.init();
  //lcd.backlight();
  //Wire.begin();
  
  dht.begin();
  pinMode(DHTPIN, INPUT);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

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
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

}

void loop() {
  delay(10000);
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 620, 215, 0, 100);

  if (isnan(h) || isnan(t)) {

    Serial.println("Failed to read from  DHT sensor!");
    return;

  }

  if (soilMoisturePercent < 0) soilMoisturePercent = 0;
  if (soilMoisturePercent > 100) soilMoisturePercent = 100;

  if (Firebase.ready() && signupOK) {
    
    if (Firebase.RTDB.setFloat(&fbdo, "DHT/humidity",h)){
      
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.println(" %");
    
    }
    else {

      Serial.println("FAILED");
      Serial.print("REASON:: " + fbdo.errorReason());
    
    }

    if (Firebase.RTDB.setFloat(&fbdo, "DHT/temperature", t)) {

      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.println(" °C");

    }
    else {

      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    
    }

    if (Firebase.RTDB.setInt(&fbdo, "SoilMoisture/Percent", soilMoisturePercent)){

      Serial.print("Soil Moisture: ");
      Serial.print(soilMoisturePercent);
      Serial.println(" %");

    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }

  Serial.println("______________________________");

  unsigned long currentMillis = millis();

  if (soilMoisturePercent <= 20 && !relayOn) {
    digitalWrite(RELAY_PIN, HIGH);
    relayOn = true;
    relayStartMillis = currentMillis;
  }

  if (relayOn && currentMillis - relayStartMillis >= relayOnDuration) {
    digitalWrite(RELAY_PIN, LOW);
    relayOn = false;
  } else if (soilMoisturePercent >= 55) {
    digitalWrite(RELAY_PIN, LOW);
    relayOn = false;
  }

}
