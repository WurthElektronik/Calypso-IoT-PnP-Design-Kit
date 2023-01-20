/**
 * \file
 * \brief API implementation for the Gateway of the WE IoT design kit.
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

/**         Functions definition         */

#ifdef __cplusplus
extern "C"
{
#endif

#define KIT_ID "calypso-test-dev-1"
/*Root CA certificate to create the TLS connection*/
#define BALTIMORE_CYBERTRUST_ROOT_CERT "-----BEGIN CERTIFICATE-----\n\
MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n\
RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n\
VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n\
DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n\
ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n\
VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n\
mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n\
IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n\
mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n\
XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n\
dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n\
jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n\
BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n\
DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n\
9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n\
jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n\
Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n\
ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n\
R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n\
-----END CERTIFICATE-----"

#define DIGICERT_GLOBAL_ROOT_G2_CERT "-----BEGIN CERTIFICATE-----\n\
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n\
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n\
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n\
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n\
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n\
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n\
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n\
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n\
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n\
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n\
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n\
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n\
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n\
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n\
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n\
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n\
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n\
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n\
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n\
MrY=\n\
-----END CERTIFICATE-----"

/*Device certificate chain*/
#define DEVICE_CERT "-----BEGIN CERTIFICATE-----\n\
-----END CERTIFICATE-----\n\
-----BEGIN CERTIFICATE-----\n\
-----END CERTIFICATE-----"

/*Device key*/
#define DEVICE_KEY "-----BEGIN EC PRIVATE KEY-----\n\
-----END EC PRIVATE KEY-----"

#define CONFIGURATION_DATA "{\n\
  \"version\": 1,\n\
	\"deviceId\": \"Calypso-129001293\",\n\
	\"scopeId\": \"0ne006E0511\",\n\
	\"DPSServer\": \"global.azure-devices-provisioning.net\",\n\
	\"modelId\": \"dtmi:wuerthelektronik:designkit:calypsoiotkit;1\",\n\
	\"SNTPServer\": \"0.de.pool.ntp.org\",\n\
	\"timezone\": \"60\",\n\
  \"WiFiSSID\": \"SSID\",\n\
  \"WiFiPassword\": \"password\",\n\
  \"WiFiSecurity\":3\n\
}"

#define AZURE_IOT_PNP_CONFIG_VERSION 1

/*Parameters from the IoT central application*/
#define SCOPE_ID "0aa000A0000"
#define MODEL_ID "dtmi:wuerthelektronik:designkit:calypsoiotkit;1"

/*File path to certificate files stored on Calypso internal storage*/
#define ROOT_CA_PATH "user/azrootca"
#define ROOT_CA_1_PATH "user/azrootca1"
#define DEVICE_CERT_PATH "user/azdevcert"
#define DEVICE_KEY_PATH "user/azdevkey"
#define DEVICE_IOT_HUB_ADDRESS "user/iotHubAddr"
#define CONFIG_FILE_PATH "user/azdevconf"

// Button labelled C on the OLED display
#define BUTTON_C (byte)5

/*Wi-Fi settings*/
#define WI_FI_CONNECT_DELAY 5000UL

/*MQTT settings*/
#define DPS_SERVER_ADDRESS "global.azure-devices-provisioning.net"
#define MQTT_PORT_SECURE 8883
#define MQTT_TLS_VERSION "TLSV1_2"
#define MQTT_CIPHER "TLS_RSA_WITH_AES_256_CBC_SHA256"

#define DEVICE_CREDENTIALS_MAX_LEN 64
#define MAX_URL_LEN 128
#define DEVICE_CLAIM_DURATION 180000

// MQTT Topics
#define DEVICE_TWIN_DESIRED_PROP_RES_TOPIC "$iothub/twin/PATCH/properties/desired/#"
#define DEVICE_TWIN_RES_TOPIC "$iothub/twin/res/#"
#define DIRECT_METHOD_TOPIC "$iothub/methods/POST/#"
#define DEVICE_TWIN_MESSAGE_PATCH "$iothub/twin/PATCH/properties/reported/?$rid="
#define DEVICE_TWIN_GET_TOPIC "$iothub/twin/GET/?$rid="

#define PROVISIONING_RESP_TOPIC "$dps/registrations/res/#"
#define PROVISIONING_REG_REQ_TOPIC "$dps/registrations/PUT/iotdps-register/?$rid="
#define PROVISIONING_STATUS_REQ_TOPIC "$dps/registrations/GET/iotdps-get-operationstatus/?$rid="

#define SNTP_TIMEZONE "+120"
#define SNTP_SERVER "0.de.pool.ntp.org"

#define DEFAULT_TELEMETRY_SEND_INTEVAL 30 // seconds
#define MAX_TELEMETRY_SEND_INTERVAL 600   // seconds
#define MIN_TELEMETRY_SEND_INTERVAL 3     // seconds

#define STATUS_SUCCESS 200
#define STATUS_UPDATE_IN_PROGRESS 202
#define STATUS_SET_BY_DEV 203
#define STATUS_BAD_REQUEST 400
#define STATUS_EXCEPTION 500
#define STATUS_CLOUD_SUCCESS 204
#define STATUS_TOO_MANY_REQUESTS 429

  extern bool sensorsPresent;
  extern volatile unsigned long telemetrySendInterval;
  TypeSerial *Device_init(void *Debug, void *CalypsoSerial);
  void Device_writeConfigFiles();
  bool Device_isConfigured();
  bool Device_isConnectedToWiFi();
  bool Device_isProvisioned();
  void Device_MQTTConnect();
  void Device_readSensors();
  void Device_PublishSensorData();
  void Device_PublishProperties();
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
  bool Device_ConfigurationComplete();
  void Device_displaySensorData();
#ifdef __cplusplus
}
#endif

#endif /* P_N_P_DEVICE_H */