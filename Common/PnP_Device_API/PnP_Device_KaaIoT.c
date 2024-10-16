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
#include "PnP_Device_KaaIoT.h"
#include "time.h"

#define MAX_PACKET_LOSS 3

// Serial Ports
//------Debug
extern TypeSerial *SerialDebug;
//------Calypso
extern TypeHardwareSerial *SerialCalypso;

// Radio Module
//------Calpyso
extern CALYPSO *calypso;

// Sensors
//------WSEN_PADS
extern PADS *sensorPADS;
//------WSEN_ITDS
extern ITDS *sensorITDS;
//------WSEN_TIDS
extern TIDS *sensorTIDS;
//------WSEN_HIDS
extern HIDS *sensorHIDS;

extern bool sensorsPresent;
extern bool deviceProvisioned;
extern bool deviceConfigured;

extern volatile unsigned long telemetrySendInterval;

extern uint8_t packetLost;

extern char kitID[DEVICE_CREDENTIALS_MAX_LEN];
char appVersion[DEVICE_CREDENTIALS_MAX_LEN] = {0};
extern char modelID[DEVICE_CREDENTIALS_MAX_LEN];
char kaaMqttServerAddress[MAX_URL_LEN] = {0};
extern uint16_t kitIDLength;
extern uint8_t reqID;
extern float lastBattVolt;

static char displayText[128];
extern char pubtopic[128];
#define MAX_PAYLOAD_LENGTH 1024
static char sensorPayload[MAX_PAYLOAD_LENGTH];
static char cmdResponseData[MAX_PAYLOAD_LENGTH];

// Certificates
const char *configurationKaaiot = KAAIOT_CONFIGURATION_DATA;

const char *fileToWrite;

static bool Device_loadConfiguration();
static char *Device_SerializeData();
static char *Device_CommandResponseData(int requestId, int statusCode, char *reasonPhrase);

static json_value *Device_GetCloudMessage();
static void removeChar(char *s, char c);

static void Device_PublishDirectCmdResponse(char *appVersion, char *token, char *commandType, int requestId, int statusCode, char *reasonPhrase);

/**
 * @brief  Initialize all components of a device
 * @param  Debug Debug port
 * @param  CalypsoSerial Calypso serial port
 * @retval Serial debug port
 */
TypeSerial *Kaaiot_Device_init(void *Debug, void *CalypsoSerial)
{
    // Kaaiot_Device_writeConfigFiles();
    sprintf(displayText, "Loading configuration...");
    SH1107_Display(1, 0, 24, displayText);
    LED_INDICATION_SHORT_DELAY;
    if (Device_loadConfiguration() == true)
    {
        deviceConfigured = true;
        sprintf(displayText, "Connecting to Wi-Fi..");
        SH1107_Display(1, 0, 24, displayText);
        Kaaiot_Device_connect_WiFi();
    }
    else
    {
        SSerial_printf(SerialDebug, "Loading config file failed\r\n");
    }

    // deviceProvisioned = true;
    return SerialDebug;
}

/**
 * @brief Function to run on completion of device configuration
 *
 * @return true
 * @return false
 */
bool Kaaiot_Device_ConfigurationComplete()
{
    if (!Calypso_simpleInit(calypso))
    {
        SSerial_printf(SerialDebug, "Calypso init failed \r\n");
        return false;
    }
    if (Device_loadConfiguration() == true)
    {
        deviceConfigured = true;
        Kaaiot_Device_connect_WiFi();
    }
    else
    {
        SSerial_printf(SerialDebug, "Loading config file failed\r\n");
        return false;
    }

    // deviceProvisioned = true;
    return deviceConfigured;
}

/**
 * @brief Load device configuration from the stored file
 *
 * @return true - Config load success
 * @return false - Config load failed
 */
static bool Device_loadConfiguration()
{
    char configBuf[512];
    uint16_t len;
    if (!Calypso_fileExists(calypso, KAAIOT_CONFIG_FILE_PATH))
    {
        sprintf(displayText, "Error! Config file\r\n\r\nnot found:\r\n\r\n%s", KAAIOT_CONFIG_FILE_PATH);
        SH1107_Display(1, 0, 0, displayText);
        SSerial_printf(SerialDebug, "Configuration file not found\r\n");
        LED_INDICATION_LONG_DELAY;
        return false;
    }

    if (!Calypso_fileExists(calypso, KAAIOT_ROOT_CA_PATH))
    {
        sprintf(displayText, "Error! Root CA\r\n\r\nnot found\r\n\r\n%s", KAAIOT_ROOT_CA_PATH);
        SH1107_Display(1, 0, 0, displayText);
        SSerial_printf(SerialDebug, "Root CA not found\r\n");
        LED_INDICATION_LONG_DELAY;
        return false;
    }

    if (!Calypso_fileExists(calypso, KAAIOT_DEVICE_CERT_PATH))
    {
        sprintf(displayText, "Error! Device cert\r\n\r\nnot found\r\n\r\n%s", KAAIOT_DEVICE_CERT_PATH);
        SH1107_Display(1, 0, 0, displayText);
        SSerial_printf(SerialDebug, "Device certificate not found\r\n");
        LED_INDICATION_LONG_DELAY;
    }

    if (!Calypso_fileExists(calypso, KAAIOT_DEVICE_KEY_PATH))
    {
        sprintf(displayText, "Error! Device key\r\n\r\nnot found\r\n\r\n%s", KAAIOT_DEVICE_KEY_PATH);
        SH1107_Display(1, 0, 0, displayText);
        SSerial_printf(SerialDebug, "Device key not found\r\n");
        LED_INDICATION_LONG_DELAY;
    }

    if (Calypso_readFile(calypso, KAAIOT_CONFIG_FILE_PATH, (char *)configBuf, 512, &len))
    {
        json_value *configurationKaaiot = json_parse(configBuf, len);
        if (configurationKaaiot == NULL)
        {
            SSerial_printf(SerialDebug, "Unable to parse config file\r\n");
            return false;
        }
        strcpy(kitID, configurationKaaiot->u.object.values[0].value->u.string.ptr);
        strcpy(appVersion, configurationKaaiot->u.object.values[1].value->u.string.ptr);
        strcpy(kaaMqttServerAddress, configurationKaaiot->u.object.values[2].value->u.string.ptr);
        strcpy(calypso->settings.sntpSettings.server, configurationKaaiot->u.object.values[3].value->u.string.ptr);
        strcpy(calypso->settings.sntpSettings.timezone, configurationKaaiot->u.object.values[4].value->u.string.ptr);
        strcpy(calypso->settings.wifiSettings.SSID, configurationKaaiot->u.object.values[5].value->u.string.ptr);
        calypso->settings.wifiSettings.securityParams.securityType = atoi(configurationKaaiot->u.object.values[6].value->u.string.ptr);
        strcpy(calypso->settings.wifiSettings.securityParams.securityKey, configurationKaaiot->u.object.values[7].value->u.string.ptr);
    }

    // MQTT Settings
    calypso->settings.mqttSettings.flags = ATMQTT_CREATE_FLAGS_URL | ATMQTT_CREATE_FLAGS_SEC | ATMQTT_CREATE_FLAGS_SKIP_DATE_VERIFY | ATMQTT_CREATE_FLAGS_SKIP_CERT_VERIFY | ATMQTT_CREATE_FLAGS_SKIP_DOMAIN_VERIFY;
    calypso->settings.mqttSettings.serverInfo.port = MQTT_PORT_SECURE;

    calypso->settings.mqttSettings.secParams.securityMethod = ATMQTT_SECURITY_METHOD_TLSV1_2;
    calypso->settings.mqttSettings.secParams.cipher = ATMQTT_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA;
    strcpy(calypso->settings.mqttSettings.secParams.CAFile, KAAIOT_ROOT_CA_PATH);
    if (Calypso_fileExists(calypso, KAAIOT_DEVICE_CERT_PATH))
    {
        strcpy(calypso->settings.mqttSettings.secParams.certificateFile, KAAIOT_DEVICE_CERT_PATH);
    }
    if (Calypso_fileExists(calypso, KAAIOT_DEVICE_KEY_PATH))
    {
        strcpy(calypso->settings.mqttSettings.secParams.privateKeyFile, KAAIOT_DEVICE_KEY_PATH);
    }

    calypso->settings.mqttSettings.connParams.protocolVersion = ATMQTT_PROTOCOL_v3_1_1;
    calypso->settings.mqttSettings.connParams.blockingSend = 0;
    calypso->settings.mqttSettings.connParams.format = Calypso_DataFormat_Base64;
    return true;
}

/**
 * @brief Check if the device is configured for the KaaIoT (exist files kaaiot.html, kaaiot.js and updated index.html)
 *
 * @return true Device is configured for the KaaIoT
 * @return false Device is not configured for the KaaIoT
 */
bool Kaaiot_Device_isConfiguredForPlatform()
{
    bool ret = false;

    ret = Calypso_fileExists(calypso, KAAIOT_HTML_FILE_PATH);
    if (!ret)
    {
        SSerial_printf(SerialDebug, "%s file not exist\r\n", KAAIOT_HTML_FILE_PATH);

        fileToWrite = KAAIOT_HTML_FILE;
        if (!Calypso_writeBigFile(calypso, KAAIOT_HTML_FILE_PATH, fileToWrite, strlen(KAAIOT_HTML_FILE)))
        {
            SSerial_printf(SerialDebug, "Unable to write %s\r\n", KAAIOT_HTML_FILE_PATH);
        }
    }

    ret = Calypso_fileExists(calypso, KAAIOT_JS_FILE_PATH);
    if (!ret)
    {
        SSerial_printf(SerialDebug, "%s file not exist\r\n", KAAIOT_JS_FILE_PATH);

        fileToWrite = KAAIOT_JS_FILE;
        if (!Calypso_writeBigFile(calypso, KAAIOT_JS_FILE_PATH, fileToWrite, strlen(KAAIOT_JS_FILE)))
        {
            SSerial_printf(SerialDebug, "Unable to write %s\r\n", KAAIOT_JS_FILE_PATH);
        }
    }

    return ret;
}

void Kaaiot_Device_deletePreviousConfig()
{
    if (Calypso_fileExists(calypso, KAAIOT_CONFIG_FILE_PATH))
    {
        Calypso_deleteFile(calypso, KAAIOT_CONFIG_FILE_PATH);
    }
}

/**
 * @brief  Write config files to calypso
 * @retval None
 */
void Kaaiot_Device_writeConfigFiles()
{
    Kaaiot_Device_disconnect_WiFi();

    if (!Calypso_writeFile(calypso, KAAIOT_CONFIG_FILE_PATH, configurationKaaiot, strlen(configurationKaaiot)))
    {
        SSerial_printf(SerialDebug, "Unable to configuration data\r\n");
    }
}

/**
 * @brief  Restart MCU
 * @retval None
 */
void Kaaiot_Device_restart()
{
    soft_reset();
}

/**
 * @brief  Check if the status of the GW is OK
 * @retval true if OK false otherwise
 */
bool Kaaiot_Device_isStatusOK()
{
    if (calypso->status == calypso_error)
    {
        return false;
    }
    else
    {
        return true;
    }
}

/**
 * @brief  Check if the device is connected to the Wi-Fi network
 * @retval True if connected false otherwise
 */
bool Kaaiot_Device_isConnectedToWiFi()
{
    if (Calypso_isIPConnected(calypso))
    {
        return true;
    }
    SSerial_printf(SerialDebug, "Device not connected to Wi-Fi\r\n");
    return false;
}

/**
 * @brief  Check if the device is up to date
 * @retval True or false
 */
bool Kaaiot_Device_isUpToDate()
{
    char versionStr[20];
    const char dot[2] = ".";
    char *majorVer, *minorVer;

    strcpy(versionStr, calypso->firmwareVersion);

    majorVer = strtok(versionStr, dot);
    minorVer = strtok(NULL, dot);

    if ((atoi(majorVer)) >= CALYPSO_FIRMWARE_MIN_MAJOR_VERSION)
    {
        if ((atoi(minorVer)) >= CALYPSO_FIRMWARE_MIN_MINOR_VERSION)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Check if the device is configured
 *
 * @return true Device is configured
 * @return false Device is not configured
 */
bool Kaaiot_Device_isConfigured()
{
    return deviceConfigured;
}

void Kaaiot_Device_configurationInProgress()
{
    char temp[20];
    static uint8_t state;
    memcpy(temp, calypso->MAC_ADDR, 20);
    removeChar(temp, ':');
    SSerial_printf(SerialDebug, "Device in configuration mode\r\n");
    SSerial_printf(SerialDebug, "Connect to the Calypso Access point to configure the device\r\n");
    SSerial_printf(SerialDebug, "Calypso Access point SSID is calypso_%s\r\n", temp);
    SSerial_printf(SerialDebug, "Restart the device after configuration\r\n");

    switch (state)
    {
    case 0:
        sprintf(displayText, "** Config mode **\r\n\r\nPerform the \r\nfollowing 5 steps\r\n\r\n\n\r\n--->");
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 1:
        sprintf(displayText, "** Config mode **\r\n\r\n1. Connect your PC to\r\nthe access point\r\n\r\ncalypso_%s\r\n\r\n--->", temp);
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 2:
        sprintf(displayText, "** Config mode **\r\n\r\n2. Open your browser\r\n\r\n\r\n\r\n\r\n--->");
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 3:
        sprintf(displayText, "** Config mode **\r\n\r\n3. Navigate to page\r\n\r\ncalypso.net/kaaiot.html\r\n\r\n--->");
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 4:
        sprintf(displayText, "** Config mode **\r\n\r\n4. Fill the\r\n   configuration\r\n\r\n\r\n\r\n--->");
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 5:
        sprintf(displayText, "** Config mode **\r\n\r\n5. Upon completion,\r\n\r\nPress reset button \r\non device");
        SH1107_Display(1, 0, 0, displayText);
        state = 0;
        break;
    default:
        break;
    }
}

/**
 * @brief  Collect data from sensors connected to the device
 * @retval None
 */
void Kaaiot_Device_readSensors()
{
    if (!PADS_readSensorData(sensorPADS))
    {
        SSerial_printf(SerialDebug, "Error reading pressure data\r\n");
    }

    if (!ITDS_readSensorData(sensorITDS))
    {
        SSerial_printf(SerialDebug, "Error reading acceleration data\r\n");
    }

    if (!TIDS_readSensorData(sensorTIDS))
    {
        SSerial_printf(SerialDebug, "Error reading temperature data\r\n");
    }

    if (!HIDS_readSensorData(sensorHIDS))
    {
        SSerial_printf(SerialDebug, "Error reading humidity data\r\n");
    }
}

/**
 * @brief  Connect GW to cloud MQTT sever
 * @retval None
 */
void Kaaiot_Device_MQTTConnect()
{
    strcpy(calypso->settings.mqttSettings.clientID, kitID);
    // sprintf(calypso->settings.mqttSettings.userOptions.userName, "%s-%s", kitID, modelID);
    strcpy(calypso->settings.mqttSettings.serverInfo.address, kaaMqttServerAddress);

    // SSerial_printf(SerialDebug, "MQTT username: %s\r\n", calypso->settings.mqttSettings.userOptions.userName);
    SSerial_printf(SerialDebug, "MQTT server to connect: %s\r\n", kaaMqttServerAddress);

    if (Calypso_MQTTconnect(calypso) == false)
    {
        if (calypso->status == calypso_MQTT_wrong_root_ca)
        {
            Calypso_MQTTDisconnect(calypso);
            SSerial_printf(calypso->serialDebug, "Wrong MQTT root ca\r\n");
        }
        else
        {
            sprintf(displayText, "Error!\r\n\r\nFailed to connect to\r\n\r\n    KaaIoT");
            SH1107_Display(1, 0, 16, displayText);
            SSerial_printf(calypso->serialDebug, "MQTT connect fail\r\n");
        }
    }
    else
    {
        sprintf(displayText, "KaaIoT connected");
        SH1107_Display(1, 0, 24, displayText);
        SSerial_printf(calypso->serialDebug, "MQTT connect success\r\n");
    }
}

/**
 * @brief  Subscribe to topics
 * @retval True if successful false otherwise
 */
bool Kaaiot_Device_SubscribeToTopics()
{
    if (calypso->status == calypso_MQTT_connected)
    {
        ATMQTT_subscribeTopic_t topics[1];
        ATMQTT_subscribeTopic_t kaaCommands;

        kaaCommands.QoS = ATMQTT_QOS_QOS0;
        sprintf(kaaCommands.topicString, KAA_COMMANDS_TOPIC, appVersion, kitID);

        topics[0] = kaaCommands;
        return (Calypso_subscribe(calypso, 0, 1, topics));
    }
    else
    {
        return false;
    }
}

/**
 * @brief  Get message from the cloud
 * @retval JSON message or NULL
 */
static json_value *Device_GetCloudMessage()
{
    json_value *response = NULL;
    if ((Calypso_MQTTgetMessage(calypso, true)) && (calypso->bufferCalypso.length > 4))
    {
        response = json_parse(calypso->bufferCalypso.data, calypso->bufferCalypso.length);
        memset(calypso->bufferCalypso.data, 0, CALYPSO_LINE_MAX_SIZE);
        calypso->bufferCalypso.length = 0;
    }
    else
    {
        // SSerial_printf(SerialDebug, "No message\r\n");
    }
    return response;
}

/**
 * @brief  Publish the values of sensors connected to the device
 * @retval None
 */
void Kaaiot_Device_PublishSensorData()
{
    Kaaiot_Device_readSensors();
    char *dataSerialized = Device_SerializeData();
#if SERIAL_DEBUG
    // SSerial_writeB(SerialDebug, dataSerialized, strlen(dataSerialized));
    // SSerial_printf(SerialDebug, "\r\n");
#endif
    pubtopic[0] = '\0';
    sprintf(pubtopic, KAA_DATA_SAMPLES_TOPIC, appVersion, kitID);
    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, dataSerialized, strlen(dataSerialized), true))
    {
        packetLost++;
        SSerial_printf(SerialDebug, "Publish failed %u\r\n", packetLost);
        if (packetLost == MAX_PACKET_LOSS)
        {
            calypso->status = calypso_error;
        }
    }
}

/**
 * @brief  Display sensor data on OLED display
 * @retval None
 */
void Kaaiot_Device_displaySensorData()
{
    sprintf(displayText, "Status: Connected\r\nP:%0.2f kPa\r\nT:%0.2f C\r\nRH:%0.2f %%\r\nAcc: x:%0.2f g\r\n     y:%0.2f g\r\n     z:%0.2f g",
            sensorPADS->data[0],
            sensorTIDS->data[0],
            sensorHIDS->data[0],
            sensorITDS->data[0],
            sensorITDS->data[1],
            sensorITDS->data[2]);
    SH1107_Display(1, 0, 0, displayText);
}

/**
 * @brief  Publish response to the command
 * @retval None
 */
static void Device_PublishDirectCmdResponse(char *appVersion, char *token, char *commandType, int requestId, int statusCode, char *reasonPhrase)
{
    char *responseData = Device_CommandResponseData(requestId, statusCode, reasonPhrase);

    pubtopic[0] = '\0';
    sprintf(pubtopic, KAA_COMMANDS_RESPONSE_TOPIC, appVersion, token, commandType);
    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, responseData, strlen(responseData), true))
    {
        SSerial_printf(SerialDebug, "Publish command response failed\r\n");
    }
}

/**
 * @brief  Process messages from the cloud
 * @retval None
 */
void Kaaiot_Device_processCloudMessage()
{
    json_value *cloudMessage = NULL;
    const char backsSlash[2] = "/";
    char *msgAppVersion;
    char *msgToken;
    char *msgCommandType;
    char *topicStatus;
    int commandId;
    char *commandPayloadKey;
    char *commandPayloadValue;
    cloudMessage = Device_GetCloudMessage();

    strtok(calypso->topicName.data, backsSlash);
    msgAppVersion = strtok(NULL, backsSlash);
    strtok(NULL, backsSlash);
    msgToken = strtok(NULL, backsSlash);
    strtok(NULL, backsSlash);
    msgCommandType = strtok(NULL, backsSlash);
    topicStatus = strtok(NULL, backsSlash);

    if (!strstr(msgAppVersion, appVersion) || !strstr(msgToken, kitID) || !strstr(topicStatus, "status"))
    {
        if (cloudMessage != NULL)
        {
            json_value_free(cloudMessage);
        }
        return;
    }

    if (!strstr(msgCommandType, "switch_on_off"))
    {
        SSerial_printf(SerialDebug, "Unexpected command type: %s\r\n", msgCommandType);
        if (cloudMessage != NULL)
        {
            json_value_free(cloudMessage);
        }
        return;
    }

    SSerial_printf(SerialDebug, "Commands received. Type: %s, appVersion: %s, token: %s.\r\n", msgCommandType, msgAppVersion, msgToken);

    if (cloudMessage->type == json_array)
    {
        unsigned int length = cloudMessage->u.array.length;
        for (unsigned int i = 0; i < length; i++)
        {
            json_value *commandElement = cloudMessage->u.array.values[i];
            json_value *commandPayloadJson = commandElement->u.object.values[1].value;

            commandId = commandElement->u.object.values[0].value->u.integer;
            commandPayloadKey = commandPayloadJson->u.object.values[0].name;
            commandPayloadValue = commandPayloadJson->u.object.values[0].value->u.string.ptr;

            SSerial_printf(SerialDebug, "Command payload. ID: %i, key: %s, value: %s, \r\n", commandId, commandPayloadKey, commandPayloadValue);

            if (0 == strncmp(commandPayloadValue, "on", strlen("on")))
            {
                Device_PublishDirectCmdResponse(appVersion, msgToken, msgCommandType, commandId, 200, "OK");
                sprintf(displayText, "State: \"%s\"", commandPayloadValue);
                SSerial_printf(SerialDebug, "State changed to \"%s\"\r\n", commandPayloadValue);
            }
            else if (0 == strncmp(commandPayloadValue, "off", strlen("off")))
            {
                Device_PublishDirectCmdResponse(appVersion, msgToken, msgCommandType, commandId, 200, "OK");
                sprintf(displayText, "State: \"%s\"", commandPayloadValue);
                SSerial_printf(SerialDebug, "State changed to \"%s\"\r\n", commandPayloadValue);
            }
            else
            {
                Device_PublishDirectCmdResponse(appVersion, msgToken, msgCommandType, commandId, 400, "Unknown state");
                sprintf(displayText, "Unknown state: %s", commandPayloadValue);
                SSerial_printf(SerialDebug, "Unknown state: %s\r\n", commandPayloadValue);
            }
            SH1107_Display(1, 0, 16, displayText);
        }
    }

    if (cloudMessage != NULL)
    {
        json_value_free(cloudMessage);
    }
}

/**
 * @brief  Connect to WiFi
 * @retval None
 */
void Kaaiot_Device_connect_WiFi()
{
    if (!Calypso_WLANconnect(calypso))
    {
        SSerial_printf(calypso->serialDebug, "WiFi connect fail\r\n");
        sprintf(displayText, "Error!!!\r\n\r\nWi-Fi connect failed\r\n\r\nCheck configuration!");
        SH1107_Display(1, 0, 0, displayText);
        return;
    }
    delay(WI_FI_CONNECT_DELAY);
    if (calypso->status == calypso_WLAN_connected)
    {
        sprintf(displayText, "Connected to Wi-Fi");
        SH1107_Display(1, 0, 24, displayText);
        LED_INDICATION_SHORT_DELAY;
    }

    sprintf(displayText, "SNTP time sync...");
    SH1107_Display(1, 0, 24, displayText);
    LED_INDICATION_SHORT_DELAY;
    if (!Calypso_setUpSNTP(calypso))
    {
        SSerial_printf(calypso->serialDebug, "SNTP config fail\r\n");
        sprintf(displayText, "SNTP time sync FAIL!");
    }
    else
    {
        sprintf(displayText, "SNTP time sync OK.");
    }
    Calypso_getTime(calypso);
    SH1107_Display(1, 0, 24, displayText);
    LED_INDICATION_SHORT_DELAY;
}

/**
 * @brief  Disconnect from WiFi
 * @retval None
 */
void Kaaiot_Device_disconnect_WiFi()
{
    if (!Calypso_WLANDisconnect(calypso))
    {
        SSerial_printf(calypso->serialDebug, "WiFi disconnect fail\r\n");
    }
    calypso->status = calypso_WLAN_disconnected;
}
/**
 * @brief  Reset cloud access token and claim status
 * @retval None
 */
void Kaaiot_Device_reset()
{
    Kaaiot_Device_disconnect_WiFi();
    /*Delete device credentials*/
    if (Calypso_fileExists(calypso, KAAIOT_CONFIG_FILE_PATH))
    {
        Calypso_deleteFile(calypso, KAAIOT_CONFIG_FILE_PATH);
    }
    if (Calypso_fileExists(calypso, KAAIOT_ROOT_CA_PATH))
    {
        Calypso_deleteFile(calypso, KAAIOT_ROOT_CA_PATH);
    }
    if (Calypso_fileExists(calypso, KAAIOT_DEVICE_CERT_PATH))
    {
        Calypso_deleteFile(calypso, KAAIOT_DEVICE_CERT_PATH);
    }
    if (Calypso_fileExists(calypso, KAAIOT_DEVICE_KEY_PATH))
    {
        Calypso_deleteFile(calypso, KAAIOT_DEVICE_KEY_PATH);
    }
    soft_reset();
}

/**
 * @brief  Start calypso provisioning
 * @retval None
 */
void Kaaiot_Device_WiFi_provisioning()
{
    if (calypso->status == calypso_provisioning)
    {
        return;
    }
    if (Calypso_StartProvisioning(calypso))
    {
        calypso->status = calypso_provisioning;
    }
    else
    {
        SSerial_printf(calypso->serialDebug, "Start provisioning fail\r\n");
    }
}

/**
 * @brief  Serialize data to send
 * @retval Pointer to serialized data
 */
static char *Device_SerializeData()
{
    uint8_t idx = 0;
    json_value *payload = json_object_new(padsProperties + tidsProperties + hidsProperties + 1);
    if (payload == NULL)
    {
        SSerial_printf(SerialDebug, "Payload memory full \r\n");
        return NULL;
    }
    json_value *acceleration = json_object_new(3);
    if (acceleration == NULL)
    {
        json_builder_free(payload);
        SSerial_printf(SerialDebug, "acceleration memory full \r\n");
        return NULL;
    }

    for (idx = 0; idx < padsProperties; idx++)
    {
        json_object_push(payload, sensorPADS->dataNames[idx],
                         json_double_new(sensorPADS->data[idx]));
    }
    for (idx = 0; idx < hidsProperties; idx++)
    {
        json_object_push(payload, sensorHIDS->dataNames[idx],
                         json_double_new(sensorHIDS->data[idx]));
    }
    for (idx = 0; idx < tidsProperties; idx++)
    {
        json_object_push(payload, sensorTIDS->dataNames[idx],
                         json_double_new(sensorTIDS->data[idx]));
    }

    for (idx = 0; idx < itdsProperties; idx++)
    {
        json_object_push(acceleration, sensorITDS->dataNames[idx],
                         json_double_new(sensorITDS->data[idx]));
    }
    json_object_push(payload, "acceleration", acceleration);
    memset(sensorPayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(sensorPayload, payload);

    json_builder_free(payload);
    json_builder_free(acceleration);
    return sensorPayload;
}

/**
 * @brief  Gets command response data
 * @retval Pointer to response data
 */
static char *Device_CommandResponseData(int requestId, int statusCode, char *reasonPhrase)
{
    json_value *payloadArray = json_array_new(1);
    json_value *payload = json_object_new(4);
    if (payload == NULL)
    {
        SSerial_printf(SerialDebug, "Payload memory full \r\n");
        return NULL;
    }

    // id
    json_object_push(payload, "id", json_integer_new(requestId));
    // statusCode
    json_object_push(payload, "statusCode", json_integer_new(statusCode));
    // reasonPhrase
    json_object_push(payload, "reasonPhrase", json_string_new(reasonPhrase));
    // payload
    json_value *responsePayload = json_object_new(0);
    if (responsePayload == NULL)
    {
        json_builder_free(payload);
        SSerial_printf(SerialDebug, "cmdPayload memory full \r\n");
        return NULL;
    }
    json_object_push(payload, "payload", responsePayload);
    json_array_push(payloadArray, payload);

    memset(cmdResponseData, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(cmdResponseData, payloadArray);

    json_builder_free(payload);
    json_builder_free(payloadArray);
    json_builder_free(responsePayload);
    return cmdResponseData;
}

/**
 * @brief  Remove a specific charecter from a string
 * @param  s input string
 * @param  c char to remove
 * @retval None
 */
static void removeChar(char *s, char c)
{

    int j, n = strlen(s);
    for (int i = j = 0; i < n; i++)
        if (s[i] != c)
            s[j++] = s[i];

    s[j] = '\0';
}

unsigned long Kaaiot_Device_getTelemetrySendInterval()
{
    return telemetrySendInterval;
}

bool Kaaiot_Device_isSensorsPresent()
{
    return sensorsPresent;
}