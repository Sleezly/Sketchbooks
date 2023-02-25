#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

extern void MqttLoop();

// MQTT callback parsing
const char* mqttGarageTopic = "home/garage";

const char* StateJsonOpened = "{ \"Door\": \"Opened\" }";
const char* StateJsonClosed = "{ \"Door\": \"Closed\" }";

enum DOOR_STATE
{
  DOOR_STATE_UNUSED = 0,

  DOOR_STATE_OPENED,
  DOOR_STATE_CLOSED,
};

DOOR_STATE LastDoorState = DOOR_STATE_UNUSED;

void callbackGarage(PubSubClient* client, short int pin)
{
  digitalWrite(pin, HIGH);
  delay(250);
  digitalWrite(pin, LOW);
}

void ChangeState(PubSubClient *client, DOOR_STATE doorState)
{
  if (LastDoorState == doorState)
  {
    return;
  }

  const char* jsonState;
  switch (doorState)
  {
    case DOOR_STATE_CLOSED:
      jsonState = StateJsonClosed;
      break;

    case DOOR_STATE_OPENED:
    default:
      jsonState = StateJsonOpened;
      break;
  }

  LastDoorState = doorState;
  client->publish(mqttGarageTopic, jsonState, true);

  Serial.print("Now [");
  Serial.print(jsonState);
  Serial.println("]");
}

void loopGarage(PubSubClient *client, short int sensorPin)
{
  int sensorValue = digitalRead(sensorPin);

  if (0 == sensorValue)
  {
    ChangeState(client, DOOR_STATE_CLOSED);
  }
  else
  {
    // Assume an opened door state when the closed sensor is not set.
    ChangeState(client, DOOR_STATE_OPENED);
  }
}
