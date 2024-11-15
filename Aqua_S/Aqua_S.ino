#include "DHT.h"
#include <SoftwareSerial.h>
#include <Arduino.h>
#include <U8g2lib.h>

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

#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Pin definitions
#define RE D4
#define DE D3
#define DHTPIN D5
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

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

bool signupOK = false;
byte values[11];

void setup() {
  Serial.begin(9600);
  mod.begin(9600);
  dht.begin();
  
  u8g2.begin();
  
  pinMode(DHTPIN, INPUT);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);

  u8g2.begin(); // Initialize the U8g2 display
  u8g2.clearDisplay();
  u8g2.setCursor(25, 15);
  u8g2.setFont(u8g2_font_ncenB08_tr); // Set font
  u8g2.drawStr(25, 15, "Initializing");
  u8g2.sendBuffer(); // Display the content
  delay(3000);

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
  delay(1200000); // Delay between readings

  byte val1, val2, val3;
  val1 = nitrogen();
  delay(250);
  val2 = phosphorous();
  delay(250);
  val3 = potassium();
  delay(250);

  u8g2.clearDisplay();
  
  // Display nitrogen
  u8g2.setFont(u8g2_font_ncenB08_tr); // Set font for text
  u8g2.setCursor(3, 12);
  u8g2.print("N: ");
  u8g2.setCursor(20, 12);
  u8g2.print(val1);
  u8g2.setCursor(45, 12);
  u8g2.print(" mg/kg");
  
  // Display phosphorous
  u8g2.setCursor(3, 22);
  u8g2.print("P: ");
  u8g2.setCursor(20, 22);
  u8g2.print(val2);
  u8g2.setCursor(45, 22);
  u8g2.print(" mg/kg");
  
  // Display potassium
  u8g2.setCursor(3, 32);
  u8g2.print("K: ");
  u8g2.setCursor(20, 32);
  u8g2.print(val3);
  u8g2.setCursor(45, 32);
  u8g2.print(" mg/kg");
  
  // Read humidity and temperature from DHT sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor! Check wiring or sensor.");
    return;
  }

  // Display humidity and temperature on the OLED screen
  u8g2.setCursor(3, 42);
  u8g2.print("Humidity: ");
  u8g2.setCursor(80, 42);
  u8g2.print(h, 1);  // Display with 1 decimal point

  u8g2.setCursor(3, 52);
  u8g2.print("Temp: ");
  u8g2.setCursor(50, 52);
  u8g2.print(t, 1);  // Display with 1 decimal point

  // Read soil moisture
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 900, 393, 0, 100);

  // Display soil moisture on the OLED screen
  u8g2.setCursor(3, 62);
  u8g2.print("Moisture: ");
  u8g2.setCursor(80, 62);
  u8g2.print(soilMoisturePercent);
  u8g2.print(" %");

  u8g2.sendBuffer(); // Update the display

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
    Serial.print(data, 1);  // Display with 1 decimal point
    Serial.println(unit);
  } else {
    Serial.println("FAILED to send " + label + fbdo.errorReason());
  }
}

byte nitrogen(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(nitro,sizeof(nitro))==4){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    }
  }
  return values[4];
}
 
byte phosphorous(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(phos,sizeof(phos))==4){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    }
  }
  return values[4];
}
 
byte potassium(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(pota,sizeof(pota))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    }
  }
  return values[4];
}
