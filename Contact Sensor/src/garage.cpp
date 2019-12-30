#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

extern void MqttLoop();

const int STALL_DELAY_IN_SECONDS = 30;

// MQTT callback parsing
const char* mqttGarageTopic = "home/garage";

const char* StateOpenedJson =     "{ \"Door\": \"Opened\" }";
const char* StateClosedJson =     "{ \"Door\": \"Closed\" }";
const char* StateStalledJson =    "{ \"Door\": \"Stalled\" }";
const char* StateOperatingJson =  "{ \"Door\": \"Moving\" }";

enum DOOR_STATE
{
  DOOR_STATE_UNUSED = 0,
  DOOR_STATE_OPENED,
  DOOR_STATE_CLOSED,
  DOOR_STATE_STALLED,
  DOOR_STATE_OPERATING,
};

DOOR_STATE doorState = DOOR_STATE_UNUSED;

void callbackGarage(PubSubClient* client, short int pin)
{
  doorState = DOOR_STATE_OPERATING;

  client->publish(mqttGarageTopic, StateOperatingJson);

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

   if (pin1 == 0)
  {
    if (doorState != DOOR_STATE_OPENED)
    {
      doorState = DOOR_STATE_OPENED;
      client->publish(mqttGarageTopic, StateOpenedJson);

      Serial.print("Now [");
      Serial.print(StateOpenedJson);
      Serial.println("]");
    }
  }
  else if (pin2 == 0)
  {
    if (doorState != DOOR_STATE_CLOSED)
    {
      doorState = DOOR_STATE_CLOSED;
      client->publish(mqttGarageTopic, StateClosedJson);

      Serial.print("Now [");
      Serial.print(StateClosedJson);
      Serial.println("]");
    }
  }
  else
  {
    if (doorState != DOOR_STATE_STALLED)
    {
      int stalledDelayCounter = STALL_DELAY_IN_SECONDS;

      while (stalledDelayCounter != 0)
      {
        delay(1000);

        MqttLoop();

        pin1 = digitalRead(led1);
        pin2 = digitalRead(led2);

        if (pin1 == 0 || pin2 == 0)
        {
          return;
        }

        stalledDelayCounter--;
      }

      doorState = DOOR_STATE_STALLED;
      client->publish(mqttGarageTopic, StateStalledJson);

      Serial.print("Now [");
      Serial.print(StateStalledJson);
      Serial.println("]");
    }
  }
}