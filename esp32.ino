#define BLYNK_TEMPLATE_ID "TMPL6plDSQ8oX"
#define BLYNK_TEMPLATE_NAME "RESERVATION"
#define BLYNK_AUTH_TOKEN "3O4uLl-Baxe-CGock0Yffy3xk5b6Q00v"
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <BlynkSimpleEsp32.h>

#define SS_PIN    5     // ESP32 pin GPIO5 
#define RST_PIN   27    // ESP32 pin GPIO27 
#define SERVO_PIN 4     // Servo control pin (GPIO2)
#define LCD_ADDR  0x27  // I2C address of the LCD
#define LCD_COLS  20    // Number of columns on the LCD
#define LCD_ROWS  4     // Number of rows on the LCD

#define LED_PIN_1 25    // LED control pin (GPIO25)
#define LED_PIN_2 26    // LED control pin (GPIO26)
#define LED_PIN_3 32    // LED control pin (GPIO32)
#define LED_PIN_4 33    // LED control pin (GPIO33)
const int ldrPin = 34;   // LDR pin
const int relayPin = 13; // Relay pin
MFRC522 rfid(SS_PIN, RST_PIN);
Servo myServo;
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

const byte card1UID[] = {0x53, 0xCE, 0x53, 0x10};
const byte card2UID[] = {0xE3, 0x9A, 0xC7, 0x0D};
const byte card3UID[] = {0x03, 0x01, 0x67, 0x10};
const byte card4UID[] = {0xD3, 0x79, 0xCB, 0x0D};

const char defaultDisplay[] = "Smart Parking";

struct CardInfo {
  bool inside = false;
  bool displayTime = false;
  unsigned long entryTime;
  unsigned long exitTime;
  bool displayActive = false;
  unsigned long displayStartTime;
  float cost;
} car1, car2, car3, car4;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

char auth[] = "3O4uLl-Baxe-CGock0Yffy3xk5b6Q00v";

// Blynk app display widgets (Virtual Pins)
#define INFO_DISPLAY_PIN V5

void setup() {
  Serial.begin(9600);
  // Blynk.begin(auth, "Aashish(private)", "9806754600");
  Blynk.begin(auth, "ICP", "ICP321@##");
  while (Blynk.connect() == false) {
    // Wait until connected
  }
  pinMode(ldrPin, INPUT);
  pinMode(relayPin, OUTPUT);
  SPI.begin();
  rfid.PCD_Init();

  myServo.attach(SERVO_PIN);
  myServo.write(0);

  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(LED_PIN_3, OUTPUT);
  pinMode(LED_PIN_4, OUTPUT);

  lcd.init();
  lcd.backlight();

  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
  displayDefault();
}

void loop() {
  Blynk.run();
  sendLEDStatusToArduino();
  timeClient.update();
  int ldrStatus = analogRead(ldrPin);



  if (ldrStatus < 4095) {
    // LDR senses darkness, turn on relay
    digitalWrite(relayPin, HIGH);
    Serial.print("RELAY ON - LDR Value: ");
    Serial.println(ldrStatus);
  } else {
    // LDR exposed to light, turn off relay
    digitalWrite(relayPin, LOW);
    Serial.print("RELAY OFF - LDR Value: ");
    Serial.println(ldrStatus);
  }

  
  if (rfid.PICC_IsNewCardPresent()) {
    if (rfid.PICC_ReadCardSerial()) {
      if (compareUID(rfid.uid.uidByte, card1UID)) {
        processCard("car1", car1);
      } else if (compareUID(rfid.uid.uidByte, card2UID)) {
        processCard("car2", car2);
      } else if (compareUID(rfid.uid.uidByte, card3UID)) {
        processCard("car3", car3);
      } else if (compareUID(rfid.uid.uidByte, card4UID)) {
        processCard("car4", car4);
      } else {
        displayCardName("Unknown");
      }

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      Serial.println("RFID_SCANNED");
      delay(3000);
    }
  }

  if (car1.displayActive && millis() - car1.displayStartTime >= 2000) {
    car1.displayActive = false;
    displayDefault();
  }

  if (car2.displayActive && millis() - car2.displayStartTime >= 2000) {
    car2.displayActive = false;
    displayDefault();
  }

  if (car3.displayActive && millis() - car3.displayStartTime >= 2000) {
    car3.displayActive = false;
    displayDefault();
  }

  if (car4.displayActive && millis() - car4.displayStartTime >= 2000) {
    car4.displayActive = false;
    displayDefault();
  }
}

void processCard(const char *cardName, CardInfo &car) {
  displayCardName(cardName);
  if (!car.inside) {
    lcd.setCursor(0, 1);
    lcd.print("Enter Time: ");
    car.entryTime = millis();
    car.displayTime = true;
    car.displayStartTime = millis();
    printTime();
    car.inside = true;
    Blynk.virtualWrite(INFO_DISPLAY_PIN, String("Enter Time: ") + String(car.entryTime));
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Exit Time:  ");
    car.exitTime = millis();
    printTime();
    car.inside = false;
    car.displayTime = false;

    unsigned long duration = (car.exitTime - car.entryTime) / 1000;
    lcd.setCursor(0, 2);
    lcd.print("Duration: ");
    lcd.print(duration);
    lcd.print("s");

    car.cost = calculateCost(duration);
    lcd.setCursor(0, 3);
    lcd.print("Cost: NRS ");
    lcd.print(car.cost, 2);

    // Generate bill and display in Serial Monitor
    Serial.println("Bill Information:");
    Serial.println("Car: " + String(cardName));
    Serial.println("Entry Time: " + String(car.entryTime));
    Serial.println("Exit Time: " + String(car.exitTime));
    Serial.println("Duration: " + String(duration) + "s");
    Serial.println("Cost: NRS " + String(car.cost, 2));

    myServo.write(60);
    delay(4000);
    myServo.write(0);
    car.displayActive = true;
    car.displayStartTime = millis();
    

    Blynk.virtualWrite(INFO_DISPLAY_PIN, String("Exit Time: ") + String(car.exitTime) +
                      String("\nDuration: ") + String(duration) + String("s") +
                      String("\nCost: NRS ") + String(car.cost, 2));
  }
  delay(2000);
  displayDefault();
}

float calculateCost(unsigned long duration) {
  return duration * 0.5;  // Adjust the formula based on your pricing model
}

void printTime() {
  lcd.print(timeClient.getFormattedTime());
}

void displayCardName(const char *cardName) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Car: ");
  lcd.print(cardName);
  Serial.print("Car Name: ");
  Serial.println(cardName);
}

void displayDefault() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(defaultDisplay);
  Blynk.virtualWrite(INFO_DISPLAY_PIN, defaultDisplay);
}

bool compareUID(byte *uid1, const byte *uid2) {
  for (int i = 0; i < 4; i++) {
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true;
}

BLYNK_WRITE(V1) {
  int ledState = param.asInt();
  digitalWrite(LED_PIN_1, ledState);
}

BLYNK_WRITE(V2) {
  int ledState = param.asInt();
  digitalWrite(LED_PIN_2, ledState);
}

BLYNK_WRITE(V3) {
  int ledState = param.asInt();
  digitalWrite(LED_PIN_3, ledState);
}

BLYNK_WRITE(V4) {
  int ledState = param.asInt();
  digitalWrite(LED_PIN_4, ledState);
}

void sendLEDStatusToArduino() {
  Serial.print("LED1:");
  Serial.println(digitalRead(LED_PIN_1));
  Serial.print("LED2:");
  Serial.println(digitalRead(LED_PIN_2));
  Serial.print("LED3:");
  Serial.println(digitalRead(LED_PIN_3));
  Serial.print("LED4:");
  Serial.println(digitalRead(LED_PIN_4));
}
