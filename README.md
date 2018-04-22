# cm4ha
cm4ha is "Controllino &amp; MQTT for Home Automation"

## Details
cm4ha is an Arduino sketch for the [Controllino board](https://controllino.biz/controllino/maxi/), to use it in a Home Automation context, interacting with the MQTT bus. 

After a subscription to a root-MQTT topic, cm4ha enables:

* **On/Off** on relays on a MQTT payload reception,
* **Pulse** on relays on a MQTT payload reception,
* **Pulse** on relays on a MQTT payload reception, with state indication by current clamp
* **On/Off** via a pulse on relays on a MQTT payload reception, with state indication by current clamp

State indications are published on the MQTT bus. 

The "current clamp" is a non-invasive current sensor connected to an analog input of the Controllino, such as https://snootlab.com/lang-en/parts/95-current-sensor-30a.html.

## Configuration
The following typedefs are usable : `onoffTopic_t`, `pulseTopic_t` and `onOffPulseTopic_t`.

### onoffTopic_t

Use this structure to describe topics that produce a on/off on output. The payload is the awaited string to enables the `on` or the `off`

### pulseTopic_t

Use this structure to describe topics that produce a short pulse on output (latching relays...). Orders are just "toggle"; see `onOffPulseTopic_t` for absolute On/Off via toggle pulse. The payload is the awaited string that enables the pulse.

An optional current clamp can be connected to an analog input to get the status (on/off).

### onOffPulseTopic_t

Use this structure to describe topics that produce a short pulse on output (latching relays...) to toogle current state returned by the current clamp. The payload is the awaited string that enables the ON or the OFF order.

The current clamp must be connected to an analog input to get the status (on/off).

## Example

In the sketch example :
* a on/off lamp is connected on the R2 relay and is controlled via the topic `api/1/room/outdoor/lamp/wall/id/1/request`
* two window rolling shutters are connected on the R6, R7, R8, R9 relays (1-open, 1-close, 2-open and 2-close) and are controlled via the topics `api/1/room/livingroom/window/shutter/id/1/request` and `api/1/room/bedroom/window/shutter/id/1/request`
* three latching relays lamps are connected on the R3, R4, R5 relays and are are controlled via the topics `api/1/room/garage/lamp/ceiling/id/1/request`, `api/1/room/entrance/lamp/ceiling/id/1/request` and `api/1/room/bedroom/lamp/ceiling/id/1/request`. The three current sensors are connected on analog input A0, A1 and A2.

## For more details
See comments in the source file for more details
