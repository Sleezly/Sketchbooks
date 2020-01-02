#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

extern void MqttLoop();

const int initialAudibleDelay = 15000;
const int initialAudibleStep = 5000;
const int minAudibledelay = 1500;
const int audibleDelaySteps = 750;

// MQTT callback parsing
const char* pubtopic = "home/pantry";
const char* StateOnJson =  "{ \"State\": \"On\" }";

bool generateBeep = true;

void callbackPantry(byte* payload)
{
  StaticJsonDocument<256> jsonDocument;
  DeserializationError error = deserializeJson(jsonDocument, payload);
  
  if (!error)
  {
    const char * stateValue = jsonDocument["State"];
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

void loopPantry(PubSubClient *client, short int led1, short int led2, short int buzzer)
{
  int pin1 = digitalRead(led1);
  int pin2 = digitalRead(led2);

  // Check if either door is open
  if (pin1 == 1 || pin2 == 1)
  {
    // Publish an 'Open Door' event to the MQTT broker
    Serial.print(pubtopic);
    Serial.println(": Open");
    client->publish(pubtopic, "{ \"Door\": \"Open\" }", true);

    // Wait a bit
    for (int i = 0; i < 100; i++)
    {
      delay(initialAudibleDelay / 100);
      MqttLoop();

      pin1 = digitalRead(led1);
      pin2 = digitalRead(led2);

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
        tone(buzzer, 4000);
        delay(150);
        noTone(buzzer);
      }


      // Wait a bit
      for (int i = 0; i < 100; i++)
      {
        delay(audibleDelay / 100);
        MqttLoop();

        pin1 = digitalRead(led1);
        pin2 = digitalRead(led2);

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
    client->publish(pubtopic, "{ \"Door\": \"Closed\" }", true);
  }
}
