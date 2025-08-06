/**
 * \file
 * \brief API implementation for the Azure IoT Central.
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
#include "azure_iot_central.h"
#include "device.h"


char kitID[DEVICE_CREDENTIALS_MAX_LEN] = {0};
char scopeID[DEVICE_CREDENTIALS_MAX_LEN] = {0};
char modelID[DEVICE_CREDENTIALS_MAX_LEN] = {0};
char azureEndPointAddress[MAX_URL_LEN] = {0};
char dpsServerAddress[MAX_URL_LEN] = {0};

static char azurePayload[MAX_PAYLOAD_LENGTH];
static char azurepubtopic[128];

uint16_t kitIDLength = 0;
uint8_t reqID = 0;

float lastBattVolt = 0;

static char displayText[128];

static void Azure_PublishVoltage(CALYPSO *calypso);
static void Azure_PublishSWVersion(CALYPSO *calypso);
static void Azure_PublishUDID(CALYPSO *calypso);
static void Azure_PublishMACAddress(CALYPSO *calypso);
static void Azure_PublishDirectCmdResponse(CALYPSO *calypso, int status, int requestID);
static void Azure_PublishSendInterval(CALYPSO *calypso, uint16_t val, uint16_t ac, uint16_t av, char *ad);
static bool Azure_PublishProvStatusReq(CALYPSO *calypso, char *operationID);
static bool Azure_PublishRegReq(CALYPSO *calypso);
static char *Azure_SerializeProvReq();
static char *Azure_SerializeVoltageData(float voltage);
static char *Azure_SerializeSendInterval(CALYPSO *calypso, uint16_t val, uint16_t ac, uint16_t av, char *ad);

/**
 * @brief Load configuration from JSON.
 * @param configuration JSON configuration object.
 * @param calypso CALYPSO structure.
 * @retval true if successful, false otherwise.
 */
bool Azure_loadConfiguration(json_value *configuration, CALYPSO *calypso)
{
    strcpy(kitID, configuration->u.object.values[1].value->u.string.ptr);
    strcpy(scopeID, configuration->u.object.values[2].value->u.string.ptr);
    strcpy(dpsServerAddress, configuration->u.object.values[3].value->u.string.ptr);
    strcpy(modelID, configuration->u.object.values[4].value->u.string.ptr);
    strcpy(calypso->settings.sntpSettings.server, configuration->u.object.values[5].value->u.string.ptr);
    strcpy(calypso->settings.sntpSettings.timezone, configuration->u.object.values[6].value->u.string.ptr);
    strcpy(calypso->settings.wifiSettings.SSID, configuration->u.object.values[7].value->u.string.ptr);
    strcpy(calypso->settings.wifiSettings.securityParams.securityKey, configuration->u.object.values[8].value->u.string.ptr);
    calypso->settings.wifiSettings.securityParams.securityType = configuration->u.object.values[9].value->u.integer;
    strcpy(calypso->settings.mqttSettings.clientID, kitID);

    sprintf(calypso->telemetryPubTopic, "devices/%s/messages/events/", kitID);

    // MQTT Settings
    calypso->settings.mqttSettings.flags = ATMQTT_CREATE_FLAGS_URL | ATMQTT_CREATE_FLAGS_SEC;
    calypso->settings.mqttSettings.serverInfo.port = MQTT_PORT_SECURE;

    calypso->settings.mqttSettings.secParams.securityMethod = ATMQTT_SECURITY_METHOD_TLSV1_2;
    calypso->settings.mqttSettings.secParams.cipher = ATMQTT_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA256;
    strcpy(calypso->settings.mqttSettings.secParams.CAFile, ROOT_CA_PATH);
    strcpy(calypso->settings.mqttSettings.secParams.certificateFile, DEVICE_CERT_PATH);
    strcpy(calypso->settings.mqttSettings.secParams.privateKeyFile, DEVICE_KEY_PATH);

    calypso->settings.mqttSettings.connParams.protocolVersion = ATMQTT_PROTOCOL_v3_1_1;
    calypso->settings.mqttSettings.connParams.blockingSend = 0;
    calypso->settings.mqttSettings.connParams.format = Calypso_DataFormat_Base64;
    return true;
}

/**
 * @brief Set the MQTT username for the device.
 * @param calypso CALYPSO structure.
 * @retval None.
 */
void Azure_setUserName(CALYPSO *calypso)
{
    sprintf(calypso->settings.mqttSettings.userOptions.userName, "%s/%s/?api-version=2021-04-12&model-id=%s", calypso->settings.mqttSettings.serverInfo.address, kitID, modelID);
}

/**
 * @brief Provision the device with Azure IoT Central.
 * @param calypso CALYPSO structure.
 * @retval true if successful, false otherwise.
 */
bool Azure_deviceProvision(CALYPSO *calypso)
{
    bool ret = false;
    const char equals[2] = "=";
    char *retryAfter;
    int retryAfterSec = 0;
    sprintf(calypso->settings.mqttSettings.userOptions.userName, "%s/registrations/%s/api-version=2021-06-01&model-id=%s", scopeID, kitID, modelID);
    strcpy(calypso->settings.mqttSettings.serverInfo.address, dpsServerAddress);

    if (!Calypso_fileExists(calypso, ROOT_CA_PATH))
    {
        sprintf(displayText, "Error:Root CA \r\nnot found");
        SH1107_Display(1, 0, 24, displayText);
        SSerial_printf(calypso->serialDebug, "Root CA not found\r\n");
        return false;
    }

    if (!Calypso_fileExists(calypso, DEVICE_CERT_PATH))
    {
        sprintf(displayText, "Error:Device cert \r\nnot found");
        SH1107_Display(1, 0, 24, displayText);
        SSerial_printf(calypso->serialDebug, "Device certificate not found\r\n");
        return false;
    }

    if (!Calypso_fileExists(calypso, DEVICE_KEY_PATH))
    {
        sprintf(displayText, "Error:Device key \r\nnot found");
        SH1107_Display(1, 0, 24, displayText);
        SSerial_printf(calypso->serialDebug, "Device key not found\r\n");
        return false;
    }

    if (Calypso_MQTTconnect(calypso) == false)
    {
        return false;
    }

    if (calypso->status == calypso_MQTT_connected)
    {
        sprintf(displayText, "Connected to DPS");
        SH1107_Display(1, 0, 24, displayText);
        ATMQTT_subscribeTopic_t provResp;
        provResp.QoS = ATMQTT_QOS_QOS1;
        strcpy(provResp.topicString, AZURE_PROVISIONING_RESP_TOPIC);

        if (!Calypso_subscribe(calypso, 0, 1, &provResp))
        {
            SSerial_printf(calypso->serialDebug, "Unable to subscribe to topic\r\n");
        }

        if (Azure_PublishRegReq(calypso))
        {
            json_value *provResponse = Device_GetCloudResponse();
            bool provDone = false;
            if (provResponse != NULL)
            {
                strtok(calypso->subTopicName.data, equals);
                strtok(NULL, equals);
                retryAfter = strtok(NULL, equals);
                retryAfterSec = atoi(retryAfter);
                while (!provDone)
                {
                    if ((retryAfterSec > 1) && (retryAfterSec < 10))
                    {
                        SSerial_printf(calypso->serialDebug, "Retry after %is\r\n", atoi(retryAfter));
                        delay(atoi(retryAfter) * 1000);
                    }
                    else
                    {
                        SSerial_printf(calypso->serialDebug, "5s\r\n");
                        delay(5000);
                    }

                    Azure_PublishProvStatusReq(calypso, provResponse->u.object.values[0].value->u.string.ptr);
                    json_value *provResponse = Device_GetCloudResponse();
                    SSerial_printf(calypso->serialDebug, "%s\r\n", provResponse->u.object.values[1].value->u.string.ptr);
                    strtok(calypso->subTopicName.data, equals);
                    strtok(NULL, equals);
                    retryAfter = strtok(NULL, equals);
                    if (0 == strncmp(provResponse->u.object.values[1].value->u.string.ptr, "assigned", strlen("assigned")))
                    {
                        provDone = true;
                        strcpy(azureEndPointAddress, provResponse->u.object.values[2].value->u.object.values[3].value->u.string.ptr);
                        if (Calypso_writeFile(calypso, DEVICE_END_POINT_ADDRESS, azureEndPointAddress, strlen(azureEndPointAddress)))
                        {
                            ret = true;
                            sprintf(displayText, "Provisioning complete");
                            SH1107_Display(1, 0, 24, displayText);
                            SSerial_printf(calypso->serialDebug, "Provisioning done: Connect to IoT hub %s\r\n", azureEndPointAddress);
                        }
                    }
                    else if (0 == strncmp(provResponse->u.object.values[1].value->u.string.ptr, "failed", strlen("failed")))
                    {
                        provDone = true;
                        ret = false;
                        sprintf(displayText, "Provisioning failed");
                        SH1107_Display(1, 0, 24, displayText);
                        SSerial_printf(calypso->serialDebug, "Provisioning failed\r\n");
                    }
                }

                json_value_free(provResponse);
            }
            else
            {
                SSerial_printf(calypso->serialDebug, "Response empty\r\n");
            }
        }
        Calypso_MQTTDisconnect(calypso);
    }
    else
    {
        sprintf(displayText, "Error: Uanable to connect\r\n to DPS");
        SH1107_Display(1, 0, 24, displayText);
        SSerial_printf(calypso->serialDebug, "Unable to connect to MQTT server \r\n");
    }

    return ret;
}


/**
 * @brief Subscribe to necessary MQTT topics.
 * @param calypso CALYPSO structure.
 * @retval true if successful, false otherwise.
 */
bool Azure_SubscribeToTopics(CALYPSO *calypso)
{
    if (calypso->status == calypso_MQTT_connected)
    {
        ATMQTT_subscribeTopic_t topics[4];
        ATMQTT_subscribeTopic_t twinRes, twinDesiredResp, directMethod;

        twinRes.QoS = ATMQTT_QOS_QOS1;
        strcpy(twinRes.topicString, AZURE_TWIN_RES_TOPIC);

        twinDesiredResp.QoS = ATMQTT_QOS_QOS1;
        strcpy(twinDesiredResp.topicString, AZURE_TWIN_DESIRED_PROP_RES_TOPIC);

        directMethod.QoS = ATMQTT_QOS_QOS1;
        strcpy(directMethod.topicString, AZURE_DIRECT_METHOD_TOPIC);

        topics[0] = twinRes;
        topics[1] = twinDesiredResp;
        topics[2] = directMethod;
        return (Calypso_subscribe(calypso, 0, 3, topics));
    }
    else
    {
        return false;
    }
}

/**
 * @brief Process incoming cloud messages.
 * @param cloudResponse JSON response from the cloud.
 * @param calypso CALYPSO structure.
 * @retval None.
 */
void Azure_ProcessCloudMessage(json_value *cloudResponse, CALYPSO *calypso)
{
    const char backsSlash[2] = "/";
    const char equals[2] = "=";
    char *token;
    char *reqIDstr;
    if(cloudResponse == NULL)
    {
        return;
    }
    if (strstr(calypso->subTopicName.data, "$iothub/twin/res/"))
    {

        strtok(calypso->subTopicName.data, backsSlash);
        strtok(NULL, backsSlash);
        strtok(NULL, backsSlash);
        token = strtok(NULL, backsSlash);

        if (atoi(token) == AZURE_STATUS_CLOUD_SUCCESS)
        {
            SSerial_printf(calypso->serialDebug, "Device property updated successfully!\r\n");
        }
        else if (atoi(token) == AZURE_STATUS_SUCCESS)
        {
            unsigned long desiredVal = 0;
            uint16_t version = 0;
            /*Received response for the properties get request*/
            if (0 == strncmp(cloudResponse->u.object.values[0].value->u.object.values[0].name, "telemetrySendFrequency", strlen("telemetrySendFrequency")))
            {
                desiredVal = (unsigned long)cloudResponse->u.object.values[0].value->u.object.values[0].value->u.integer;
                version = (unsigned long)cloudResponse->u.object.values[0].value->u.object.values[1].value->u.integer;
                /*Defualt value set by the cloud */
                if ((desiredVal > MAX_TELEMETRY_SEND_INTERVAL) || (desiredVal < MIN_TELEMETRY_SEND_INTERVAL))
                {
                    // value out of range, send response
                    Azure_PublishSendInterval(calypso, desiredVal, AZURE_STATUS_BAD_REQUEST, version, "invalid parameter");
                }
                else
                {
                    // set the value
                    telemetrySendInterval = desiredVal * 1000;
                    Azure_PublishSendInterval(calypso, desiredVal, AZURE_STATUS_SUCCESS, version, "success");

                    sprintf(displayText, "Property updated\r\nsend interval: %lu s", desiredVal);
                    SH1107_Display(1, 0, 24, displayText);
                }
            }
            else
            {
                /*No default value available, setting the value from the device*/
                version = (unsigned long)0;
                desiredVal = (unsigned long)DEFAULT_TELEMETRY_SEND_INTEVAL;
                Azure_PublishSendInterval(calypso, desiredVal, AZURE_STATUS_SET_BY_DEV, version, "initialize");
            }
        }
        else
        {
            SSerial_printf(calypso->serialDebug, "Request failed error:%i\r\n", atoi(token));
        }
    }
    else if (strstr(calypso->subTopicName.data, "$iothub/twin/PATCH/properties/desired/"))
    {
        /*Request to update writable property from cloud*/
        unsigned long desiredVal = (unsigned long)cloudResponse->u.object.values[1].value->u.integer;
        uint16_t version = (uint16_t)cloudResponse->u.object.values[0].value->u.integer;

        SSerial_printf(calypso->serialDebug, "desired val %i, version %i\r\n", desiredVal, version);
        if ((desiredVal > MAX_TELEMETRY_SEND_INTERVAL) || (desiredVal < MIN_TELEMETRY_SEND_INTERVAL))
        {
            // value out of range, send response
            Azure_PublishSendInterval(calypso, desiredVal, AZURE_STATUS_BAD_REQUEST, version, "invalid parameter");
        }
        else
        {
            // set the value
            telemetrySendInterval = desiredVal * 1000;
            Azure_PublishSendInterval(calypso, desiredVal, AZURE_STATUS_SUCCESS, version, "success");
            sprintf(displayText, "Property updated\r\nsend interval: %lu s", desiredVal);
            SH1107_Display(1, 0, 24, displayText);
        }
    }
    else if (strstr(calypso->subTopicName.data, "iothub/methods/POST/setLEDColor/"))
    {

        strtok(calypso->subTopicName.data, backsSlash);
        strtok(NULL, backsSlash);
        strtok(NULL, backsSlash);
        strtok(NULL, backsSlash);
        token = strtok(NULL, backsSlash);
        strtok(token, equals);
        reqIDstr = strtok(NULL, equals);

        int red = cloudResponse->u.object.values[0].value->u.integer;
        int green = cloudResponse->u.object.values[1].value->u.integer;
        int blue = cloudResponse->u.object.values[2].value->u.integer;

        /*Direct command to set LED color*/
        if ((red < 0) || (red > 0xFF) ||
            (green < 0) || (green > 0xFF) ||
            (blue < 0) || (blue > 0xFF))
        {
            // value out of range, send response
            Azure_PublishDirectCmdResponse(calypso, AZURE_STATUS_BAD_REQUEST, atoi(reqIDstr));
        }
        else
        {
            // value valid, set and send response
            uint32_t color = ((uint32_t)(red << 16) + (uint32_t)(green << 8) + (uint32_t)blue);
            neopixelSet(color);
            Azure_PublishDirectCmdResponse(calypso, AZURE_STATUS_SUCCESS, atoi(reqIDstr));
            sprintf(displayText, "LED color set\r\nR: %u\r\nG: %u\r\nB: %u", red, green, blue);
            SH1107_Display(1, 0, 16, displayText);
        }
    }
}

/**
 * @brief Publish the device voltage (Read-only property).
 * @param calypso CALYPSO structure.
 * @retval None.
 */
static void Azure_PublishVoltage(CALYPSO *calypso)
{
    float currentVoltage = getBatteryVoltage();

    char *dataSerializedVolt = Azure_SerializeVoltageData(currentVoltage);

    reqID++;
    sprintf(azurepubtopic, "%s%u", AZURE_TWIN_MESSAGE_PATCH, reqID);

    SSerial_printf(calypso->serialDebug, "%s\r\n", dataSerializedVolt);
    if (!Calypso_MQTTPublishData(calypso, azurepubtopic, 1, dataSerializedVolt, strlen(dataSerializedVolt), true))
    {
        SSerial_printf(calypso->serialDebug, "Properties Publish failed\r\n");
        sprintf(displayText, "Error: Property update\r\n failed");
        SH1107_Display(1, 0, 8, displayText);
    }
    else
    {
        sprintf(displayText, "Property updated\r\nBatt voltage: %0.2f V", currentVoltage);
        SH1107_Display(1, 0, 8, displayText);
    }

    lastBattVolt = currentVoltage;
}

/**
 * @brief Publish the software version of the device.
 * @param calypso CALYPSO structure.
 * @retval None.
 */
static void Azure_PublishSWVersion(CALYPSO *calypso)
{
    json_value *fwVersion = json_object_new(2);
    json_object_push(fwVersion, "__t", json_string_new("c"));
    json_object_push(fwVersion, "swVersion", json_string_new(calypso->firmwareVersion));
    json_value *payload = json_object_new(1);
    json_object_push(payload, "calypso", fwVersion);
    memset(azurePayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(azurePayload, payload);
    json_builder_free(payload);
    json_builder_free(fwVersion);

    reqID++;
    azurepubtopic[0] = '\0';
    sprintf(azurepubtopic, "%s%u", AZURE_TWIN_MESSAGE_PATCH, reqID);

    if (!Calypso_MQTTPublishData(calypso, azurepubtopic, 1, azurePayload, strlen(azurePayload), true))
    {
        SSerial_printf(calypso->serialDebug, "Properties Publish failed\r\n");
        sprintf(displayText, "Error: Property update\r\n failed");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sprintf(displayText, "Property updated\r\nVersion:%s ", calypso->firmwareVersion);
        SH1107_Display(1, 0, 8, displayText);
    }
}

/**
 * @brief Publish the unique device identifier (UDID).
 * @param calypso CALYPSO structure.
 * @retval None.
 */
static void Azure_PublishUDID(CALYPSO *calypso)
{
    json_value *jsonUdid = json_object_new(2);
    json_object_push(jsonUdid, "__t", json_string_new("c"));
    json_object_push(jsonUdid, "udid", json_string_new(calypso->udid));

    json_value *payload = json_object_new(1);
    json_object_push(payload, "calypso", jsonUdid);
    memset(azurePayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(azurePayload, payload);
    json_builder_free(payload);
    json_builder_free(jsonUdid);

    reqID++;
    azurepubtopic[0] = '\0';
    sprintf(azurepubtopic, "%s%u", AZURE_TWIN_MESSAGE_PATCH, reqID);

    if (!Calypso_MQTTPublishData(calypso, azurepubtopic, 1, azurePayload, strlen(azurePayload), true))
    {
        SSerial_printf(calypso->serialDebug, "Properties Publish failed\r\n");
        sprintf(displayText, "Error: Property update\r\n failed");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sprintf(displayText, "Property updated\r\nUDID:\r\n%s ", calypso->udid);
        SH1107_Display(1, 0, 8, displayText);
    }
}

/**
 * @brief Publish the MAC address of the device.
 * @param calypso CALYPSO structure.
 * @retval None.
 */
static void Azure_PublishMACAddress(CALYPSO *calypso)
{
    json_value *macAddr = json_object_new(2);
    json_object_push(macAddr, "__t", json_string_new("c"));
    json_object_push(macAddr, "macAddress", json_string_new(calypso->MAC_ADDR));

    json_value *payload = json_object_new(1);
    json_object_push(payload, "calypso", macAddr);
    memset(azurePayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(azurePayload, payload);
    json_builder_free(payload);
    json_builder_free(macAddr);

    reqID++;
    azurepubtopic[0] = '\0';
    sprintf(azurepubtopic, "%s%u", AZURE_TWIN_MESSAGE_PATCH, reqID);

    SSerial_printf(calypso->serialDebug, "%s\r\n", azurePayload);

    if (!Calypso_MQTTPublishData(calypso, azurepubtopic, 1, azurePayload, strlen(azurePayload), true))
    {
        SSerial_printf(calypso->serialDebug, "Properties Publish failed\r\n");
        sprintf(displayText, "Error: Property update\r\n failed");
        SH1107_Display(1, 0, 24, displayText);
    }
    else
    {
        sprintf(displayText, "Property updated\r\nMAC ADDRESS:\r\n%s ", calypso->MAC_ADDR);
        SH1107_Display(1, 0, 8, displayText);
    }
}

/**
 * @brief Publish various properties of the device.
 * @param calypso CALYPSO structure.
 * @retval None.
 */
void Azure_PublishProperties(CALYPSO *calypso)
{

    /*Request the desired values for writable properties*/
    reqID++;
    azurepubtopic[0] = '\0';
    sprintf(azurepubtopic, "%s%u", AZURE_TWIN_GET_TOPIC, reqID);

    if (!Calypso_MQTTPublishData(calypso, azurepubtopic, 1, NULL, 0, true))
    {
        SSerial_printf(calypso->serialDebug, "Properties Publish failed\r\n");
    }

    /*Process the response*/
    Device_processCloudMessage();

    Azure_PublishVoltage(calypso);
    Device_processCloudMessage();

    Azure_PublishMACAddress(calypso);
    Device_processCloudMessage();

    Azure_PublishUDID(calypso);
    Device_processCloudMessage();

    Azure_PublishSWVersion(calypso);
    Device_processCloudMessage();
}

/**
 * @brief Publish response to a direct command.
 * @param calypso CALYPSO structure.
 * @param status Status code of the response.
 * @param requestID ID of the request.
 * @retval None.
 */
static void Azure_PublishDirectCmdResponse(CALYPSO *calypso, int status, int requestID)
{
    azurepubtopic[0] = '\0';
    sprintf(azurepubtopic, "$iothub/methods/res/%i/?$rid=%i", status, requestID);
    if (!Calypso_MQTTPublishData(calypso, azurepubtopic, 1, NULL, 0, true))
    {
        SSerial_printf(calypso->serialDebug, "Publish method response failed\r\n");
    }
}


/**
 * @brief Publish the telemetry send interval (Writable property).
 * @param calypso CALYPSO structure.
 * @param val Value of the send interval.
 * @param ac Acknowledgment code.
 * @param av Acknowledgment version.
 * @param ad Acknowledgment description.
 * @retval None.
 */
static void Azure_PublishSendInterval(CALYPSO *calypso, uint16_t val, uint16_t ac, uint16_t av, char *ad)
{
    reqID++;
    azurepubtopic[0] = '\0';
    sprintf(azurepubtopic, "%s%u", AZURE_TWIN_MESSAGE_PATCH, reqID);
    char *dataSerializedInterval = Azure_SerializeSendInterval(calypso, val, ac, av, ad);
    SSerial_printf(calypso->serialDebug, "%s\r\n", dataSerializedInterval);
    if (!Calypso_MQTTPublishData(calypso, azurepubtopic, 1, dataSerializedInterval, strlen(dataSerializedInterval), true))
    {
        SSerial_printf(calypso->serialDebug, "Properties Publish failed\r\n");
    }
}


/**
 * @brief Publish device provisioning status request.
 * @param calypso CALYPSO structure.
 * @param operationID Operation ID for the provisioning status request.
 * @retval true if successful, false otherwise.
 */
static bool Azure_PublishProvStatusReq(CALYPSO *calypso, char *operationID)
{
    bool ret = false;
    char provStatusTopic[256];
    char *payload = 0;

    reqID++;
    sprintf(provStatusTopic, "%s%u&operationId=%s", AZURE_PROVISIONING_STATUS_REQ_TOPIC, reqID, operationID);

    if (!Calypso_MQTTPublishData(calypso, provStatusTopic, 1, payload, 0, true))
    {
        ret = false;
        SSerial_printf(calypso->serialDebug, "Provision Publish failed\n\r");
    }
    else
    {
        ret = true;
    }
    return ret;
}

/**
 * @brief Publish device registration request.
 * @param calypso CALYPSO structure.
 * @retval true if successful, false otherwise.
 */
static bool Azure_PublishRegReq(CALYPSO *calypso)
{
    bool ret = false;
    char provReqTopic[128];

    reqID++;
    sprintf(provReqTopic, "%s%u", AZURE_PROVISIONING_REG_REQ_TOPIC, reqID);

    char *provReq = Azure_SerializeProvReq();

    if (!Calypso_MQTTPublishData(calypso, provReqTopic, 1, provReq, strlen(provReq), true))
    {
        ret = false;
        SSerial_printf(calypso->serialDebug, "Provision Publish failed\n\r");
    }
    else
    {
        ret = true;
    }
    return ret;
}

/**
 * @brief Serialize provisioning request data.
 * @retval Pointer to serialized data.
 */
static char *Azure_SerializeProvReq()
{
    json_value *prov_payload_ = json_object_new(1);
    json_value *prov_modelID_ = json_object_new(1);

    json_object_push(prov_modelID_, "modelId",
                        json_string_new(modelID));
    json_object_push(prov_payload_, "registrationId",
                        json_string_new(kitID));
    json_object_push(prov_payload_, "payload",
                        prov_modelID_);

    memset(azurePayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(azurePayload, prov_payload_);

    json_builder_free(prov_payload_);
    json_builder_free(prov_modelID_);

    return azurePayload;
}

/**
 * @brief Serialize voltage data to send.
 * @param voltage Voltage value to serialize.
 * @retval Pointer to serialized data.
 */
static char *Azure_SerializeVoltageData(float voltage)
{
    json_value *payload = json_object_new(1);
    json_object_push(payload, "batteryVoltage", json_double_new(voltage));

    memset(azurePayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(azurePayload, payload);

    json_builder_free(payload);
    return azurePayload;
}

/**
 * @brief Serialize telemetry send interval data.
 * @param calypso CALYPSO structure.
 * @param val Value of the send interval.
 * @param ac Acknowledgment code.
 * @param av Acknowledgment version.
 * @param ad Acknowledgment description.
 * @retval Pointer to serialized data.
 */
static char *Azure_SerializeSendInterval(CALYPSO *calypso, uint16_t val, uint16_t ac, uint16_t av, char *ad)
{
    json_value *payload = json_object_new(1);
    json_value *obj = json_object_new(1);

    json_object_push(obj, "value", json_integer_new(val));
    json_object_push(obj, "ac", json_integer_new(ac));
    json_object_push(obj, "av", json_integer_new(av));
    json_object_push(obj, "ad", json_string_new(ad));

    json_object_push(payload, "telemetrySendFrequency", obj);
    memset(azurePayload, 0, MAX_PAYLOAD_LENGTH);
    json_serialize(azurePayload, payload);

    json_builder_free(payload);
    json_builder_free(obj);
    return azurePayload;
}
