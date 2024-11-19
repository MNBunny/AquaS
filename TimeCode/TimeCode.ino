#include <Firebase_ESP_Client.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <time.h>

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
#define RELAY1_PIN D4
#define RELAY2_PIN D5
#define RELAY3_PIN D3
#define RELAY4_PIN D6
#define DHTPIN D0
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN A0

DHT dht(DHTPIN, DHTTYPE);

const char* NTP_SERVER = "time.google.com";  // A reliable NTP server
const char* TZ_INFO    = "PHT-8";  // Philippines Time (UTC+8)

tm timeinfo;
time_t now;
long unsigned lastNTPtime;
unsigned long lastEntryTime;

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

  Serial.println("\nNTP Time Test");

  configTime(0, 0, NTP_SERVER);  // Sync time from NTP server
  setenv("TZ", TZ_INFO, 1);      // Set timezone to PHT (Philippines Time)

  if (getNTPtime(10)) {  
    // Wait up to 10 seconds to sync the time
  } else {
    Serial.println("Time not set");
    ESP.restart();
  }
  showTime(&timeinfo);
  lastNTPtime = time(&now);
  lastEntryTime = millis();

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
}

void loop() 
{
  getNTPtime(10);
  showTime(&timeinfo);

  // Read sensor values
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
    if (Firebase.RTDB.setFloat(&fbdo, "DHT/humidity", h) &&
        Firebase.RTDB.setFloat(&fbdo, "DHT/temperature", t) &&
        Firebase.RTDB.setInt(&fbdo, "SoilMoisture/Percent_2", soilMoisturePercent)) {
      Serial.println("Data Sent to Firebase");
    } else {
      Serial.println("Failed to send data to Firebase.");
    }

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
    u8g2.print(soilMoisturePercent);
    u8g2.print(" %");
    u8g2.sendBuffer();

    // Watering logic
    if (soilMoisturePercent <= 10) {
      Serial.println("Watering plants immediately due to dryness.");
      digitalWrite(RELAY1_PIN, HIGH);
      delay(5000);
      digitalWrite(RELAY1_PIN, LOW);
    } else if (timeinfo.tm_hour == 5 || timeinfo.tm_hour == 17) {
      Serial.println("Scheduled watering.");
      digitalWrite(RELAY1_PIN, HIGH);
      delay(5000);
      digitalWrite(RELAY1_PIN, LOW);
    } else {
      Serial.println("No watering needed.");
    }
  }

  delay(1000);  // Add a small delay to avoid overwhelming the system
}

bool getNTPtime(int sec) {
  uint32_t start = millis();
  do {
    time(&now);
    localtime_r(&now, &timeinfo);
    delay(10);
  } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
  
  if (timeinfo.tm_year <= (2016 - 1900)) 
    return false;  // The NTP call was not successful
  
  return true;
}

void showTime(tm *localTime) {
  // Print time to serial terminal
  Serial.print(localTime->tm_mday);
  Serial.print('/');
  Serial.print(localTime->tm_mon + 1);
  Serial.print('/');
  Serial.print(localTime->tm_year - 100);
  Serial.print('-');
  Serial.print(localTime->tm_hour);
  Serial.print(':');
  Serial.print(localTime->tm_min);
  Serial.print(':');
  Serial.print(localTime->tm_sec);
  Serial.print(" Day of Week ");
  Serial.println(localTime->tm_wday);

  // Display time on OLED
  char time_output[30];
  
  u8g2.clearBuffer();  // Clear the display buffer

  u8g2.setFont(u8g2_font_ncenB08_tr);  // Set the font for the OLED
  u8g2.setCursor(0, 10);
  sprintf(time_output, "%02d:%02d:%02d", localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
  u8g2.print(time_output);

  u8g2.setCursor(0, 30);
  sprintf(time_output, "%02d/%02d/%02d", localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_year - 100);
  u8g2.print(time_output);

  u8g2.setCursor(0, 50);
  u8g2.print(getDOW(localTime->tm_wday));

  u8g2.sendBuffer();  // Send the buffer to the display
}

const char* getDOW(uint8_t tm_wday)
{
  switch (tm_wday)
  {
    case 0: return "Sunday";
    case 1: return "Monday";
    case 2: return "Tuesday";
    case 3: return "Wednesday";
    case 4: return "Thursday";
    case 5: return "Friday";
    case 6: return "Saturday";
    default: return "";  // Return empty string for invalid values
  }
}

