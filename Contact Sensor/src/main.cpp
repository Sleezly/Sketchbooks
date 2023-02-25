#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define GARAGE
//#define PANTRY

extern void loopPantry(PubSubClient *client, short int led1, short int led2, short int buzzer);
extern void loopGarage(PubSubClient *client, short int sensorPin);

extern void callbackPantry(byte* payload);
extern void callbackGarage(PubSubClient* client, short int relay);

#ifdef PANTRY
const char* subtopic = "home/alarms";
#else
const char* subtopic = "home/garage/set";
#endif

// WIFI
const char* ssid     = "Unknown";
const char* password = "uppercase1337";

// MQTT
const char* mqtthost = "192.168.1.5";
const int   mqttport = 1883;
const char* mqttuser = "esp8266";
const char* mqttpass = "yavin333";

char mqttUniqueId[16] = {0};

#ifdef PANTRY
// Buzzer Pins Pins
const short BUZZER_PIN = 14;
const short GROUND_PIN_3 = 12; 

// LED 1
const short BUILTIN_LED1 = 2;
const short GROUND_PIN_1 = 0;

// LED 2
const short BUILTIN_LED2 = 16;
const short GROUND_PIN_2 = 5;
#else
// Relay Pin
const short PIN_RELAY = 14;

// LED 2
const short SENSOR_PIN_IN = 2;
const short SENSOR_PIN_GROUND = 4;
#endif

WiFiClient espClient;
PubSubClient client(espClient);

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

#ifdef PANTRY
  callbackPantry(payload);
#else
  callbackGarage(&client, PIN_RELAY);
#endif
}

void setup()
{
  Serial.begin(9600);

  delay(100);

#ifdef PANTRY
  // Set grounding pins
  pinMode(GROUND_PIN_1, OUTPUT);
  pinMode(GROUND_PIN_2, OUTPUT);
  pinMode(GROUND_PIN_3, OUTPUT);

  pinMode(BUILTIN_LED1, INPUT);
  pinMode(BUILTIN_LED2, INPUT);

  digitalWrite(GROUND_PIN_1, LOW);
  digitalWrite(GROUND_PIN_2, LOW);
  digitalWrite(GROUND_PIN_3, LOW);
#else
  // Relay
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);

  // Contact Sensors
  pinMode(SENSOR_PIN_IN, INPUT);
  pinMode(SENSOR_PIN_GROUND, OUTPUT);
  digitalWrite(SENSOR_PIN_GROUND, LOW);
#endif

  delay(100);

  // Generate a unique ID for our MQTT client
  snprintf(mqttUniqueId, 25, "ESP8266-Garage-%08X", ESP.getChipId());

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

#ifdef PANTRY
  loopPantry(&client, BUILTIN_LED1, BUILTIN_LED2, BUZZER_PIN);
#else
  loopGarage(&client, SENSOR_PIN_IN);
#endif

  delay(500);
}
