#  How to create an demo IoT application using the Calypso IoT design kit with KaaIoT?
## **Prerequisites**
The IoT design kit comes with the Firmware pre-installed only for the Azure platform. You need to update the firmware to the latest version from [this repository](https://github.com/PavelAnikeichyk/Calypso-IoT-PnP-Design-Kit/tree/kaaiot) to enable connection to the Kaa IoT platform.

The following items are necessary to go through this process and connect to Kaa.

- Design kit with M0 Feather stacked with Calypso Wi-Fi FeatherWing, the FeatherWing sensor, and the OLED FeatherWing.

- Micro-USB cable to power up the design kit stack.

- A Wi-Fi access point (IEEE 802.11 b/g/n compatible) with internet access.

- Wi-Fi-enabled computer with a browser (Chrome or Edge browsers are recommended).

- Kaa account. You can get a [free trial](https://www.kaaiot.com/free-trial).

![power-up](images/power-up.jpg)

:warning:  The design kit will always be delivered with the latest firmware version (> v2.2.0). If you use a Calypso FeatherWing that you received separately, make sure that the Calypso FeatherWing has a firmware version > v2.2.0 [(Get more info).](https://www.we-online.com/components/products/manual/2610011025000_Calypso%20261001102500x%20Manual_rev2.0.pdf#page128) 

## **Quick start guide**
This section will guide you through the process of creating an end-to-end IoT solution. This process involves the following 4 steps, at the end of which you should be able to see the telemetry data from the device on the cloud platform.

Let’s describe each of these steps in detail.

[**1. Create a Kaa application and endpoint:**](#creating-an-kaaiot-application) In this step, we create an application and endpoint on the [Kaa Cloud](https://www.kaaiot.com/products/kaa-iot-cloud).

[**2.	Get TLS certificates and private key:**](#download-certificates-from-kaaiot) This step involves obtaining the root certificate.

[**3.	Configure the device:**](#configure-the-device) This step fully configures the device to connect to the cloud.

[**4. View and interact with device on the Kaa IoT platform:**](#view-the-device-telemetry) View device information and telemetry and [interact](#send-commands-to-device) with the device from the cloud platform.

## **Creating a Kaa application and endpoint**

a.	Sign in to the [Kaa](https://cloud.kaaiot.com/).
 If you don’t have a subscription, please create an account by clicking the [link](https://www.kaaiot.com/free-trial).

 b. From the [Device management](https://cloud.kaaiot.com/devices/device-management) page, select "Applications" and press "Add application" (name it "Wurth application"). 

 ![KaaIoT Wurth application](images/kaaiot-wurth-application.png)

:information_source: The next step will involve creating the endpoint. In order to create the endpoint, we will require a token. As usual, we use a unique token, such as a device serial number or IMEI. For simplicity, we will use "calypso-token" as a token.

 c.	From the [Device management](https://cloud.kaaiot.com/devices/device-management) page, select "Devices", then choose your new application, and press "Add device".

 ![KaaIoT create endpoint](images/kaaiot-create-endpoint.png)

## **Get TLS certificates and private key**

In order to securely connect the device to the IoT central application, the device needs a Root CA certificate, a device private key, and a device certificate. The device's private key and certificate are optional. To get the certificates and private key, go to the [Credentials](https://cloud.kaaiot.com/devices/credentials/certificates) page. 

"Get root certificate" button allows you to see the Root CA. Save this certificate to file with the name "kaarootca". 

:warning:  Please pay attention that files should have Unix-based newline characters.

"Add TLS client certificate" button creates the device private key (save to file with name "kaadevkey") and certificate (save to file with name "kaadevcert"). For simplicity, we will connect with only the Root CA file. 
    
## **Configure the device**
          
In this step, the device will be configured to be able to connect to the Kaa IoT platform.
          
- Make sure to stack all four boards correctly, placing the Adafruit FeatherWing OLED on top.
          
- Power up the IoT design kit stack via USB or a Li-Po connector on the Adafruit M0 Feather board.
          
![power-up](images/power-up.jpg)

- Compile and upload the latest firmware to the IOT design kit. For instance, you can use Visual Studio Code and PlatformIO. Restart the device.

- After a short initialization process, the device checks the existence of a **platform.json** file. The file contains information about the selected platform because, after the update, the device supports 2 IoT platforms: Azure and Kaa Cloud. The file doesn't exist, and it is created with the default platform as Kaa. 

  ![Platform not configured](images/device-platform-not-configured.png)

- Next, the device looks for the Kaa config file, **kaadevconf.json**. The file is initially absent and will be created later during the configuration process.

  ![Kaaiot config not found](images/kaaiot-config-not-found.png)

- The web interface will be used to configure the device. The main web menu file is index.html. This file will be overwritten to add the Kaa IoT platform, and the original file will be saved as index_src.html for backup.

  ![index_src not exist](images/kaaiot-index_src_not-exist.png) 

  ![index_src created](images/kaaiot-index_src_created.png)

- After that, the device will inform you that the IoT platform is not configured and will wait for you to start the configuration process. 

  ![Platform not configured](images/iot-platform-not-configured.png)

- Next, the following message appears on the display: "Device not configured. To configure, double-press button C". To switch platforms, long-press on button B. The platform.json file will be changed, and after the restart, the device starts with another platform selected.
            
- Double-press button C on the OLED display FeatherWing to enter the configuration mode.

  ![Wait To Config](images/WaitToConfig.png)        
              
- In the configuration mode, perform the following steps.

  ![Config Mode](images/configMode.png)
  
  a.  In the configuration mode, the Calypso Wi-Fi module is set to access point mode with an SSID **"calypso_<MAC_ADDRESS>"** and password **"calypsowlan"**. Connect your PC (Laptop/tablet/smartphone) to this access point displayed on the screen.

![Connect to AP](images/connect-to-ap.png)  

  b. On the PC, open a browser.
              
  c. In the browser, navigate to [calypso.net](calypso.net). Press on the left menu.                     

  ![Main menu](images/main-menu.png)

  d. Press on the KaaIoT. There is the main menu. Also this page accessible directly on [calypso.net/kaaiot.html](http://calypso.net/kaaiot.html).

  ![KaaIoT main menu](images/kaaiot-config-menu.png) 

  e. Fill in the fields. Click the "Choose Files" button to open the file browser. Navigate to the location where the configuration files are. Select the Root CA file (kaarootca). Click on the "Upload" button. It uploads the configuration and files. 

  ![KaaIoT select certificate](images/kaaiot-select-certificate.png)

  ![KaaIoT fill configuration](images/kaaiot-fill-configuration.png)

  f. If successful, the message "Success: 204 No content" at the bottom of the page will indicate the successful configuration of the device.

  ![KaaIoT upload success](images/kaaiot-upload-success.png)
 
  g. Restart the device by clicking the "Reset" button.
              
  ![Reset](images/reset.jpg)
                  
- On restarting, the device goes through the following steps automatically:
              
  a. Initialize the hardware.

  ![KaaIoT selected IoT platform](images/kaaiot-selected-iot-platform.png)
  ![KaaIoT loading configuration](images/kaaiot-loading-configuration.png)

  When only Root CA is used the device informs about the absence of device certificate and device private key.

  ![KaaIoT device cert absent](images/kaaiot-device-cert-absent.png)
  ![KaaIoT device private key absent](images/kaaiot-device-key-absent.png)
              
  b. Connect to the configured Wi-Fi network.

  ![KaaIoT connect to WiFi](images/kaaiot-connect-to-wifi.png)

  c. Synchronize date and time with SNTP server.

  ![KaaIoT SNTP sync](images/kaaiot-sntp-sync.png)
              
  d. The device connects securely to the Kaa IoT platform and starts exchange of data.

  ![KaaIoT connect to platform](images/kaaiot-connect-to-platform.png)
  ![KaaIoT connected](images/kaaiot-connected.png)
              
During the subsequent boot-up, the device directly connects to the platform using the saved address and starts exchanging data with the platform.
             
![KaaIoT send data](images/kaaiot-send-data.png)             

Connect the PC to the Internet by reconnecting to your local Wi-Fi network.

**Congratulations!** The setup is now complete, and it's time to check the telemetry data on the cloud.

## **View and interact with device on the Kaa IoT platform**

To access the device on the Kaa IoT platform, navigate to "Devices -> Wurth application -> <Endpoint ID>".

![KaaIoT endpoint](images/kaaiot-endpoint.png)
 
## **Send commands to device**
  
In order to send a command to the device, click on the commands tab on the device page. Enter command type *switch_on_off* and enter the command body *{"state":"on"}*

![KaaIoT command on](images/kaaiot-command-on.png)

Click on Run. The message is processed by the device, which shows the command state on the display and sends a response.
  
![KaaIoT device receive command](images/kaaiot-device-command-on.png)  

The Kaa IoT platform shows that command was executed.

![KaaIoT command executed](images/kaaiot-command-executed.png)

## **Factory resetting the device**

To reset the device to factory state, press button C once, then press and hold button C till the following message is displayed on the screen: "Reset device to factory state". This procedure resets the device to its default state. Follow the device configuration process defined earlier to reconfigure the device.
