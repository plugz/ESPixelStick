/*
* ESPixelStick.h
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

#ifndef ESPIXELSTICK_H_
#define ESPIXELSTICK_H_

const char VERSION[] = "3.1-dev-amwa";
const char BUILD_DATE[] = __DATE__;

/*****************************************/
/*        BEGIN - Configuration          */
/*****************************************/

/* Output Mode - There can be only one! (-Conor MacLeod) */
#define ESPS_MODE_PIXEL
//#define ESPS_MODE_SERIAL

#define LED_COUNT 40

/*****************************************/
/*         END - Configuration           */
/*****************************************/

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>

#if defined(ESPS_MODE_PIXEL)
#include "PixelDriver.h"
#elif defined(ESPS_MODE_SERIAL)
#include "SerialDriver.h"
#endif

#define DATA_PIN        2       /* Pixel output - GPIO2 */
#define BUTTON_PIN      0       /* Button input - GPIO0, same as bootloader button */
#define EEPROM_BASE     0       /* EEPROM configuration base address */
#define UNIVERSE_MAX    512     /* Max channels in a DMX Universe */
#define PIXEL_LIMIT     1360    /* Total pixel limit - 40.85ms for 8 universes */
#define RENARD_LIMIT    2048    /* Channel limit for serial outputs */
#define E131_TIMEOUT    1000    /* Force refresh every second an E1.31 packet is not seen */
#define CONNECT_TIMEOUT 15000   /* 15 seconds */
#define REBOOT_DELAY    100     /* Delay for rebooting once reboot flag is set */
#if 1
#define LOG_PORT        Serial  /* Serial port for console logging */
#else
class NoSerialClass
{
public:
    template<typename... T>
    void begin(T...) {}
    template<typename... T>
    void print(T...) {}
    template<typename... T>
    void println(T...) {}
};
extern NoSerialClass NoSerial;
#define LOG_PORT NoSerial
#define NOSERIALCLASS 1
#endif

/* Configuration file params */
#define CONFIG_MAX_SIZE 2048    /* Sanity limit for config file */

/* Pixel Types */
class DevCap {
 public:
    bool MPIXEL : 1;
    bool MSERIAL : 1;
    bool MPWM : 1;
    uint8_t toInt() {
        return (MPWM << 2 | MSERIAL << 1 | MPIXEL);
    }
};

/*
enum class DevMode : uint8_t {
    MPIXEL,
    MSERIAL
};
*/

/* Test Modes */
enum class TestMode : uint8_t {
    DISABLED,
    STATIC,
    CHASE,
    RAINBOW,
    VIEW_STREAM
};

typedef struct {
    uint8_t r, g, b;    /* Hold requested color */
    uint16_t step;      /* Step in testing routine */
    uint32_t last;      /* Last update */
} testing_t;

/* Configuration structure */
typedef struct {
    /* ArtNet */
    uint16_t    universe;       /* Universe to listen for */
    uint16_t    universe_limit; /* Universe boundary limit */
    uint16_t    channel_start;  /* Channel to start listening at - 1 based */
    uint16_t    channel_count;  /* Number of channels */

    /* Pixels */
    PixelType   pixel_type;     /* Pixel type */
    PixelColor  pixel_color;    /* Pixel color order */
    bool        gamma;          /* Use gamma map? */
    float       gammaVal;       /* gamma value to use */
    float       briteVal;       /* brightness lto use */
} config_t;


/* Forward Declarations */
void serializeConfig(String &jsonString, bool pretty = false, bool creds = false);
void dsNetworkConfig(JsonObject &json);
void dsDeviceConfig(JsonObject &json);
void saveConfig();

void setStatic(uint8_t r, uint8_t g, uint8_t b);

#endif /* ESPIXELSTICK_H_ */
