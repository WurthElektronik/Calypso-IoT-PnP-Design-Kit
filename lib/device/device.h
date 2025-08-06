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

#ifndef P_N_P_DEVICE_H
#define P_N_P_DEVICE_H

/**         Includes         */

#include <stdint.h>
#include "calypsoBoard.h"
#include "ConfigPlatform.h"
#include "sensorBoard.h"
#include "json-builder.h"

/**         Functions definition         */

#ifdef __cplusplus
extern "C"
{
#endif

#define CONFIG_FILE_PATH "user/devconf"


#define HOST_FIRMWARE_VERSION "2.1.0"

#define MOSQUITTO_CONFIG_VERSION 0
#define AZURE_IOT_PNP_CONFIG_VERSION 1
#define AWS_IOT_CORE_CONFIG_VERSION 2
#define KAA_IOT_CONFIG_VERSION 3

// Button labelled C on the OLED display
#define BUTTON_C (byte)5
#define BUTTON_B (byte)6
#define BUTTON_A (byte)9

/*Wi-Fi settings*/
#define WI_FI_CONNECT_DELAY 5000UL

#define MAX_PAYLOAD_LENGTH 1024

#define DEVICE_CREDENTIALS_MAX_LEN 64
#define MAX_URL_LEN 128

/*File path to certificate files stored on Calypso internal storage*/
#define ROOT_CA_PATH "user/rootca"
#define DEVICE_CERT_PATH "user/devcert"
#define DEVICE_KEY_PATH "user/devkey"
#define DEVICE_END_POINT_ADDRESS "user/endPointAddr"

#define MQTT_PORT_UNSECURE 1883
#define MQTT_PORT_SECURE 8883
#define MQTT_TLS_VERSION "TLSV1_2"

#define DEFAULT_TELEMETRY_SEND_INTEVAL 30 // seconds
#define MAX_TELEMETRY_SEND_INTERVAL 600   // seconds
#define MIN_TELEMETRY_SEND_INTERVAL 3     // seconds


#define CALYPSO_FIRMWARE_MIN_MAJOR_VERSION 2
#define CALYPSO_FIRMWARE_MIN_MINOR_VERSION 2

  extern bool sensorsPresent;
  extern volatile unsigned long telemetrySendInterval;
  TypeSerial *Device_init(void *Debug, void *CalypsoSerial);
  void Device_writeConfigFiles();
  bool Device_isConfigured();
  bool Device_isConnectedToWiFi();
  bool Device_isProvisioned();
  void Device_ConnectToCloud();
  void Device_readSensors();
  void Device_PublishSensorData();
  void Device_listOfFiles();
  void Device_connect_WiFi();
  void Device_disconnect_WiFi();
  void Device_WiFi_provisioning();
  bool Device_provision();
  void Device_configurationInProgress();
  bool Device_SubscribeToTopics();
  void Device_reset();
  void Device_restart();
  bool Device_isStatusOK();
  void Device_processCloudMessage();
  void Device_displaySensorData();
  bool Device_isUpToDate();
  json_value *Device_GetCloudResponse();
#ifdef __cplusplus
}
#endif

#endif /* P_N_P_DEVICE_H */