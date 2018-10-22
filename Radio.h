
/******************************************
 * RADIO PIPE ADDRESSES
 *
 * There are effectively 256 pipe "addresses" which can be read or written to. The whole address is about 5 bytes
 * but only one can easily be set for our purposes. See RF24.openReadingPipe for details.
 *
 * To allow client-specific data transmission, we allocate pipes 128-255 for client specific data.
 *
 */
#define RADIO_PIPE_ADDR_COMMON "_DINO"

// Pipe used for meta communication. Does not contain animation information. Format and behavior are defined by the
// main application logic, not animations.
#define RADIO_PIPE_ADDR_META uint8_t(0)

// Pipe used for animations that is shared between all clients
#define RADIO_PIPE_ADDR_ANIMATION_COMMON uint8_t(1)

// Start of client-specific animation addresses
#define RADIO_PIPE_ADDR_ANIMATION_CLIENT_SPECIFIC_START uint8_t(128)



/******************************************
 * RADIO PIPE INDEXES
 *
 * There are 6 available pipe indexes in the radio. 0 is for writing or reading, 1-5 are only for reading.
 *
 * These are the pipe-index allocations for the system.
 */

// The single writing pipe. The address of this pipe is changed before each write.
#define RADIO_PIPE_INDEX_WRITING uint8_t(0)

// The metadata reading pipe. All radios listen to a single address on this pipe.
#define RADIO_PIPE_INDEX_META uint8_t(1)

// The animation common pipe. All animation-receiving radios listen to a single address on this pipe.
#define RADIO_PIPE_INDEX_ANIMATION_COMMON uint8_t(2)

// The animation client-specific pipe. All animation-receiving radios listen to a unique address on this pipe.
#define RADIO_PIPE_INDEX_ANIMATION_CLIENT_SPECIFIC uint8_t(3)


/**
 * Initializes the radio sub-system. Does _not_ setup receiving or transmitting.
 */
bool radioInit();

/**
 * Sets up the radio pipes.
 */
void radioSetupPipes(
    bool receivingAnimation,
    uint8_t clientIndex
);

/**
 * Transmits data to the given pipe address. The maximum data size is 32 bytes.
 */
void radioTransmit(
    uint8_t pipeAddress,
    const uint8_t * data,
    uint8_t length
);