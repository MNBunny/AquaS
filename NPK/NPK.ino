#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
 
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
#define RE D4
#define DE D3
 
const byte code[]= {0x01, 0x03, 0x00, 0x1e, 0x00, 0x03, 0x34, 0x0D};
const byte nitro[] = {0x01,0x03, 0x00, 0x1e, 0x00, 0x01, 0xB5, 0xCC};
const byte phos[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xE4, 0x0C};
const byte pota[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xC0};

byte values[11];
SoftwareSerial mod(D7, D6);

void setup() {
  Serial.begin(9600);
  mod.begin(9600);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initialize with the I2C addr 0x3C (128x64)
  delay(500);
  display.clearDisplay();
  display.setCursor(25, 15);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println(" NPK Sensor");
  display.setCursor(25, 35);
  display.setTextSize(1);
  display.print("Initializing");
  display.display();
  delay(3000);
}
 
void loop() {
  byte val1,val2,val3,val4;
  val1 = nitrogen();
  delay(250);
  val2 = phosphorous();
  delay(250);
  val3 = potassium();
  delay(250);
//  val4 = all();
 // delay(250);
  
  
  Serial.print("Nitrogen: ");
  Serial.print(val1);
  Serial.println(" mg/kg");
  Serial.print("Phosphorous: ");
  Serial.print(val2);
  Serial.println(" mg/kg");
  Serial.print("Potassium: ");
  Serial.print(val3);
  Serial.println(" mg/kg");
  delay(2000);
 
  display.clearDisplay();
  
 
  display.setTextSize(2);
  display.setCursor(0, 5);
  display.print("N: ");
  display.print(val1);
  display.setTextSize(1);
  display.print(" mg/kg");
 
  display.setTextSize(2);
  display.setCursor(0, 25);
  display.print("P: ");
  display.print(val2);
  display.setTextSize(1);
  display.print(" mg/kg");
 
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print("K: ");
  display.print(val3);
  display.setTextSize(1);
  display.print(" mg/kg");
 
  display.display();
}

/*
byte all() {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);

  // Send NPK read command
  for (byte i = 0; i < sizeof(code); i++) {
    mod.write(code[i]);
  }

  delay(10);
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  // Wait for the response
  while (mod.available() < 9); // Ensure at least 9 bytes are available
  for (byte i = 0; i < 9; i++) {
    if (mod.available()) {
      values[i] = mod.read();
    }
  }

  // Extract and print NPK values
  Serial.println("NPK Sensor Values:");
  byte nitrogenValue = values[4];  // Nitrogen
  byte phosphorousValue = values[6]; // Phosphorous
  byte potassiumValue = values[8];  // Potassium

  Serial.print("Nitrogen: ");
  Serial.print(nitrogenValue);
  Serial.println(" mg/kg");
  Serial.print("Phosphorous: ");
  Serial.print(phosphorousValue);
  Serial.println(" mg/kg");
  Serial.print("Potassium: ");
  Serial.print(potassiumValue);
  Serial.println(" mg/kg");
  
  return nitrogenValue; // Return the nitrogen value or any value you prefer
}
*/



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
    Serial.print(values[i],HEX);
    }
    Serial.println();
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
    Serial.print(values[i],HEX);
    }
    Serial.println();
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
    Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}