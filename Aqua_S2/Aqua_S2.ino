// Include the necessary libraries
#include <Firebase_ESP_Client.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

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

// Wi-Fi credentials and Firebase configurations
#define WIFI_SSID "HUAWEI-Zvkm"
#define WIFI_PASSWORD "jKNK4gmG"
#define API_KEY "AIzaSyBdUTGzi9iQ3asge53BP3UfLALtBghNggQ"
#define DATABASE_URL "https://swmscp-9078d-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

// Pins definition
#define RELAY1_PIN D4 // Watering relay
#define RELAY2_PIN D5 // Fertilizer relay
#define RELAY3_PIN D3 // Mixing relay
#define RELAY4_PIN D6 // Another function relay (if needed)

#define SOIL_MOISTURE_PIN A0

// Set up the U8g2 display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Define NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000); // UTC+8 (Philippine Time), update every 60 seconds

void setup() {
  Serial.begin(9600);
  
  u8g2.begin(); // Initialize the U8g2 display
  u8g2.clearDisplay();
  u8g2.setCursor(25, 15);
  u8g2.setFont(u8g2_font_ncenB08_tr); // Set font
  u8g2.drawStr(25, 15, "Initializing");
  u8g2.sendBuffer(); // Display the content
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
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize relay pins
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  // Initialize NTP client
  timeClient.begin();
}

void loop() {
  delay(600000);  // 10 minutes in milliseconds

  // Reading current soil moisture sensor value
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN); // Get analog reading
  int soilMoisturePercent = map(soilMoistureValue, 900, 393, 0, 100); // Map to percentage

  timeClient.update(); // Update NTP client for current time
  String currentTime = getCurrentTime(); // Fetch the current time

  if (Firebase.ready() && signupOK) {
    int soilMoisturePercentRealtime = 0;

    // 1st Reading: Get current soil moisture in real-time from Firebase
    if (Firebase.RTDB.getInt(&fbdo, "SoilMoisture/Percent_1")) {
      soilMoisturePercentRealtime = fbdo.intData();
    }

    if (Firebase.RTDB.getInt(&fbdo, "SoilMoisture/Percent_2")) {
      soilMoisturePercentRealtime = fbdo.intData();
    }

    // Send "2nd Reading" of soil moisture to Firebase under a different path
    if (Firebase.RTDB.setInt(&fbdo, "SoilMoisture/Percent_2", soilMoisturePercent)) {
      Serial.print("2nd Soil Moisture Reading Sent: ");
      Serial.println(soilMoisturePercent);
    } else {
      Serial.println("Failed to send 2nd Soil Moisture reading.");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    u8g2.clearDisplay(); // Clear the display once at the beginning
    u8g2.setFont(u8g2_font_ncenB08_tr); // Set font for consistency

    // Display soil moisture percentage
    u8g2.setCursor(3, 12); // Set cursor for the first line
    u8g2.print("Moisture: "); 
    u8g2.setCursor(80, 12); // Move cursor to display the value
    u8g2.print(soilMoisturePercent); // Display soil moisture value
    u8g2.print(" %");

    // Display combined soil moisture percentage
    u8g2.setCursor(3, 22); // Set cursor for the second line
    u8g2.print("Cmb Moist: "); 
    u8g2.setCursor(80, 22); // Move cursor to display the value
    u8g2.print(soilMoisturePercentRealtime); // Display combined soil moisture value
    u8g2.print(" %");

    // Display current time
    u8g2.setCursor(3, 32); // Set cursor for the third line
    u8g2.print("Time: ");
    u8g2.print(currentTime); // Display current time

    u8g2.sendBuffer(); // Send the display buffer once to update the OLED


    // Immediate watering if soil moisture percent_2 is 10% or less
    if (soilMoisturePercentRealtime <= 10) { // Water immediately if soil moisture is 10% or less
      Serial.println("Watering plants immediately due to dryness.");
      digitalWrite(RELAY1_PIN, HIGH);
      delay(5000); // Water for 5 seconds
      digitalWrite(RELAY1_PIN, LOW);
    }
    // Do not water if soil moisture percent_2 is 40% or higher
    else if (soilMoisturePercentRealtime >= 40) { 
      Serial.println("Soil moisture is sufficient, no watering needed.");
    }

    // Schedule watering at 5 AM and 5 PM
    String timeOfDay = currentTime.substring(0, 5); // Get HH:MM from time
    if (timeOfDay == "05:00" || timeOfDay == "17:00") {
      Serial.println("Scheduled watering.");
      digitalWrite(RELAY1_PIN, HIGH);
      delay(5000); // Water for 5 seconds
      digitalWrite(RELAY1_PIN, LOW);
    }
  }

  Serial.println("______________________________");
}

// Function to get the current Philippine time in "HH:MM" format
String getCurrentTime() {
  // Update the NTP time to ensure accuracy
  timeClient.update();
  
  // Extract the HH:MM formatted string directly
  String formattedTime = timeClient.getFormattedTime().substring(0, 5); // Get only "HH:MM"
  
  return formattedTime;
}

