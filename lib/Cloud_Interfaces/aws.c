/**
 * \file
 * \brief API implementation AWS.
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
#include "aws.h"

static char displayText[128];
/**
 * @brief Load configuration from JSON.
 * @param configuration JSON configuration object.
 * @param calypso CALYPSO structure.
 * @retval true if successful, false otherwise.
 */
bool AWS_loadConfiguration(json_value *configuration, CALYPSO *calypso)
{
    char endPointAddress[MAX_URL_LEN] = {0};
    
    strcpy(calypso->settings.mqttSettings.clientID, configuration->u.object.values[1].value->u.string.ptr);
    strcpy(endPointAddress, configuration->u.object.values[2].value->u.string.ptr);

    if (!Calypso_fileExists(calypso, DEVICE_END_POINT_ADDRESS))
    {
        if (!Calypso_writeFile(calypso, DEVICE_END_POINT_ADDRESS, endPointAddress, strlen(endPointAddress)))
        {
            return false;
        }
    }
    strcpy(calypso->settings.sntpSettings.server, configuration->u.object.values[3].value->u.string.ptr);
    strcpy(calypso->settings.sntpSettings.timezone, configuration->u.object.values[4].value->u.string.ptr);
    strcpy(calypso->settings.wifiSettings.SSID, configuration->u.object.values[5].value->u.string.ptr);
    strcpy(calypso->settings.wifiSettings.securityParams.securityKey, configuration->u.object.values[6].value->u.string.ptr);
    calypso->settings.wifiSettings.securityParams.securityType = configuration->u.object.values[7].value->u.integer;

    // MQTT Settings
    calypso->settings.mqttSettings.flags = ATMQTT_CREATE_FLAGS_URL | ATMQTT_CREATE_FLAGS_SEC ;
    calypso->settings.mqttSettings.serverInfo.port = MQTT_PORT_SECURE;

    calypso->settings.mqttSettings.secParams.securityMethod = ATMQTT_SECURITY_METHOD_TLSV1_2;
    calypso->settings.mqttSettings.secParams.cipher = ATMQTT_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA256;
    strcpy(calypso->settings.mqttSettings.secParams.CAFile, ROOT_CA_PATH);
    strcpy(calypso->settings.mqttSettings.secParams.certificateFile, DEVICE_CERT_PATH);
    strcpy(calypso->settings.mqttSettings.secParams.privateKeyFile, DEVICE_KEY_PATH);

    calypso->settings.mqttSettings.connParams.protocolVersion = ATMQTT_PROTOCOL_v3_1_1;
    calypso->settings.mqttSettings.connParams.blockingSend = 0;
    calypso->settings.mqttSettings.connParams.format = Calypso_DataFormat_Base64;

    sprintf(calypso->telemetryPubTopic, AWS_TELEMETRY_PUBLISH_TOPIC, calypso->settings.mqttSettings.clientID);
    return true;
}

/**
 * @brief Subscribe to necessary MQTT topics.
 * @param calypso CALYPSO structure.
 * @retval true if successful, false otherwise.
 */
bool AWS_SubscribeToTopics(CALYPSO *calypso)
{
    if (calypso->status == calypso_MQTT_connected)
    {
        ATMQTT_subscribeTopic_t commandTopic;

        commandTopic.QoS = ATMQTT_QOS_QOS1;
        sprintf(commandTopic.topicString, AWS_COMMAND_TOPIC, calypso->settings.mqttSettings.clientID);

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
void AWS_ProcessCloudMessage(json_value *cloudResponse, CALYPSO *calypso)
{
    if(cloudResponse == NULL)
    {
        return;
    }
    char cmdTopic[128];
    sprintf(cmdTopic, AWS_COMMAND_TOPIC, calypso->settings.mqttSettings.clientID);
    if (strstr(calypso->subTopicName.data, AWS_COMMAND_TOPIC))
    {
        int red = cloudResponse->u.object.values[0].value->u.integer;
        int green = cloudResponse->u.object.values[1].value->u.integer;
        int blue = cloudResponse->u.object.values[2].value->u.integer;

        if ((red < 0) || (red > 0xFF) ||
        (green < 0) || (green > 0xFF) ||
        (blue < 0) || (blue > 0xFF))
        {
            // value out of range, send response
            sprintf(displayText, "Values out of range");
            SH1107_Display(1, 0, 16, displayText);
        }
        else
        {
            // value valid, set and send response
            uint32_t color = ((uint32_t)(red << 16) + (uint32_t)(green << 8) + (uint32_t)blue);
            neopixelSet(color);
            sprintf(displayText, "LED color set\r\nR: %u\r\nG: %u\r\nB: %u", red, green, blue);
            SH1107_Display(1, 0, 16, displayText);
        }

    }
}