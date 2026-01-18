#include <Wire.h>
#include <Adafruit_LiquidCrystal.h>

// ---------- LCD ----------
Adafruit_LiquidCrystal lcd(0);

// ---------- PIN DEFINITIONS ----------
const int motorPin  = 9;     // Fan (PWM)
const int ledPin    = 13;    // Room light
const int buzzerPin = 8;     // Buzzer
const int buttonPin = 7;     // AUTO / MANUAL
const int pirPin    = 6;     // PIR sensor
const int ldrPin    = A0;    // LDR
const int tempPin   = A1;    // TMP36
const int gasPin    = A2;    // MQ-2 analog

// ---------- SETTINGS ----------
const int darknessLevel = 50;
const float minTemp = 25.0;
const float maxTemp = 60.0;
const int gasThreshold = 520;

// ---------- TIMING ----------
unsigned long lastUpdate = 0;
const unsigned long interval = 500;
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 300;

// ---------- VARIABLES ----------
bool autoMode = true;
bool lastButtonState = HIGH;
float filteredTemp = 25.0;

void setup() {
  pinMode(motorPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pirPin, INPUT);

  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.setBacklight(1);

  lcd.setCursor(0,0);
  lcd.print(" Smart Room ");
  lcd.setCursor(0,1);
  lcd.print(" Version 4.0 ");
  delay(2000);
  lcd.clear();
}

void loop() {

  // ---------- BUTTON (AUTO / MANUAL) ----------
  bool buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH &&
      millis() - lastButtonTime > debounceDelay) {
    autoMode = !autoMode;
    lastButtonTime = millis();
  }
  lastButtonState = buttonState;

  // ---------- GAS ALERT (HIGHEST PRIORITY) ----------
  int gasValue = analogRead(gasPin);

  if (gasValue > gasThreshold) {
    analogWrite(motorPin, 255);
    digitalWrite(ledPin, HIGH);
    digitalWrite(buzzerPin, HIGH);

    lcd.setCursor(0,0);
    lcd.print("!! GAS ALERT !!");
    lcd.setCursor(0,1);
    lcd.print("Fan MAX        ");

    Serial.print("GAS ALERT! Value = ");
    Serial.println(gasValue);
    return;   // Override everything
  }

  digitalWrite(buzzerPin, LOW);

  // ---------- MANUAL MODE ----------
  if (!autoMode) {
    analogWrite(motorPin, 0);
    digitalWrite(ledPin, LOW);

    lcd.setCursor(0,0);
    lcd.print("MODE: MANUAL   ");
    lcd.setCursor(0,1);
    lcd.print("System OFF    ");
    return;
  }

  // ---------- PIR CHECK ----------
  if (digitalRead(pirPin) == LOW) {
    analogWrite(motorPin, 0);
    digitalWrite(ledPin, LOW);

    lcd.setCursor(0,0);
    lcd.print("No Motion     ");
    lcd.setCursor(0,1);
    lcd.print("System Idle  ");
    return;
  }

  // ---------- READ SENSORS ----------
  int ldrValue = analogRead(ldrPin);
  int tempRaw  = analogRead(tempPin);

  float voltage = tempRaw * (5.0 / 1024.0);
  float tempC = (voltage - 0.5) * 100;

  filteredTemp = 0.8 * filteredTemp + 0.2 * tempC;

  // ---------- LIGHT CONTROL ----------
  String lightState;
  if (ldrValue < darknessLevel) {
    digitalWrite(ledPin, HIGH);
    lightState = "DARK";
  } else {
    digitalWrite(ledPin, LOW);
    lightState = "BRIGHT";
  }

  // ---------- FAN CONTROL ----------
  int fanSpeed = 0;
  String mode;

  if (filteredTemp < minTemp) {
    fanSpeed = 0;
    mode = "NORMAL";
  }
  else if (filteredTemp <= maxTemp) {
    fanSpeed = map(filteredTemp * 10,
                   minTemp * 10,
                   maxTemp * 10,
                   0, 255);
    mode = "COOLING";
  }
  else {
    fanSpeed = 255;
    mode = "ALERT!";
    digitalWrite(buzzerPin, HIGH);
  }

  analogWrite(motorPin, fanSpeed);

  // ---------- LCD OUTPUT ----------
  lcd.setCursor(0,0);
  lcd.print("A:");
  lcd.print(mode);
  lcd.print(" ");
  lcd.print(filteredTemp,1);
  lcd.print("C ");

  lcd.setCursor(0,1);
  lcd.print("Fan:");
  lcd.print(fanSpeed);
  lcd.print(" ");
  lcd.print(lightState);
}
