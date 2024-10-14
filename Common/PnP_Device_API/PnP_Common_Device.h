/**
 * \file
 * \brief API implementation for the device of the WE IoT design kit.
 *
 * \copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * \page License
 *
 * THE SOFTWARE INCLUDING THE SOURCE CODE IS PROVIDED “AS IS”. YOU ACKNOWLEDGE THAT WÜRTH ELEKTRONIK
 * EISOS MAKES NO REPRESENTATIONS AND WARRANTIES OF ANY KIND RELATED TO, BUT NOT LIMITED
 * TO THE NON-INFRINGEMENT OF THIRD PARTIES’ INTELLECTUAL PROPERTY RIGHTS OR THE
 * MERCHANTABILITY OR FITNESS FOR YOUR INTENDED PURPOSE OR USAGE. WÜRTH ELEKTRONIK EISOS DOES NOT
 * WARRANT OR REPRESENT THAT ANY LICENSE, EITHER EXPRESS OR IMPLIED, IS GRANTED UNDER ANY PATENT
 * RIGHT, COPYRIGHT, MASK WORK RIGHT, OR OTHER INTELLECTUAL PROPERTY RIGHT RELATING TO ANY
 * COMBINATION, MACHINE, OR PROCESS IN WHICH THE PRODUCT IS USED. INFORMATION PUBLISHED BY
 * WÜRTH ELEKTRONIK EISOS REGARDING THIRD-PARTY PRODUCTS OR SERVICES DOES NOT CONSTITUTE A LICENSE
 * FROM WÜRTH ELEKTRONIK EISOS TO USE SUCH PRODUCTS OR SERVICES OR A WARRANTY OR ENDORSEMENT
 * THEREOF
 *
 * THIS SOURCE CODE IS PROTECTED BY A LICENSE.
 * FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED
 * IN THE ROOT DIRECTORY OF THIS PACKAGE
 */

#ifndef P_N_P_COMMON_DEVICE_H
#define P_N_P_COMMON__DEVICE_H

/**         Includes         */

#include <stdint.h>
#include "calypsoBoard.h"
#include "ConfigPlatform.h"
#include "sensorBoard.h"

/**         Functions definition         */

#ifdef __cplusplus
extern "C"
{
#endif

/*File path to files stored on Calypso internal storage*/
#define INDEX_HTML_SRC_FILE_PATH "/www/index_src.html"
#define INDEX_HTML_FILE_PATH "/www/index.html"
#define PLATFORM_HTML_FILE_PATH "/www/platform.html"
#define PLATFORM_JS_FILE_PATH "/www/js/platform.js"
#define PLATFORM_CONFIG_FILE_PATH "user/platform.json"

#define INDEX_HTML_SRC_FILE "<!doctypehtml><meta content=\"text/html; charset=utf-8\"http-equiv=Content-Type><meta content=\"width=device-width,initial-scale=1\"name=viewport><title>Wuerth Elektronik eiSos Calypso</title><link href=css/bootstrap.min.css rel=stylesheet><link href=css/style.css rel=stylesheet><script src=js/jquery-3.6.0.min.js></script><script src=js/general.js></script><div class=wrapper><div class=header id=main><div class=headerIn onclick=openNav()><div class=row><div class=\"col-md-3 menuTop\"></div><div class=col-md-9><h1>Calypso WLAN module</h1></div></div></div></div><div class=sidebar id=mySidebar onmouseleave=closeNav()><a href=javascript:void(0); onclick=closeNav() class=closebtn>×</a> <a href=javascript:void(0); onclick='openNavElement(\"baseFrame\")'>Home</a> <a href=javascript:void(0); onclick='openNavElement(\"settingsFrame\")'>Settings</a> <a href=javascript:void(0); onclick='openNavElement(\"otaFrame\")'>OTA</a> <a href=javascript:void(0); onclick='openNavElement(\"gpioFrame\")'>GPIO</a> <a href=javascript:void(0); onclick='openNavElement(\"userFrame\")'>User settings</a> <a href=javascript:void(0); onclick='openNavElement(\"customFrame\")'>Custom</a> <a href=javascript:void(0); onclick='openNavElement(\"fileFrame\")'>File upload</a> <a href=javascript:void(0); onclick='openNavElement(\"azureFrame\")'>Azure</a> <a href=javascript:void(0); onclick='openNavElement(\"kaaiotFrame\")'>KaaIoT</a> <a href=javascript:void(0); onclick='openNavElement(\"helpFrame\")'>About</a></div><div class=contentSplash><div class=frame id=baseFrame style=display:block><iframe data-src=base.html id=ibaseFrame src=base.html></iframe></div><div class=frame id=otaFrame><iframe data-src=ota.html id=iotaFrame></iframe></div><div class=frame id=settingsFrame><iframe data-src=settings.html id=isettingsFrame></iframe></div><div class=frame id=gpioFrame><iframe data-src=gpio.html id=igpioFrame></iframe></div><div class=frame id=userFrame><iframe data-src=usersettings.html id=iuserFrame></iframe></div><div class=frame id=customFrame><iframe data-src=custom.html id=icustomFrame></iframe></div><div class=frame id=fileFrame><iframe data-src=file.html id=ifileFrame></iframe></div><div class=frame id=azureFrame><iframe data-src=azure.html id=iazureFrame></iframe></div><div class=frame id=kaaiotFrame><iframe data-src=kaaiot.html id=ikaaiotFrame></iframe></div><div class=frame id=helpFrame><iframe data-src=help.html id=ihelpFrame></iframe></div><br></div></div>"

#define INDEX_HTML_FILE "<!DOCTYPE HTML>\
<html>\
\
<head>\
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
    <title>Wuerth Elektronik eiSos Calypso</title>\
    <link rel=\"stylesheet\" type=\"text/css\" href=\"css/bootstrap.min.css\">\
    <link rel=\"stylesheet\" type=\"text/css\" href=\"css/style.css\">\
    <script type=\"text/javascript\" src=\"js/jquery-3.6.0.min.js\"></script>\
    <script type=\"text/javascript\" src=\"js/general.js\"></script>\
</head>\
\
<body>\
    <div class=\"wrapper\">\
        <div class=\"header\" id=\"main\">\
            <div class=\"headerIn\" onclick=\"openNav()\">\
                <div class=\"row\">\
                    <div class=\"col-md-3 menuTop\"></div>\
                    <div class=\"col-md-9\">\
                        <h1>Calypso WLAN module</h1>\
                    </div>\
                </div>\
            </div>\
        </div>\
\
        <div id=\"mySidebar\" class=\"sidebar\" onmouseleave=\"closeNav()\">\
\
            <a href=\"javascript:void(0);\" class=\"closebtn\" onclick=\"closeNav()\">×</a>\
            <a href=\"javascript:void(0);\" onclick=\"openNavElement('baseFrame')\">Home</a>\
            <a href=\"javascript:void(0);\" onclick=\"openNavElement('settingsFrame')\">Settings</a>\
            <a href=\"javascript:void(0);\" onclick=\"openNavElement('otaFrame')\">OTA</a>\
            <a href=\"javascript:void(0);\" onclick=\"openNavElement('gpioFrame')\">GPIO</a>\
            <a href=\"javascript:void(0);\" onclick=\"openNavElement('userFrame')\">User settings</a>\
            <a href=\"javascript:void(0);\" onclick=\"openNavElement('customFrame')\">Custom</a>\
            <a href=\"javascript:void(0);\" onclick=\"openNavElement('fileFrame')\">File upload</a>\
            <a href=\"javascript:void(0);\" onclick=\"openNavElement('azureFrame')\">Azure</a>\
            <a href=\"javascript:void(0);\" onclick=\"openNavElement('kaaiotFrame')\">KaaIoT</a>\
            <a href=\"javascript:void(0);\" onclick=\"openNavElement('helpFrame')\">About</a>\
          </div>\
\
        <div class=\"contentSplash\">\
            <div class=\"frame\" id=\"baseFrame\" style=\"display: block;\">\
                <iframe id=\"ibaseFrame\" src=\"base.html\" data-src=\"base.html\"></iframe>\
            </div>\
            <div class=\"frame\" id=\"otaFrame\">\
                <iframe id=\"iotaFrame\" data-src=\"ota.html\"></iframe>\
            </div>\
            <div class=\"frame\" id=\"settingsFrame\">\
                <iframe id=\"isettingsFrame\" data-src=\"settings.html\"></iframe>\
            </div>\
            <div class=\"frame\" id=\"gpioFrame\">\
                <iframe id=\"igpioFrame\" data-src=\"gpio.html\"></iframe>\
            </div>\
            <div class=\"frame\" id=\"userFrame\">\
                <iframe id=\"iuserFrame\" data-src=\"usersettings.html\"></iframe>\
            </div>\
            <div class=\"frame\" id=\"customFrame\">\
                <iframe id=\"icustomFrame\" data-src=\"custom.html\"></iframe>\
            </div>\
            <div class=\"frame\" id=\"fileFrame\">\
                <iframe id=\"ifileFrame\" data-src=\"file.html\"></iframe>\
            </div>\
            <div class=\"frame\" id=\"azureFrame\">\
                <iframe id=\"iazureFrame\" data-src=\"azure.html\"></iframe>\
            </div>\
            <div class=\"frame\" id=\"kaaiotFrame\">\
                <iframe id=\"ikaaiotFrame\" data-src=\"kaaiot.html\"></iframe>\
            </div>\
            <div class=\"frame\" id=\"helpFrame\">\
                <iframe id=\"ihelpFrame\" data-src=\"help.html\"></iframe>\
            </div>\
            <br>\
        </div>\
    </div>\
</body>\
</html>"

#define LED_INDICATION_SHORT_DELAY delay(1000)
#define LED_INDICATION_LONG_DELAY delay(5000)

#define BUTTON_A (byte)9
#define BUTTON_B (byte)6
// Button labelled C on the OLED display
#define BUTTON_C (byte)5

#define DEVICE_CREDENTIALS_MAX_LEN 48

    typedef enum
    {
        AZURE,
        KAAIOT
    } IoT_platforms_t;

    extern const char *configuration;

    IoT_platforms_t getPlatform();

    TypeSerial *Device_init(void *Debug, void *CalypsoSerial);
    void deadLoop();
    void Device_deletePreviousConfigIfExist();
    bool Device_writeConfigFile(const char *path, const char *data);
    void Device_writeConfigFiles();
    bool Device_loadPlatformId();
    bool Device_isIotPlatformConfigured();
    bool Device_isConfigured();
    bool Device_isConnectedToWiFi();
    void Device_MQTTConnect();
    void Device_readSensors();
    void Device_PublishSensorData();
    void Device_connect_WiFi();
    void Device_disconnect_WiFi();
    void Device_WiFi_provisioning();
    void Device_configurationInProgress();
    bool Device_SubscribeToTopics();
    void Device_reset();
    void Device_restart();
    bool Device_isStatusOK();
    void Device_processCloudMessage();
    bool Device_ConfigurationComplete();
    void Device_displaySensorData();
    bool Device_isUpToDate();
    unsigned long Device_getTelemetrySendInterval();
    bool Device_isSensorsPresent();
    void Device_displayMessageWithDelay(const char *message);
#ifdef __cplusplus
}
#endif

#endif /* P_N_P_COMMON_DEVICE_H */