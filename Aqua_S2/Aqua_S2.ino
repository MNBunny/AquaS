#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Firebase_ESP_Client.h>
#include <Arduino.h>

#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Wi-Fi credentials and Firebase configurations
#define WIFI_SSID "HUAWEI-Zvkm"
#define WIFI_PASSWORD "jKNK4gmG"
#define API_KEY "AIzaSyBdUTGzi9iQ3asge53BP3UfLALtBghNggQ"
#define DATABASE_URL "https://swmscp-9078d-default-rtdb.firebaseio.com/" 

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

// Relay pin definitions
#define RELAY1_PIN D1 // Watering relay
#define RELAY2_PIN D2 // Fertilizer relay
#define RELAY3_PIN D3 // Mixing relay
#define RELAY4_PIN D4 // Another function relay (if needed)

#define SOIL_MOISTURE_PIN A0

void setup() {
  Serial.begin(9600);
  
  // Initialize OLED display with correct I2C address
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C, OLED_RESET);
  display.clearDisplay();
  display.display();
  
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

  // Uncomment if using token status callback
  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize relay pins
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
}

void loop() {
  // Reading current soil moisture sensor value
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN); // Get analog reading
  int soilMoisturePercent = map(soilMoistureValue, 900, 393, 0, 100); // Map to percentage

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

    // Display data on OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Soil Moisture: "); 
    display.print(soilMoisturePercentRealtime); 
    display.println(" %");

    // Get current time
    String currentTime = getCurrentTime();
    display.print("Time: "); 
    display.println(currentTime);
    display.display();

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

  delay(60000); // Delay between each loop iteration (adjust as necessary)
}

// Function to get the current time (you can implement this using NTP or RTC)
String getCurrentTime() {
  // Dummy implementation for current time; replace with actual time retrieval logic
  return "12:00"; // Return current time in "HH:MM" format
}
