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

#include <string.h>

#include "json-builder.h"
#include "PnP_Common_Device.h"
#include "PnP_Device_Azure.h"
#include "PnP_Device_KaaIoT.h"

IoT_platforms_t platform = KAAIOT;

// Serial Ports
//------Debug
TypeSerial *SerialDebug;
//------Calypso
TypeHardwareSerial *SerialCalypso;

// Radio Module
//------Calpyso
CALYPSO *calypso;

// Sensors
//------WSEN_PADS
PADS *sensorPADS;
//------WSEN_ITDS
ITDS *sensorITDS;
//------WSEN_TIDS
TIDS *sensorTIDS;
//------WSEN_HIDS
HIDS *sensorHIDS;

bool sensorsPresent = false;
bool deviceProvisioned = false;
bool deviceConfigured = false;

volatile unsigned long telemetrySendInterval = (unsigned long)(DEFAULT_TELEMETRY_SEND_INTEVAL * 1000);

uint8_t packetLost = 0;

char kitID[DEVICE_CREDENTIALS_MAX_LEN];
char modelID[DEVICE_CREDENTIALS_MAX_LEN];
uint16_t kitIDLength = 0;
uint8_t reqID = 0;
float lastBattVolt = 0;

char pubtopic[128];

extern char displayText[];

IoT_platforms_t getPlatform()
{
    return platform;
}

void deadLoop()
{
    while (true)
    {
        delay(1000);
    }
}

TypeSerial *Device_init(void *Debug, void *CalypsoSerial)
{
    CalypsoSettings calypsoParams;

    memset(&calypsoParams.wifiSettings, 0, sizeof(calypsoParams.wifiSettings));
    memset(&calypsoParams.mqttSettings, 0, sizeof(calypsoParams.mqttSettings));
    memset(&calypsoParams.sntpSettings, 0, sizeof(calypsoParams.sntpSettings));
    SerialDebug = SSerial_create(Debug);

    SerialCalypso = HSerial_create(CalypsoSerial);

    SSerial_begin(SerialDebug, 115200);

    HSerial_beginP(SerialCalypso, 921600,
                   (uint8_t)((0x10ul) | (0x1ul) | (0x400ul)));

    calypso = Calypso_Create(SerialDebug, SerialCalypso, &calypsoParams);

    sensorPADS = PADSCreate(SerialDebug);
    sensorITDS = ITDSCreate(SerialDebug);
    sensorTIDS = TIDSCreate(SerialDebug);
    sensorHIDS = HIDSCreate(SerialDebug);

    SSerial_printf(SerialDebug, "Starting the application...\r\n");

    if (!Calypso_simpleInit(calypso))
    {
        SSerial_printf(SerialDebug, "Calypso init failed \r\n");
        sprintf(displayText, "Calypso Init Failed...");
        SH1107_Display(1, 0, 24, displayText);
    }

    if (!PADS_simpleInit(sensorPADS))
    {
        SSerial_printf(SerialDebug, "PADS init failed \r\n");
        sprintf(displayText, "PADS Init Failed...");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sensorsPresent = true;
    }

    if (!ITDS_simpleInit(sensorITDS))
    {
        SSerial_printf(SerialDebug, "ITDS init failed \r\n");
        sprintf(displayText, "ITDS Init Failed...");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sensorsPresent = true;
    }

    if (!TIDS_simpleInit(sensorTIDS))
    {
        SSerial_printf(SerialDebug, "TIDS init failed \r\n");
        sprintf(displayText, "TIDS Init Failed...");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sensorsPresent = true;
    }

    if (!HIDS_simpleInit(sensorHIDS))
    {
        SSerial_printf(SerialDebug, "HIDS init failed \r\n");
        sprintf(displayText, "HIDS Init Failed...");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sensorsPresent = true;
    }
    packetLost = 0;

    Device_loadPlatformId();

    if (platform == KAAIOT)
    {
        return Kaaiot_Device_init(Debug, CalypsoSerial);
    }
    if (platform == AZURE)
    {
        return Azure_Device_init(Debug, CalypsoSerial);
    }
    SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    return NULL;
}

bool Device_ConfigurationComplete()
{
    if (platform == KAAIOT)
    {
        return Kaaiot_Device_ConfigurationComplete();
    }
    else if (platform == AZURE)
    {
        return Azure_Device_ConfigurationComplete();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
        return NULL;
    }
}

void Device_deletePreviousConfigIfExist()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_deletePreviousConfig();
    }
}

bool Device_writeConfigFile(const char *path, const char *data)
{
    if (!Calypso_writeFile(calypso, path, data, strlen(data)))
    {
        SSerial_printf(SerialDebug, "Unable to write file: %s\r\n", path);
        return false;
    }
    return true;
}

void Device_writeConfigFiles()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_writeConfigFiles();
    }
    else if (platform == AZURE)
    {
        Azure_Device_writeConfigFiles();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

void Device_restart()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_restart();
    }
    else if (platform == AZURE)
    {
        Azure_Device_restart();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

bool Device_isStatusOK()
{
    if (platform == KAAIOT)
    {
        return Kaaiot_Device_isStatusOK();
    }
    else if (platform == AZURE)
    {
        return Azure_Device_isStatusOK();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
        return NULL;
    }
}

bool Device_isConnectedToWiFi()
{
    if (platform == KAAIOT)
    {
        return Kaaiot_Device_isConnectedToWiFi();
    }
    else if (platform == AZURE)
    {
        return Azure_Device_isConnectedToWiFi();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
        return NULL;
    }
}

bool Device_isUpToDate()
{
    if (platform == KAAIOT)
    {
        return Kaaiot_Device_isUpToDate();
    }
    else if (platform == AZURE)
    {
        return Azure_Device_isUpToDate();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
        return NULL;
    }
}

static int getFileLength(char *path)
{
    char buffer[100];

    if (Calypso_fileExists(calypso, path))
    {
        // parse file length
        if (0 == strncmp(calypso->bufferCalypso.data, "+filegetinfo:", 13))
        {
            strncpy(buffer, calypso->bufferCalypso.data, sizeof(buffer));

            // Use strtok to split the string by commas
            char *token = strtok(buffer, ",");

            int tokenIndex = 0, fileSize = 0;

            // Iterate through tokens
            while (token != NULL)
            {
                tokenIndex++;
                // The second token is the file size
                if (tokenIndex == 2)
                {
                    fileSize = atoi(token); // Convert the token (which is a string) to an integer
                    break;
                }
                token = strtok(NULL, ",");
            }

            return fileSize;
        }
    }
    return -1;
}

bool Device_loadPlatformId()
{
    bool ret = true;
    char configBuf[256];
    uint16_t len;

    // check if platform.json exist
    if (!Calypso_fileExists(calypso, PLATFORM_CONFIG_FILE_PATH))
    {
        SSerial_printf(SerialDebug, "%s file not exist\r\n", PLATFORM_CONFIG_FILE_PATH);
        if (Device_writeConfigFile(PLATFORM_CONFIG_FILE_PATH, "{\"platform\":\"KAAIOT\"}"))
        {
            SSerial_printf(SerialDebug, "Platform default is KAAIOT\r\n");
            platform = KAAIOT;
            sprintf(displayText, "Error! File not exist\r\n%s\r\n\r\nPlatform not\r\nconfigured.\r\n\r\nLoad default\r\nplatform: KAAIOT", PLATFORM_CONFIG_FILE_PATH);
        }
        else
        {
            SSerial_printf(SerialDebug, "Can't write file: %s\r\n", PLATFORM_CONFIG_FILE_PATH);
            sprintf(displayText, "Error!!!\r\n\r\nCan't write file\r\n\r\n%s", PLATFORM_CONFIG_FILE_PATH);
            ret = false;
        }
        Device_displayMessageWithDelay(displayText);
    }
    else
    {
        if (Calypso_readFile(calypso, PLATFORM_CONFIG_FILE_PATH, (char *)configBuf, 256, &len))
        {
            json_value *platformConfig = json_parse(configBuf, len);
            if (platformConfig == NULL)
            {
                SSerial_printf(SerialDebug, "Unable to parse config file %s\r\n", PLATFORM_CONFIG_FILE_PATH);
                sprintf(displayText, "Error!!!\r\n\r\nCan't parse file\r\n\r\n%s", PLATFORM_CONFIG_FILE_PATH);
                Device_displayMessageWithDelay(displayText);
                return false;
            }
            if (strcmp("KAAIOT", platformConfig->u.object.values[0].value->u.string.ptr) == 0)
            {
                platform = KAAIOT;
                sprintf(displayText, "Selected IoT platform\r\n\r\n       KAAIOT");
                Device_displayMessageWithDelay(displayText);
            }
            else if (strcmp("AZURE", platformConfig->u.object.values[0].value->u.string.ptr) == 0)
            {
                platform = AZURE;
                sprintf(displayText, "Selected IoT platform\r\n\r\n       AZURE");
                Device_displayMessageWithDelay(displayText);
            }
            else
            {
                SSerial_printf(SerialDebug, "Unknown platform value: %s\r\n", platformConfig->u.object.values[0].value->u.string.ptr);
                sprintf(displayText, "Selected IoT platform:\r\n\r\n       UNKNOWN");
                Device_displayMessageWithDelay(displayText);
                return false;
            }
        }
    }
    return ret;
}

bool Device_isIotPlatformConfigured()
{
    bool ret = true;

    // create index_src.html - it is original index.html file
    if (!Calypso_fileExists(calypso, INDEX_HTML_SRC_FILE_PATH))
    {
        SSerial_printf(SerialDebug, "%s file not exist\r\n", INDEX_HTML_SRC_FILE_PATH);
        sprintf(displayText, "Error!!!\r\n\r\nFile not exist\r\n\r\n%s\r\n", INDEX_HTML_SRC_FILE_PATH);
        Device_displayMessageWithDelay(displayText);

        if (!Calypso_writeBigFile(calypso, INDEX_HTML_SRC_FILE_PATH, INDEX_HTML_SRC_FILE, strlen(INDEX_HTML_SRC_FILE)))
        {
            SSerial_printf(SerialDebug, "Unable to write file %s\r\n", INDEX_HTML_SRC_FILE_PATH);
            sprintf(displayText, "Error!!!\r\nUnable to write file\r\n%s", INDEX_HTML_SRC_FILE_PATH);
            Device_displayMessageWithDelay(displayText);
            return false; // to avoid rewriting of index.html
        }
        else
        {
            sprintf(displayText, "Backup index.html\r\nfile created:\r\n\r\n%s\r\n", INDEX_HTML_SRC_FILE_PATH);
            Device_displayMessageWithDelay(displayText);
        }
    }

    // check if the index.html file updated and rewrite it
    if (!Calypso_fileExists(calypso, INDEX_HTML_FILE_PATH))
    {
        SSerial_printf(SerialDebug, "%s file not exist\r\n", INDEX_HTML_FILE_PATH);
        sprintf(displayText, "Error!!!\r\n\r\nFile not exist\r\n\r\n%s\r\n", INDEX_HTML_FILE_PATH);
        Device_displayMessageWithDelay(displayText);

        if (!Calypso_writeBigFile(calypso, INDEX_HTML_FILE_PATH, INDEX_HTML_FILE, strlen(INDEX_HTML_FILE)))
        {
            SSerial_printf(SerialDebug, "Unable to write file %s\r\n", INDEX_HTML_FILE_PATH);
            sprintf(displayText, "Error!!!\r\nUnable to write file\r\n%s", INDEX_HTML_FILE_PATH);
            Device_displayMessageWithDelay(displayText);
            ret = false;
        }
        else
        {
            sprintf(displayText, "File created:\r\n\r\n%s\r\n", INDEX_HTML_FILE_PATH);
            Device_displayMessageWithDelay(displayText);
        }
    }
    else
    {
        int fileSize = getFileLength(INDEX_HTML_FILE_PATH);
        if (fileSize != strlen(INDEX_HTML_FILE))
        {
            SSerial_printf(SerialDebug, "%s file not updated. Current size:%i. Expected size:%i\r\n",
                           INDEX_HTML_FILE_PATH, fileSize, strlen(INDEX_HTML_FILE));
            Calypso_deleteFile(calypso, INDEX_HTML_FILE_PATH);

            if (!Calypso_writeBigFile(calypso, INDEX_HTML_FILE_PATH, INDEX_HTML_FILE, strlen(INDEX_HTML_FILE)))
            {
                SSerial_printf(SerialDebug, "Unable to write %s\r\n", INDEX_HTML_FILE_PATH);
                sprintf(displayText, "Error!!!\r\n\r\nUnable to write\r\n\r\n%s\r\n", INDEX_HTML_FILE_PATH);
                Device_displayMessageWithDelay(displayText);
                ret = false;
            }
            else
            {
                sprintf(displayText, "File updated:\r\n\r\n%s\r\n", INDEX_HTML_FILE_PATH);
                Device_displayMessageWithDelay(displayText);
            }
        }
    }

    if (platform == KAAIOT && !Kaaiot_Device_isConfiguredForPlatform())
    {
        SSerial_printf(SerialDebug, "Device was not configured for the KaaIoT\r\n");
        return false;
    }

    return ret;
}

bool Device_isConfigured()
{
    if (platform == KAAIOT)
    {
        return Kaaiot_Device_isConfigured();
    }
    else if (platform == AZURE)
    {
        return Azure_Device_isConfigured();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
        return NULL;
    }
}

void Device_configurationInProgress()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_configurationInProgress();
    }
    else if (platform == AZURE)
    {
        Azure_Device_configurationInProgress();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

void Device_readSensors()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_readSensors();
    }
    else if (platform == AZURE)
    {
        Azure_Device_readSensors();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

void Device_MQTTConnect()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_MQTTConnect();
    }
    else if (platform == AZURE)
    {
        Azure_Device_MQTTConnect();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

bool Device_SubscribeToTopics()
{
    if (platform == KAAIOT)
    {
        return Kaaiot_Device_SubscribeToTopics();
    }
    else if (platform == AZURE)
    {
        return Azure_Device_SubscribeToTopics();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
        return NULL;
    }
}

void Device_PublishSensorData()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_PublishSensorData();
    }
    else if (platform == AZURE)
    {
        Azure_Device_PublishSensorData();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

void Device_displaySensorData()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_displaySensorData();
    }
    else if (platform == AZURE)
    {
        Azure_Device_displaySensorData();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

void Device_processCloudMessage()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_processCloudMessage();
    }
    else if (platform == AZURE)
    {
        Azure_Device_processCloudMessage();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

void Device_connect_WiFi()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_connect_WiFi();
    }
    else if (platform == AZURE)
    {
        Azure_Device_connect_WiFi();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

void Device_disconnect_WiFi()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_disconnect_WiFi();
    }
    else if (platform == AZURE)
    {
        Azure_Device_disconnect_WiFi();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

void Device_reset()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_reset();
    }
    else if (platform == AZURE)
    {
        Azure_Device_reset();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

void Device_WiFi_provisioning()
{
    if (platform == KAAIOT)
    {
        Kaaiot_Device_WiFi_provisioning();
    }
    else if (platform == AZURE)
    {
        Azure_Device_WiFi_provisioning();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
    }
}

unsigned long Device_getTelemetrySendInterval()
{
    if (platform == KAAIOT)
    {
        return Kaaiot_Device_getTelemetrySendInterval();
    }
    else if (platform == AZURE)
    {
        return Azure_Device_getTelemetrySendInterval();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
        return 0;
    }
}

bool Device_isSensorsPresent()
{
    if (platform == KAAIOT)
    {
        return Kaaiot_Device_isSensorsPresent();
    }
    else if (platform == AZURE)
    {
        return Azure_Device_isSensorsPresent();
    }
    else
    {
        SSerial_printf(SerialDebug, "Platform not specified.\r\n");
        return NULL;
    }
}

void Device_displayMessageWithDelay(const char *message)
{
    SH1107_Display(1, 0, 0, message);
    LED_INDICATION_LONG_DELAY;
}