#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT11.h>
#include <RTClib.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
DHT11 dht11(7);
RTC_DS3231 rtc;

// Screens
unsigned long screenMillis = 0;
int screen = 0;
bool manualScreen = false;

// Pins
int lightpin = A0;
int buzzerPin = 4;
int silenceBtn = 5;
int toggleBtn = 6;

// Thresholds
int tempMax = 25;
int tempMin = 16;
int humidityMax = 60;
int lightNightMax = 700;
int lightDayMin = 600;

// Buzzer
bool buzzerSilenced = false;
int lastSilenceVal = 0;
int lastToggleVal = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  pinMode(buzzerPin, OUTPUT);
  pinMode(silenceBtn, INPUT);
  pinMode(toggleBtn, INPUT);
  Serial.begin(9600);
  rtc.begin();
  // Comment out after first upload!
  // rtc.adjust(DateTime(2026, 3, 31, 14, 44, 0));
}

void loop() {
  unsigned long currentMillis = millis();

  // Get time from DS3231
  DateTime now = rtc.now();
  int hours = now.hour();
  int minutes = now.minute();
  int seconds = now.second();

  // Read sensors
  int lightval = analogRead(lightpin);
  int temp = dht11.readTemperature();
  int humidity = dht11.readHumidity();

  // Check conditions
  bool tooDark = (hours >= 6 && hours < 22 && lightval < lightDayMin);
  bool tooBright = ((hours >= 22 || hours < 6) && lightval > lightNightMax);
  bool tooHot = (temp > tempMax);
  bool tooCold = (temp < tempMin);
  bool tooHumid = (humidity > humidityMax);
  bool alertNeeded = tooDark || tooBright || tooHot || tooCold || tooHumid;

  // Silence button
  int silenceVal = digitalRead(silenceBtn);
  if (silenceVal == 1 && lastSilenceVal == 0) {
    buzzerSilenced = !buzzerSilenced;
    delay(50);
  }
  lastSilenceVal = silenceVal;

  // Toggle screen button
  int toggleVal = digitalRead(toggleBtn);
  if (toggleVal == 1 && lastToggleVal == 0) {
    screen++;
    if (screen > 1) screen = 0;
    lcd.clear();
    manualScreen = true;
    delay(50);
  }
  lastToggleVal = toggleVal;

  // Jump to alert screen instantly if alert needed
  if (alertNeeded && !buzzerSilenced) {
    if (screen != 1) {
      screen = 1;
      lcd.clear();
    }
  }
  // Auto cycle screens every 5 seconds if no alert
  else if (!manualScreen) {
    if (currentMillis - screenMillis >= 5000) {
      screenMillis = currentMillis;
      screen++;
      if (screen > 1) screen = 0;
      lcd.clear();
    }
  }

  // Buzzer
  if (alertNeeded && !buzzerSilenced) {
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(buzzerPin, LOW);
  }

  // Screen 0: All readings
  if (screen == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    if (hours < 10) lcd.print("0");
    lcd.print(hours);
    lcd.print(":");
    if (minutes < 10) lcd.print("0");
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10) lcd.print("0");
    lcd.print(seconds);

    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.print(temp);
    lcd.print((char)223);
    lcd.print("C");

    lcd.setCursor(0, 2);
    lcd.print("Humidity: ");
    lcd.print(humidity);
    lcd.print("%");

    lcd.setCursor(0, 3);
    if (tooBright) {
      lcd.print("Light: TOO HIGH ");
    } else if (tooDark) {
      lcd.print("Light: TOO LOW  ");
    } else {
      lcd.print("Light: OK       ");
    }
  }

  // Screen 1: Conditions summary
  if (screen == 1) {
    lcd.setCursor(0, 0);
    lcd.print(tooHot ? "Temp:  TOO HIGH!" : tooCold ? "Temp:  TOO LOW! " : "Temp:  OK       ");
    lcd.setCursor(0, 1);
    lcd.print(tooHumid ? "Humid: TOO HIGH!" : "Humid: OK       ");
    lcd.setCursor(0, 2);
    lcd.print(tooBright ? "Light: TOO HIGH!" : tooDark ? "Light: TOO LOW! " : "Light: OK       ");
    lcd.setCursor(0, 3);
    if (alertNeeded) {
      lcd.print("!! ALERT ACTIVE!!");
    } else {
      lcd.print("All conditions OK");
    }
  }

  // Serial monitor output
  Serial.println("--- CONDITIONS ---");
  Serial.print("Time: "); Serial.print(hours); Serial.print(":"); Serial.print(minutes); Serial.print(":"); Serial.println(seconds);
  Serial.print("Temp: "); Serial.print(temp); Serial.println("C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println("%");
  Serial.print("Light: "); Serial.println(lightval);
  Serial.print("Silence Btn: "); Serial.println(silenceVal);
  Serial.print("Toggle Btn: "); Serial.println(toggleVal);
  Serial.print("Buzzer: "); Serial.println(alertNeeded && !buzzerSilenced ? "ON" : "OFF");
  Serial.println("------------------");

  delay(200);
}