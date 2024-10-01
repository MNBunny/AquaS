#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Wi-Fi credentials and Firebase configurations
#define WIFI_SSID "GlobeAtHome_d7d38_2.4"
#define WIFI_PASSWORD "Jy6YEfHQ"
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

void setup() {
  Serial.begin(9600);
  
  // Initialize OLED display
  display.begin(SSD1306_I2C_ADDRESS, OLED_RESET);
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
  if (Firebase.ready() && signupOK) {
    float humidity = 0;
    float temperature = 0;
    int soilMoisturePercent = 0;

    // Read values from Firebase
    if (Firebase.RTDB.getFloat(&fbdo, "DHT/humidity")) {
      humidity = fbdo.floatData();
    }
    if (Firebase.RTDB.getFloat(&fbdo, "DHT/temperature")) {
      temperature = fbdo.floatData();
    }
    if (Firebase.RTDB.getInt(&fbdo, "SoilMoisture/Percent")) {
      soilMoisturePercent = fbdo.intData();
    }

    // Display data on OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Humidity: "); display.print(humidity); display.println(" %");
    display.print("Temp: "); display.print(temperature); display.println(" °C");
    display.print("Soil Moisture: "); display.print(soilMoisturePercent); display.println(" %");
    
    // Get current time
    String currentTime = getCurrentTime(); // Implement a function to get current time
    display.print("Time: "); display.println(currentTime);
    display.display();

    // Watering rules
    if (soilMoisturePercent < 20) { // Adjust this value as needed for dryness level
      Serial.println("Watering plants immediately.");
      digitalWrite(RELAY1_PIN, HIGH);
      delay(5000); // Water for 5 seconds
      digitalWrite(RELAY1_PIN, LOW);
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
