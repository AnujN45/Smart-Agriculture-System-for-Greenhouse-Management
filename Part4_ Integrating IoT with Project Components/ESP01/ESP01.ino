#define BLYNK_TEMPLATE_ID   "TMPL38tzyxtbV"
#define BLYNK_TEMPLATE_NAME "Smart Irrigation"
#define BLYNK_AUTH_TOKEN    "vqPsHJyA9FHzZP_US-dOl_V-qs9SHzOo"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>


char ssid[] = "vivo T1";
char pass[] = "ANUJ 34 ASN";

void setup()
{
  
  Serial.begin(9600);
  delay(50);
  Serial.println("=== ESP BOOTED - SKETCH AT 9600 ===");
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  if (Blynk.connected()) Serial.println("connected");
  else Serial.println("Attempting Blynk connection...");
}

void loop()
{
  checkConnection();
  Blynk.run();

  
  char input[21];
  memset(input, 0, sizeof(input));
  if (Serial.available()) {
    int readCount = Serial.readBytes(input, 20); 
    (void)readCount;
    char *data = strtok(input, "\n");
    while (data && strlen(data) > 1)
    {
      switch (data[0])
      {
        case 't':
          Blynk.virtualWrite(V0, atoi(&data[1]));
          break;
        case 'h':
          Blynk.virtualWrite(V1, atoi(&data[1]));
          break;
        case 'p':
          Blynk.virtualWrite(V2, atoi(&data[1]));
          break;
        case 'm':
          Blynk.virtualWrite(V3, atoi(&data[1]));
          break;
        case 'w':
          Blynk.virtualWrite(V4, atoi(&data[1]));
          break;
      }
      data = strtok(NULL, "\n");
    }
  }
}

BLYNK_WRITE(V2)
{
  
  Serial.println("p" + String(param.asInt()));
}

void checkConnection()
{
  if (!Blynk.connected())
  {
    Serial.println("disconnected");
    
    do {
      Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
      delay(2000);
      
    } while (!Blynk.connected());
    Serial.println("connected");
  }
}
