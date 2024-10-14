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

#include "PnP_Common_Device.h"

// Serial Ports
//------User Debug Interface
TypeSerial *Debug;

enum
{
    invalidFirmwareVersion,
    waitingForConfig,
    configuringDevice,
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

char displayText[150];
char displayEventText[120];

extern IoT_platforms_t platform;

bool previousConfigDeleted = false;

// Long press callback
void OnBtnLongPress_A()
{
}

void OnBtnLongPress_B()
{
    // switch to another platform
    if ((platform == AZURE) && Device_writeConfigFile(PLATFORM_CONFIG_FILE_PATH, "{\"platform\":\"KAAIOT\"}"))
    {
        SSerial_printf(Debug, "Platform config switched to KAAIOT. Reboot device to apply changes\r\n");
        sprintf(displayEventText, "Platform config\r\n\r\nswitched to KAAIOT.\r\n\r\nReboot device\r\n\r\nto apply changes");
        Device_displayMessageWithDelay(displayEventText);
        if (statusFlag == waitingForConfig)
        {
            SH1107_Display(1, 0, 0, displayText);
        }
    }
    else if ((platform == KAAIOT) && Device_writeConfigFile(PLATFORM_CONFIG_FILE_PATH, "{\"platform\":\"AZURE\"}"))
    {
        SSerial_printf(Debug, "Platform config switched to AZURE. Reboot device to apply changes\r\n");
        sprintf(displayEventText, "Platform config\r\n\r\nswitched to AZURE.\r\n\r\nReboot device\r\n\r\nto apply changes");
        Device_displayMessageWithDelay(displayEventText);
        if (statusFlag == waitingForConfig)
        {
            SH1107_Display(1, 0, 0, displayText);
        }
    }
    else
    {
        SSerial_printf(Debug, "Unknown platform selected\r\n");
        return;
    }
}

void OnBtnLongPress_C()
{
    statusFlag = factoryReset;
}

void OnBtnPress_A()
{
}

void OnBtnPress_B()
{
}

// Switch the device to configuration mode
void OnBtnPress_C()
{
    buttonPressCount++;
    if (buttonPressCount >= 1)
    {
        if (statusFlag == waitingForConfig)
        {
            statusFlag = configuringDevice;
            previousConfigDeleted = true;
        }
        buttonPressCount = 0;
    }
}

void setup()
{
    delay(5000);

    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);

    /*Initialize the OLED display*/
    SH1107_Init();

    sprintf(displayText, "Initializing device\r\n...");
    SH1107_Display(1, 0, 24, displayText);

    // Initialize the Calypso Wi-Fi module, sensors and the serial port for debug
    Debug = Device_init(&Serial, &Serial1);

    // Initialize the button S2
    buttonInit(BUTTON_A_ID, BUTTON_A, OnBtnPress_A, OnBtnLongPress_A);
    buttonInit(BUTTON_B_ID, BUTTON_B, OnBtnPress_B, OnBtnLongPress_B);
    buttonInit(BUTTON_C_ID, BUTTON_C, OnBtnPress_C, OnBtnLongPress_C);

    /*Initialize the Neo-pixel LED*/
    neopixelInit();
    neopixelSet(NEO_PIXEL_RED);

    if (!Device_isIotPlatformConfigured())
    {
        SSerial_printf(Debug, "Device IoT platform not configured. Use web menu to configure.\r\n");
        sprintf(displayText, "Error! IoT platform\r\n\r\nnot configured\r\n\r\nUse following info\r\n\r\nto configure.");
        neopixelSet(NEO_PIXEL_RED);
        Device_displayMessageWithDelay(displayText);
    }

    if (Device_isUpToDate())
    {
        if (!Device_isConfigured()) /*Check if the device has the config file*/
        {
            statusFlag = waitingForConfig;
            SSerial_printf(Debug, "Waiting for a device configuration...\r\n");
            SSerial_printf(Debug, "Push the button C twice to enter configuration mode\r\n");
            sprintf(displayText, "Device not configured\r\n\r\n\r\nTo switch platform:\r\n<- btn B long press\r\n\r\nTo configure:\r\n<- btn C double click");
            SH1107_Display(1, 0, 0, displayText);
            neopixelSet(NEO_PIXEL_RED);
        }
        else if (!Device_isConnectedToWiFi())
        {
            statusFlag = waitingForConfig;
            SSerial_printf(Debug, "Device was not connected to WiFi. Maybe WiFi configuration is wrong. Check device configuration\r\n");
            SSerial_printf(Debug, "Push the button C twice to enter configuration mode\r\n");
            sprintf(displayText, "WiFi not connected\r\nCheck configuration!\r\n\r\nTo switch platform:\r\n<- btn B long press\r\n\r\nTo configure:\r\n<- btn C double click");
            SH1107_Display(1, 0, 0, displayText);
            neopixelSet(NEO_PIXEL_RED);
        }
        else
        {
            /*Connect to Platform*/
            SSerial_printf(Debug, "Device is configured\r\n");
            sprintf(displayText, "Device is configured");
            SH1107_Display(1, 0, 24, displayText);
            LED_INDICATION_SHORT_DELAY;
            statusFlag = connectingToCloud;
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
        LED_INDICATION_LONG_DELAY;
        break;
    }
    break;
    case waitingForConfig:
        /*Waiting on button press from the user*/
        break;
    case configuringDevice:
    {
        if (!previousConfigDeleted)
        {
            Device_deletePreviousConfigIfExist();
            previousConfigDeleted = true;
        }
        Device_WiFi_provisioning();
        Device_configurationInProgress();
        neopixelSet(NEO_PIXEL_RED);
        delay(3500);
        neopixelSet(NEO_PIXEL_OFF);
        delay(3500);
    }
    break;
    case connectingToCloud:
    {
        neopixelSet(NEO_PIXEL_ORANGE);
        if (platform == KAAIOT)
        {
            sprintf(displayText, "Connecting to \r\n\r\nKaaIoT...");
        }
        else if (platform == AZURE)
        {
            sprintf(displayText, "Connecting to \r\n\r\nAzure...");
        }
        SH1107_Display(1, 0, 16, displayText);

        /*Connect to the cloud*/
        Device_MQTTConnect();
        Device_SubscribeToTopics();
        neopixelSet(NEO_PIXEL_GREEN);
        statusFlag = idle;

        break;
    }
    case idle:
    {
        if (!Device_isStatusOK())
        {
            Device_restart();
        }
        Device_processCloudMessage();
        if (Device_isSensorsPresent() == true)
        {
            interval = micros() - startTime;
            if (interval >= (Device_getTelemetrySendInterval() * 1000))
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
        LED_INDICATION_SHORT_DELAY;
        break;
    }
    case factoryReset:
    {
        sprintf(displayText, "Reset device to \r\n\r\nFactory state");
        SH1107_Display(1, 0, 16, displayText);
        LED_INDICATION_SHORT_DELAY;
        neopixelSet(NEO_PIXEL_RED);
        Device_reset();
    }
    break;
    default:
        break;
    }
    buttonUpdate();
}
