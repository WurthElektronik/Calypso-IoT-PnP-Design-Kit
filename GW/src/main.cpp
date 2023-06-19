/**
 * \file
 * \brief Main file for the gateway application of the WE IoT design kit.
 *
 * \copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * \page License
 *
 * THE SOFTWARE INCLUDING THE SOURCE CODE IS PROVIDED “AS IS”. YOU ACKNOWLEDGE
 * THAT WÜRTH ELEKTRONIK EISOS MAKES NO REPRESENTATIONS AND WARRANTIES OF ANY
 * KIND RELATED TO, BUT NOT LIMITED TO THE NON-INFRINGEMENT OF THIRD PARTIES’
 * INTELLECTUAL PROPERTY RIGHTS OR THE MERCHANTABILITY OR FITNESS FOR YOUR
 * INTENDED PURPOSE OR USAGE. WÜRTH ELEKTRONIK EISOS DOES NOT WARRANT OR
 * REPRESENT THAT ANY LICENSE, EITHER EXPRESS OR IMPLIED, IS GRANTED UNDER ANY
 * PATENT RIGHT, COPYRIGHT, MASK WORK RIGHT, OR OTHER INTELLECTUAL PROPERTY
 * RIGHT RELATING TO ANY COMBINATION, MACHINE, OR PROCESS IN WHICH THE PRODUCT
 * IS USED. INFORMATION PUBLISHED BY WÜRTH ELEKTRONIK EISOS REGARDING
 * THIRD-PARTY PRODUCTS OR SERVICES DOES NOT CONSTITUTE A LICENSE FROM WÜRTH
 * ELEKTRONIK EISOS TO USE SUCH PRODUCTS OR SERVICES OR A WARRANTY OR
 * ENDORSEMENT THEREOF
 *
 * THIS SOURCE CODE IS PROTECTED BY A LICENSE.
 * FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED
 * IN THE ROOT DIRECTORY OF THIS PACKAGE
 */

#include "PnP_Device.h"

// Serial Ports
//------User Debug Interface
TypeSerial *Debug;

enum
{
    invalidFirmwareVersion,
    waitingForConfig,
    configuringDevice,
    provisioning,
    connectingToCloud,
    idle,
    sendSensorData,
    errorState,
    factoryReset,
    numOfStatus
} statusFlag;

/*Timer to periodically send sensor data*/
unsigned long startTime = 0;
unsigned long interval = 0;
volatile uint8_t buttonPressCount = 0;

char displayText[100];

// Long press callback
void OnBtnLongPress()
{
    statusFlag = factoryReset;
}

// Switch the device to configuration mode
void OnBtnPress()
{
    buttonPressCount++;
    if (buttonPressCount >= 1)
    {
        if (statusFlag == waitingForConfig)
        {
            statusFlag = configuringDevice;
        }
        buttonPressCount = 0;
    }
}

void setup()
{
    delay(1000);

    /*Initialize the OLED display*/
    SH1107_Init();

    sprintf(displayText, "Intializing device...");
    SH1107_Display(1, 0, 24, displayText);

    // Initialize the Calypso Wi-Fi module, sensors and the serial port for debug
    Debug = Device_init(&Serial, &Serial1);

    // Initialize the button S2
    buttonInit(BUTTON_C, OnBtnPress, OnBtnLongPress);

    /*Initialize the Neo-pixel LED*/
    neopixelInit();
    neopixelSet(NEO_PIXEL_RED);
    if (Device_isUpToDate())
    {

        /*Check if the device is connected to Wi-Fi and has the config file*/
        if ((!Device_isConnectedToWiFi()) || (!Device_isConfigured()))
        {
            statusFlag = waitingForConfig;
            SSerial_printf(Debug, "Waiting for a device configuration...\r\n");
            SSerial_printf(Debug, "Push the button C twice to enter configuration mode\r\n");
            sprintf(displayText, "Device not configured\r\n\r\nTo configure\r\n\r\ndouble press\r\n\r\n<- button C");
            SH1107_Display(1, 0, 0, displayText);
            neopixelSet(NEO_PIXEL_RED);
        }
        else
        {
            /*Check if the device is provisioned in the cloud*/
            if (Device_isProvisioned() == true)
            {
                /*Connect to IoT hub directly*/
                SSerial_printf(Debug, "Device is provisioned\r\n");
                sprintf(displayText, "Device is provisioned");
                SH1107_Display(1, 0, 24, displayText);
                statusFlag = connectingToCloud;
            }
            else
            {
                /*Start provisioning*/
                sprintf(displayText, "Provisioning in Progress...");
                SH1107_Display(1, 0, 24, displayText);
                statusFlag = provisioning;
            }
        }
    }
    else
    {
        statusFlag = invalidFirmwareVersion;
    }
}

void loop()
{
    switch (statusFlag)
    {
    case invalidFirmwareVersion:
    {
        neopixelSet(NEO_PIXEL_RED);
        /*The kit requires a minimum software version on Calypso*/
        SSerial_printf(Debug, "Older firmware detected\r\n");
        sprintf(displayText, "Calypso Firmware old \r\n\r\nUpdate Calypso");
        SH1107_Display(1, 0, 16, displayText);
        delay(5000);
        break;
    }
    break;
    case waitingForConfig:
        /*Waiting on button press from the user*/
        break;
    case configuringDevice:
    {
        Device_WiFi_provisioning();
        Device_configurationInProgress();
        neopixelSet(NEO_PIXEL_RED);
        delay(3500);
        neopixelSet(NEO_PIXEL_OFF);
        delay(3500);
    }
    break;
    case provisioning:
    {
        neopixelSet(NEO_PIXEL_ORANGE);
        if (Device_provision())
        {
            statusFlag = connectingToCloud;
        }
        else
        {
            sprintf(displayText, "Error:Provisioning failed");
            SH1107_Display(1, 0, 24, displayText);
            statusFlag = errorState;
        }
    }
    break;
    case connectingToCloud:
    {
        neopixelSet(NEO_PIXEL_ORANGE);
        sprintf(displayText, "Connecting to \r\nIoT central...");
        SH1107_Display(1, 0, 24, displayText);
        /*Get the access token*/
        if (Device_isProvisioned() == true)
        {
            /*Connect to the cloud*/
            Device_MQTTConnect();
            Device_SubscribeToTopics();
            Device_PublishProperties();
            neopixelSet(NEO_PIXEL_GREEN);
            statusFlag = idle;
        }
        break;
    }
    case idle:
    {
        if (!Device_isStatusOK())
        {
            Device_restart();
        }
        Device_processCloudMessage();
        if (sensorsPresent == true)
        {
            interval = micros() - startTime;
            if (interval >= (telemetrySendInterval * 1000))
            {
                /*Time to send sensor data*/
                statusFlag = sendSensorData;
                startTime = micros();
                interval = 0;
            }
        }
    }
    break;
    case sendSensorData:
    {
        SSerial_printf(Debug, "Publishing sensor data...\r\n");
        Device_PublishSensorData();
        Device_displaySensorData();
        statusFlag = idle;
        break;
    }
    case errorState:
    {
        neopixelSet(NEO_PIXEL_RED);
        /*End up here only if the device is not configured correctly*/
        SSerial_printf(Debug, "Error, unknown state...\r\n");
        sprintf(displayText, "Error state: \r\nReset/reconfigure device");
        SH1107_Display(1, 0, 24, displayText);
        delay(1000);
        break;
    }
    case factoryReset:
    {
        sprintf(displayText, "Reset device to \r\nFactory state");
        SH1107_Display(1, 0, 24, displayText);
        neopixelSet(NEO_PIXEL_RED);
        Device_reset();
    }
    break;
    default:
        break;
    }
    buttonUpdate();
}
