#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

extern void MqttLoop();

// MQTT callback parsing
const char* mqttGarageTopic = "home/garage";

const char* StateJsonOpened = "{ \"Door\": \"Opened\" }";
const char* StateJsonClosed = "{ \"Door\": \"Closed\" }";
const char* StateJsonStall =  "{ \"Door\": \"Ajar\" }";

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
  client->publish(mqttGarageTopic, jsonState);

  Serial.print("Now [");
  Serial.print(jsonState);
  Serial.println("]");
}

bool CheckSensor(PubSubClient *client, DOOR_STATE doorStateToCheck, short int sensorPin)
{
  int timesToCheck = 10;
  int sensorValue = digitalRead(sensorPin);

  while (sensorValue == 0 && timesToCheck > 0)
  {
    delay(10);

    MqttLoop();

    sensorValue = digitalRead(sensorPin);
    timesToCheck--;
  }

  if (sensorValue == 0)
  {
    ChangeState(client, doorStateToCheck);
    return true;
  }

  return false;
}

void loopGarage(PubSubClient *client, short int sensorPin1, short int sensorPin2)
{
  bool anyDoorSensorsSet = false;
  
  anyDoorSensorsSet |= CheckSensor(client, DOOR_STATE_OPENED, sensorPin1);
  anyDoorSensorsSet |= CheckSensor(client, DOOR_STATE_CLOSED, sensorPin2);

  if (!anyDoorSensorsSet)
  {
    // Assume an opened garage door when no sensors are set.
    ChangeState(client, DOOR_STATE_OPENED);
  }
}
