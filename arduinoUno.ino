#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);  // I2C address 0x27, 20 columns, 4 rows

const int irPin1 = 10;    // IR sensor pin 1
const int irPin2 = 11;    // IR sensor pin 2
const int irPin3 = 12;    // IR sensor pin 3
const int irPin4 = 13;    // IR sensor pin 4
const int trigPin = 8;    // Ultrasonic sensor trigger pin
const int echoPin = 7;    // Ultrasonic sensor echo pin
const int motorPin = 9;   // Motor control pin
const int ledPin1 = 2;    // LED pin 1
const int ledPin2 = 3;    // LED pin 2
const int ledPin3 = 4;    // LED pin 3
const int ledPin4 = 5;    // LED pin 4
const int buzzerPin = 6;  // Buzzer pin

Servo motor;  // Create a servo object

bool isOccupied1 = false;
bool isOccupied2 = false;
bool isOccupied3 = false;
bool isOccupied4 = false;

bool blynkLED1State = false;
bool blynkLED2State = false;
bool blynkLED3State = false;
bool blynkLED4State = false;

void setup() {
  Serial.begin(9600);

  // Initialize LCD with I2C address 0x27, 20 columns, 4 rows
  lcd.init();
  lcd.backlight();

  pinMode(irPin1, INPUT);
  pinMode(irPin2, INPUT);
  pinMode(irPin3, INPUT);
  pinMode(irPin4, INPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(motorPin, OUTPUT);
  motor.attach(motorPin);  // Attach the servo to the motor pin

  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin4, OUTPUT);

  pinMode(buzzerPin, OUTPUT);

  Serial.println("Initialization complete.");
}

void loop() {
  // Read the state of each IR sensor
  bool state1 = digitalRead(irPin1);
  bool state2 = digitalRead(irPin2);
  bool state3 = digitalRead(irPin3);
  bool state4 = digitalRead(irPin4);

  // Update occupied status
  isOccupied1 = !state1;
  isOccupied2 = !state2;
  isOccupied3 = !state3;
  isOccupied4 = !state4;

  // Read distance from ultrasonic sensor
  int distance = getDistance();

  // Update the LCD display
  updateDisplay();

  // Operate the motor, buzzer, and LEDs based on distance and occupancy
  operateMotor(distance);
  controlLEDs();

  // Display distance in the Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Read Blynk LED states
  readBlynkLEDStates();

  delay(1000);  // Adjust the delay based on your testing requirements
}

int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  return pulseIn(echoPin, HIGH) * 0.034 / 2;
}

void updateDisplay() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Slot 1: ");
  lcd.print(isOccupied1 ? "Occupied" : "Vacant");
  if (blynkLED1State) lcd.print(" R");

  lcd.setCursor(0, 1);
  lcd.print("Slot 2: ");
  lcd.print(isOccupied2 ? "Occupied" : "Vacant");
  if (blynkLED2State) lcd.print(" R");

  lcd.setCursor(0, 2);
  lcd.print("Slot 3: ");
  lcd.print(isOccupied3 ? "Occupied" : "Vacant");
  if (blynkLED3State) lcd.print(" R");

  lcd.setCursor(0, 3);
  lcd.print("Slot 4: ");
  lcd.print(isOccupied4 ? "Occupied" : "Vacant");
  if (blynkLED4State) lcd.print(" R");
}

void operateMotor(int distance) {
  // Check if all slots are occupied
  if (isOccupied1 && isOccupied2 && isOccupied3 && isOccupied4) {
    // All slots are occupied, stop the motor
    motor.write(0);

    // Check if an object is detected within 5 cm
    if (distance < 5) {
      // Activate the buzzer and blink "PARKING FULL" on the LCD
      digitalWrite(buzzerPin, HIGH);
      lcd.setCursor(0, 0);
      lcd.clear();
      lcd.print("Parking FULL");
      delay(2000);  // Blink duration
      lcd.clear();
      digitalWrite(buzzerPin, LOW);
      delay(500);  // Pause between blinks
    }

    return;
  }

  // Example logic: Rotate the motor to 90 degrees for 3 seconds if the distance is less than 10 cm
  if (distance < 10) {
    motor.write(60);          // Set motor position to 90 degrees
    delay(3000);              // Rotate for 3 seconds
    motor.write(0);           // Return motor to 0 degrees
    delay(1000);              // Wait for 1 second (adjust as needed)
  }
}

void controlLEDs() {
  // Control LEDs based on occupancy
  digitalWrite(ledPin1, isOccupied1 || blynkLED1State ? HIGH : LOW);
  digitalWrite(ledPin2, isOccupied2 || blynkLED2State ? HIGH : LOW);
  digitalWrite(ledPin3, isOccupied3 || blynkLED3State ? HIGH : LOW);
  digitalWrite(ledPin4, isOccupied4 || blynkLED4State ? HIGH : LOW);
}

void readBlynkLEDStates() {
  while (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    if (data.startsWith("LED1:")) {
      blynkLED1State = data.substring(5).toInt();
    } else if (data.startsWith("LED2:")) {
      blynkLED2State = data.substring(5).toInt();
    } else if (data.startsWith("LED3:")) {
      blynkLED3State = data.substring(5).toInt();
    } else if (data.startsWith("LED4:")) {
      blynkLED4State = data.substring(5).toInt();
    }
  }
}
