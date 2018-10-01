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

#include <Bounce2.h>
#include "ESPixelStick.h"
#include "RGBEffectWrapper.hpp"
#include "RGBEffectWrapperDMX.hpp"
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

config_t            config;         // Current configuration
uint16_t            uniLast = 1;    // Last Universe to listen for
bool                reboot = false; // Reboot flag
uint32_t            lastUpdate;     // Update timeout tracker

// button
Bounce bounce;

// Output Drivers
PixelDriver     pixels;         // Pixel object
RGBEffectWrapper buttonRgbEffect;
RGBEffectWrapperDMX artnetRgbEffect;
static bool needRefresh = false;

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

/*
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
    LOG_PORT.print(buff);
}
*/

static char dmxIn[85 + 1 + 2 + 6];
static int receivedUniverse = 0;
static int receivedSequence = 0;
static unsigned int receivedSize = 0;

static void readData(struct SnifferPacket2* snifferPacket, uint16_t len) {
    (void)len; // ignore len, data is 112 bytes anyway

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
    if (receivedSize > 9)
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
    memset(dmxIn, 0, sizeof(dmxIn));
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
    loadConfig();
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

    // Initialize for our pixel type
    pixels.begin(config.pixel_type, config.pixel_color, config.channel_count / 3);
    pixels.setGamma(config.gamma);
    updateGammaTable(config.gammaVal, config.briteVal);
    buttonRgbEffect.begin(pixels.getData(), LED_COUNT);
    artnetRgbEffect.begin(pixels.getData(), LED_COUNT);

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

    config.inputMode = InputMode::ARTNET_YCBCR;

    // Validate it
    validateConfig();
}

/////////////////////////////////////////////////////////
//
//  Button handling
//
/////////////////////////////////////////////////////////

// Set input mode to BUTTON_FIXTURE after 5 events
static void buttonEvent()
{
    static unsigned int buttonEventCount = 0;
    ++buttonEventCount;
    if (buttonEventCount > 5)
        config.inputMode = InputMode::BUTTON_FIXTURE;
}

void handleButtonLongPressBegin()
{
    LOG_PORT.println("LONG PRESS BEGIN");
    buttonRgbEffect.startFlash();

    buttonEvent();
}

void handleButtonLongPressEnd()
{
    LOG_PORT.println("LONG PRESS END");
    buttonRgbEffect.stopFlash();

    buttonEvent();
}

void handleButtonClick()
{
    LOG_PORT.println("DOUBLE CLICK RELEASE");
    buttonRgbEffect.nextMode();

    buttonEvent();
}

void handleButtonDoubleClick()
{
    LOG_PORT.println("CLICK RELEASE");
    buttonRgbEffect.nextColor();

    buttonEvent();
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
static void convertFromYCbCr5bit(uint8_t* rgbFromYCbCr, uint8_t const* ycbcr5bit, unsigned int ycbcr5bitSize)
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

static void handleArtNet()
{
    if (receivedSize == 0)
        return;

    unsigned int inSize = receivedSize;
    receivedSize = 0;

    // Ignore artnet once the button mode has been activated
    if (config.inputMode == InputMode::BUTTON_FIXTURE)
        return;

    // universe coded on 7 bits
    if ((receivedUniverse & 0x7f) != config.universe)
        return;

    if ((receivedUniverse & 0x80) == 0)
    {
        config.inputMode = InputMode::ARTNET_YCBCR;

        unsigned int maxInSize = ((LED_COUNT * 3) * 9) / 12;
        if (inSize > maxInSize)
            inSize = maxInSize;

        convertFromYCbCr5bit(pixels.getData(), (uint8_t*)dmxIn, inSize);
        needRefresh = true;
    }
    else
    {
        config.inputMode = InputMode::ARTNET_FIXTURE;

        artnetRgbEffect.setInputDMX((uint8_t*)dmxIn, inSize);
    }
}

void loop() {
    // Reboot handler
    if (reboot) {
        delay(REBOOT_DELAY);
        ESP.restart();
    }

    handleButton();
    handleArtNet();

    if ((config.inputMode == InputMode::BUTTON_FIXTURE)
            && buttonRgbEffect.refreshPixels(millis()))
        needRefresh = true;
    else if ((config.inputMode == InputMode::ARTNET_FIXTURE)
            && artnetRgbEffect.refreshPixels(millis()))
        needRefresh = true;
/* Streaming refresh */
    if (needRefresh && pixels.canRefresh()) {
        pixels.show();
        needRefresh = false;
    }
}
