#include <Ticker.h>
#include <OHAUTlib.h>
#include "version.h"

#define DEVICE_TYPE "RINGDOOR"

#define ON 1
#define HIGHZ 0
#define DOOR_OPEN_TIME 3000
#define RING_DEBOUNCE_TIME 3000

OHAUTservice ohaut;

int led_pin = 13;
int open_door_pin = 15;
int ringbell_pin = 12;

void start_door_open(bool publish=true);
void stop_door_open(bool publish=true);

void setup(void){

    /* start the serial port and switch on the PCB led */
    Serial.begin(115200);

    ohaut.set_led_pin(led_pin);
    digitalWrite(open_door_pin, HIGHZ);
    pinMode(ringbell_pin, INPUT);
    pinMode(open_door_pin, OUTPUT);

    ohaut.on_http_server_ready = [](ESP8266WebServer *server) {
        server->on("/openDoor", HTTP_GET,  [server]() {
                start_door_open();
                server->send(200, "application/json", "{\"result\": \"0\", \"message:\": "
                                                      "\"door opened correctly\"}");
        });
    };

    ohaut.on_mqtt_ready = [](MQTTDevice* mqtt) {
        ohaut.mqtt->setHandler("open", [](byte *data, unsigned int length) {
            if (length == 1) {
                if (data[0] == '0') {
                    stop_door_open(false);
                } else if (data[0] == '1') {
                    start_door_open(false);
                }
            }
        });
    };

    ohaut.on_ota_start = [](){
        // ensure door-open is not active
        digitalWrite(open_door_pin, HIGHZ);
    };

    ohaut.setup(DEVICE_TYPE, VERSION, "ring");
}

long int last_millis_ring = 0;
int last_ringbell=-1;
long int stop_millis = 0;

void loop(void){
    ohaut.handle();

    // handle ring bell

    int ringbell = digitalRead(ringbell_pin);
    if ((ringbell != last_ringbell) && (millis()-last_millis_ring)>RING_DEBOUNCE_TIME) {
        last_ringbell = ringbell;
        ohaut.mqtt->publish("ring", ringbell ? "0":"1");
        last_millis_ring = millis();
    }

    // stop opening the door after some time

    if (stop_millis && stop_millis<millis()) {
        stop_door_open();
        stop_millis = 0;
    }
}

void start_door_open(bool publish) {
    // The MOSFET activates on a '1', and connects
    // the output to GROUND
    digitalWrite(open_door_pin, ON);
    if (publish)
        ohaut.mqtt->publish("open", "1");

    // make sure we stop opening after DOOR_OPEN_TIME milliseconds
    stop_millis = millis() + DOOR_OPEN_TIME;
    if (!stop_millis) stop_millis++;
}

void stop_door_open(bool publish) {
    // deactivate the MOSFET, and leave the output
    // floating (high impedance)
    digitalWrite(open_door_pin, HIGHZ);
    if(publish)
        ohaut.mqtt->publish("open", "0");
}
