/**
 * \file
 * \brief API implementation for the Kaa IoT.
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
#include "kaaiot.h"

static char displayText[128];
static char appVersion[32];
static char kitID[128];
static char endPointToken[64];
static char cmdResponseData[MAX_PAYLOAD_LENGTH];

static void Kaa_PublishDirectCmdResponse(CALYPSO *calypso, char *appVersion, char *token, char *commandType, int requestId, int statusCode, char *reasonPhrase);
static char *Kaa_CommandResponseData(CALYPSO *calypso, int requestId, int statusCode, char *reasonPhrase);
/**
 * @brief Load configuration from JSON.
 * @param configuration JSON configuration object.
 * @param calypso CALYPSO structure.
 * @retval true if successful, false otherwise.
 */
bool Kaa_loadConfiguration(json_value *configuration, CALYPSO *calypso)
{
    char endPointAddress[MAX_URL_LEN] = {0};
    strcpy(kitID, configuration->u.object.values[1].value->u.string.ptr);
    strcpy(endPointAddress, configuration->u.object.values[2].value->u.string.ptr);

    if (!Calypso_fileExists(calypso, DEVICE_END_POINT_ADDRESS))
    {
        if (!Calypso_writeFile(calypso, DEVICE_END_POINT_ADDRESS, endPointAddress, strlen(endPointAddress)))
        {
            return false;
        }
    }
    strcpy(endPointToken, configuration->u.object.values[3].value->u.string.ptr);
    strcpy(appVersion, configuration->u.object.values[4].value->u.string.ptr);
    strcpy(calypso->settings.mqttSettings.clientID, endPointToken);

    strcpy(calypso->settings.sntpSettings.server, configuration->u.object.values[5].value->u.string.ptr);
    strcpy(calypso->settings.sntpSettings.timezone, configuration->u.object.values[6].value->u.string.ptr);
    strcpy(calypso->settings.wifiSettings.SSID, configuration->u.object.values[7].value->u.string.ptr);
    strcpy(calypso->settings.wifiSettings.securityParams.securityKey, configuration->u.object.values[8].value->u.string.ptr);
    calypso->settings.wifiSettings.securityParams.securityType = configuration->u.object.values[9].value->u.integer;

    // MQTT Settings
    calypso->settings.mqttSettings.flags = ATMQTT_CREATE_FLAGS_URL ;// | ATMQTT_CREATE_FLAGS_SEC | ATMQTT_CREATE_FLAGS_SKIP_DATE_VERIFY | ATMQTT_CREATE_FLAGS_SKIP_CERT_VERIFY | ATMQTT_CREATE_FLAGS_SKIP_DOMAIN_VERIFY;
 
    calypso->settings.mqttSettings.serverInfo.port = MQTT_PORT_UNSECURE;

    /*calypso->settings.mqttSettings.secParams.securityMethod = ATMQTT_SECURITY_METHOD_TLSV1_2;
    calypso->settings.mqttSettings.secParams.cipher = ATMQTT_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA;
    strcpy(calypso->settings.mqttSettings.secParams.CAFile, ROOT_CA_PATH);*/

    calypso->settings.mqttSettings.connParams.protocolVersion = ATMQTT_PROTOCOL_v3_1_1;
    calypso->settings.mqttSettings.connParams.blockingSend = 0;
    calypso->settings.mqttSettings.connParams.format = Calypso_DataFormat_Base64;

    sprintf(calypso->telemetryPubTopic, KAA_TELEMETRY_PUBLISH_TOPIC, appVersion, endPointToken);

    return true;
}

/**
 * @brief Subscribe to necessary MQTT topics.
 * @param calypso CALYPSO structure.
 * @retval true if successful, false otherwise.
 */
bool Kaa_SubscribeToTopics(CALYPSO *calypso)
{
    if (calypso->status == calypso_MQTT_connected)
    {
        ATMQTT_subscribeTopic_t commandTopic;

        commandTopic.QoS = ATMQTT_QOS_QOS1;
        sprintf(commandTopic.topicString, KAA_COMMANDS_TOPIC, appVersion, endPointToken);

        return (Calypso_subscribe(calypso, 0, 1, &commandTopic));
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
void Kaa_ProcessCloudMessage(json_value *cloudResponse, CALYPSO *calypso)
{

    const char backsSlash[2] = "/";
    char *msgAppVersion;
    char *msgToken;
    char *msgCommandType;
    char *topicStatus;
    int commandId;

    if(cloudResponse == NULL)
    {
        return;
    }


    strtok(calypso->subTopicName.data, backsSlash);
    msgAppVersion = strtok(NULL, backsSlash);
    strtok(NULL, backsSlash);
    msgToken = strtok(NULL, backsSlash);
    strtok(NULL, backsSlash);
    msgCommandType = strtok(NULL, backsSlash);
    topicStatus = strtok(NULL, backsSlash);

    SSerial_printf(calypso->serialDebug, "Commands received. Type: %s, appVersion: %s, token: %s, topic Status: %s.\r\n", msgCommandType, msgAppVersion, msgToken, topicStatus);

    if (strstr(msgCommandType, "setled"))
    {

        if (cloudResponse->type == json_array)
        {
            json_value *commandElement = cloudResponse->u.array.values[0];
            json_value *commandPayloadJson = commandElement->u.object.values[1].value;

            commandId = commandElement->u.object.values[0].value->u.integer;

            int red = commandPayloadJson->u.object.values[0].value->u.integer;
            int green = commandPayloadJson->u.object.values[1].value->u.integer;
            int blue = commandPayloadJson->u.object.values[2].value->u.integer;

            if ((red < 0) || (red > 0xFF) ||
            (green < 0) || (green > 0xFF) ||
            (blue < 0) || (blue > 0xFF))
            {
                // value out of range, send response
                sprintf(displayText, "Values out of range");
                SH1107_Display(1, 0, 16, displayText);
                Kaa_PublishDirectCmdResponse(calypso, appVersion, msgToken, msgCommandType, commandId, 400, "Payload invalid");
            }
            else
            {
                // value valid, set and send response
                uint32_t color = ((uint32_t)(red << 16) + (uint32_t)(green << 8) + (uint32_t)blue);
                neopixelSet(color);
                sprintf(displayText, "LED color set\r\nR: %u\r\nG: %u\r\nB: %u", red, green, blue);
                SH1107_Display(1, 0, 16, displayText);
                Kaa_PublishDirectCmdResponse(calypso, appVersion, msgToken, msgCommandType, commandId, 200, "OK");
            }

            
        }

    }

}

/**
 * @brief  Publish response to the command
 * @retval None
 */
static void Kaa_PublishDirectCmdResponse(CALYPSO *calypso, char *appVersion, char *token, char *commandType, int requestId, int statusCode, char *reasonPhrase)
{
    char *responseData = Kaa_CommandResponseData(calypso, requestId, statusCode, reasonPhrase);

    char pubtopic[MQTT_MAX_TOPIC_LENGTH];
    sprintf(pubtopic, KAA_COMMANDS_RESPONSE_TOPIC, appVersion, token, commandType);
    if (!Calypso_MQTTPublishData(calypso, pubtopic, 1, responseData, strlen(responseData), true))
    {
        SSerial_printf(calypso->serialDebug, "Publish command response failed\r\n");
    }
}

/**
 * @brief  Gets command response data
 * @retval Pointer to response data
 */
static char *Kaa_CommandResponseData(CALYPSO *calypso, int requestId, int statusCode, char *reasonPhrase)
{
    json_value *payloadArray = json_array_new(1);
    json_value *payload = json_object_new(4);
    if (payload == NULL)
    {
        SSerial_printf(calypso->serialDebug, "Payload memory full \r\n");
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
        SSerial_printf(calypso->serialDebug, "cmdPayload memory full \r\n");
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
