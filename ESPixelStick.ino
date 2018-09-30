/*
* ESPixelStick.ino
*
* Project: ESPixelStick - An ESP8266 and E1.31 based pixel driver
* Copyright (c) 2016 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

/*****************************************/
/*         END - Configuration           */
/*****************************************/

#include <Ticker.h>
#include <ESPAsyncE131.h>
#include <ArduinoJson.h>
#include <Bounce2.h>
#include <Hash.h>
#include <SPI.h>
#include "ESPixelStick.h"
#include "RGBEffectWrapper.hpp"
#include "wshandler.h"
#include "gamma.h"
#include "conversions.h"

extern "C" {
#include <user_interface.h>
}

// Debugging support
#if defined(DEBUG)
extern "C" void system_set_os_print(uint8 onoff);
extern "C" void ets_install_putc1(void* routine);

static void _u0_putc(char c){
  while(((U0S >> USTXC) & 0x7F) == 0x7F);
  U0F = c;
}
#endif

/////////////////////////////////////////////////////////
// 
//  Globals
//
/////////////////////////////////////////////////////////

#ifdef NOSERIALCLASS
NoSerialClass NoSerial;
#endif

// Configuration file
const char CONFIG_FILE[] = "/config.json";

ESPAsyncE131        e131(10);       // ESPAsyncE131 with X buffers
testing_t           testing;        // Testing mode
config_t            config;         // Current configuration
uint32_t            *seqError;      // Sequence error tracking for each universe
uint16_t            uniLast = 1;    // Last Universe to listen for
bool                reboot = false; // Reboot flag
uint8_t             *seqTracker;    // Current sequence numbers for each Universe */
uint32_t            lastUpdate;     // Update timeout tracker

// button
Bounce bounce;

// Output Drivers
#if defined(ESPS_MODE_PIXEL)
PixelDriver     pixels;         // Pixel object
RGBEffectWrapper rgbEffect;
static bool needRefresh = false;
#elif defined(ESPS_MODE_SERIAL)
SerialDriver    serial;         // Serial object
#else
#error "No valid output mode defined."
#endif

/////////////////////////////////////////////////////////
// 
//  Forward Declarations
//
/////////////////////////////////////////////////////////

void loadConfig();
void updateConfig();

// Radio config
RF_PRE_INIT() {
    wifi_set_phy_mode(PHY_MODE_11G);    // Force 802.11g mode
    system_phy_set_powerup_option(31);  // Do full RF calibration on power-up
    system_phy_set_max_tpw(82);         // Set max TX power
}

/// WIFI SNIFF {

#define TYPE_MANAGEMENT       0x00
#define TYPE_CONTROL          0x01
#define TYPE_DATA             0x02
#define SUBTYPE_PROBE_REQUEST 0x04

struct RxControl {
    signed rssi:8; // signal intensity of packet
    unsigned rate:4;
    unsigned is_group:1;
    unsigned:1;
    unsigned sig_mode:2; // 0:is 11n packet; 1:is not 11n packet;
    unsigned legacy_length:12; // if not 11n packet, shows length of packet.
    unsigned damatch0:1;
    unsigned damatch1:1;
    unsigned bssidmatch0:1;
    unsigned bssidmatch1:1;
    unsigned MCS:7; // if is 11n packet, shows the modulation and code used (range from 0 to 76)
    unsigned CWB:1; // if is 11n packet, shows if is HT40 packet or not
    unsigned HT_length:16;// if is 11n packet, shows length of packet.
    unsigned Smoothing:1;
    unsigned Not_Sounding:1;
    unsigned:1;
    unsigned Aggregation:1;
    unsigned STBC:2;
    unsigned FEC_CODING:1; // if is 11n packet, shows if is LDPC packet or not.
    unsigned SGI:1;
    unsigned rxend_state:8;
    unsigned ampdu_cnt:8;
    unsigned channel:4; //which channel this packet in.
    unsigned:12;
};

struct SnifferPacket2 {
    RxControl rx_ctrl;
    uint8_t data[112];
    uint16_t cnt;
    uint16_t len;
};

static void printDataSpan(uint16_t size, uint8_t* data) {
    char buff[129];
    for(uint16_t i = 0; i < size; i++) {
        if ((data[i] >= '0' && data[i] <= '9')
                || (data[i] >= 'a' && data[i] <= 'z')
                || (data[i] >= 'A' && data[i] <= 'Z'))
            buff[i] = data[i];
        else
            buff[i] = '.';
    }

    buff[size] = '\0';
    Serial.print(buff);
}

static char dmxIn[85 + 1 + 2 + 6];
static int receivedUniverse = 0;
static int receivedSequence = 0;
static int receivedSize = 0;

static void readData(struct SnifferPacket2* snifferPacket, uint16_t len) {
    uint8_t* data = snifferPacket->data;
    uint8_t* macData = data + 4;
    uint8_t* essidData = data + 24;
    uint8_t* strData = data + 27;
    if (data[0] != 0x90) // type ATIM
        return;
    if (memcmp(macData, "Ar", 2) || memcmp(macData + 6, "ArtRaw", 6))
        return;

    receivedUniverse = macData[2];
    receivedSequence = macData[3];
    receivedSize = macData[4];
    if (receivedSize > sizeof(dmxIn))
        receivedSize = sizeof(dmxIn);

    dmxIn[0] = macData[5];
    memcpy(dmxIn + 1, essidData, 2);
    memcpy(dmxIn + 3, macData + 12, 6);
    memcpy(dmxIn + 9, strData, receivedSize - 9);
}

/**
 * Callback for promiscuous mode
 */
static void ICACHE_FLASH_ATTR sniffer_callback(uint8_t *buffer, uint16_t len) {
    if (len == 128)
    {
        SnifferPacket2 *snifferPacket = (SnifferPacket2*) buffer;
        readData(snifferPacket, len);
    }
}

#define DISABLE 0
#define ENABLE  1

/// } WIFI SNIFF

void setup() {
    // Initial pin states
    pinMode(DATA_PIN, OUTPUT);
    digitalWrite(DATA_PIN, LOW);

    bounce.attach(BUTTON_PIN, INPUT_PULLUP);
    bounce.read();
    bounce.interval(3);

    // Setup serial log port
    LOG_PORT.begin(115200);
    delay(10);


    /// WIFI SNIFF {
    wifi_set_opmode(STATION_MODE);
    wifi_set_channel(1);
    wifi_promiscuous_enable(DISABLE);
    delay(10);
    wifi_set_promiscuous_rx_cb(sniffer_callback);
    delay(10);
    wifi_promiscuous_enable(ENABLE);
    /// } WIFI SNIFF

#if defined(DEBUG)
    ets_install_putc1((void *) &_u0_putc);
    system_set_os_print(1);
#endif

    LOG_PORT.println("");
    LOG_PORT.print(F("ESPixelStick v"));
    for (uint8_t i = 0; i < strlen_P(VERSION); i++)
        LOG_PORT.print((char)(pgm_read_byte(VERSION + i)));
    LOG_PORT.print(F(" ("));
    for (uint8_t i = 0; i < strlen_P(BUILD_DATE); i++)
        LOG_PORT.print((char)(pgm_read_byte(BUILD_DATE + i)));
    LOG_PORT.println(")");

    pixels.setPin(DATA_PIN);
    updateConfig();
    pixels.show();
}

/////////////////////////////////////////////////////////
// 
//  JSON / Configuration Section
//
/////////////////////////////////////////////////////////

// Configuration Validations
void validateConfig() {
    // ArtNet Limits
    if (config.universe < 0)
        config.universe = 0;

    if (config.universe_limit > UNIVERSE_MAX || config.universe_limit < 1)
        config.universe_limit = UNIVERSE_MAX;

    if (config.channel_start < 1)
        config.channel_start = 1;
    else if (config.channel_start > config.universe_limit)
        config.channel_start = config.universe_limit;

    // Generic channel limits for pixels
    if (config.channel_count % 3)
        config.channel_count = (config.channel_count / 3) * 3;

    if (config.channel_count > PIXEL_LIMIT * 3)
        config.channel_count = PIXEL_LIMIT * 3;
    else if (config.channel_count < 1)
        config.channel_count = 1;

    // GECE Limits
    if (config.pixel_type == PixelType::GECE) {
        config.pixel_color = PixelColor::RGB;
        if (config.channel_count > 63 * 3)
            config.channel_count = 63 * 3;
    }

    // Default gamma value
    if (config.gammaVal <= 0) {
        config.gammaVal = 2.2;
    }

    // Default brightness value
    if (config.briteVal <= 0) {
        config.briteVal = 1.0;
    }
}

void updateConfig() {
    // Validate first
    validateConfig();

    // Find the last universe we should listen for
    uint16_t span = config.channel_start + config.channel_count - 1;
    if (span % config.universe_limit)
        uniLast = config.universe + span / config.universe_limit;
    else
        uniLast = config.universe + span / config.universe_limit - 1;

    // Setup the sequence error tracker
    uint8_t uniTotal = (uniLast + 1) - config.universe;

    if (seqTracker) free(seqTracker);
    if ((seqTracker = static_cast<uint8_t *>(malloc(uniTotal))))
        memset(seqTracker, 0x00, uniTotal);

    if (seqError) free(seqError);
    if ((seqError = static_cast<uint32_t *>(malloc(uniTotal * 4))))
        memset(seqError, 0x00, uniTotal * 4);

    // Zero out packet stats
    e131.stats.num_packets = 0;

    // Initialize for our pixel type
    pixels.begin(config.pixel_type, config.pixel_color, config.channel_count / 3);
    pixels.setGamma(config.gamma);
    updateGammaTable(config.gammaVal, config.briteVal);
    rgbEffect.begin(pixels.getData(), LED_COUNT);

    LOG_PORT.print(F("- Listening for "));
    LOG_PORT.print(config.channel_count);
    LOG_PORT.print(F(" channels, from Universe "));
    LOG_PORT.print(config.universe);
    LOG_PORT.print(F(" to "));
    LOG_PORT.println(uniLast);
}

// Load configugration JSON file
void loadConfig() {
    // Zeroize Config struct
    memset(&config, 0, sizeof(config));

    // Override config
    config.universe = 0;
    config.universe_limit = UNIVERSE_MAX;
    config.channel_start = 1;
    config.channel_count = LED_COUNT * 3;

    config.pixel_type = PixelType::WS2811;
    config.pixel_color = PixelColor::GRB;
    config.gamma = true;
    config.gammaVal = 2.2f;
    config.briteVal = 1.0f;

    // Validate it
    validateConfig();
}

/////////////////////////////////////////////////////////
//
//  Set routines for Testing
//
/////////////////////////////////////////////////////////

void setStatic(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t i = 0;
    while (i <= config.channel_count - 3) {
#if defined(ESPS_MODE_PIXEL)
        pixels.setValue(i++, r);
        pixels.setValue(i++, g);
        pixels.setValue(i++, b);
#elif defined(ESPS_MODE_SERIAL)
        serial.setValue(i++, r);
        serial.setValue(i++, g);
        serial.setValue(i++, b);
#endif
    }
}

void handleButtonLongPressBegin()
{
    LOG_PORT.println("LONG PRESS BEGIN");
    rgbEffect.startFlash();
}

void handleButtonLongPressEnd()
{
    LOG_PORT.println("LONG PRESS END");
    rgbEffect.stopFlash();
}

void handleButtonClick()
{
    LOG_PORT.println("DOUBLE CLICK RELEASE");
    rgbEffect.nextMode();
}

void handleButtonDoubleClick()
{
    LOG_PORT.println("CLICK RELEASE");
    rgbEffect.nextColor();
}

void handleButton()
{
    static bool longPressDone = false;
    static bool clickedOnce = false;
    static int prevTime = 0;
    if (bounce.read() == LOW)
    {
        if (!longPressDone && millis() - prevTime > 500)
        {
            handleButtonLongPressBegin();
            longPressDone = true;
        }
    }
    else
    {
        if (clickedOnce && millis() - prevTime > 300)
        {
            handleButtonClick();
            clickedOnce = false;
        }
    }

    if (bounce.update())
    {
        prevTime = millis();
        if (bounce.read() == LOW)
        {
            longPressDone = false;
        }
        else
        {
            if (!longPressDone)
            {
                if (clickedOnce)
                {
                    clickedOnce = false;
                    handleButtonDoubleClick();
                }
                else
                {
                    clickedOnce = true;
                }
            }
            else
            {
                handleButtonLongPressEnd();
            }
        }
    }
}


/////////////////////////////////////////////////////////
//
//  Main Loop
//
/////////////////////////////////////////////////////////
static void convertFromYCbCr(uint8_t* rgbFromYCbCr, uint8_t const* ycbcr5bit, unsigned int ycbcr5bitSize)
{
    uint8_t cb[4];
    uint8_t cr[4];
    for (unsigned int i = 0; i < ycbcr5bitSize / 9; ++i)
    {
        conv_cbcr_from_5bit(ycbcr5bit + (i * 9) + 4, cb, cr);
        for (unsigned int j = 0; j < 4; ++j)
            conv_ycbcr_to_rgb(rgbFromYCbCr + (i * 3 * 4) + (j * 3),
                    ycbcr5bit[i * 9 + j], cb[j], cr[j]);
    }
}

void loop() {
    e131_packet_t packet;

    // Reboot handler
    if (reboot) {
        delay(REBOOT_DELAY);
        ESP.restart();
    }

    if (true) {
        // Parse a packet and update pixels
        if (!e131.isEmpty()) {
            e131.pull(&packet);
            uint16_t universe = htons(packet.universe);
            uint8_t *data = packet.property_values + 1;
            if ((universe >= config.universe) && (universe <= uniLast)) {
                // Universe offset and sequence tracking
                uint8_t uniOffset = (universe - config.universe);
                if (packet.sequence_number != seqTracker[uniOffset]++) {
                    seqError[uniOffset]++;
                    seqTracker[uniOffset] = packet.sequence_number + 1;
                }

                // Offset the channels if required
                uint16_t offset = 0;
                offset = config.channel_start - 1;

                // Find start of data based off the Universe
                int16_t dataStart = uniOffset * config.universe_limit - offset;

                // Calculate how much data we need for this buffer
                uint16_t dataStop = config.channel_count;
                uint16_t channels = htons(packet.property_value_count) - 1;
                if (config.universe_limit < channels)
                    channels = config.universe_limit;
                if ((dataStart + channels) < dataStop)
                    dataStop = dataStart + channels;

                // Set the data
                uint16_t buffloc = 0;

                // ignore data from start of first Universe before channel_start
                if (dataStart < 0) {
                    dataStart = 0;
                    buffloc = config.channel_start - 1;
                }

//                fixture.updateInput(data + dataStart, dataStop - dataStart);
//                for (int i = dataStart; i < dataStop; i++) {
//#if defined(ESPS_MODE_PIXEL)
//                    pixels.setValue(i, data[buffloc]);
//#elif defined(ESPS_MODE_SERIAL)
//                    serial.setValue(i, data[buffloc]);
//#endif
//                    buffloc++;
//                }
            }
        }
    }

    handleButton();

    if (receivedSize > 0)
    {
        static int logCounter = 0;
        ++logCounter;

        if (logCounter%50==0)
        {
            LOG_PORT.print("dmx[0]=");
            LOG_PORT.print(dmxIn[0], DEC);
            LOG_PORT.print("dmx[1]=");
            LOG_PORT.print(dmxIn[1], DEC);
            LOG_PORT.print("dmx[2]=");
            LOG_PORT.print(dmxIn[2], DEC);
            LOG_PORT.print("dmx[3]=");
            LOG_PORT.print(dmxIn[3], DEC);
            LOG_PORT.print("dmx[4]=");
            LOG_PORT.print(dmxIn[4], DEC);
            LOG_PORT.print("dmx[5]=");
            LOG_PORT.print(dmxIn[5], DEC);
        }


        uint8_t rgbFromYCbCr[120];
        uint8_t* input = (uint8_t*)dmxIn;
        int size = receivedSize;
        if ((receivedUniverse & 0x80) == 0)
        {
            if (logCounter%50==0)LOG_PORT.println("converting from ycbcr");
            input = rgbFromYCbCr;
            size = receivedSize + (receivedSize / 3);
            convertFromYCbCr(rgbFromYCbCr, (uint8_t*)dmxIn, receivedSize);
        }
        else
        {
            if (logCounter%50==0)LOG_PORT.println("not converting from ycbcr");
        }
        if (size > LED_COUNT * 3)
            size = LED_COUNT * 3;
        memcpy(pixels.getData(), input, size);
        receivedSize = 0;
        needRefresh = true;
    }
    //if (rgbEffect.refreshPixels(millis()))
    //    needRefresh = true;
/* Streaming refresh */
#if defined(ESPS_MODE_PIXEL)
    if (needRefresh && pixels.canRefresh()) {
        pixels.show();
        needRefresh = false;
    }
#elif defined(ESPS_MODE_SERIAL)
    if (serial.canRefresh())
        serial.show();
#endif
}
