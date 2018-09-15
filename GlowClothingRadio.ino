/*
   Dec 2014 - TMRh20 - Updated
   Derived from examples by J. Coliz <maniacbug@ymail.com>
*/
/**
 * Example for efficient call-response using ack-payloads
 *
 * This example continues to make use of all the normal functionality of the radios including
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionlly as well.
 * This allows very fast call-response communication, with the responding radio never having to
 * switch out of Primary Receiver mode to send back a payload, but having the option to switch to
 * primary transmitter if wanting to initiate communication instead of respond to a commmunication.
 */

#include <SPI.h>
#include <FastLED.h>
#include "RF24.h"

#define PIN_LED 4
#define NUM_LEDS 50

// This is an array of leds.  One item for each led in your strip.
CRGB leds[NUM_LEDS];



extern HardwareSerial Serial;

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool isTransmitter = true;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(7,8);
/**********************************************************/
// Topology
byte addresses[][6] = {"1Node","2Node"};              // Radio pipe addresses for the 2 nodes to communicate.

// Role management: Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.
typedef enum { role_ping_out = 1, role_pong_back } role_e;                 // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};  // The debug-friendly names of those roles

byte counter = 1;                                                          // A single byte to keep track of the data being sent back and forth


void setup(){

    Serial.begin(115200);

    // Setup and configure radio

    radio.begin();
    radio.setAutoAck(false);

    if (isTransmitter)
        Serial.println("Transmitter Mode!");
    else
        Serial.println("Receiver Mode!");

    FastLED.addLeds<WS2812B, PIN_LED, RGB>(leds, NUM_LEDS);

    //radio.enableAckPayload();                     // Allow optional ack payloads
    radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads

    if(isTransmitter){
        radio.openWritingPipe(addresses[1]);        // Both radios listen on the same pipes by default, but opposite addresses
    }else{
        radio.openReadingPipe(1, addresses[1]);
        radio.startListening();
    }

    //radio.write(&counter, sizeof(counter), true);          // Pre-load an ack-paylod into the FIFO buffer for pipe 1
    //radio.printDetails();
}

void loop(void) {

    byte gotByte = 16;
/****************** Ping Out Role ***************************/

    if (isTransmitter){                               // Radio is in ping mode
        Serial.print(F("Now sending "));                         // Use a simple byte counter as payload
        Serial.println(counter);
        counter ++;

        unsigned long time = micros();                          // Record the current microsecond count

        if (radio.write(&counter, 1, true) ){                         // Send the counter variable to the other radio
            Serial.println(F("Sent successful"));

        }else{        Serial.println(F("Sending failed.")); }          // If no ack response, sending failed
    }


/****************** Pong Back Role ***************************/

    else {
        byte pipeNo;                          // Declare variables for the pipe and the byte received

        while(radio.available(&pipeNo)){              // Read all available payloads
            radio.read( &gotByte, 1 );

            Serial.print(F("Got Data: "));
            Serial.println(gotByte);
        }
    }

    fill_rainbow(leds, NUM_LEDS, gotByte, 3);
    FastLED.setBrightness(16);
    FastLED.show();
}
