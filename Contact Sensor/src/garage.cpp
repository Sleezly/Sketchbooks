#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

extern void MqttLoop();

// MQTT callback parsing
const char* mqttGarageTopic = "home/garage";

const char* StateOpenedJson = "{ \"Door\": \"Opened\" }";
const char* StateClosedJson = "{ \"Door\": \"Closed\" }";
const char* StateMovingJson = "{ \"Door\": \"Moving\" }";

enum DOOR_STATE
{
  DOOR_STATE_UNUSED = 0,

  DOOR_STATE_OPENED,
  DOOR_STATE_CLOSED,
  DOOR_STATE_MOVING,
};

DOOR_STATE doorState = DOOR_STATE_UNUSED;

int DelayCounter = -1;

void callbackGarage(PubSubClient* client, short int pin)
{
  doorState = DOOR_STATE_MOVING;
  client->publish(mqttGarageTopic, StateMovingJson);

  Serial.print("Now [");
  Serial.print(StateOperatingJson);
  Serial.println("]");

  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
}

void loopGarage(PubSubClient *client, short int led1, short int led2)
{
  int pin1 = digitalRead(led1);
  int pin2 = digitalRead(led2);

  if (pin1 == 0 && pin2 != 0)
  {
    if (doorState != DOOR_STATE_OPENED)
    {
      doorState = DOOR_STATE_OPENED;
      client->publish(mqttGarageTopic, StateOpenedJson);

      Serial.print("Now [");
      Serial.print(StateOpenedJson);
      Serial.println("]");
    }

    return;
  }
  
  if (pin1 != 0 && pin2 == 0)
  {
    if (doorState != DOOR_STATE_CLOSED)
    {
      doorState = DOOR_STATE_CLOSED;
      client->publish(mqttGarageTopic, StateClosedJson);

      Serial.print("Now [");
      Serial.print(StateClosedJson);
      Serial.println("]");
    }

    return;
  }
  
  if (doorState != DOOR_STATE_MOVING)
  {
    doorState = DOOR_STATE_MOVING;
    client->publish(mqttGarageTopic, StateMovingJson);

    Serial.print("Now [");
    Serial.print(StateMovingJson);
    Serial.println("]");
  }
}