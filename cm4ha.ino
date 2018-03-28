// cm4ha - Controllino & MQTT for Home Automation
// cm4ha is an Arduino sketch for the Controllino board, to use it in a Home Automation context, 
// interacting with the MQTT bus. See comments in the source file for more details.
// Contributors: Adrien van den Bossche <vandenbo@univ-tlse2.fr>
// Code under GPLv3 license

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Controllino.h>

#define TOPIC_BASE "api/1/"
#define TOPIC_SUSCRIBE "api/1/#"
#define TOPIC_STATUS "status/controllino/1"

#define TOPIC_MAX_LEN 128
#define PAYLOAD_MAX_LEN 32
#define PULSE_DURATION 200 // ms

#define CLAMP_MIN_VALUE 5 // must be adjusted depending on the connected clamps
#define NO_CLAMP -1
#define NB_SAMPLES 50
#define DELAY_BETWEEN_SAMPLES 500 //microseconds
// 50 samples every 0.5ms = 25ms: enough for 50Hz/20ms

byte mac[] = {  0x28, 0x18, 0x92, 0x01, 0xFE, 0xEF }; // MAC address of the controllino
IPAddress ip(192, 168, 95, 27); // IP address of the controllino
IPAddress server(192, 168, 95, 232); // IP address of the MQTT broker

EthernetClient ethClient;
PubSubClient client(ethClient);
uint16_t samples[NB_SAMPLES];


typedef struct {
  char topic[TOPIC_MAX_LEN];      // MQTT topic
  char payload[PAYLOAD_MAX_LEN];  // MQTT payload
  int pin;                        // Relay pin number
  int level;                      // Output value associated to the payload
} onoffTopic_t;


typedef struct {
  char topic[TOPIC_MAX_LEN];      // MQTT topic
  char payload[PAYLOAD_MAX_LEN];  // MQTT payload to toggle
  int pin;                        // Relay pin number
  int state;                      // Initial state
  int current_clamp_pin;          // Optional clamp (NO_CLAMP if not used)
} pulseTopic_t;


typedef struct {
  char topic[TOPIC_MAX_LEN];        // MQTT topic
  char payloadOn[PAYLOAD_MAX_LEN];  // MQTT payload for ON order
  char payloadOff[PAYLOAD_MAX_LEN]; // MQTT payload for OFF order
  int pin;                          // Relay pin number
  int state;                        // Initial state
  int current_clamp_pin;            // Optional clamp (NO_CLAMP if not used)
} onOffPulseTopic_t;


// === onoffTopic_t
// use this structure to describe topcis that produce a on/off on output
// the payload is the awaited string to enable the on or the off

onoffTopic_t onoffTopic[] = {
  // Topic as a string                           payload   controllino pin level
  { "api/1/room/outside/lamp/wall/id/1/",        "on",     CONTROLLINO_R2, HIGH },
  { "api/1/room/outside/lamp/wall/id/1/",        "off",    CONTROLLINO_R2, LOW  },
};


// === pulseTopic_t
// use this structure to describe topcis that produce a short pulse on output (latching relays...)
// orders are just "toggle"; see onOffPulseTopic_t for toggle+on+off
// the payload is the awaited string that enable the pulse
// an optional current clamp can be connected to an analog input to get the status (on/off)

pulseTopic_t pulseTopic[] = {
  // Topic as a string                             payload   controllino pin   state   current_clamp_pin
  { "api/1/room/livingroom/window/shutter/id/1/",  "open",   CONTROLLINO_R6,   0,      NO_CLAMP },
  { "api/1/room/livingroom/window/shutter/id/1/",  "close",  CONTROLLINO_R7,   0,      NO_CLAMP },
  { "api/1/room/bedroom/window/shutter/id/1/",     "open",   CONTROLLINO_R8,   0,      NO_CLAMP },
  { "api/1/room/bedroom/window/shutter/id/1/",     "close",  CONTROLLINO_R9,   0,      NO_CLAMP },
};


// === onOffPulseTopic_t
// use this structure to describe topcis that produce a short pulse on output (latching relays...) to toogle current state
// returned by the clamp. The payload is the awaited string that enable the ON or the OFF order
// the current clamp must be connected to an analog input to get the status (on/off).

onOffPulseTopic_t onOffPulseTopic[] = {
  // Topic as a string                           payload_on   payload_off   controllino pin   state   current_clamp_pin
  { "api/1/room/garage/lamp/ceiling/id/1/",      "on",        "off",        CONTROLLINO_R5,   0,      CONTROLLINO_A0 },
  { "api/1/room/entrance/lamp/ceiling/id/1/",    "on",        "off",        CONTROLLINO_R4,   0,      CONTROLLINO_A1 },
  { "api/1/room/bedroom/lamp/ceiling/id/1/",     "on",        "off",        CONTROLLINO_R3,   0,      CONTROLLINO_A2 },
};


void callback(char* topic, byte* payload, unsigned int length)
{
  char str[TOPIC_MAX_LEN];
  
  payload[length] = 0; // End of str: consider payload as a str

  //// Check pulseTopic_t types ///////////////////////////////////////////
  for (int i = 0; i < (sizeof(pulseTopic) / sizeof(pulseTopic_t)); i++)
  {
    //Serial.print(i);
    sprintf(str, "%s%s", pulseTopic[i].topic, "request");
    if ( strcmp(topic, str) == 0 )
    {
      //Serial.print(pulseTopic[i].topic);
      if ( strcmp(payload, pulseTopic[i].payload) == 0 )
      {
        //Serial.print(pulseTopic[i].payload);
        digitalWrite(pulseTopic[i].pin, HIGH);
        delay(PULSE_DURATION);
        digitalWrite(pulseTopic[i].pin, LOW);
        delay(PULSE_DURATION);
        break;
      }
    }
  }

  //// Check onoffTopic_t types ///////////////////////////////////////////
  for (int i = 0; i < (sizeof(onoffTopic) / sizeof(onoffTopic_t)); i++)
  {
    //Serial.print(i);
    sprintf(str, "%s%s", onoffTopic[i].topic, "request");
    if ( strcmp(topic, str) == 0 )
    {
      //Serial.print(pulseTopic[i].topic);
      if ( strcmp(payload, onoffTopic[i].payload) == 0 )
      {
        //Serial.print(pulseTopic[i].payload);
        digitalWrite(onoffTopic[i].pin, onoffTopic[i].level);
        break;
      }
    }
  }

  //// Check onOffPulseTopic_t types //////////////////////////////////////
  for (int i = 0; i < (sizeof(onOffPulseTopic) / sizeof(onOffPulseTopic_t)); i++)
  {
    //Serial.print(i);
    sprintf(str, "%s%s", onOffPulseTopic[i].topic, "request");
    if ( strcmp(topic, str) == 0 )
    {
      Serial.print(onOffPulseTopic[i].topic);
      if (
          ( ( ( strcmp(payload, onOffPulseTopic[i].payloadOn) == 0 )
           && ( onOffPulseTopic[i].state) == 0 ) ) 
        ||
          ( ( ( strcmp(payload, onOffPulseTopic[i].payloadOff) == 0 )
           && ( onOffPulseTopic[i].state) == 1 ) ) 
        )
      {
        //Serial.print(pulseTopic[i].payload);
        digitalWrite(onOffPulseTopic[i].pin, HIGH);
        delay(PULSE_DURATION);
        digitalWrite(onOffPulseTopic[i].pin, LOW);
        delay(PULSE_DURATION);
        break;
      }
    }
  }
}


void reconnect()
{
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("controllino1")) {
      Serial.println("connected");
      client.publish(TOPIC_STATUS, "on");
      client.subscribe(TOPIC_SUSCRIBE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      delay(1000);
    }
  }
}


uint16_t getSampleMax(uint16_t samples[])
{
  uint16_t max = 0;

  for (int i=0; i<NB_SAMPLES; i++)
    if ( samples[i] > max ) max = samples[i];
  return max;
}


void getSamples(uint16_t samples[], int pin)
{
  for (int i=0; i<NB_SAMPLES; i++)
  {
     samples[i] = analogRead(pin);
     delayMicroseconds(DELAY_BETWEEN_SAMPLES);
  }
}


void setup()
{
  Serial.begin(115200);

  pinMode(CONTROLLINO_R0, OUTPUT);
  pinMode(CONTROLLINO_R1, OUTPUT);
  pinMode(CONTROLLINO_R2, OUTPUT);
  pinMode(CONTROLLINO_R3, OUTPUT);
  pinMode(CONTROLLINO_R4, OUTPUT);
  pinMode(CONTROLLINO_R5, OUTPUT);
  pinMode(CONTROLLINO_R6, OUTPUT);
  pinMode(CONTROLLINO_R7, OUTPUT);
  pinMode(CONTROLLINO_R8, OUTPUT);
  pinMode(CONTROLLINO_R9, OUTPUT);

  digitalWrite(CONTROLLINO_R0, LOW);
  digitalWrite(CONTROLLINO_R1, LOW);
  digitalWrite(CONTROLLINO_R2, LOW);
  digitalWrite(CONTROLLINO_R3, LOW);
  digitalWrite(CONTROLLINO_R4, LOW);
  digitalWrite(CONTROLLINO_R5, LOW);
  digitalWrite(CONTROLLINO_R6, LOW);
  digitalWrite(CONTROLLINO_R7, LOW);
  digitalWrite(CONTROLLINO_R8, LOW);
  digitalWrite(CONTROLLINO_R9, LOW);

  pinMode(CONTROLLINO_D0, OUTPUT);
  pinMode(CONTROLLINO_D1, OUTPUT);
  pinMode(CONTROLLINO_D2, OUTPUT);
  pinMode(CONTROLLINO_D3, OUTPUT);
  pinMode(CONTROLLINO_D4, OUTPUT);
  pinMode(CONTROLLINO_D5, OUTPUT);
  pinMode(CONTROLLINO_D6, OUTPUT);
  pinMode(CONTROLLINO_D7, OUTPUT);
  pinMode(CONTROLLINO_D8, OUTPUT);
  pinMode(CONTROLLINO_D9, OUTPUT);
  pinMode(CONTROLLINO_D10, OUTPUT);
  pinMode(CONTROLLINO_D11, OUTPUT);

  digitalWrite(CONTROLLINO_D0, LOW);
  digitalWrite(CONTROLLINO_D1, LOW);
  digitalWrite(CONTROLLINO_D2, LOW);
  digitalWrite(CONTROLLINO_D3, LOW);
  digitalWrite(CONTROLLINO_D4, LOW);
  digitalWrite(CONTROLLINO_D5, LOW);
  digitalWrite(CONTROLLINO_D6, LOW);
  digitalWrite(CONTROLLINO_D7, LOW);
  digitalWrite(CONTROLLINO_D8, LOW);
  digitalWrite(CONTROLLINO_D9, LOW);
  digitalWrite(CONTROLLINO_D10, LOW);
  digitalWrite(CONTROLLINO_D11, LOW);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  pinMode(A6, INPUT);
  pinMode(A7, INPUT);
  pinMode(A8, INPUT);
  pinMode(A9, INPUT);
  
  client.setServer(server, 1883);
  client.setCallback(callback);

  Ethernet.begin(mac, ip);
  delay(2000);
}


void loop()
{
  uint16_t max_value;
  char topic[TOPIC_MAX_LEN];
  char payload[PAYLOAD_MAX_LEN];

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // For each pulseTopic_t output, get on/off status using the current clamp, if used
  for (int i = 0; i < (sizeof(pulseTopic) / sizeof(pulseTopic_t)); i++)
  {
    if ( pulseTopic[i].current_clamp_pin != NO_CLAMP )
    {
      int state;
      getSamples(samples, pulseTopic[i].current_clamp_pin);
      max_value = getSampleMax(samples);
      state = (max_value > CLAMP_MIN_VALUE)? 1:0;
      if ( state != pulseTopic[i].state )
      {
        sprintf(topic, "%s%s", pulseTopic[i].topic, "indication");
        if ( state ) sprintf(payload, "on");
        else sprintf(payload, "off");
        client.publish(topic, payload, true); // Send with retained flag true
        pulseTopic[i].state = state;      
      }
      //Serial.print(i); Serial.print("\t"); Serial.print(max_value); 
      //Serial.print("\t"); Serial.println(pulseTopic[i].state);
    }
  }

  // For each onOffPulseTopic_t output, get on/off status using the current clamp, if used
  for (int i = 0; i < (sizeof(onOffPulseTopic) / sizeof(onOffPulseTopic_t)); i++)
  {
    if ( onOffPulseTopic[i].current_clamp_pin != NO_CLAMP )
    {
      int state;
      getSamples(samples, onOffPulseTopic[i].current_clamp_pin);
      max_value = getSampleMax(samples);
      state = (max_value > CLAMP_MIN_VALUE)? 1:0;
      if ( state != onOffPulseTopic[i].state )
      {
        sprintf(topic, "%s%s", onOffPulseTopic[i].topic, "indication");
        if ( state ) sprintf(payload, "on");
        else sprintf(payload, "off");
        client.publish(topic, payload, true); // Send with retained flag true
        onOffPulseTopic[i].state = state;      
      }
      //Serial.print(i); Serial.print("\t"); Serial.print(max_value); 
      //Serial.print("\t"); Serial.println(pulseTopic[i].state);
    }
  }
}
