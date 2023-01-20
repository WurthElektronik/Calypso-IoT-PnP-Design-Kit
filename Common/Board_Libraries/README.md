# Sensor Board
The sensorboard files provide drivers to control the sensors on the sensor featherWing:
```
- The Humidity Sensor "WSEN_HIDS_2523020210001" 
- The Acelerometer Sensor "WSEN_ITDS_2533020201601" 
- The Pressure Sensor "WSEN_PADS_2511020213301" 
- The Temperature Sensor "WSEN_TIDS_2521020222501"
```
The implemented functions in the drivers allow the user to configure the sensor and get different sensor data .

# ThyoneI Board

The **thyoneI.c** and **ThyoneI.h** files provide drivers to control different features of the Thyone-I module present on the Thyone-I Wireless FeatherWing.\
While in the **ThyoneI.h**, all data types and structures are defined, in the the **thyoneI.c** file, the features functions are implemented.\
As example of these functions, you find the : 
```
- THYONEI_receiveData(); : this function receives data from Thyone.
- ThyoneI_TransmitBroadcast(); : this function provides a simple broadcast data transmission. 
- ThyoneI_Sleep(); : this function Sets the ThyoneI to sleep mode
... and other  functions.
```
 
# Calypso Board

The calypso board provides Wi-Fi connectivity to the IoT design kit.\
Through the implemented functions, the Calypso is able to Connect to the access point with the defined parameters in the settings and to disconnect from it.
```
bool Calypso_WLANconnect();
bool Calypso_WLANDisconnect();
```
Other functions like the : 
```
bool Calypso_MQTTconnect();
bool Calypso_MQTTPublishData();
```
let the Calypso **Create** and **Connect** to the **MQTT broker** and then **Publish** data to the same.


# Secure element : The atecc608a 

The atecc608a files provide provide drivers to configure and use the secure element.\
The atecc608a provides a HW Support for **Asymmetric Sign, Verification, key exchange,** through functions like : 
```
bool atecc608a_verify_signature();
bool atecc608a_test_sign_verify();
```
These security files provide also a HW Support for **Symmetric Algorithms** like the **AES-128: Encrypt/Decrypt**.\
This Support is implemented by functions like : 
```
bool atecc608a_decrypt_data();
bool atecc608a_encrypt_data();
```
