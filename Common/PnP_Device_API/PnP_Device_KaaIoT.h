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

#ifndef P_N_P_DEVICE_KAAIOT_H
#define P_N_P_DEVICE_KAAIOT_H

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

#define KAAIOT_HTML_FILE "<html><script type=\"text/javascript\" src=\"js/jquery-3.6.0.min.js\"></script><script type=\"text/javascript\" src=\"js/settings.js\"></script><script type=\"text/javascript\" src=\"js/general.js\"></script><link rel=\"stylesheet\" type=\"text/css\" href=\"css/bootstrap.min.css\"><link rel=\"stylesheet\" type=\"text/css\" href=\"css/style.css\"><body onload=\"cbLoad()\"><form id=\"kaaiot-form\"><div class=\"alert alert-success alert-dismissable\"><div class=\"alertwrapper clearfix\"><div class=\"alertcontent\"><h4>Info!</h4>To upload a KaaIoT configuration, fill out the form below.</div></div></div><div class=\"headerInSub\"><div class=\"subTitle\">KaaIoT Configuration Upload</div></div><div class=\"c\"><fieldset class=\"wrapSection\"><legend>Enter KaaIoT Device Configuration</legend><div class=\"row mb-3\"><label class=\"form-label\">Token</label> <input class=\"form-control\" type=\"text\" id=\"token\" maxlength=\"50\" required></div><div class=\"row mb-3\"><label class=\"form-label\">Application Version</label> <input class=\"form-control\" type=\"text\" id=\"appVersion\" maxlength=\"40\" required></div><div class=\"row mb-3\"><label class=\"form-label\">MQTT Server</label> <input class=\"form-control\" type=\"text\" id=\"mqttServer\" maxlength=\"100\" placeholder=\"mqtt.cloud.kaaiot.com\" required></div><div class=\"row mb-3\"><label class=\"form-label\">SNTP Server</label> <input class=\"form-control\" type=\"text\" id=\"sntpServer\" maxlength=\"100\" placeholder=\"0.de.pool.ntp.org\" required></div><div class=\"row mb-3\"><label class=\"form-label\">Timezone(UTC ± minutes)</label> <input class=\"form-control\" type=\"number\" id=\"timezone\" maxlength=\"3\" placeholder=\"60\" required></div><div class=\"row mb-3\"><label class=\"form-label\">WiFi SSID</label> <input class=\"form-control\" type=\"text\" id=\"wifiSsid\" maxlength=\"40\" required></div><div class=\"row mb-3\"><label class=\"form-label\">WiFi Security Type</label> <select class=\"form-control\" id=\"wifiSecurityType\" required><option value=\"0\">Open</option><option value=\"1\">WEP</option><option value=\"2\">WEP Shared</option><option value=\"3\">WPA_WPA2</option><option value=\"4\">WPA2_PLUS</option><option value=\"5\">WPA3</option></select></div><div class=\"row mb-3\"><label class=\"form-label\">WiFi Security Key</label> <input class=\"form-control\" type=\"password\" id=\"wifiKey\" maxlength=\"40\" required></div><div class=\"row mb-3\"><label for=\"formFile\" class=\"form-label\">All files (Device certificate (kaadevcert), device key (kaadevkey) and Root CA (kaarootca))</label> <input class=\"form-control\" type=\"file\" id=\"file-input\" multiple=\"multiple\"></div><div class=\"row mb-3\"><div class=\"col-md-4 offset-md-8\"><input name=\"Upload\" type=\"submit\" value=\"UPLOAD\" class=\"btn btn-primary\" style=\"height:40px\"></div></div><div class=\"row\"><ul class=\"list-group\" id=\"file-list-display\"></ul></div><div class=\"result\" id=\"result\" style=\"display: none\"></div></fieldset></div></form></body><script type=\"text/javascript\" src=\"js/kaaiot.js\"></script></html>"

#define KAAIOT_JS_FILE "(function() {\
  var form = document.getElementById('kaaiot-form');\
  var fileInput = document.getElementById('file-input');\
  var fileListDisplay = document.getElementById('file-list-display');\
\
  form.addEventListener('submit', function(evnt) {\
    evnt.preventDefault();\
    var kaadevconf = {\
      token: document.getElementById('token').value,\
      appVersion: document.getElementById('appVersion').value,\
      mqttServer: document.getElementById('mqttServer').value,\
      sntpServer: document.getElementById('sntpServer').value,\
      timezone: document.getElementById('timezone').value,\
      wifiSsid: document.getElementById('wifiSsid').value,\
      wifiSecurityType: document.getElementById('wifiSecurityType').value,\
      wifiKey: document.getElementById('wifiKey').value\
    };\
    var jsonFile = createJSONFile(kaadevconf);\
    sendFile(jsonFile);\
\
    fileList.forEach(function(file) {\
      sendFile(file);\
    });\
  });\
\
  var fileList = [];\
  var renderFileList, sendFile;\
\
  fileInput.addEventListener('change', function(evnt) {\
    fileList = [];\
    for (var i = 0; i < fileInput.files.length; i++) {\
      fileList.push(fileInput.files[i]);\
    }\
    renderFileList();\
  });\
\
  renderFileList = function() {\
    fileListDisplay.innerHTML = '';\
    fileList.forEach(function(file, index) {\
      var fileDisplayEl = document.createElement('li');\
      fileDisplayEl.classList.add('list-group-item');\
      fileDisplayEl.id = file.name;\
      fileDisplayEl.innerHTML = file.name;\
      fileListDisplay.appendChild(fileDisplayEl);\
    });\
  };\
\
  function createJSONFile(data) {\
    var jsonString = JSON.stringify(data, null, 2);\
    var blob = new Blob([jsonString], {\
      type: 'application/json'\
    });\
    var file = new File([blob], \"kaadevconf.json\", {\
      type: 'application/json'\
    });\
    return file;\
  }\
\
  function sendFile(file) {\
    var formData = new FormData();\
    var request = new XMLHttpRequest();\
    request.onreadystatechange = () => {\
      if (request.readyState === XMLHttpRequest.DONE) {\
        var fileNameStr = \"#\" + request.responseURL.substring(request.responseURL.indexOf('=') + 1);\
        if (request.status === 200 || request.status === 204) {\
          if (request.responseText != \"\") {\
            $('.result').html(request.responseText);\
          } else {\
            $('.result').html(\"Success: \" + request.status + \": \" + request.statusText);\
            $(fileNameStr).html($(fileNameStr).html() + \" - uploaded.\");\
          }\
        } else {\
          $('.result').html(\"Error \" + request.status + \": \" + request.statusText);\
          $(fileNameStr).html($(fileNameStr).html() + \" - error, file not uploaded!\");\
        }\
      }\
      $('.result').css('display', 'block');\
    };\
    request.onerror = () => {\
      console.log(\"Failed to upload configuration\");\
      $('.result').html(\"Error \" + request.status + \": \" + request.statusText);\
      document.getElementById('result').style.display = 'block';\
    };\
    formData.set('file', file);\
    var put_uri = \"file?filename=\" + file.name;\
    request.open(\"PUT\", put_uri, true);\
    request.send(file);\
  }\
  })();"

#define KAAIOT_CONFIGURATION_DATA "{\n\
	\"token\": \"kaa-token\",\n\
	\"appVersion\": \"kaa-app-version\",\n\
	\"mqttServer\": \"mqtt.cloud.kaaiot.com\",\n\
	\"sntpServer\": \"0.de.pool.ntp.org\",\n\
	\"timezone\": \"60\",\n\
  \"wifiSsid\": \"wifi-ssid\",\n\
  \"wifiSecurityType\": 3,\n\
  \"wifiKey\": \"wifi-key\"\n\
}"

/*File path to files stored on Calypso internal storage*/
#define KAAIOT_HTML_FILE_PATH "/www/kaaiot.html"
#define KAAIOT_JS_FILE_PATH "/www/js/kaaiot.js"
#define KAAIOT_CONFIG_FILE_PATH "user/kaadevconf.json"

#define KAAIOT_ROOT_CA_PATH "user/kaarootca"
#define KAAIOT_DEVICE_CERT_PATH "user/kaadevcert"
#define KAAIOT_DEVICE_KEY_PATH "user/kaadevkey"

/*Wi-Fi settings*/
#define WI_FI_CONNECT_DELAY 5000UL

/*MQTT settings*/
#define MQTT_PORT_SECURE 8883

#define MAX_URL_LEN 128

#define KAA_DATA_SAMPLES_TOPIC "kp1/%s/dcx/%s/json"
#define KAA_COMMANDS_TOPIC "kp1/%s/cex/%s/command/#"
#define KAA_COMMANDS_RESPONSE_TOPIC "kp1/%s/cex/%s/result/%s"

#define KAA_COMMANDS_TOPIC_TO_COMPARE "kp1/%s/cex/%s/command/"

#define DEFAULT_TELEMETRY_SEND_INTEVAL 30 // seconds
#define MAX_TELEMETRY_SEND_INTERVAL 600   // seconds
#define MIN_TELEMETRY_SEND_INTERVAL 3     // seconds

#define STATUS_SUCCESS 200
#define STATUS_SET_BY_DEV 203
#define STATUS_BAD_REQUEST 400
#define STATUS_EXCEPTION 500
#define STATUS_CLOUD_SUCCESS 204

#define CALYPSO_FIRMWARE_MIN_MAJOR_VERSION 2
#define CALYPSO_FIRMWARE_MIN_MINOR_VERSION 2

  extern bool sensorsPresent;
  extern volatile unsigned long telemetrySendInterval;
  TypeSerial *Kaaiot_Device_init(void *Debug, void *CalypsoSerial);
  void Kaaiot_Device_deletePreviousConfig();
  void Kaaiot_Device_writeConfigFiles();
  bool Kaaiot_Device_isConfiguredForPlatform();
  bool Kaaiot_Device_isConfigured();
  bool Kaaiot_Device_isConnectedToWiFi();
  void Kaaiot_Device_MQTTConnect();
  void Kaaiot_Device_readSensors();
  void Kaaiot_Device_PublishSensorData();
  void Kaaiot_Device_connect_WiFi();
  void Kaaiot_Device_disconnect_WiFi();
  void Kaaiot_Device_WiFi_provisioning();
  void Kaaiot_Device_configurationInProgress();
  bool Kaaiot_Device_SubscribeToTopics();
  void Kaaiot_Device_reset();
  void Kaaiot_Device_restart();
  bool Kaaiot_Device_isStatusOK();
  void Kaaiot_Device_processCloudMessage();
  bool Kaaiot_Device_ConfigurationComplete();
  void Kaaiot_Device_displaySensorData();
  bool Kaaiot_Device_isUpToDate();
  unsigned long Kaaiot_Device_getTelemetrySendInterval();
  bool Kaaiot_Device_isSensorsPresent();
#ifdef __cplusplus
}
#endif

#endif /* P_N_P_DEVICE_H */