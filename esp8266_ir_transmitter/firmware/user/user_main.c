/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

/*
This is example code for the esphttpd library. It's a small-ish demo showing off 
the server, including WiFi connection management capabilities, some IO and
some pictures of cats.
*/

#include <esp8266.h>
#include "httpd.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "cgiflash.h"
#include "stdout.h"
#include "auth.h"
#include "espfs.h"
#include "captdns.h"
#include "webpages-espfs.h"
#include "cgi-test.h"
#include "espmissingincludes.h"

#include "ac.h"
#include "mqtt.h"


// TODO move applicable definitions to Makefile
#define MQTT_HOST "10.0.0.3"
#define MQTT_PORT 1883
#define DEFAULT_SECURITY 0
#define MQTT_CLIENT_ID "AC_01"
#define MQTT_USER "USER"
#define MQTT_PASS "PASS"
#define MQTT_KEEPALIVE 120
#define MQTT_CLEAN_SESSION 1

static ETSTimer mqttTimer;
static MQTT_Client mqttClient;


//Function that tells the authentication system what users/passwords live on the system.
//This is disabled in the default build; if you want to try it, enable the authBasic line in
//the builtInUrls below.
int myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen) {
	if (no==0) {
		os_strcpy(user, "admin");
		os_strcpy(pass, "s3cr3t");
		return 1;
//Add more users this way. Check against incrementing no for each user added.
//	} else if (no==1) {
//		os_strcpy(user, "user1");
//		os_strcpy(pass, "something");
//		return 1;
	}
	return 0;
}


#ifdef ESPFS_POS
CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_ESPFS,
	.fw1Pos=ESPFS_POS,
	.fw2Pos=0,
	.fwSize=ESPFS_SIZE,
};
#define INCLUDE_FLASH_FNS
#endif
#ifdef OTA_FLASH_SIZE_K
CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_FW,
	.fw1Pos=0x1000,
	.fw2Pos=((OTA_FLASH_SIZE_K*1024)/2)+0x1000,
	.fwSize=((OTA_FLASH_SIZE_K*1024)/2)-0x1000,
	.tagName=OTA_TAGNAME
};
#define INCLUDE_FLASH_FNS
#endif

/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
HttpdBuiltInUrl builtInUrls[]={
	{"*", cgiRedirectApClientToHostname, "esp8266.nonet"},
	{"/", cgiRedirect, "/index.tpl"},
	{"/ac.tpl", cgiEspFsTemplate, tplAC},
	{"/index.tpl", cgiEspFsTemplate, tplCounter},
	{"/ac.cgi", cgiAC, NULL},
//	{"/flash/*", authBasic, myPassFn},
#ifdef INCLUDE_FLASH_FNS
	{"/flash/next", cgiGetFirmwareNext, &uploadParams},
	{"/flash/upload", cgiUploadFirmware, &uploadParams},
#endif
	{"/flash/reboot", cgiRebootFirmware, NULL},

	//Routines to make the /wifi URL and everything beneath it work.

//Enable the line below to protect the WiFi configuration with an username/password combo.
//	{"/wifi/*", authBasic, myPassFn},

	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
	{"/wifi/connect.cgi", cgiWiFiConnect, NULL},
	{"/wifi/connstatus.cgi", cgiWiFiConnStatus, NULL},
	{"/wifi/setmode.cgi", cgiWiFiSetMode, NULL},

	{"/test", cgiRedirect, "/test/index.html"},
	{"/test/", cgiRedirect, "/test/index.html"},
	{"/test/test.cgi", cgiTestbed, NULL},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};


static void ICACHE_FLASH_ATTR sendData(void *arg)
{
    const uint8_t bufferLength = 16;
    char buffer[bufferLength];
    int len, integer, decimal;
    ACStatus status = get();

    // Send temperature in degrees C
    integer = status.temperatureIn;
    decimal = (status.temperatureIn - integer) * 10;
    len = sprintf(buffer, "%d.%d", integer, decimal);
    MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/tempIN", buffer, len, 0, 0);

    integer = status.temperatureOut;
    decimal = (status.temperatureOut - integer) * 10;
    len = sprintf(buffer, "%d.%d", integer, decimal);
    MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/tempOUT", buffer, len, 0, 0);

    len = sprintf(buffer, "%s", status.started ? "ON" : "OFF");
    MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/power", buffer, len, 0, 0);

    // Do not send fan speed and mode until settings are stored in FLASH not to be reset on reboot
    /*
    len = sprintf(buffer, "%d", status.settings.temp);
    MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/tempSetpoint", buffer, len, 0, 0);

    switch (status.settings.fan) {
        case AUTO:
            MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/fan", "AUTO", 4, 0, 0);
            break;
        case MAX:
            MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/fan", "MAX", 3, 0, 0);
            break;
        case MED:
            MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/fan", "MED", 3, 0, 0);
            break;
        case MIN:
            MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/fan", "MIN", 3, 0, 0);
            break;
    }

    switch (status.settings.mode) {
        case SUN:
            MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/mode", "SUN", 3, 0, 0);
            break;
        case FAN:
            MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/mode", "FAN", 3, 0, 0);
            break;
        case COOL:
            MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/mode", "COOL", 4, 0, 0);
            break;
        case SMART:
            MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/mode", "SMART", 5, 0, 0);
            break;
        case DROPS:
            MQTT_Publish(&mqttClient, "/" MQTT_CLIENT_ID "/state/mode", "DROPS", 5, 0, 0);
            break;
    }
    */
}

static void ICACHE_FLASH_ATTR wifiConnectCb(uint8_t status)
{
  if (status == STATION_GOT_IP) {
    MQTT_Connect(&mqttClient);
  } else {
    MQTT_Disconnect(&mqttClient);
  }
}

static void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args)
{
  MQTT_Client* client = (MQTT_Client*)args;
  os_printf("MQTT: Connected\r\n");
  MQTT_Subscribe(client, "/" MQTT_CLIENT_ID "/command/power", 0);
  MQTT_Subscribe(client, "/" MQTT_CLIENT_ID "/command/temperature", 0);
  MQTT_Subscribe(client, "/" MQTT_CLIENT_ID "/command/fan", 0);
  MQTT_Subscribe(client, "/" MQTT_CLIENT_ID "/command/mode", 0);
  MQTT_Publish(client, "/" MQTT_CLIENT_ID "/status", "connected", 9, 0, 0);
  sendData(NULL);
}

static void ICACHE_FLASH_ATTR mqttDisconnectedCb(uint32_t *args)
{
  MQTT_Client* client = (MQTT_Client*)args;
  os_printf("MQTT: Disconnected\r\n");
}

static void ICACHE_FLASH_ATTR mqttPublishedCb(uint32_t *args)
{
  MQTT_Client* client = (MQTT_Client*)args;
  os_printf("MQTT: Published\r\n");
}


static void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
    char *topicBuf = (char*)os_zalloc(topic_len + 1),
          *dataBuf = (char*)os_zalloc(data_len + 1);

    os_memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;
    os_memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;
    os_printf("Received topic: %s, data: %s \r\n", topicBuf, dataBuf);
    if (os_strcmp(topicBuf, "/" MQTT_CLIENT_ID "/command/power") == 0)
    {
        if (os_strcmp(dataBuf, "ON") == 0)
            start();
        else if (os_strcmp(dataBuf, "OFF") == 0)
            stop();
    }
    else if (os_strcmp(topicBuf, "/" MQTT_CLIENT_ID "/command/temperature") == 0)
    {
        setTemperature(atoi(dataBuf));
    }
    else if (os_strcmp(topicBuf, "/" MQTT_CLIENT_ID "/command/fan") == 0)
    {
        if (os_strcmp(dataBuf, "AUTO") == 0)
            setFanSpeed(AUTO);
        else if (os_strcmp(dataBuf, "MAX") == 0)
            setFanSpeed(MAX);
        else if (os_strcmp(dataBuf, "MED") == 0)
            setFanSpeed(MED);
        else if (os_strcmp(dataBuf, "MIN") == 0)
            setFanSpeed(MIN);
    }
    else if (os_strcmp(topicBuf, "/" MQTT_CLIENT_ID "/command/mode") == 0)
    {
        if (os_strcmp(dataBuf, "SUN") == 0)
            setMode(SUN);
        else if (os_strcmp(dataBuf, "COOL") == 0)
            setMode(COOL);
        else if (os_strcmp(dataBuf, "FAN") == 0)
            setMode(FAN);
        else if (os_strcmp(dataBuf, "SMART") == 0)
            setMode(SMART);
        else if (os_strcmp(dataBuf, "DROPS") == 0)
            setMode(DROPS);
    }

    os_free(topicBuf);
    os_free(dataBuf);
}


static ETSTimer WiFiLinker;
typedef void (*WifiCallback)(uint8_t);
WifiCallback wifiCb = wifiConnectCb;
static uint8_t wifiStatus = STATION_IDLE, lastWifiStatus = STATION_IDLE;
static void ICACHE_FLASH_ATTR wifi_check_ip(void *arg)
{
  struct ip_info ipConfig;
  os_timer_disarm(&WiFiLinker);
  wifi_get_ip_info(STATION_IF, &ipConfig);
  wifiStatus = wifi_station_get_connect_status();
  if (wifiStatus == STATION_GOT_IP && ipConfig.ip.addr != 0)
  {
    os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
    os_timer_arm(&WiFiLinker, 2000, 0);
  }
  else
  {
    if (wifi_station_get_connect_status() == STATION_WRONG_PASSWORD)
    {
      os_printf("STATION_WRONG_PASSWORD\r\n");
      wifi_station_connect();
    }
    else if (wifi_station_get_connect_status() == STATION_NO_AP_FOUND)
    {
      os_printf("STATION_NO_AP_FOUND\r\n");
      wifi_station_connect();
    }
    else if (wifi_station_get_connect_status() == STATION_CONNECT_FAIL)
    {
      os_printf("STATION_CONNECT_FAIL\r\n");
      wifi_station_connect();
    }
    else
    {
      //os_printf("STATION_IDLE\r\n");
    }

    os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
    os_timer_arm(&WiFiLinker, 500, 0);
  }
  if (wifiStatus != lastWifiStatus) {
    lastWifiStatus = wifiStatus;
    if (wifiCb)
      wifiCb(wifiStatus);
  }
}

static void ICACHE_FLASH_ATTR mqttInit(void)
{
  MQTT_InitConnection(&mqttClient, (uint8_t *) MQTT_HOST, MQTT_PORT, DEFAULT_SECURITY);

  if ( !MQTT_InitClient(&mqttClient,
              (uint8_t *) MQTT_CLIENT_ID,
              (uint8_t *) MQTT_USER,
              (uint8_t *) MQTT_PASS,
              MQTT_KEEPALIVE,
              MQTT_CLEAN_SESSION)
     )
  {
    os_printf("Failed to initialize properly. Check MQTT version.\n");
    return;
  }
  MQTT_InitLWT(&mqttClient, (uint8_t *) ("/" MQTT_CLIENT_ID "/status"), (uint8_t *) "offline", 0, 0);
  MQTT_OnConnected(&mqttClient, mqttConnectedCb);
  MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
  MQTT_OnPublished(&mqttClient, mqttPublishedCb);
  MQTT_OnData(&mqttClient, mqttDataCb);

  os_timer_disarm(&WiFiLinker);
  os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
  os_timer_arm(&WiFiLinker, 1000, 0);
}


//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void user_init(void) {
	stdoutInit();
    acInit();
	captdnsInit();

	// 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
	// where image is written in flash that is defined in Makefile.
#ifdef ESPFS_POS
	espFsInit((void*)(0x40200000 + ESPFS_POS));
#else
	espFsInit((void*)(webpages_espfs_start));
#endif
	httpdInit(builtInUrls, 80);
    mqttInit();

    // register repeated task 'sendData'
    os_timer_disarm(&mqttTimer);
    os_timer_setfn(&mqttTimer, (os_timer_func_t *)sendData, NULL);
    os_timer_arm(&mqttTimer, 20000, true);

	os_printf("\nReady\n");
}

void user_rf_pre_init() {
	//Not needed, but some SDK versions want this defined.
}

