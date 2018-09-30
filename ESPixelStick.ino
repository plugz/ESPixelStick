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
/*        BEGIN - Configuration          */
/*****************************************/

/* Output Mode has been moved to ESPixelStick.h */

/* Fallback configuration if config.json is empty or fails */
const char ssid[] = "tarte_a_la_moutarde";
const char passphrase[] = "abattoir2";

/*****************************************/
/*         END - Configuration           */
/*****************************************/

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncUDP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncE131.h>
#include <ArduinoJson.h>
#include <Bounce2.h>
#include <Hash.h>
#include <SPI.h>
#include "ESPixelStick.h"
#include "EFUpdate.h"
#include "RGBEffectWrapper.hpp"
#include "wshandler.h"
#include "pwm.h"
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

// MQTT State
const char MQTT_LIGHT_STATE_TOPIC[] = "/light/status";
const char MQTT_LIGHT_COMMAND_TOPIC[] = "/light/switch";

// MQTT Brightness
const char MQTT_LIGHT_BRIGHTNESS_STATE_TOPIC[] = "/brightness/status";
const char MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC[] = "/brightness/set";

// MQTT Colors (rgb)
const char MQTT_LIGHT_RGB_STATE_TOPIC[] = "/rgb/status";
const char MQTT_LIGHT_RGB_COMMAND_TOPIC[] = "/rgb/set";

// MQTT Payloads by default (on/off)
const char LIGHT_ON[] = "ON";
const char LIGHT_OFF[] = "OFF";

// MQTT variables used to store the state, the brightness and the color of the light
boolean m_rgb_state = false;
uint8_t m_rgb_brightness = 100;
uint8_t m_rgb_red = 255;
uint8_t m_rgb_green = 255;
uint8_t m_rgb_blue = 255;

// MQTT buffer used to send / receive data
const uint8_t MSG_BUFFER_SIZE = 20;
char m_msg_buffer[MSG_BUFFER_SIZE]; 

// Configuration file
const char CONFIG_FILE[] = "/config.json";

ESPAsyncE131        e131(10);       // ESPAsyncE131 with X buffers
testing_t           testing;        // Testing mode
config_t            config;         // Current configuration
uint32_t            *seqError;      // Sequence error tracking for each universe
uint16_t            uniLast = 1;    // Last Universe to listen for
bool                reboot = false; // Reboot flag
AsyncWebServer      web(HTTP_PORT); // Web Server
AsyncWebSocket      ws("/ws");      // Web Socket Plugin
uint8_t             *seqTracker;    // Current sequence numbers for each Universe */
uint32_t            lastUpdate;     // Update timeout tracker
WiFiEventHandler    wifiConnectHandler;     // WiFi connect handler
WiFiEventHandler    wifiDisconnectHandler;  // WiFi disconnect handler
Ticker              wifiTicker; // Ticker to handle WiFi
AsyncMqttClient     mqtt;       // MQTT object
Ticker              mqttTicker; // Ticker to handle MQTT

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
void initWifi();
void initWeb();
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

//    Serial.print("uni=");
//    Serial.print(receivedUniverse, DEC);
//    Serial.print(", seq=");
//    Serial.print(receivedSequence, DEC);
//    Serial.print(", len=");
//    Serial.print(receivedSize, DEC);
//    Serial.print(", data=");
//    printDataSpan(receivedSize, data + 29);
//    Serial.println();
//
//    Serial.println(int(data[27]), DEC);
//    Serial.println(int(data[28]), DEC);
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

/**
 * Callback for channel hoping
 */
//void channelHop()
//{
//    // hoping channels 1-14
//    uint8 new_channel = wifi_get_channel() + 1;
//    if (new_channel > 14)
//        new_channel = 1;
//    wifi_set_channel(new_channel);
//}

#define DISABLE 0
#define ENABLE  1

/// } WIFI SNIFF

void setup() {
    // Configure SDK params
    //wifi_set_sleep_type(NONE_SLEEP_T);

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

    // setup the channel hoping callback timer
    //os_timer_disarm(&channelHop_timer);
    //os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
    //os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);
    /// } WIFI SNIFF

#if defined(DEBUG)
    ets_install_putc1((void *) &_u0_putc);
    system_set_os_print(1);
#endif

    // Enable SPIFFS
    SPIFFS.begin();

    LOG_PORT.println("");
    LOG_PORT.print(F("ESPixelStick v"));
    for (uint8_t i = 0; i < strlen_P(VERSION); i++)
        LOG_PORT.print((char)(pgm_read_byte(VERSION + i)));
    LOG_PORT.print(F(" ("));
    for (uint8_t i = 0; i < strlen_P(BUILD_DATE); i++)
        LOG_PORT.print((char)(pgm_read_byte(BUILD_DATE + i)));
    LOG_PORT.println(")");

    // Load configuration from SPIFFS and set Hostname
    loadConfig();
//    WiFi.hostname(config.hostname);

    // Setup WiFi Handlers
    //wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);

    // Setup MQTT Handlers
    //if (config.mqtt) {
    //    mqtt.onConnect(onMqttConnect);
    //    mqtt.onDisconnect(onMqttDisconnect);
    //    mqtt.onMessage(onMqttMessage);
    //    mqtt.setServer(config.mqtt_ip.c_str(), config.mqtt_port);
    //    if (config.mqtt_user.length() > 0)
    //        mqtt.setCredentials(config.mqtt_user.c_str(), config.mqtt_password.c_str());
    //}


    // Configure the outputs
#if defined (ESPS_SUPPORT_PWM)
    //setupPWM();
#endif

#if defined (ESPS_MODE_PIXEL)
    pixels.setPin(DATA_PIN);
    updateConfig();
    pixels.show();
#else
    updateConfig();
#endif
}

/////////////////////////////////////////////////////////
//
//  WiFi Section
//
/////////////////////////////////////////////////////////

void initWifi() {
    // Switch to station mode and disconnect just in case
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    connectWifi();
    uint32_t timeout = millis();
    while (WiFi.status() != WL_CONNECTED) {
        LOG_PORT.print(".");
        delay(500);
        if (millis() - timeout > CONNECT_TIMEOUT) {
            LOG_PORT.println("");
            LOG_PORT.println(F("*** Failed to connect ***"));
            break;
        }
    }
}

void connectWifi() {
    delay(secureRandom(100, 500));

    LOG_PORT.println("");
    LOG_PORT.print(F("Connecting to "));
    LOG_PORT.print(config.ssid);
    LOG_PORT.print(F(" as "));
    LOG_PORT.println(config.hostname);

    WiFi.begin(config.ssid.c_str(), config.passphrase.c_str());
    if (config.dhcp) {
        LOG_PORT.print(F("Connecting with DHCP"));
    } else {
        // We don't use DNS, so just set it to our gateway
        WiFi.config(IPAddress(config.ip[0], config.ip[1], config.ip[2], config.ip[3]),
                    IPAddress(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3]),
                    IPAddress(config.netmask[0], config.netmask[1], config.netmask[2], config.netmask[3]),
                    IPAddress(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3])
        );
        LOG_PORT.print(F("Connecting with Static IP"));
    }
}

void onWifiConnect(const WiFiEventStationModeGotIP &event) {
    LOG_PORT.println("");
    LOG_PORT.print(F("Connected with IP: "));
    LOG_PORT.println(WiFi.localIP());

    // Setup MQTT connection if enabled
    if (config.mqtt)
        connectToMqtt();

    // Setup mDNS / DNS-SD
    //TODO: Reboot or restart mdns when config.id is changed?
     char chipId[7] = { 0 };
    snprintf(chipId, sizeof(chipId), "%06x", ESP.getChipId());
    MDNS.setInstanceName(config.id + " (" + String(chipId) + ")");
    if (MDNS.begin(config.hostname.c_str())) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        MDNS.addService("e131", "udp", E131_DEFAULT_PORT);
        MDNS.addServiceTxt("e131", "udp", "TxtVers", String(RDMNET_DNSSD_TXTVERS));
        MDNS.addServiceTxt("e131", "udp", "ConfScope", RDMNET_DEFAULT_SCOPE);
        MDNS.addServiceTxt("e131", "udp", "E133Vers", String(RDMNET_DNSSD_E133VERS));
        MDNS.addServiceTxt("e131", "udp", "CID", String(chipId));
        MDNS.addServiceTxt("e131", "udp", "Model", "ESPixelStick");
        MDNS.addServiceTxt("e131", "udp", "Manuf", "Forkineye");
    } else {
        LOG_PORT.println(F("*** Error setting up mDNS responder ***"));
    }
}

void onWiFiDisconnect(const WiFiEventStationModeDisconnected &event) {
    LOG_PORT.println(F("*** WiFi Disconnected ***"));

    // Pause MQTT reconnect while WiFi is reconnecting
    mqttTicker.detach();
    wifiTicker.once(2, connectWifi);
}

// Subscribe to "n" universes, starting at "universe"
void multiSub() {
    uint8_t count;
    ip_addr_t ifaddr;
    ip_addr_t multicast_addr;

    count = uniLast - config.universe + 1;
    ifaddr.addr = static_cast<uint32_t>(WiFi.localIP());
    for (uint8_t i = 0; i < count; i++) {
        multicast_addr.addr = static_cast<uint32_t>(IPAddress(239, 255,
                (((config.universe + i) >> 8) & 0xff),
                (((config.universe + i) >> 0) & 0xff)));
        igmp_joingroup(&ifaddr, &multicast_addr);
    }
}

/////////////////////////////////////////////////////////
//
//  MQTT Section
//
/////////////////////////////////////////////////////////

void connectToMqtt() {
    LOG_PORT.print(F("- Connecting to MQTT Broker "));
    LOG_PORT.println(config.mqtt_ip);
    mqtt.connect();
}

void onMqttConnect(bool sessionPresent) {
    LOG_PORT.println(F("- MQTT Connected"));

    // Setup subscriptions
    mqtt.subscribe(String(config.mqtt_topic + MQTT_LIGHT_COMMAND_TOPIC).c_str(), 0);
    mqtt.subscribe(String(config.mqtt_topic + MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC).c_str(), 0);
    mqtt.subscribe(String(config.mqtt_topic + MQTT_LIGHT_RGB_COMMAND_TOPIC).c_str(), 0);

    // Publish state
    publishRGBState();
    publishRGBBrightness();
    publishRGBColor();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    LOG_PORT.println(F("*** MQTT Disconnected ***"));
    if (WiFi.isConnected()) {
        mqttTicker.once(2, connectToMqtt);
    }
}

void onMqttMessage(char* topic, char* p_payload,
        AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

    String payload;
    for (uint8_t i = 0; i < len; i++)
        payload.concat((char)p_payload[i]);
/*
LOG_PORT.print("Topic: ");
LOG_PORT.print(topic);
LOG_PORT.print(" | Payload: ");
LOG_PORT.println(payload);
*/
    // Handle message topic
    if (String(config.mqtt_topic + MQTT_LIGHT_COMMAND_TOPIC).equals(topic)) {
    // Test if the payload is equal to "ON" or "OFF"
        if (payload.equals(String(LIGHT_ON))) {
            config.testmode = TestMode::MQTT;
            if (m_rgb_state != true) {
                m_rgb_state = true;
                setStatic(m_rgb_red * m_rgb_brightness / 100,
                        m_rgb_green * m_rgb_brightness / 100,
                        m_rgb_blue * m_rgb_brightness / 100);
                publishRGBState();
            }
        } else if (payload.equals(String(LIGHT_OFF))) {
            config.testmode = TestMode::DISABLED;
            if (m_rgb_state != false) {
                m_rgb_state = false;
                setStatic(0, 0, 0);
                publishRGBState();
            }
        }
    } else if (String(config.mqtt_topic + MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC).equals(topic)) {
        uint8_t brightness = payload.toInt();
        if (brightness > 100)
            brightness = 100;
        m_rgb_brightness = brightness;
        setStatic(m_rgb_red * m_rgb_brightness / 100,
                m_rgb_green * m_rgb_brightness / 100,
                m_rgb_blue * m_rgb_brightness / 100);
        publishRGBBrightness();
    } else if (String(config.mqtt_topic + MQTT_LIGHT_RGB_COMMAND_TOPIC).equals(topic)) {
        // Get the position of the first and second commas
        uint8_t firstIndex = payload.indexOf(',');
        uint8_t lastIndex = payload.lastIndexOf(',');
    
        m_rgb_red = payload.substring(0, firstIndex).toInt();
        m_rgb_green = payload.substring(firstIndex + 1, lastIndex).toInt();
        m_rgb_blue = payload.substring(lastIndex + 1).toInt();
        setStatic(m_rgb_red * m_rgb_brightness / 100,
                m_rgb_green * m_rgb_brightness / 100,
                m_rgb_blue * m_rgb_brightness / 100);
        publishRGBColor();
    }
}

// Called to publish the state of the led (on/off)
void publishRGBState() {
    if (m_rgb_state) {
        mqtt.publish(String(config.mqtt_topic + MQTT_LIGHT_STATE_TOPIC).c_str(),
                0, true, LIGHT_ON);
    } else {
        mqtt.publish(String(config.mqtt_topic + MQTT_LIGHT_STATE_TOPIC).c_str(),
                0, true, LIGHT_OFF);
    }
}

// Called to publish the brightness of the led (0-100)
void publishRGBBrightness() {
    snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", m_rgb_brightness);
    mqtt.publish(String(config.mqtt_topic + MQTT_LIGHT_BRIGHTNESS_STATE_TOPIC).c_str(),
            0, true, m_msg_buffer);
}

// Called to publish the colors of the led (xx(x),xx(x),xx(x))
void publishRGBColor() {
    snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d,%d,%d", m_rgb_red, m_rgb_green, m_rgb_blue);
    mqtt.publish(String(config.mqtt_topic + MQTT_LIGHT_RGB_STATE_TOPIC).c_str(),
            0, true, m_msg_buffer);
}


/////////////////////////////////////////////////////////
// 
//  Web Section
//
/////////////////////////////////////////////////////////

// Configure and start the web server
void initWeb() {
    // Handle OTA update from asynchronous callbacks
    Update.runAsync(true);

    // Add header for SVG plot support?
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    // Setup WebSockets
    ws.onEvent(wsEvent);
    web.addHandler(&ws);

    // Heap status handler
    web.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    // JSON Config Handler
    web.on("/conf", HTTP_GET, [](AsyncWebServerRequest *request) {
        String jsonString;
        serializeConfig(jsonString, true);
        request->send(200, "text/json", jsonString);
    });

    // gamma debugging Config Handler
    web.on("/gamma", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        for (int i = 0; i < 256; i++) {
          response->printf ("%5d,", GAMMA_TABLE[i]);
          if (i % 16 == 15) {
            response->printf("\r\n");
          }
        }
        request->send(response);
    });

    // Firmware upload handler
    web.on("/updatefw", HTTP_POST, [](AsyncWebServerRequest *request) {
        ws.textAll("X6");
    }, handle_fw_upload);

    // Static Handler
    web.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
    //web.serveStatic("/config.json", SPIFFS, "/config.json");

    web.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Page not found");
    });

    web.begin();

    LOG_PORT.print(F("- Web Server started on port "));
    LOG_PORT.println(HTTP_PORT);
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

    // Set default MQTT port if missing
    if (config.mqtt_port == 0)
        config.mqtt_port = MQTT_PORT;

    // Generate default MQTT topic if needed
    if (!config.mqtt_topic.length()) {
        char chipId[7] = { 0 };
        snprintf(chipId, sizeof(chipId), "%06x", ESP.getChipId());
        config.mqtt_topic = "diy/esps/" + String(chipId);
    }

#if defined(ESPS_SUPPORT_PWM)
    config.devmode.MPWM = true;
#endif

#if defined(ESPS_MODE_PIXEL)
    // Set Mode
//    config.devmode = DevMode::MPIXEL;
    config.devmode.MPIXEL = true;
    config.devmode.MSERIAL = false;

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
#elif defined(ESPS_MODE_SERIAL)
    // Set Mode
//    config.devmode = DevMode::MSERIAL;
    config.devmode.MPIXEL = false;
    config.devmode.MSERIAL = true;

    // Generic serial channel limits
    if (config.channel_count > RENARD_LIMIT)
        config.channel_count = RENARD_LIMIT;
    else if (config.channel_count < 1)
        config.channel_count = 1;

    if (config.serial_type == SerialType::DMX512 && config.channel_count > UNIVERSE_MAX)
        config.channel_count = UNIVERSE_MAX;

    // Baud rate check
    if (config.baudrate > BaudRate::BR_460800)
        config.baudrate = BaudRate::BR_460800;
    else if (config.baudrate < BaudRate::BR_38400)
        config.baudrate = BaudRate::BR_57600;
#endif
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
#if defined(ESPS_MODE_PIXEL)
    pixels.begin(config.pixel_type, config.pixel_color, config.channel_count / 3);
    pixels.setGamma(config.gamma);
    updateGammaTable(config.gammaVal, config.briteVal);
    rgbEffect.begin(pixels.getData(), LED_COUNT);
#elif defined(ESPS_MODE_SERIAL)
    serial.begin(&SEROUT_PORT, config.serial_type, config.channel_count, config.baudrate);
#endif

    LOG_PORT.print(F("- Listening for "));
    LOG_PORT.print(config.channel_count);
    LOG_PORT.print(F(" channels, from Universe "));
    LOG_PORT.print(config.universe);
    LOG_PORT.print(F(" to "));
    LOG_PORT.println(uniLast);

    // Setup IGMP subscriptions if multicast is enabled
    if (config.multicast)
        multiSub();
}

// De-Serialize Network config
void dsNetworkConfig(JsonObject &json) {
    // Fallback to embedded ssid and passphrase if null in config
    config.ssid = json["network"]["ssid"].as<String>();
    if (!config.ssid.length())
        config.ssid = ssid;

    config.passphrase = json["network"]["passphrase"].as<String>();
    if (!config.passphrase.length())
        config.passphrase = passphrase;

    // Network
    for (int i = 0; i < 4; i++) {
        config.ip[i] = json["network"]["ip"][i];
        config.netmask[i] = json["network"]["netmask"][i];
        config.gateway[i] = json["network"]["gateway"][i];
    }
    config.dhcp = json["network"]["dhcp"];
    config.ap_fallback = json["network"]["ap_fallback"];

    // Generate default hostname if needed
    config.hostname = json["network"]["hostname"].as<String>();
    if (!config.hostname.length()) {
        char chipId[7] = { 0 };
        snprintf(chipId, sizeof(chipId), "%06x", ESP.getChipId());
        config.hostname = "esps-" + String(chipId);
    }
}

// De-serialize Device Config
void dsDeviceConfig(JsonObject &json) {
    // Device
    config.id = json["device"]["id"].as<String>();

    // E131
    config.universe = json["e131"]["universe"];
    config.universe_limit = json["e131"]["universe_limit"];
    config.channel_start = json["e131"]["channel_start"];
    config.channel_count = json["e131"]["channel_count"];
    config.multicast = json["e131"]["multicast"];

    // MQTT
    config.mqtt = json["mqtt"]["enabled"];
    config.mqtt_ip = json["mqtt"]["ip"].as<String>();
    config.mqtt_port = json["mqtt"]["port"];
    config.mqtt_user = json["mqtt"]["user"].as<String>();
    config.mqtt_password = json["mqtt"]["password"].as<String>();
    config.mqtt_topic = json["mqtt"]["topic"].as<String>();

#if defined(ESPS_MODE_PIXEL)
    /* Pixel */
    config.pixel_type = PixelType(static_cast<uint8_t>(json["pixel"]["type"]));
    config.pixel_color = PixelColor(static_cast<uint8_t>(json["pixel"]["color"]));
    config.gamma = json["pixel"]["gamma"];
    config.gammaVal = json["pixel"]["gammaVal"];
    config.briteVal = json["pixel"]["briteVal"];

#elif defined(ESPS_MODE_SERIAL)
    /* Serial */
    config.serial_type = SerialType(static_cast<uint8_t>(json["serial"]["type"]));
    config.baudrate = BaudRate(static_cast<uint32_t>(json["serial"]["baudrate"]));
#endif

#if defined(ESPS_SUPPORT_PWM)
    /* PWM */
    config.pwm_global_enabled = json["pwm"]["enabled"];
    config.pwm_freq = json["pwm"]["freq"];
    config.pwm_gamma = json["pwm"]["gamma"];
    config.pwm_gpio_invert = 0;
    config.pwm_gpio_digital = 0;
    config.pwm_gpio_enabled = 0;
    for (int gpio = 0; gpio < NUM_GPIO; gpio++) {
        if (valid_gpio_mask & 1<<gpio) {
            config.pwm_gpio_dmx[gpio] = json["pwm"]["gpio" + (String)gpio + "_channel"];
            if (json["pwm"]["gpio" + (String)gpio + "_invert"])
                config.pwm_gpio_invert |= 1<<gpio;
            if (json["pwm"]["gpio" + (String)gpio + "_digital"])
                config.pwm_gpio_digital |= 1<<gpio;
            if (json["pwm"]["gpio" + (String)gpio + "_enabled"])
                config.pwm_gpio_enabled |= 1<<gpio;
        }
    }
#endif
}

// Load configugration JSON file
void loadConfig() {
    // Zeroize Config struct
    memset(&config, 0, sizeof(config));

    // Load CONFIG_FILE json. Create and init with defaults if not found
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        LOG_PORT.println(F("- No configuration file found."));
        config.ssid = ssid;
        config.passphrase = passphrase;
        saveConfig();
    } else {
        // Parse CONFIG_FILE json
        size_t size = file.size();
        if (size > CONFIG_MAX_SIZE) {
            LOG_PORT.println(F("*** Configuration File too large ***"));
            return;
        }

        std::unique_ptr<char[]> buf(new char[size]);
        file.readBytes(buf.get(), size);

        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        if (!json.success()) {
            LOG_PORT.println(F("*** Configuration File Format Error ***"));
            return;
        }

        dsNetworkConfig(json);
        dsDeviceConfig(json);

        LOG_PORT.println(F("- Configuration loaded."));
    }

    // Override config
    config.id = "ESPixelPlug";
    config.testmode = TestMode::DISABLED;

    config.ssid = ssid;
    config.passphrase = passphrase;
    config.dhcp = true;

    config.universe = 0;
    config.universe_limit = UNIVERSE_MAX;
    config.channel_start = 1;
    config.channel_count = LED_COUNT * 3;
    config.multicast = true;

    config.pixel_type = PixelType::WS2811;
    config.pixel_color = PixelColor::GRB;
    config.gamma = true;
    config.gammaVal = 2.2f;
    config.briteVal = 1.0f;

    // Validate it
    validateConfig();
}

// Serialize the current config into a JSON string
void serializeConfig(String &jsonString, bool pretty, bool creds) {
    // Create buffer and root object
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();

    // Device
    JsonObject &device = json.createNestedObject("device");
    device["id"] = config.id.c_str();
    device["mode"] = config.devmode.toInt();

    // Network
    JsonObject &network = json.createNestedObject("network");
    network["ssid"] = config.ssid.c_str();
    if (creds)
        network["passphrase"] = config.passphrase.c_str();
    network["hostname"] = config.hostname.c_str();
    JsonArray &ip = network.createNestedArray("ip");
    JsonArray &netmask = network.createNestedArray("netmask");
    JsonArray &gateway = network.createNestedArray("gateway");
    for (int i = 0; i < 4; i++) {
        ip.add(config.ip[i]);
        netmask.add(config.netmask[i]);
        gateway.add(config.gateway[i]);
    }
    network["dhcp"] = config.dhcp;
    network["ap_fallback"] = config.ap_fallback;

    // MQTT
    JsonObject &_mqtt = json.createNestedObject("mqtt");
    _mqtt["enabled"] = config.mqtt;
    _mqtt["ip"] = config.mqtt_ip.c_str();
    _mqtt["port"] = config.mqtt_port;
    _mqtt["user"] = config.mqtt_user.c_str();
    _mqtt["password"] = config.mqtt_password.c_str();
    _mqtt["topic"] = config.mqtt_topic.c_str();

    // E131
    JsonObject &e131 = json.createNestedObject("e131");
    e131["universe"] = config.universe;
    e131["universe_limit"] = config.universe_limit;
    e131["channel_start"] = config.channel_start;
    e131["channel_count"] = config.channel_count;
    e131["multicast"] = config.multicast;

#if defined(ESPS_MODE_PIXEL)
    // Pixel
    JsonObject &pixel = json.createNestedObject("pixel");
    pixel["type"] = static_cast<uint8_t>(config.pixel_type);
    pixel["color"] = static_cast<uint8_t>(config.pixel_color);
    pixel["gamma"] = config.gamma;
    pixel["gammaVal"] = config.gammaVal;
    pixel["briteVal"] = config.briteVal;

#elif defined(ESPS_MODE_SERIAL)
    // Serial
    JsonObject &serial = json.createNestedObject("serial");
    serial["type"] = static_cast<uint8_t>(config.serial_type);
    serial["baudrate"] = static_cast<uint32_t>(config.baudrate);
#endif

#if defined(ESPS_SUPPORT_PWM)
    // PWM
    JsonObject &pwm = json.createNestedObject("pwm");
    pwm["enabled"] = config.pwm_global_enabled;
    pwm["freq"] = config.pwm_freq;
    pwm["gamma"] = config.pwm_gamma;
    
    for (int gpio = 0; gpio < NUM_GPIO; gpio++ ) {
        if (valid_gpio_mask & 1<<gpio) {
            pwm["gpio" + (String)gpio + "_channel"] = static_cast<uint16_t>(config.pwm_gpio_dmx[gpio]);
            pwm["gpio" + (String)gpio + "_enabled"] = static_cast<bool>(config.pwm_gpio_enabled & 1<<gpio);
            pwm["gpio" + (String)gpio + "_invert"] = static_cast<bool>(config.pwm_gpio_invert & 1<<gpio);
            pwm["gpio" + (String)gpio + "_digital"] = static_cast<bool>(config.pwm_gpio_digital & 1<<gpio);
        }
    }
#endif

    if (pretty)
        json.prettyPrintTo(jsonString);
    else
        json.printTo(jsonString);
}

// Save configuration JSON file
void saveConfig() {
    // Update Config
    updateConfig();

    // Serialize Config
    String jsonString;
    serializeConfig(jsonString, true, true);

    // Save Config
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        LOG_PORT.println(F("*** Error creating configuration file ***"));
        return;
    } else {
        file.println(jsonString);
        LOG_PORT.println(F("* Configuration saved."));
    }
}


/////////////////////////////////////////////////////////
//
//  Set routines for Testing and MQTT
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

    if (config.testmode == TestMode::DISABLED || config.testmode == TestMode::VIEW_STREAM) {
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
    } else {  // Other testmodes
        switch (config.testmode) {
            case TestMode::STATIC: {
                setStatic(testing.r, testing.g, testing.b);
                break;
            }

            case TestMode::CHASE:
                // Run chase routine
                if (millis() - testing.last > 100) {
                    // Rime for new step
                    testing.last = millis();
#if defined(ESPS_MODE_PIXEL)
                    // Clear whole string
                    for (int y =0; y < config.channel_count; y++)
                        pixels.setValue(y, 0);
                    // Set pixel at step
                    int ch_offset = testing.step*3;
                    pixels.setValue(ch_offset++, testing.r);
                    pixels.setValue(ch_offset++, testing.g);
                    pixels.setValue(ch_offset, testing.b);
                    testing.step++;
                    if (testing.step >= (config.channel_count/3))
                        testing.step = 0;
#elif defined(ESPS_MODE_SERIAL)
                    for (int y =0; y < config.channel_count; y++)
                        serial.setValue(y, 0);
                    // Set pixel at step
                    serial.setValue(testing.step++, 0xFF);
                    if (testing.step >= config.channel_count)
                        testing.step = 0;
#endif
                }
                break;

            case TestMode::RAINBOW:
                // Run rainbow routine
                if (millis() - testing.last > 50) {
                    testing.last = millis();
                    uint16_t i, WheelPos, num_pixels;
                    num_pixels = config.channel_count / 3;
                    if (testing.step > 255) {
                        testing.step=0;
                    }
                    for (i=0; i < (num_pixels); i++) {
                        int ch_offset = i*3;
                        WheelPos = 255 - (((i * 255 / num_pixels) + testing.step) & 255);
#if defined(ESPS_MODE_PIXEL)
                        if (WheelPos < 85) {
                            pixels.setValue(ch_offset++, 255 - WheelPos * 3);
                            pixels.setValue(ch_offset++, 0);
                            pixels.setValue(ch_offset, WheelPos * 3);
                        } else if (WheelPos < 170) {
                            WheelPos -= 85;
                            pixels.setValue(ch_offset++, 0);
                            pixels.setValue(ch_offset++, WheelPos * 3);
                            pixels.setValue(ch_offset, 255 - WheelPos * 3);
                        } else {
                            WheelPos -= 170;
                            pixels.setValue(ch_offset++, WheelPos * 3);
                            pixels.setValue(ch_offset++,255 - WheelPos * 3);
                            pixels.setValue(ch_offset, 0);
                        }
#elif defined(ESPS_MODE_SERIAL)
                        if (WheelPos < 85) {
                            serial.setValue(ch_offset++, 255 - WheelPos * 3);
                            serial.setValue(ch_offset++, 0);
                            serial.setValue(ch_offset, WheelPos * 3);
                        } else if (WheelPos < 170) {
                            WheelPos -= 85;
                            serial.setValue(ch_offset++, 0);
                            serial.setValue(ch_offset++, WheelPos * 3);
                            serial.setValue(ch_offset, 255 - WheelPos * 3);
                        } else {
                            WheelPos -= 170;
                            serial.setValue(ch_offset++, WheelPos * 3);
                            serial.setValue(ch_offset++,255 - WheelPos * 3);
                            serial.setValue(ch_offset, 0);
                        }
#endif
                    }

                    testing.step++;
                }
                break;
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

/* Update the PWM outputs */
#if defined(ESPS_SUPPORT_PWM)
  handlePWM();
#endif
}
