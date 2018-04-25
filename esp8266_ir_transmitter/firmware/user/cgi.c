/*
Some random cgi routines. Used in the LED example and the page that returns the entire
flash as a binary. Also handles the hit counter on the main page.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <esp8266.h>
#include "cgi.h"
#include "ac.h"


static ACSettings acSettings;

int ICACHE_FLASH_ATTR cgiAC(HttpdConnData *connData) {
    int len;
    char buff[1024];

    if (connData->conn == NULL) {
        // Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    // ON/OFF toggle
    len = httpdFindArg(connData->post->buff, "onoff", buff, sizeof(buff));
    if (len != 0) {
        acSettings.onOff = true;
    } else {
        acSettings.onOff = false;
    }

    // Temperature setpoint
    len = httpdFindArg(connData->post->buff, "temp", buff, sizeof(buff));
    if (len != 0) {
        uint8_t temp = atoi(buff);
        if (temp >= 16 && temp <= 31) {
            acSettings.temp = temp;
        }
    }

    // Fan speed
    len = httpdFindArg(connData->post->buff, "fan", buff, sizeof(buff));
    if (len != 0) {
        acSettings.fan = AUTO; //!< default fan speed
        if (os_strcmp(buff, "MAX")) {
            acSettings.fan = MAX;
        } else if (os_strcmp(buff, "MED")) {
            acSettings.fan = MED;
        } else if (os_strcmp(buff, "MIN")) {
            acSettings.fan = MIN;
        }
    }

    // A/C mode
    len = httpdFindArg(connData->post->buff, "mode", buff, sizeof(buff));
    if (len != 0) {
        acSettings.mode = COOL; //!< default mode
        if (os_strcmp(buff, "SUN")) {
            acSettings.mode = SUN;
        } else if (os_strcmp(buff, "FAN")) {
            acSettings.mode = FAN;
        } else if (os_strcmp(buff, "COOL")) {
            acSettings.mode = COOL;
        } else if (os_strcmp(buff, "SMART")) {
            acSettings.fan = SMART;
        } else if (os_strcmp(buff, "DROPS")) {
            acSettings.fan = DROPS;
        }
    }

    // Sleep toggle
    len = httpdFindArg(connData->post->buff, "sleep", buff, sizeof(buff));
    if (len != 0) {
        acSettings.sleep = true;
    } else {
        acSettings.sleep = false;
    }

    send(acSettings);

    httpdRedirect(connData, "ac.tpl");
    return HTTPD_CGI_DONE;
}

//Template code for the AC page.
int ICACHE_FLASH_ATTR tplAC(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return HTTPD_CGI_DONE;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "ledstate")==0) {
//		if (currLedState) {
//			os_strcpy(buff, "on");
//		} else {
//			os_strcpy(buff, "off");
//		}
	}
	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}

static long hitCounter=0;

//Template code for the counter on the index page.
int ICACHE_FLASH_ATTR tplCounter(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return HTTPD_CGI_DONE;

	if (os_strcmp(token, "counter")==0) {
		hitCounter++;
		os_sprintf(buff, "%ld", hitCounter);
	}
	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
