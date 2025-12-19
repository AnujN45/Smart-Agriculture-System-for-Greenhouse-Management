// Improved Part4 - UNO + ESP-01 (SoftwareSerial) -> safer read & debug
// Replace SSID/PASS and make sure ESP UART is 9600

// libraries
#include <SoftwareSerial.h>
#include "DHT.h"

// pins / serial
SoftwareSerial espSerial(2, 3); // UNO pin2 = RX<-ESP TX, pin3 = TX->ESP RX (use level shifter on TX->ESP RX)
DHT dht(4, DHT11);
#define moisturePin A0
#define relayPin 8

// constants
const int DryValue = 1023;
const int WetValue = 338;

// variables
int  moisture = 0;
int  temperature = 0;
int  humidity = 0;
int  waterLevel = 0;
bool pump = false;
bool isConnected = false;

/* ---------- helpers ---------- */
// Map raw analog to percent (0 = dry, 100 = wet)
int rawToSoilPercent(int raw) {
  int pct = map(raw, DryValue, WetValue, 0, 100);
  pct = constrain(pct, 0, 100);
  return pct;
}

/* ---------- ESP comms (non-blocking, safe) ---------- */
void readFromEsp() {
  if (!espSerial.available()) return;

  // Read up to 20 bytes or until newline(s), with small timeout
  char input[22];
  memset(input, 0, sizeof(input));
  int idx = 0;
  unsigned long start = millis();
  while (millis() - start < 50) {
    while (espSerial.available() && idx < 20) {
      char c = (char)espSerial.read();
      input[idx++] = c;
      // small safety: reset timeout on receiving data
      start = millis();
    }
    if (!espSerial.available()) break;
  }
  input[idx] = '\0';

  if (idx == 0) return; // nothing useful

  Serial.print("RAW ESP IN: ");
  Serial.println(input);

  char *data = strtok(input, "\n");
  while (data != NULL) {
    size_t len = strlen(data);
    if (len > 0) {
      switch (data[0]) {
        case 'd': // disconnected message from ESP
          isConnected = false;
          Serial.println("ESP: disconnected");
          break;
        case 'c': // connected
          isConnected = true;
          Serial.println("ESP: connected");
          // optional: sendData immediately on connect
          // sendData();
          break;
        case 'p': // pump command from ESP (p1 or p0)
          if (len >= 2) {
            if (data[1] == '1') pump = turnOnPump();
            else pump = turnOffPump();
          }
          break;
        default:
          // unknown prefix - ignore or log
          Serial.print("ESP unknown prefix: ");
          Serial.println(data);
          break;
      }
    }
    data = strtok(NULL, "\n");
  }
}

/* ---------- send sensor data to ESP ---------- */
void sendData() {
  // Use println so each is newline terminated
  espSerial.print("t"); espSerial.println(String(temperature));
  espSerial.print("h"); espSerial.println(String(humidity));
  espSerial.print("p"); espSerial.println(String(pump ? 1 : 0));
  espSerial.print("m"); espSerial.println(String(moisture));
  espSerial.print("w"); espSerial.println(String(waterLevel));
  Serial.println("Sent data to ESP");
}

/* ---------- pump control ---------- */
bool turnOnPump() {
  digitalWrite(relayPin, LOW); // active LOW typical for many relay modules; change if yours is different
  espSerial.println("p1");      // keep original behavior (sending p1 back)
  Serial.println("Pump ON");
  return true;
}

bool turnOffPump() {
  digitalWrite(relayPin, HIGH);
  espSerial.println("p0");
  Serial.println("Pump OFF");
  return false;
}

/* ---------- Arduino setup & loop ---------- */
void setup() {
  // Serial for debug to PC
  Serial.begin(9600);
  Serial.println("Starting improved Part4 sketch...");

  // start ESP serial - IMPORTANT: ESP-01 must be set to 9600 for SoftwareSerial reliability
  espSerial.begin(9600);
  delay(200);

  // dht & lcd & water-level inits
  dht.begin();
  initLCD();
  initWaterLevel();

  // relay
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // ensure off at start (active LOW)

  // small startup delay
  delay(200);
}

void loop() {
  // Read incoming control messages from ESP
  readFromEsp();

  // Read sensors
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  printOnLCD(temperature, humidity);

  int raw = analogRead(moisturePin);
  // Convert to percent where 0 = dry, 100 = wet (matches previous logic)
  moisture = rawToSoilPercent(raw);

  waterLevel = checkWaterLevel();

  Serial.print("Soil raw: "); Serial.print(raw);
  Serial.print(" -> %: "); Serial.print(moisture);
  Serial.print("  T: "); Serial.print(temperature);
  Serial.print("  H: "); Serial.println(humidity);

  // send data to ESP/cloud only if ESP reported connected
  if (isConnected) sendData();

  // pump logic: your existing rule: if moisture < 30 => pump ON
  if (moisture < 30) {
    pump = turnOnPump();
    delay(1000);
  } else if (pump) {
    pump = turnOffPump();
  }

  delay(1000);
}






#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

void    initLCD() {
  lcd.init();
  lcd.clear();
}

void    printOnLCD(int temperature, int humidity) {
    lcd.backlight();
    lcd.setCursor(3, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print((char)223);
    lcd.print("C");
    lcd.setCursor(1, 1);
    lcd.print("Humidity: ");
    lcd.print(humidity);
    lcd.print("%");
}


#define echoPin 7
#define trigPin 6
#define greenPin 10
#define redPin 11

unsigned long maxDistance;
unsigned long maxDistance_75;
unsigned long maxDistance_50;
unsigned long maxDistance_25;
unsigned long distance;
unsigned long duration;

void  initWaterLevel() {
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  maxDistance = duration * 0.034;
  maxDistance_75 = maxDistance * 0.75;
  maxDistance_50 = maxDistance * 0.50;
  maxDistance_25 = maxDistance * 0.25;
}

int  checkWaterLevel() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034;

  if (distance >= maxDistance)
  {
    // LED Red
    digitalWrite(redPin, HIGH);
    digitalWrite(greenPin, LOW);
  }
  else if (distance >= maxDistance_75)
  {
    // LED Orange
    digitalWrite(redPin, 255);
    digitalWrite(greenPin, 10);
  }
  else if (distance >= maxDistance_50)
  {
    // LED Yellow
    digitalWrite(redPin, 255);
    digitalWrite(greenPin, 255);
  }
  else if (distance >= maxDistance_25)
  {
    // LED Greenish
    digitalWrite(redPin, 10);
    digitalWrite(greenPin, 255);
  }
  else
  {
    // LED Green
    digitalWrite(redPin, 0);
    digitalWrite(greenPin, 255);
  }

  return (100 - distance * 100 / maxDistance);
}
