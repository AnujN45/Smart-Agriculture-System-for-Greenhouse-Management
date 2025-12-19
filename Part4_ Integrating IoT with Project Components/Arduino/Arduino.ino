#include <SoftwareSerial.h>
#include "DHT.h"


void initLCD();
void initWaterLevel();
int checkWaterLevel();
void printOnLCD(int t, int h);


SoftwareSerial espSerial(2, 3);
DHT dht(4, DHT11);
#define moisturePin A0
#define relayPin 9


const int DryValue = 1023;
const int WetValue = 338;


int moisture = 0;
int temperature = 0;
int humidity = 0;
int waterLevel = 0;
bool pump = false;
bool isConnected = false;


int rawToSoilPercent(int raw) {
  int pct = map(raw, DryValue, WetValue, 0, 100);
  pct = constrain(pct, 0, 100);
  return pct;
}


void readFromEsp() {
  if (!espSerial.available()) return;

  
  char input[22];
  memset(input, 0, sizeof(input));
  int idx = 0;
  unsigned long start = millis();
  while (millis() - start < 50) {
    while (espSerial.available() && idx < 20) {
      char c = (char)espSerial.read();
      input[idx++] = c;
      
      start = millis();
    }
    if (!espSerial.available()) break;
  }
  input[idx] = '\0';

  if (idx == 0) return; 

  Serial.print("RAW ESP IN: ");
  Serial.println(input);

  char *data = strtok(input, "\n");
  while (data != NULL) {
    size_t len = strlen(data);
    if (len > 0) {
      switch (data[0]) {
        case 'd': 
          isConnected = false;
          Serial.println("ESP: disconnected");
          break;
        case 'c': 
          isConnected = true;
          Serial.println("ESP: connected");
          
          break;
        case 'p': 
          if (len >= 2) {
            if (data[1] == '1') pump = turnOnPump();
            else pump = turnOffPump();
          }
          break;
        default:
          
          Serial.print("ESP unknown prefix: ");
          Serial.println(data);
          break;
      }
    }
    data = strtok(NULL, "\n");
  }
}


void sendData() {
  
  espSerial.print("t"); espSerial.println(String(temperature));
  espSerial.print("h"); espSerial.println(String(humidity));
  espSerial.print("p"); espSerial.println(String(pump ? 1 : 0));
  espSerial.print("m"); espSerial.println(String(moisture));
  espSerial.print("w"); espSerial.println(String(waterLevel));
  Serial.println("Sent data to ESP");
}


bool turnOnPump() {
  digitalWrite(relayPin, LOW); 
  espSerial.println("p1");     
  Serial.println("Pump ON");
  return true;
}

bool turnOffPump() {
  digitalWrite(relayPin, HIGH);
  espSerial.println("p0");
  Serial.println("Pump OFF");
  return false;
}


void setup() {
  
  Serial.begin(9600);
  Serial.println("Starting improved Part4 sketch...");

  
  espSerial.begin(9600);
  delay(200);

  
  dht.begin();
  initLCD();
  initWaterLevel();

  
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); 

  
  delay(200);
}

void loop() {
  
  readFromEsp();

  
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = (int)round(t);
  else Serial.println("DHT readTemperature() failed");

  if (!isnan(h)) humidity = (int)round(h);
  else Serial.println("DHT readHumidity() failed");

  
  printOnLCD(temperature, humidity);

  int raw = analogRead(moisturePin);
  
  moisture = rawToSoilPercent(raw);

  waterLevel = checkWaterLevel();

  Serial.print("Soil raw: "); Serial.print(raw);
  Serial.print(" -> M: "); Serial.print(moisture);
  Serial.print("  T: "); Serial.print(temperature);
  Serial.print("  H: "); Serial.println(humidity);

  
  if (isConnected) sendData();

  
  if (!pump && moisture < 30) {
    pump = turnOnPump();
  } else if (pump && moisture >= 45) { 
    pump = turnOffPump();
  }

  delay(1000);
}
