/**
 * \file
 * \brief API implementation for the device of the WE IoT design kit.
 *
 * \copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
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
#include "device.h"
#include "time.h"
#include "debug.h"

#include "mosquitto.h"
#include "azure_iot_central.h"
#include "aws.h"
#include "kaaiot.h"

#define MAX_PACKET_LOSS 3

// Serial Ports
//------Calypso
TypeHardwareSerial *SerialCalypso;

// Radio Module
//------Calpyso
CALYPSO *calypso;

float PADS_pressure, PADS_temp;
float ITDS_accelX, ITDS_accelY, ITDS_accelZ, ITDS_temp;
bool ITDS_doubleTapEvent, ITDS_freeFallEvent;
float TIDS_temp;
float HIDS_humidity, HIDS_temp;

bool sensorsPresent;

int messageID = 0;
int sessionID = 0;
bool deviceProvisioned = false;
bool deviceConfigured = false;
bool hids1_detected = false;
bool hids2_detected = false;

uint8_t configVersion = 0;

volatile unsigned long telemetrySendInterval = (unsigned long)(DEFAULT_TELEMETRY_SEND_INTEVAL * 1000);

uint8_t packetLost = 0;


char endPointAddress[MAX_URL_LEN] = {0};
uint16_t endPointAddrLen = 0;

static char displayText[128];

static char sensorPayload[MAX_PAYLOAD_LENGTH];

static bool Device_loadConfiguration();
static char *Device_SerializeData();
static void removeChar(char *s, char c);

/**
 * @brief Initialize all components of a device.
 * @param Debug Debug port.
 * @param CalypsoSerial Calypso serial port.
 * @retval Serial debug port.
 */
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

    if (!sensorBoard_Init())
    {
        SSerial_printf(SerialDebug,"I2C init failed \r\n");
        sensorsPresent = false;
    }

    // Initialize the sensors in default mode
    if (!PADS_2511020213301_simpleInit())
    {
        SSerial_printf(SerialDebug,"PADS init failed \r\n");
        sensorsPresent = false;
    }
    else
    {
        sensorsPresent = true;
    }

    if (!ITDS_2533020201601_simpleInit())
    {
        SSerial_printf(SerialDebug,"ITDS init failed \r\n");
        sensorsPresent = false;
    }
    else
    {
        sensorsPresent = true;
    }

    if (!TIDS_2521020222501_simpleInit())
    {
        SSerial_printf(SerialDebug,"TIDS init failed \r\n");
        sensorsPresent = false;
    }
    else
    {
        sensorsPresent = true;
    }

    if (!HIDS_2525020210002_simpleInit())
    {
        //Check if the old HIDS sensor is present
        if (!HIDS_2525020210001_simpleInit())
        {
            SSerial_printf(SerialDebug,"HIDS init failed \r\n");
            sensorsPresent = false;
        }
        else
        {
            sensorsPresent = true;
            hids1_detected = true;
        }
    }
    else
    {
        sensorsPresent = true;
        hids2_detected = true;
    }

    SSerial_printf(SerialDebug, "Starting the application v%s\r\n", HOST_FIRMWARE_VERSION);

    if (!Calypso_simpleInit(calypso))
    {
        SSerial_printf(SerialDebug, "Calypso init failed \r\n");
        sprintf(displayText, "Calypso Init Failed...");
        SH1107_Display(1, 0, 24, displayText);
    }
    messageID = 0;
    packetLost = 0;

    // Device_writeConfigFiles();
    sprintf(displayText, "Loading configuration...");
    SH1107_Display(1, 0, 24, displayText);
    if (Device_loadConfiguration() == true)
    {
        deviceConfigured = true;
        sprintf(displayText, "Connecting to Wi-Fi..");
        SH1107_Display(1, 0, 24, displayText);
        Device_connect_WiFi();
    }
    else
    {
        SSerial_printf(SerialDebug, "Laoding config file failed\r\n");
    }

    if (Calypso_fileExists(calypso, DEVICE_END_POINT_ADDRESS))
    {
        Calypso_readFile(calypso, DEVICE_END_POINT_ADDRESS, (char *)endPointAddress, MAX_URL_LEN, &endPointAddrLen);
        deviceProvisioned = true;
    }
    else
    {
        deviceProvisioned = false;
        memset(endPointAddress, 0, MAX_URL_LEN);
        endPointAddrLen = MAX_URL_LEN;
    }
    return SerialDebug;
}
/**
 * @brief Load device configuration from the stored file
 *
 * @return true - Config load success
 * @return false - Config load failed
 */
bool Device_loadConfiguration()
{
    char configBuf[512];
    uint16_t len;
    if (!Calypso_fileExists(calypso, CONFIG_FILE_PATH))
    {
        return false;
    }

    if (Calypso_readFile(calypso, CONFIG_FILE_PATH, (char *)configBuf, 512, &len))
    {
        json_value *configuration = json_parse(configBuf, len);
        if (configuration == NULL)
        {
            SSerial_printf(SerialDebug, "Unable to parse config file\r\n");
            return false;
        }
        
        configVersion = configuration->u.object.values[0].value->u.integer;
        switch (configVersion)
        {
            case MOSQUITTO_CONFIG_VERSION:
                Mosquitto_loadConfiguration(configuration, calypso);
                break;

            case AZURE_IOT_PNP_CONFIG_VERSION:
                Azure_loadConfiguration(configuration, calypso);
                break;

            case AWS_IOT_CORE_CONFIG_VERSION:
                AWS_loadConfiguration(configuration, calypso);
                break;
            
            case KAA_IOT_CONFIG_VERSION:
                Kaa_loadConfiguration(configuration, calypso);
                break;
            
            default:
                return false;
                break;
        }
       
    }
    else
    {
        return false;
    }
    return true;
}

/**
 * @brief Restart MCU.
 * @retval None.
 */
void Device_restart()
{
    soft_reset();
}

/**
 * @brief Check if the status of the GW is OK.
 * @retval true if OK, false otherwise.
 */
bool Device_isStatusOK()
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
 * @brief Check if the device is connected to the Wi-Fi network.
 * @retval true if connected, false otherwise.
 */
bool Device_isConnectedToWiFi()
{
    if (Calypso_isIPConnected(calypso))
    {
        return true;
    }
    SSerial_printf(SerialDebug, "Device not connected to Wi-Fi\r\n");
    return false;
}

/**
 * @brief Check if the device is up to date.
 * @retval true if up to date, false otherwise.
 */
bool Device_isUpToDate()
{
    char versionStr[20];
    const char dot[2] = ".";
    char *majorVer, *minorVer;

    strcpy(versionStr, calypso->firmwareVersion);

    majorVer = strtok(versionStr, dot);
    minorVer = strtok(NULL, dot);

    if (((atoi(majorVer)) < CALYPSO_FIRMWARE_MIN_MAJOR_VERSION) ||
        (((atoi(majorVer)) == CALYPSO_FIRMWARE_MIN_MAJOR_VERSION) && (atoi(minorVer)) < CALYPSO_FIRMWARE_MIN_MINOR_VERSION))
    {
        return false;
    }
    return true;
}


/**
 * @brief Check if the device is provisioned.
 * @retval true if provisioned, false otherwise.
 */
bool Device_isProvisioned()
{
    return deviceProvisioned;
}

/**
 * @brief Check if the device is configured.
 * @retval true if configured, false otherwise.
 */
bool Device_isConfigured()
{
    return deviceConfigured;
}

/**
 * @brief Publish provisioning request and get/save access token.
 * @retval true if successful, false otherwise.
 */
bool Device_provision()
{
    switch (configVersion)
    {
    case AZURE_IOT_PNP_CONFIG_VERSION:
        return (Azure_deviceProvision(calypso));
        break;
    
    default:
        return false;
        break;
    }

}

/**
 * @brief Indicate that device configuration is in progress.
 * @retval None.
 */
void Device_configurationInProgress()
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
        sprintf(displayText, "** Config mode **\r\n\r\nDev ID: %s \r\n\r\nPerform the \r\nfollowing 5 steps\r\n", temp);
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 1:
        sprintf(displayText, "** Config mode **\r\n\r\nDev ID: %s \r\n\r\n1. Connect your PC to\r\nthe access point\r\n\r\ncalypso_%s", temp, temp);
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 2:
        sprintf(displayText, "** Config mode **\r\n\r\nDev ID: %s \r\n\r\n2. Open your browser\r\n", temp);
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 3:
        sprintf(displayText, "** Config mode **\r\n\r\nDev ID: %s \r\n\r\n3. Navigate to page\r\n\r\ncalypso.net/azure.html", temp);
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 4:
        sprintf(displayText, "** Config mode **\r\n\r\nDev ID: %s \r\n\r\n4. Select and upload \r\nthe config files\r\n\r\n", temp);
        SH1107_Display(1, 0, 0, displayText);
        state++;
        break;
    case 5:
        sprintf(displayText, "** Config mode **\r\n\r\nDev ID: %s \r\n\r\n5. Upon completion,\r\nPress reset button", temp);
        SH1107_Display(1, 0, 0, displayText);
        state = 0;
        break;
    default:
        break;
    }
}

/**
 * @brief Collect data from sensors connected to the device.
 * @retval None.
 */
void Device_readSensors()
{
    if (PADS_2511020213301_readSensorData(&PADS_pressure, &PADS_temp))
    {
        SSerial_printf(SerialDebug,"WSEN_PADS: Atm. Pres: %f kPa Temp: %f °C\r\n",
                       PADS_pressure, PADS_temp);
    }
    if (ITDS_2533020201601_readSensorData(&ITDS_accelX, &ITDS_accelY,
                                          &ITDS_accelZ, &ITDS_temp))
    {
        SSerial_printf(SerialDebug,
            "WSEN_ITDS(Acceleration): X:%f g Y:%f g  Z:%f g Temp: "
            "%f °C \r\n",
            ITDS_accelX, ITDS_accelY, ITDS_accelZ, ITDS_temp);
    }
    if (ITDS_2533020201601_readDoubleTapEvent(&ITDS_doubleTapEvent))
    {
        SSerial_printf(SerialDebug,"WSEN_ITDS(DoubleTap): State %s \r\n",
                       ITDS_doubleTapEvent ? "True" : "False");
    }
    if (ITDS_2533020201601_readFreeFallEvent(&ITDS_freeFallEvent))
    {
        SSerial_printf(SerialDebug,"WSEN_ITDS(FreeFall): State %s \r\n",
                       ITDS_freeFallEvent ? "True" : "False");
    }
    if (TIDS_2521020222501_readSensorData(&TIDS_temp))
    {
        SSerial_printf(SerialDebug,"WSEN_TIDS(Temperature): %f °C\r\n", TIDS_temp);
    }

    if(hids1_detected == true)
    {
        if (HIDS_2525020210001_readSensorData(&HIDS_humidity, &HIDS_temp))
        {
            SSerial_printf(SerialDebug,"WSEN_HIDS: RH: %f %% Temp: %f °C\r\n", HIDS_humidity,
                        HIDS_temp);
        }
    }

    if(hids2_detected == true)
    {
        if (HIDS_2525020210002_readSensorData(&HIDS_humidity, &HIDS_temp))
        {
            SSerial_printf(SerialDebug,"WSEN_HIDS: RH: %f %% Temp: %f °C\r\n", HIDS_humidity,
                        HIDS_temp);
        }
    }
}


/**
 * @brief Connect device to cloud MQTT server.
 * @retval None.
 */
void Device_ConnectToCloud()
{
    strcpy(calypso->settings.mqttSettings.serverInfo.address, endPointAddress);
    if (configVersion == AZURE_IOT_PNP_CONFIG_VERSION)
    {
        Azure_setUserName(calypso);
    }
    if (Calypso_MQTTconnect(calypso) == true)
    {
        sprintf(displayText, "Connected to \r\nend point");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sprintf(displayText, "Error: Failed to connect\r\nto end point");
        SH1107_Display(1, 0, 24, displayText);
        SSerial_printf(calypso->serialDebug, "MQTT connect fail\r\n");
    }
    Device_SubscribeToTopics();
    if (configVersion == AZURE_IOT_PNP_CONFIG_VERSION)
    {
        Azure_PublishProperties(calypso);
    }
}

/**
 * @brief Subscribe to topics.
 * @retval true if successful, false otherwise.
 */
bool Device_SubscribeToTopics()
{
    bool ret;
    switch (configVersion)
    {
        case MOSQUITTO_CONFIG_VERSION:
            ret = Mosquitto_SubscribeToTopics(calypso);
            break;

        case AZURE_IOT_PNP_CONFIG_VERSION:
            ret = Azure_SubscribeToTopics(calypso);
            break;

        case AWS_IOT_CORE_CONFIG_VERSION:
            ret = AWS_SubscribeToTopics(calypso);
            break;
        
        case KAA_IOT_CONFIG_VERSION:
            ret = Kaa_SubscribeToTopics(calypso);
            break;

        default:
            ret = false;
            break;
    }
    return ret;
}



/**
 * @brief Wait for response from the cloud.
 * @retval JSON message or NULL.
 */
json_value *Device_GetCloudResponse()
{
    json_value *response = NULL;
    if ((Calypso_MQTTgetMessage(calypso, true)) && (calypso->rxData.length > 4))
    {
        response = json_parse(calypso->rxData.data, calypso->rxData.length);
        memset(calypso->rxData.data, 0, CALYPSO_LINE_MAX_SIZE);
        calypso->rxData.length = 0;
    }
    else
    {
        //SSerial_printf(SerialDebug, "No message\r\n");
    }
    return response;
}

/**
 * @brief Publish the values of sensors connected to the device.
 * @retval None.
 */
void Device_PublishSensorData()
{
    Device_readSensors();
    char *dataSerialized = Device_SerializeData();
#if SERIAL_DEBUG
    // SSerial_writeB(SerialDebug, dataSerialized, strlen(dataSerialized));
    // SSerial_printf(SerialDebug, "\r\n");
#endif
    if (!Calypso_MQTTPublishData(calypso, calypso->telemetryPubTopic, 1, dataSerialized, strlen(dataSerialized), true))
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
 * @brief Display sensor data on OLED display.
 * @retval None.
 */
void Device_displaySensorData()
{
    sprintf(displayText, "Status: Connected\r\nP:%0.2f kPa\r\nT:%0.2f C\r\nRH:%0.2f %%\r\nAcc: x:%0.2f g\r\n     y:%0.2f g\r\n     z:%0.2f g",
            PADS_pressure,
            TIDS_temp,
            HIDS_humidity,
            ITDS_accelX,
            ITDS_accelY,
            ITDS_accelZ);
    SH1107_Display(1, 0, 0, displayText);
}

/**
 * @brief Process messages from the cloud.
 * @retval None.
 */
void Device_processCloudMessage()
{
    json_value *cloudResponse = NULL;

    cloudResponse = Device_GetCloudResponse();

    switch (configVersion)
    {
        case MOSQUITTO_CONFIG_VERSION:
            Mosquitto_ProcessCloudMessage(cloudResponse, calypso);
            break;

        case AZURE_IOT_PNP_CONFIG_VERSION:
            Azure_ProcessCloudMessage(cloudResponse, calypso);
            break;

        case AWS_IOT_CORE_CONFIG_VERSION:
            AWS_ProcessCloudMessage(cloudResponse, calypso);
            break;

        case KAA_IOT_CONFIG_VERSION:
            Kaa_ProcessCloudMessage(cloudResponse, calypso);
            break;

        default:
            break;
    }
    if (cloudResponse != NULL)
    {
        json_value_free(cloudResponse);
    }
}

/**
 * @brief Connect to WiFi.
 * @retval None.
 */
void Device_connect_WiFi()
{
    if (!Calypso_WLANconnect(calypso))
    {
        SSerial_printf(calypso->serialDebug, "WiFi connect fail\r\n");
        sprintf(displayText, "Error: Wi-Fi connect \r\nfailed");
        SH1107_Display(1, 0, 24, displayText);
        return;
    }
    delay(WI_FI_CONNECT_DELAY);
    if (calypso->status == calypso_WLAN_connected)
    {
        sprintf(displayText, "Connected to Wi-Fi");
        SH1107_Display(1, 0, 24, displayText);
    }
    if (!Calypso_setUpSNTP(calypso))
    {
        SSerial_printf(calypso->serialDebug, "SNTP config fail\r\n");
    }
}

/**
 * @brief Disconnect from WiFi.
 * @retval None.
 */
void Device_disconnect_WiFi()
{
    if (!Calypso_WLANDisconnect(calypso))
    {
        SSerial_printf(calypso->serialDebug, "WiFi disconnect fail\r\n");
    }
    calypso->status = calypso_WLAN_disconnected;
}

/**
 * @brief Reset cloud access token and claim status.
 * @retval None.
 */
void Device_reset()
{
    Device_disconnect_WiFi();
    /*Delete device credentials*/
    if (Calypso_fileExists(calypso, DEVICE_END_POINT_ADDRESS))
    {
        Calypso_deleteFile(calypso, DEVICE_END_POINT_ADDRESS);
    }
    if (Calypso_fileExists(calypso, CONFIG_FILE_PATH))
    {
        Calypso_deleteFile(calypso, CONFIG_FILE_PATH);
    }
    soft_reset();
}

/**
 * @brief Start Calypso provisioning.
 * @retval None.
 */
void Device_WiFi_provisioning()
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
 * @brief Serialize data to send.
 * @retval Pointer to serialized data.
 */
static char *Device_SerializeData()
{
    json_value *payload = json_object_new(4);
    if (payload == NULL)
    {
        SSerial_printf(SerialDebug, "Payload memory full \r\n");
        return NULL;
    }

    json_object_push(payload, "pressure",
                     json_double_new(PADS_pressure));

    json_object_push(payload, "humidity",
                     json_double_new(HIDS_humidity));

    json_object_push(payload, "temperature",
                     json_double_new(TIDS_temp));

    json_value *acceleration = json_object_new(3);
    if (acceleration == NULL)
    {
        json_builder_free(payload);
        SSerial_printf(SerialDebug, "acceleration memory full \r\n");
        return NULL;
    }

    json_object_push(acceleration, "x",
                     json_double_new(ITDS_accelX));
    json_object_push(acceleration, "y",
                     json_double_new(ITDS_accelY));
    json_object_push(acceleration, "z",
                     json_double_new(ITDS_accelZ));

    json_object_push(payload, "acceleration", acceleration);

    memset(sensorPayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(sensorPayload, payload);

    json_builder_free(payload);
    json_builder_free(acceleration);
    return sensorPayload;
}

/**
 * @brief Remove a specific character from a string.
 * @param s Input string.
 * @param c Character to remove.
 * @retval None.
 */
static void removeChar(char *s, char c)
{

    int j, n = strlen(s);
    for (int i = j = 0; i < n; i++)
        if (s[i] != c)
            s[j++] = s[i];

    s[j] = '\0';
}