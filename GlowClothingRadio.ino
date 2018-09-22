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
#include <Button.h>

#include "RF24.h"
#include "configOverride.h"

#ifndef PIN_LED
#define PIN_LED 4
#endif

#ifndef NUM_LEDS
#define NUM_LEDS 32
#endif

#ifndef IS_TRANSMITTER
#define IS_TRANSMITTER true
#endif

#ifndef STATUS_LED_COUNT
#define STATUS_LED_COUNT 8
#endif

// This is an array of leds.  One item for each led in your strip.
CRGB leds[NUM_LEDS];

struct LedConfig {
    byte brightness;
    uint16_t packetIndex;
} ledConfig = { 16, 0 };

extern HardwareSerial Serial;


/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(/*CE PIN*/ 7, /*CS PIN*/ 8);
/**********************************************************/
// Topology
byte addresses[][6] = {"1Node", "2Node"};              // Radio pipe addresses for the 2 nodes to communicate.

// Role management: Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.
typedef enum {
    role_ping_out = 1, role_pong_back
} role_e;                 // The various roles supported by this sketch
const char *role_friendly_name[] = {"invalid", "Ping out", "Pong back"};  // The debug-friendly names of those roles

byte counter = 1;                                                          // A single byte to keep track of the data being sent back and forth


Button button(/*PIN */ 3, 1, 1, 100);

void fillLeds(CRGB color, uint8_t start=0, uint8_t count=NUM_LEDS) {
    for (uint8_t i=start; i<start+count; i++)
        leds[i] = color;
}


void progressBar(
        uint8_t fraction,
        CRGB color,
        uint8_t start=0,
        uint8_t count=NUM_LEDS
) {
    uint8_t mappedProgress = map8(fraction, 0, count-1);
    if (fraction > 0 && mappedProgress == 0)
        mappedProgress = 1;

    fillLeds(color, start, mappedProgress);
    fillLeds(CRGB::Black, start + mappedProgress, count-mappedProgress);
}

void showSolidColor(CRGB color) {
    fillLeds(color);
    FastLED.show();
}

bool setupRadio() {
    Serial.print("Starting Radio... ");

    if (radio.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAIL");
        return false;
    }

    Serial.print("Configuring Radio Auto Ack... ");
    radio.setAutoAck(false);
    Serial.println("OK");

    Serial.print("Enabling Dynamic Payloads... ");
    radio.enableDynamicPayloads();
    Serial.println("OK");

    if (IS_TRANSMITTER) {
        Serial.print("Opening TRANSMIT Pipe... ");
        radio.openWritingPipe(addresses[1]);
        Serial.println("OK");
    } else {
        Serial.print("Opening RECEIVE Pipe... ");
        radio.openReadingPipe(1, addresses[1]);
        radio.startListening();
        Serial.println("OK");
    }

    return true;
}

void setup() {

    Serial.begin(115200);

    // Setup and configure radio

    Serial.println("Starting LEDs...");
    FastLED.addLeds<WS2812B, PIN_LED, GRB>(leds, NUM_LEDS);

    showSolidColor(CRGB(0, 0, 64));

    while (! setupRadio()) {
        Serial.println();
        Serial.println("Waiting 250ms for retry...");
        showSolidColor(CRGB(64, 0, 0));
        delay(250);
    }

    Serial.println();
    Serial.println("Setup Successful");
    showSolidColor(CRGB(0, 64, 0));

    //radio.write(&counter, sizeof(counter), true);          // Pre-load an ack-paylod into the FIFO buffer for pipe 1
    //radio.printDetails();
}

void cylonAnimation(
     uint8_t start=0,
     uint8_t count=NUM_LEDS
) {
    fillLeds(CRGB::Black, start, count);
    uint8_t fraction = triwave8(uint8_t(ledConfig.packetIndex));
    leds[start + map8(fraction, 0, count-1)] = CRGB::Red;
}


void(* resetFunc) (void) = 0;

void transmitterLoop() {
    ledConfig.packetIndex ++;

    button.read();
    if (button.wasReleased()) {
        if (ledConfig.brightness == 0)
            ledConfig.brightness = 1;
        else
            ledConfig.brightness *= 2;

        Serial.print("Brightness changed to ");
        Serial.print(ledConfig.brightness);
        Serial.println();
    }

    cylonAnimation(STATUS_LED_COUNT, NUM_LEDS-STATUS_LED_COUNT);

    // Record the current microsecond count
    unsigned long time = micros();

    if (radio.write(&ledConfig, sizeof(ledConfig), true)) {
        fillLeds(CRGB::Blue, 0, STATUS_LED_COUNT);
    } else {
        fillLeds(CRGB::Red, 0, STATUS_LED_COUNT);
        FastLED.show();
        Serial.println("Send failed. Rebooting.");
        delay(1000);
        resetFunc();
    }

    FastLED.setBrightness(ledConfig.brightness);
    FastLED.show();
}

struct ReceivedPacket {
    long receivedAtMs;
    uint16_t packetIndex;
};

ReceivedPacket recentPackets[64];

void receiverLoop() {
    static long lastPacketReceivedAtMs = millis();
    static uint8_t health = 1;

    byte pipeNo;

    LedConfig newConfig = { 16, 0 };
    bool readPacket = false;
    while (radio.available(&pipeNo)) {
        radio.read(&newConfig, sizeof(newConfig));
        readPacket = true;
    }

    if (! readPacket) {
        if (millis() - lastPacketReceivedAtMs > 10000) {
            FastLED.setBrightness(ledConfig.brightness);
            showSolidColor(CRGB::Yellow);
        }

        return;
    }

    if (newConfig.packetIndex > ledConfig.packetIndex || (millis() - lastPacketReceivedAtMs) > 1000 || ledConfig.packetIndex > 65000) {
        lastPacketReceivedAtMs = millis();
    } else {
        Serial.print("Received out of order packet. last index=");
        Serial.print(ledConfig.packetIndex);
        Serial.print("new index=");
        Serial.print(newConfig.packetIndex);
        Serial.println();
        return;
    }

    uint16_t missedPacketCount = newConfig.packetIndex - ledConfig.packetIndex - 1;
    if (missedPacketCount < 1) {
        if (health <= 255-2)
            health += 2;
    }
    else if (missedPacketCount < 2) {
        if (health <= 255-1)
            health += 1;
    } else if (missedPacketCount > 100) {
        if (health > 10)
            health -= 10;
    } else if (missedPacketCount > 10) {
        if (health > 5)
            health -= 5;
    } else if (missedPacketCount > 3) {
        if (health > 1)
            health --;
    }

    ledConfig = newConfig;

    Serial.print("Health: ");
    Serial.print(health);
    Serial.print("; Missed Count: ");
    Serial.print(missedPacketCount);
    Serial.println();

    progressBar(
        health,
        health > 250
            ? CRGB::Blue
            : CRGB(CRGB::Red).lerp8(CRGB::Green, health),
        0,
        STATUS_LED_COUNT
    );

    cylonAnimation(STATUS_LED_COUNT, NUM_LEDS-STATUS_LED_COUNT);

    FastLED.setBrightness(ledConfig.brightness);
    FastLED.show();
}

void loop(void) {
    if (IS_TRANSMITTER) {
        transmitterLoop();
    } else {
        receiverLoop();
    }
}
