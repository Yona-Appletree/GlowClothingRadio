#include <SPI.h>
#include <FastLED.h>
#include <Button.h>
#include <CircularBuffer.h>
#include <RF24.h>

#include "Radio.h"
#include "Logging.h"

///////////////////
// CONFIGURATION

// Kahli's initial NRF24L01 wirings, with the LEDs on Pin 4
//#include "config/setups/kahliReceiverPin4.h"

// Kahli's initial NRF24L01 wirings, with the LEDs on Pin 3
//#include "config/setups/kahliReceiverPin3.h"

// Yona's STM32 testing rig
#include "config/setups/yonaStm32Tester.h"

// Yona's Prototype high-power transmitter board
//#include "config/setups/yonaTransmitter.h"

// Yona's Nano-based PCB
//#include "config/setups/yonaNanoBoard.h"

///////////////////
///////////////////

#include "config/configOverrides.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Lifecycle Functions

void setup() {
	Serial.begin(115200);

	while (! radioInit()) {
		LOG_STR("Waiting before retry.\n");
	}
}

void loop(void) {

}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Radio Definitions

RF24 radio(PIN_CE, PIN_CS);
uint8_t radioLastTransmitPipeAddress = 0;


/**
 * Initializes the radio sub-system. Does _not_ setup receiving or transmitting.
 */
bool radioInit() {
	LOG_STR("Initializing Radio\n");

	LOG_STR("\n\tPIN_CE=");
	LOG_NUM(PIN_CE);
	LOG_STR("\n\tPIN_CS=");
	LOG_NUM(PIN_CS);

	if (radio.begin()) {
		LOG_STR("\n\tSUCCESS");
	} else {
		LOG_STR("\n\tFAILURE");
		return false;
	}

	LOG_STR("Configuring Radio... ");
	radio.setAutoAck(false);
	radio.enableDynamicPayloads();
	radio.setRetries(0, 0);
	LOG_STR("Done\n");

	LOG_STR("Switching to 2mbps data rate... ");
	if (radio.setDataRate(RF24_2MBPS)) {
		LOG_STR("OK\n");
	} else {
		LOG_STR("FAIL\n");
		return false;
	}

	return true;
}

/**
 * Sets up the radio pipes.
 */
void radioSetupPipes(
	bool receivingAnimation,
	uint8_t clientIndex
) {
	radio.stopListening();

	uint8_t addr[] = RADIO_PIPE_ADDR_COMMON;

	addr[0] = RADIO_PIPE_ADDR_META;
	radio.openReadingPipe(RADIO_PIPE_INDEX_META, addr);

	addr[0] = radioLastTransmitPipeAddress;
	radio.openWritingPipe(addr);

	if (receivingAnimation) {
		addr[0] = RADIO_PIPE_ADDR_ANIMATION_COMMON;
		radio.openReadingPipe(RADIO_PIPE_INDEX_ANIMATION_COMMON, addr);

		addr[0] = RADIO_PIPE_ADDR_ANIMATION_CLIENT_SPECIFIC_START + clientIndex;
		radio.openReadingPipe(RADIO_PIPE_INDEX_ANIMATION_CLIENT_SPECIFIC, addr);
	}
}

/**
 * Transmits data to the given pipe address. The maximum data size is 32 bytes.
 */
void radioTransmit(
	uint8_t pipeAddress,
	const uint8_t * data,
	uint8_t length
) {
	if (radioLastTransmitPipeAddress != pipeAddress) {
		uint8_t addr[] = RADIO_PIPE_ADDR_COMMON;
		addr[0] = radioLastTransmitPipeAddress;
		radio.openWritingPipe(addr);
	}

	radio.writeFast(data, length, true);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fix FastLED code bloat

namespace __gnu_cxx {
	// on't suck in exceptions, they bloat the code size, see https://github.com/FastLED/FastLED/issues/142

	void __verbose_terminate_handler() {
		while (1);
	}
}















































































// This is an array of leds.  One item for each led in your strip.
CRGB leds[ATTACHED_LED_COUNT];

#define RECEIVER_COUNT 50
#define FRAME_LED_COUNT (5*7)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

struct LedConfig {
	byte brightness;
	uint64_t packetIndex;
} ledConfig = {16, 0};


/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(PIN_CE, PIN_CS);

Button button(/*PIN */ PIN_BUTTON, 1, 1, 100);

uint8_t receiverIndex = 0;

void fillLeds(CRGB color, uint8_t start = 0, uint8_t count = ATTACHED_LED_COUNT) {
	for (uint8_t i = start; i < start + count; i++)
		leds[i] = color;
}

void progressBar(
	uint8_t fraction,
	CRGB color,
	uint8_t start = 0,
	uint8_t count = ATTACHED_LED_COUNT
) {
	uint8_t mappedProgress = map8(fraction, 0, count - 1);
	if (fraction > 0 && mappedProgress == 0)
		mappedProgress = 1;

	fillLeds(color, start, mappedProgress);
	fillLeds(CRGB::Black, start + mappedProgress, count - mappedProgress);


	// End marker
	if (count > 2) {
		leds[start + count - 1] = color;
	}
}

void showSolidColor(CRGB color) {
	fillLeds(color);
	FastLED.show();
}

void startListening() {
	byte radioAddress[6] = "DINOS";
	radioAddress[0] = receiverIndex;

	radio.closeReadingPipe(1);

	Serial.print("Opening RECEIVE Pipe ");
	Serial.print(receiverIndex);
	Serial.println("...");
	radio.openReadingPipe(1, radioAddress);
	radio.startListening();
	Serial.println("OK");
}

bool setupRadio() {
	Serial.print("Starting Radio ");

	Serial.print("PIN_CE=");
	Serial.print(PIN_CE);
	Serial.print(" PIN_CS=");
	Serial.print(PIN_CS);
	Serial.println("...");

	SPI.begin();

	if (radio.begin()) {
		Serial.println("OK");
	} else {
		Serial.println("FAIL");
		return false;
	}

	Serial.print("Configuring Radio... ");
	radio.setAutoAck(false);
	radio.enableDynamicPayloads();
	//radio.setPayloadSize(sizeof(ledConfig));
	radio.setRetries(0, 0);
	Serial.println("OK");

	Serial.print("Using 2mbps... ");
	if (radio.setDataRate(RF24_2MBPS)) {
		Serial.println("OK");
	} else {
		Serial.println("FAIL");
		return false;
	}

	if (!IS_TRANSMITTER)
		startListening();

	return true;
}


//void setup() {
//	Serial.begin(115200);
//	Serial.println();
//	Serial.println();
//
//	// Setup and configure radio
//
//	Serial.print("Starting LEDs ");
//	Serial.print("PIN_LED=");
//	Serial.println(PIN_LED);
//	FastLED.addLeds<WS2812B, PIN_LED, LED_ORDER>(leds, ATTACHED_LED_COUNT);
//
//	showSolidColor(CRGB(0, 0, 8));
//
//	delay(100);
//
//	while (!setupRadio()) {
//		Serial.println();
//		Serial.println("Waiting 500ms for retry...");
//		showSolidColor(CRGB(8, 0, 0));
//		delay(500);
//	}
//
//	Serial.println();
//	Serial.println("Setup Successful");
//	showSolidColor(CRGB(0, 64, 0));
//
//	//radio.write(&counter, sizeof(counter), true);          // Pre-load an ack-paylod into the FIFO buffer for pipe 1
//	//radio.printDetails();
//}

void cylonAnimation(
	uint8_t start = 0,
	uint8_t count = ATTACHED_LED_COUNT
) {
	fillLeds(CRGB::Black, start, count);
	uint8_t fraction = triwave8(uint8_t(ledConfig.packetIndex));
	leds[start + map8(fraction, 0, count - 1)] = CRGB::White;
}


void (*resetFunc)(void) = 0;

void transmitterLoop() {
	ledConfig.packetIndex++;

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

	cylonAnimation(STATUS_LED_COUNT, ATTACHED_LED_COUNT - STATUS_LED_COUNT);

	CRGB frameData[FRAME_LED_COUNT];
	byte radioAddress[6] = "DINOS";

	for (uint8_t i = 0; i < RECEIVER_COUNT; i++) {
		radioAddress[0] = i;
		radio.openWritingPipe(radioAddress);

		fill_rainbow(frameData, FRAME_LED_COUNT, millis() / 10 * i, 3);
		radio.write(frameData, FRAME_LED_COUNT * 3, true);
	}

	Serial.print("packetIndex=");
	Serial.print((uint32_t) ledConfig.packetIndex);
	Serial.print(" fraction=");
	Serial.print((uint32_t) ledConfig.packetIndex);
	Serial.println();

	FastLED.setBrightness(ledConfig.brightness);
	FastLED.show();
}

struct ReceivedPacket {
	long receivedAtMs;
	uint64_t packetIndex;
};

CircularBuffer<ReceivedPacket, SIGNAL_HEALTH_BUFFER_SIZE> recentPackets;

void receiverLoop() {
	static long lastPacketReceivedAtMs = millis();

	byte pipeNo;

	bool receivedPacket = false;

	long durationSincePrevPacket = (millis() - recentPackets.last().receivedAtMs);
	button.read();

	if (button.wasReleased()) {
		receiverIndex++;
		if (receiverIndex >= RECEIVER_COUNT)
			receiverIndex = 0;

		startListening();
	}

	CRGB readFrame[FRAME_LED_COUNT];
	memset(readFrame, 0, sizeof(readFrame));

	while (radio.available(&pipeNo)) {
		radio.read(readFrame, FRAME_LED_COUNT * 3);

		receivedPacket = true;
	}

	if (receivedPacket) {
		Serial.println("Received packet");


		uint8_t ledsFromPacket = min(FRAME_LED_COUNT, ATTACHED_LED_COUNT);
		memcpy(leds, readFrame, ledsFromPacket * sizeof(CRGB));


		FastLED.setBrightness(ledConfig.brightness);
		FastLED.show();
	}
}
//
//void loop(void) {
//	if (IS_TRANSMITTER) {
//		transmitterLoop();
//	} else {
//		receiverLoop();
//	}
//}

/* Don't suck in exceptions, they bloat the code size, see https://github.com/FastLED/FastLED/issues/142 */
namespace __gnu_cxx {
	void __verbose_terminate_handler() {
		while (1);
	}
}
