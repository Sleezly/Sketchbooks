#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Contact Sensor 1 Pins
const short int BUILTIN_LED2 = 16;
const short int GROUND_PIN_2 = 5;

// Contact Sensor 2 Pins
const short int GROUND_PIN_1 = 0; 
const short int BUILTIN_LED1 = 2;

// Buzzer Pins
const short int BUZZER_PIN = 14;
const short int GROUND_PIN_3 = 12; 

const int initialAudibleDelay = 15000;
const int initialAudibleStep = 5000;
const int minAudibledelay = 1500;
const int audibleDelaySteps = 750;

// WIFI
const char* ssid     = "TOM24";
const char* password = "secret";   <-- SECRET

// MQTT
const char* mqtthost = "192.168.1.2";
const int   mqttport = 1883;
const char* mqttuser = "esp8266";
const char* mqttpass = "secret"; // <-- SECRET
const char* pubtopic = "home/pantry";
const char* subtopic = "home/alarms";

const char* StateOnJson =  "{ \"State\": \"On\" }";

char mqttUniqueId[16] = {0};

WiFiClient espClient;
PubSubClient client(espClient);
bool generateBeep = true;

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] [");
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println("]");

  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& parsed = jsonBuffer.parseObject(payload);

  if (parsed.success())
  {
    const char * stateValue = parsed["State"];
    Serial.print("State: [");
    Serial.print(stateValue);
    Serial.println("]");

    if (strcmp("On", stateValue) == 0 || strcmp("on", stateValue) == 0)
    {
      generateBeep = true;
    }
    else if (strcmp("Off", stateValue) == 0 || strcmp("off", stateValue) == 0)
    {
      generateBeep = false;
    }
  }
}

void setup()
{
  Serial.begin(9600);

  delay(100);

  pinMode(BUILTIN_LED1, INPUT);
  pinMode(BUILTIN_LED2, INPUT);

  // Set grounding pins
  pinMode(GROUND_PIN_1, OUTPUT);
  pinMode(GROUND_PIN_2, OUTPUT);
  pinMode(GROUND_PIN_3, OUTPUT);

  digitalWrite(GROUND_PIN_1, LOW);
  digitalWrite(GROUND_PIN_2, LOW);
  digitalWrite(GROUND_PIN_3, LOW);

  delay(100);

  // Generate a unique ID for our MQTT client
  snprintf(mqttUniqueId, 25, "ESP8266-%08X", ESP.getChipId());

  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  delay(500);

  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 


  // Connect to MQTT broker
  client.setServer(mqtthost, mqttport);
  client.setCallback(callback);
}

void MqttLoop()
{
  // Reconnect to the MQTT broker when needed
  while (!client.connected())
  {
    Serial.println("MQTT attempting to connect");

    if (client.connect(mqttUniqueId, mqttuser, mqttpass))
    {
      Serial.println("MQTT connected");

      if (client.subscribe(subtopic))
      {
        Serial.print("MQTT subscribed to ");
        Serial.println(subtopic);        
      }
    }
    else
    {
      Serial.print("MQTT failed to connect, state=");
      Serial.println(client.state());

      delay(2000);
    }
  }

  // Check the MQTT subscriber for messages
  client.loop();
}

void loop()
{
  MqttLoop();

  int pin1 = digitalRead(BUILTIN_LED1);
  int pin2 = digitalRead(BUILTIN_LED2);

  // Check if either door is open
  if (pin1 == 1 || pin2 == 1)
  {

    // Publish an 'Open Door' event to the MQTT broker
    Serial.print(pubtopic);
    Serial.println(": Open");
    client.publish(pubtopic, "{ \"Door\": \"Open\" }");

    // Wait a bit
    for (int i = 0; i < 100; i++)
    {
      delay(initialAudibleDelay / 100);
      MqttLoop();

      pin1 = digitalRead(BUILTIN_LED1);
      pin2 = digitalRead(BUILTIN_LED2);

      if (pin1 == 0 && pin2 == 0)
      {
        break;
      }
    }


    // Emit beeps until the door is closed
    int audibleDelay = initialAudibleStep;
    while (pin1 == 1 || pin2 == 1)
    {
      // Emit a beep
      if (generateBeep)
      {
        tone(BUZZER_PIN, 4000);
        delay(150);
        noTone(BUZZER_PIN);
      }


      // Wait a bit
      for (int i = 0; i < 100; i++)
      {
        delay(audibleDelay / 100);
        MqttLoop();

        pin1 = digitalRead(BUILTIN_LED1);
        pin2 = digitalRead(BUILTIN_LED2);

        if (pin1 == 0 && pin2 == 0)
        {
          break;
        }
      }


      // Step down the wait duration
      audibleDelay -= audibleDelaySteps;
      if (audibleDelay < minAudibledelay)
      { 
        audibleDelay = minAudibledelay;
      }
    }


    // Publish a 'Closed Door' event to the MQTT broker
    Serial.print(pubtopic);
    Serial.println(": Closed");
    client.publish(pubtopic, "{ \"Door\": \"Closed\" }");
  }


  delay(500);
}
