# Quick start guide using Azure IoT Central

[Azure IoT Central](https://azure.microsoft.com/en-us/products/iot-central) is an IoT application platform as a service (aPaaS), developed by Microsoft that reduces the burden and cost of developing, managing, and maintaining enterprise-grade IoT solutions. Choosing to build with IoT Central allows the opportunity to focus time, money, and energy on transforming the business with IoT data, rather than just maintaining and updating a complex and continually evolving IoT infrastructure.

The web UI lets quick connection of devices, monitoring of device conditions, creating rules, and managing millions of devices and their data throughout their life cycle. Furthermore, it enables acting on device insights by extending IoT intelligence into line-of-business applications. IoT Central is a ready-made environment for IoT solution development. It’s an application platform as a service (aPaaS) IoT solution and its primary interface is a web UI. There is also a REST API that lets interaction with the application programmatically.

More information about the IoT central platform can be found [here.](https://learn.microsoft.com/en-us/azure/iot-central/)

This document will guide you through the process of configuring the Azure IoT hub as well as the Calypso IoT design kit and sending sensor data from the kit to cloud. This process involves the following 5 steps at the end of which you should be able to see the telemetry data from the device on the cloud platform.

Each of these steps are described in detail in the subsequent sections.

[**1. Create an IoT Central Application:**](#creating-an-iot-central-application) In this step, we create an instance of the Azure IoT central platform from the scratch.

[**2.	Create device config files:**](#create-device-configuration-files) In this step all the files that are necessary to configure a device and the IoT platform are created.

[**3.	Upload Root Certificate to IoT Central:**](#upload-the-root-certificate-to-iot-central) In this step, the root certificate created in the previous step is uploaded to the cloud platform to configure the same.

[**4.	 Configure the device:**](#configure-the-device) This step fully configures the device to connect to cloud.

[**5. Interact with the device on the cloud:**](#view-the-device-default-dashboard) View device dashboard and [interact](#send-commands-to-device) with the device from the cloud platform.

# **Creating an IoT central application**

 a.	Sign in to the [Azure portal](https://portal.azure.com/).
 If a subscription does not exit, please create a subscription as described in this [link](https://learn.microsoft.com/en-us/dynamics-nav/how-to--sign-up-for-a-microsoft-azure-subscription).

 b. From the Azure homepage, select the "+ Create a resource button" and then enter "IoT Central application" in the Search the Marketplace field. 

 c.	Select "IoT Central application" from the search results and the select "Create".

![Create IoT Central](images/createiotcertral.PNG)

On the Basics tab, complete the fields as follows: 

![Create Basic](images/createbasic.PNG)

 a. **Subscription**: Select the subscription to use for the application.

 b.	**Resource group:** Select a resource group or create a new one. To create a new one, select Create new and fill in the name you want to use. To use an existing re source group, select that resource group. For more information, see Manage Azure Resource Manager resource groups.

 c.	**Resource name**: Type in a name for the IoT central application. For example, "wuerthelektronik".

 d.	**Application URL**: This will be automatically set to < Resource name>.azureiotcentral.com. In this example, "wuerthelektronik.azureiotcentral.com"

 e.	**Template**: From the drop down, select "Custom application".

 f.	**Region**: Select the region in which the application will be located. Select a location that is geographically the closest. FOr example, West Europe.

 g.	**Pricing Plan**: Choose the appropriate pricing tier. The standard tier 0 is good to start prototyping. More details on pricing can be found at, https://azure.microsoft.com/en-us/pricing/details/iot-central
  
 h.	Click on **"Review + Create"**. 
  
 i.	On the following page, review the terms and click on **"Create"**. 
  
 j.	Wait for the deployment to complete. After the process is complete, click on "Go to resource" button to open the application. 
 
 ![Deployment Complete](images/depComplete.PNG)
  
 k.	Click on the IoT central application URL to open the newly created IoT central platform.
 
  ![Application URL](images/appurl.PNG)
  
 l.	In the IoT central app open, after expanding the menu bar, click on "Permissions > Device connection groups" and note down the ID scope parameter for use in further steps.

![Home](images/home.png)

![ID Scope](images/idscope.PNG)

# **Create device configuration files**

In order to securely connect the device to IoT central application, the device needs to implement certain methods for authentication. In this case, the X.509 certificate based authentication is implemented. This method requires creation of certificates for every device. In order to enable easy prototyping, Würth Elektronik eiSos’s Certificate creation tool can be used. This tool creates all the certificates necessary to get started.

- 	Download the WE certificate generator tool from https://www.we-online.com/certificategenerator 

- 	Unzip to a suitable location on the computer and open the executable ```WECertificateUploader.exe```

Inside the WE Certificate Generator tool, fill in the following fields to generate the required certificates for the cloud service

![Certificate Generator Tool_Create Config File](images/Azcreatecfgfile.PNG)

1. Select "Azure IoT central" from the "Select IoT platform" dropdown.

2.	SSID and Password: Select your Wi-Fi network and type in the password. The device will connect to internet through this Wi-Fi network.

3.	Security: Check if the selected security type is correct for the selected Wi-Fi network.

4.	NTP server: If needed change the time server of choice that the module will use to get the current time.

5.	Time zone: Select the appropriate time zone.

6.	Device ID: This is the name of the device as it appears in the IoT central APP. It needs to be unique per device. In this example, the name "test-dev" is used.

7.	Scope ID: Type in the ID scope noted in the previous section. ID scope is unique per application but common across devices.


##	**Device root certificate:**
This is the self-signed certificate that acts as the root of trust for all devices. The device root certificate is used to generate leaf certificates. Each device has a unique leaf certificate that identifies the device. The root certificate can be generated once and used for generating leaf certificates for several devices. The WE certificate uploader tool allows creation of a new root certificate, saving the same and loading it back for subsequent usage. 

![Device Root Certificate](images/deviceroot.png)

1. On first time use, set the validity time in month. For example, 1 month.

2. Click on "Create root certificate" to create a new root certificate. 

3. [**Optional**] If a root certificate already exists, click on "Load root certificate". This opens a file browser. Browse to the correct location to choose the previously used root certificate. Use .pfx format certificate for loading.
  
4. [**Optional**]  Click on "Save root certificate" to save the generated root certificate for future use. This can be used generate the certificate in .pfx format which is the format  used during loading a certificate.

5. [**Optional**]  Click on "Export root PEM" to export the certificate in PEM format. This file needs to be uploaded to the IoT central application

6. [**Optional**]  Click on "Display root certificate" to view the certificate in the standard Windows format.

##	**Device certificate:**
Every device requires a unique device certificate to securely connect to the IoT central application. Each device certificate generated is exclusively linked to the device through the device ID and cannot be used on any other device.

![Device Certificate](images/devicecert.png)

1. For every device ID, set the validity time in months. For example, 1 month.
2. Click on "Create device certificate" to create a new device certificate
3. [**Optional**] Click on "Export device PEM" to export the certificate in PEM format. This file needs to be uploaded to the device.  
4. [**Optional**] Click on "Display device certificate" to view the certificate in the standard Windows format.

##	**Device private key:**

This is the private key corresponding to the public key in the device certificate and is also uniquely linked to a device ID. Click on "Export device private key" to export the key in PEM format. This file needs to be uploaded to the device as will be explained in the subsequent sections.

![Device Key](images/devicekey.png) 

After completing the processes of generating the root and device certificates, export all the files by clicking "**Export All Files**". All the certificate files will be created and stored under "Azure/<device_id>" folder inside the main directory.

![Certificate Generator Tool Steps](images/AZcertgenstep.PNG)

## **Certificates Generated in the output directory:**

![Azure Device Certificate](images/azdevcert.PNG) 

After exporting all the necessary files, a new directory will be created in the same folder as the executable file with all the necessary configuration file and certificates.

  
# **Upload the Root certificate to IoT central**
    
In this step, the device root certificate is uploaded to IoT central and a policy is set to allow all devices with leaf certificate that are generated from this root to connect to the platform.

If an organization does not exist, create an organization as described in this [link](https://learn.microsoft.com/en-us/azure/iot-central/core/howto-create-organizations).
    
![Create New Profile](images/createNewProfile.png)
  
  - In the IoT central app open, "Permissions > Device connection groups" and click on "+New"
  
![Create_Enrollment Group](images/create_enrollmentgroup.PNG)

  - In the subsequent window,
    * a. Enter a name for the enrollment group.
    * b. Set the "Automatically connect devices in this group" to "On".
    * c. Set the group type to "IoT devices"  
    * d. Set the attestation type to "Certificates (X.509)"
    * e. Click on "Save".
    

![Manage Certificate](images/managecert.png)
- Once the enrollment group is created, the root certificate needs to be added to this group.
In order to do this,
  * a. Click on "Manage primary" in the "Certificates (X.509)" section of the enrollment group.
  * b. In the pop-up window, click on "Add certificate" and select the device root file generated using the WE certificate generator. This can be found under "Azure/<device_id>/device_root_cert"
  * c. Set the "Set certificate status to verified on upload" option to "On".
  * d. Click on "Upload"
  * e. On completion of the upload process, close the pop-up window.
    
# **Configure the device**
          
The IoT design kit comes with the Firmware pre-installed. In this step, a one-time configuration of the kit is done which enables connection to the desired Wi-Fi network and the previously created IoT central application
          
- Ensure that all the four boards are stacked up correctly with the Adafruit FeatherWing OLED on the top.
          
- Power up the IoT design kit stack via USB or a Li-Po connector on the Adafruit M0 Feather board.
          
![power-up](images/power-up.jpg)
            
- After a short initialization process, the device waits for the user to start the configuration process. The following message appears on the display "Device not configured. To configure double press button C".
            
- Double press button C on the OLED display FeatherWing to enter the configuration mode.

![Wait To Config](images/WaitToConfig.jpg)
              
              
- In the configuration mode, perform the following five steps,

  ![Config Mode](images/configMode.jpg)
  
  a. In the configuration mode, the Calypso Wi-Fi module is set to access point mode with an SSID "calypso_<MAC_ADDRESS>" and password "calypsowlan". Connect your PC (Laptop/tablet/smartphone) to this access point, displayed on the screen.

![Connect to AP](images/connectToAP.png)  
 
  b. On the PC open a browser.
              
  c. In the browser, navigate to calypso.net/azure.html.                     

![Upload](images/upload.png)     

![Select All Files](images/selectAllFiles.png)
  d. Click on the "Choose Files" button. This opens the file browser. Browse to the location where the configuration files were generated as described in the previous section. Select all the files in the directory and click on "Upload" button. On success, the message "Success: 204 No content" at the bottom of the page indicates successful configuration of the device.

 
  e. Restart the device by clicking the "Reset" button.
              
![Reset](images/reset.jpg)
                  
- On restarting, the device goes through the following steps automatically,
              
  a. Initialize the hardware.
              
  b. Connect to the configured Wi-Fi network.
              
  c. Connect to the Microsoft Azure Device provisioning service (DPS).
            
  d. After authentication, the DPS assigns the address of the IoT hub (Device management service of Azure) to connect.
              
  e. This address is saved in the secure storage of the Calypso Wi-Fi module for further use.
              
  f. Finally, the device connects securely to the IoT central platform and starts exchange of data.
              
At this stage, the device is fully configured, securely connected and ready to use. On subsequent boot-up the device directly connects to the platform using the saved address and starts exchanging data with the platform.
             
![Connected](images/connected.jpg)  

Connect the PC to the Internet by reconnect to the your local Wi-Fi network.

**Congratulations!** The set-up is now complete. It's not time to check the telemetry data on the cloud.

# **View the device default dashboard**

To access the device on the IoT platform, navigate to "Devices -> All devices -> <Device ID>".
  
Click on the device name to open the device page.
  
![Select Device](images/selectDevice.png)

In the "about" tab, the properties are displayed.
  
![About](images/about.png)
  
The telemetry data is displayed graphically in the "Overview" tab.
  
![Overview](images/overview.png)
 
# **Send commands to device**
  
In order to send a command to change the mini neo-pixel LED on the device, click on the command tab on the device page. Enter the RGB values and click on Run. 
A few sample colors,

| Color | R | G | B |
| :----------------  | :------: | :------: | :------: |
| Red | 255 | 0 | 0 |
| Green | 0 | 255 | 0 |
| Blue | 0 | 0| 255|
| Yellow | 255 | 255 | 0 |
| White | 255 | 255 | 255 |
| Cyan | 0 | 255 | 255 |

The message is processed by the device and the color of the LED is changed accordingly.
  
![Run Command](images/runCommand.png)
    
# **Update device property:**  
This device allows changing the frequency with which the sensor data is read and sent to the platform. This can be done from the IoT central platform. To do so, open click on "Manage device drop down and select "Device properties". This opens the property update page.

![Device property](images/devprop.png)
  
To update the send frequency, type in ""telemetrySendFrequency": "<Send frequency in seconds>" and click on "Send to device". This updates the device's send frequency on the device.

![Telemetry send frequency](images/devprop1.png)

# **Factory resetting the device**

In order to reset the device to factory state, double press "button B". The following message is displayed on the screen, "Reset device to factory state". 
This procedure resets the device to default state. Follow the device configuration process defined earlier to reconfigure the device.