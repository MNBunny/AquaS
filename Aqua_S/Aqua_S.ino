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

void setup() {
  Serial.begin(9600);
  //lcd.init();
  //lcd.backlight();
  //Wire.begin();
  
  dht.begin();
  pinMode(DHTPIN, INPUT);

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
  //delay(10000); // 10-seconds delay
  delay(600000); // 10-minute delay
  //delay(1800000); // 30-minute delay
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 900, 393, 0, 100);

  if (isnan(h) || isnan(t)) {

    Serial.println("Failed to read from  DHT sensor!");
    return;

  }

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

    if (Firebase.RTDB.setInt(&fbdo, "SoilMoisture/Percent_1", soilMoisturePercent)){

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

}