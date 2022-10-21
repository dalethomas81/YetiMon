/*

  https://github.com/dalethomas81/YetiMon
  Copyright (C) 2021  Dale Thomas

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  DaleThomas@me.com

*/
/*************Board settings************

    NodeMCU:
    "board": "esp8266:esp8266:nodemcuv2",
    "configuration": "xtal=160,vt=flash,exception=legacy,ssl=all,eesz=4M1M,led=2,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=115200",

*****************************************/

#include <ArduinoJson.h>
#include "LittleFS.h"             // need the upload tool here https://github.com/earlephilhower/arduino-esp8266littlefs-plugin
#include <ESPAsyncWebServer.h>    // https://github.com/me-no-dev/ESPAsyncWebServer
#include <WebSocketsServer.h>     // https://github.com/Links2004/arduinoWebSockets
#include <ESPAsyncWiFiManager.h>  // https://github.com/alanswx/ESPAsyncWiFiManager
#include <WebSocketsClient.h>
#include <ArduinoOTA.h>
#include <StreamString.h>

/* global defines */
#define CONST_DEVICE_ID           "YetiMon-"
#define WIFI_LISTENING_PORT       80
#define SERIAL_BAUD_RATE          115200

/* global variables */
const char version[] = "build "  __DATE__ " " __TIME__; 
String BUILD_DATE(version);
String DEVICE_ID(CONST_DEVICE_ID);

/* web server */
AsyncWebServer server(WIFI_LISTENING_PORT);
WebSocketsServer webSocket_Server = WebSocketsServer(WIFI_LISTENING_PORT+1);
WebSocketsClient webSocket_Client;

/* wifi */
DNSServer dns;
char ws_server_ip[16] = "192.168.1.1";
char ws_server_port[6] = "81";
char ws_server_url[10] = "/";
bool shouldSaveConfig = false;
void checkWifi() {
  if ((WiFi.status() != WL_CONNECTED) || (WiFi.localIP().toString() == "0.0.0.0")) { 
    //Serial.println("Reconnecting to Wifi");
    WiFi.reconnect();
    //connectWifi(0);
  }
}
void saveConfigCallback () {
  //Serial.println("Should save config");
  shouldSaveConfig = true;
}
void setupWifiManager() {   
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  AsyncWiFiManagerParameter custom_ws_server_ip("server", "ws server ip", ws_server_ip, 16);
  AsyncWiFiManagerParameter custom_ws_server_port("port", "ws server port", ws_server_port, 6);
  AsyncWiFiManagerParameter custom_ws_server_url("url", "ws server url", ws_server_url, 10);
  // Local intialization. Once its business is done, there is no need to keep it around
  AsyncWiFiManager wifiManager(&server,&dns);
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  //add all your parameters here
  wifiManager.addParameter(&custom_ws_server_ip);
  wifiManager.addParameter(&custom_ws_server_port);
  wifiManager.addParameter(&custom_ws_server_url);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimum quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality();
  
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  wifiManager.autoConnect(DEVICE_ID.c_str());

  //read updated parameters
  strcpy(ws_server_ip, custom_ws_server_ip.getValue());
  strcpy(ws_server_port, custom_ws_server_port.getValue());
  strcpy(ws_server_url, custom_ws_server_url.getValue()); 
  
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    saveConfig();
  }
  //WiFi.mode(WIFI_STA);
  //WiFi.begin();
  //WiFi.setAutoReconnect(true);
  //WiFi.persistent(true);
}
void handleEraseConfig() {
    Serial.println("Erasing configuration and restarting.");
    ESP.eraseConfig();
    ESP.reset();
}

/* ota */
//#define OTA_PASSWORD    "ESPsAreGreat!"
bool OtaInProgress = false;
int progress_last = 0;
void setupOTA(){
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(DEVICE_ID.c_str());
  // No authentication by default
  //ArduinoOTA.setPassword((const char *)OTA_PASSWORD); // using password isnt supported in vscode...yet
  ArduinoOTA.onStart([]() {
    OtaInProgress = true;
    webSocket_Server.onEvent(NULL);
    webSocket_Client.onEvent(NULL);
    LittleFS.end();
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    OtaInProgress = false;
    Serial.println("\nEnd");
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (progress != progress_last) {
      progress_last = progress;
      Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    OtaInProgress = false;
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed. Firewall issue?");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

/* utility */
#define HEARTBEAT_TIME  1000
uint8_t HEARTBEAT_VALUE;
bool HeartbeatOn = false;
unsigned long timer_heartbeat;
unsigned long timer_eraseConfig;
void handleHeartbeat(){
  if (millis() - timer_heartbeat > HEARTBEAT_TIME ){
    timer_heartbeat = millis();
    //broadcastUpdates(); // send out websockets updates
    if (HEARTBEAT_VALUE>=100) {
      HEARTBEAT_VALUE = 0;
    } else {
      HEARTBEAT_VALUE = HEARTBEAT_VALUE + 1;
    }
    if (HeartbeatOn) {
      HeartbeatOn = false;
      //Serial.println("Heartbeat Off");
    } else {
      HeartbeatOn = true;
      //Serial.println("Heartbeat On");
    }
  }
}

/* flash */
void initLittleFS() {

  Serial.println(F("Inizializing FS..."));
  if (LittleFS.begin()){
      Serial.println(F("done."));
  }else{
      Serial.println(F("fail."));
  }

  // // To format all space in LittleFS
  //  //LittleFS.format();

  // // Get all information of your LittleFS
  // FSInfo fs_info;
  // LittleFS.info(fs_info);

  // Serial.println("File system info.");

  // Serial.print("Total space:      ");
  // Serial.print(fs_info.totalBytes);
  // Serial.println("byte");

  // Serial.print("Total space used: ");
  // Serial.print(fs_info.usedBytes);
  // Serial.println("byte");

  // Serial.print("Block size:       ");
  // Serial.print(fs_info.blockSize);
  // Serial.println("byte");

  // Serial.print("Page size:        ");
  // Serial.print(fs_info.totalBytes);
  // Serial.println("byte");

  // Serial.print("Max open files:   ");
  // Serial.println(fs_info.maxOpenFiles);

  // Serial.print("Max path length:  ");
  // Serial.println(fs_info.maxPathLength);

  // Serial.println();

  // // Open dir folder
  // Dir dir = LittleFS.openDir("/");
  // // Cycle all the content
  // while (dir.next()) {
  //     // get filename
  //     Serial.print(dir.fileName());
  //     Serial.print(" - ");
  //     // If element have a size display It else write 0
  //     if(dir.fileSize()) {
  //         File f = dir.openFile("r");
  //         Serial.println(f.size());
  //         f.close();
  //     }else{
  //         Serial.println("0");
  //     }
  // }
}

/* webserver */
void setupWebserver() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ // https://github.com/me-no-dev/ESPAsyncWebServer
    request->send(LittleFS, "/MAIN.html");
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/favicon.ico");
  });
  server.on("/apple-touch-icon.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/apple-touch-icon.png");
  });
  server.on("/favicon-16x16.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/favicon-16x16.png");
  });
  server.on("/apple-touch-icon.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/apple-touch-icon.png");
  });
  server.on("/favicon-32x32.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/favicon-32x32.png");
  });
  server.on("/favicon-16x16.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/favicon-16x16.png");
  });
  server.on("/site.webmanifest", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/site.webmanifest");
  });
  server.begin();
}

/* i/o */
#define ERASE_CONFIG_TIME             10000   // how long to hold down the button to erase config in ms
#define GPIO_13_LED                   D4      // D4 is LED on nodemcu
#define GPIO_0_BUTTON                 D3      // D3 is flash button on nodemcu
#define GPIO_12_RELAY                 D0      // D0 is HIGH at boot
#define GPIO_VOLTAGE_INPUT            A0      // A0 is the only analog input on esp8266
#define AVG_VOLTAGE_SIZE              100     // size of the array for smoothing analog in
#define SAMPLE_INTERVAL               100     // how often to sample the voltage in ms
#define UPPER_VOLTAGE_LIMIT           11.50   // charging - 11.27v is 32%
#define LOWER_VOLTAGE_LIMIT           10.70   // discharging - 10.57v is about 6%
#define LIMIT_DELAY                   60000   // how long to be under/over the limits in ms
#define BROADCAST_RATE_LIMIT          2000    // limit the rate we can print/broadcast updates in ms
#define BROADCAST_TOLERANCE           0.01    // voltage tolerance to print/broadcase
#define DEBOUNCE_DELAY                50      // the debounce time for hardware buttons
int buttonState = HIGH; 
int previousButtonState = buttonState;
bool relayState = LOW;                        // our relay is off when LOW
bool previousRelayState = relayState;
unsigned long lastDebounceTime = 0;
int sensorValue = 0;
float voltageReadings[AVG_VOLTAGE_SIZE] = {0.0};
int voltageArrayIndex = 0;
float previousBroadcastedVoltage, voltageValue, totalVoltage, averageVoltage;
unsigned long previousLimitMillis, previousSampleMillis, previousPrintMillis;
void setupPins() {
  pinMode(GPIO_13_LED, OUTPUT);
  digitalWrite(GPIO_13_LED, !relayState); // nodemcu led is off when low
  
  pinMode(GPIO_12_RELAY, OUTPUT);
  digitalWrite(GPIO_12_RELAY, relayState); // our relay is off when high
  
  pinMode(GPIO_0_BUTTON, INPUT_PULLUP);
}
void handleIO() {
  // read the state of the switch into a local variable:
  int reading = digitalRead(GPIO_0_BUTTON);

  // if button is held down for the alloted time the config portal will start
  if (reading > 0) { // keep resetting timer until pressed
    timer_eraseConfig = millis();
  }
  if (millis() - timer_eraseConfig > ERASE_CONFIG_TIME) {
    handleEraseConfig();
  }

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != previousButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        //relayState = !relayState;
      }

      broadcastUpdates(); // send out websockets updates

    }
  }

  // set the LED:
  digitalWrite(GPIO_13_LED, !relayState);
  digitalWrite(GPIO_12_RELAY, relayState);

  // save the reading. Next time through the loop, it'll be the previousButtonState:
  previousButtonState = reading;
}
void monitorVoltage() {
    unsigned long currentMillis = millis();
    
    if (currentMillis - previousSampleMillis >= SAMPLE_INTERVAL) {
      previousSampleMillis = currentMillis;
      
      sensorValue = analogRead(GPIO_VOLTAGE_INPUT);
      // sensorValue * scaled-input-to-controller * scaled-input-to-voltage-divider
      //voltageValue = sensorValue * (5.0 / 1023.0) * (15.0 / 5.0);
      //voltageValue = sensorValue * (3.3 / 4095.0) * (15.0 / 2.4);
      voltageValue = sensorValue * (3.3 / 1023.0) * (15.0 / 2.4);
  
      // smoothing
      voltageReadings[voltageArrayIndex] = voltageValue;
      if (voltageArrayIndex >= AVG_VOLTAGE_SIZE - 1){
        voltageArrayIndex = 0;
      } else {
        voltageArrayIndex++;
      }
      totalVoltage = 0.0;
      for (int i = 0; i <= AVG_VOLTAGE_SIZE - 1; i++){
        totalVoltage = totalVoltage + voltageReadings[i];
      }
      averageVoltage = totalVoltage / AVG_VOLTAGE_SIZE;
    }
  
    if (averageVoltage < LOWER_VOLTAGE_LIMIT){
      if (currentMillis - previousLimitMillis >= LIMIT_DELAY) {
        relayState = HIGH;
      }
    } else if (averageVoltage > UPPER_VOLTAGE_LIMIT) {
      if (currentMillis - previousLimitMillis >= LIMIT_DELAY) {
        relayState = LOW;
      }
    } else {
      previousLimitMillis = currentMillis;
    }

    // if voltage value changes enough send out an update to clients
    if (((averageVoltage > previousBroadcastedVoltage + BROADCAST_TOLERANCE ||
        averageVoltage < previousBroadcastedVoltage - BROADCAST_TOLERANCE) &&
        currentMillis - previousPrintMillis >= BROADCAST_RATE_LIMIT) ||
        relayState != previousRelayState){
          
        previousPrintMillis = currentMillis;
        previousBroadcastedVoltage = averageVoltage;
        previousRelayState = relayState;
        broadcastUpdates();

        Serial.print("sensor=");
        Serial.print(sensorValue);  
        Serial.print(" voltage=");
        Serial.print(voltageValue);  
        Serial.print(" | avg=");
        Serial.print(averageVoltage); 
        Serial.print(" | output1=");
        Serial.print(relayState);
        Serial.println();
      
    }
}

/* websockets */
#define PAYLOAD_SIZE_IN 128
#define PAYLOAD_SIZE_OUT 200
void parseWebSocketMessage(JsonDocument& jsonBuffer); // need prototype because complex argument and arduino gets confused
void setupWebsocket() {
  webSocket_Server.begin();
  webSocket_Server.onEvent(webSocketEvent_Server);
  
  webSocket_Client.begin(ws_server_ip, atoi(ws_server_port), ws_server_url); // server address, port and URL
  webSocket_Client.onEvent(webSocketEvent_Client);
  //webSocket_Client.setAuthorization("user", "Password");
  webSocket_Client.setReconnectInterval(5000);
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  //webSocket_Client.enableHeartbeat(15000, 3000, 2);
}
void broadcastUpdates() {
  /*
  {
  "topic":"status",
    "data": {
      "DEVICE_ID": "ESP-xxxxxx",
      "HEARTBEAT_VALUE": 100,
      "BUTTON_STATE": true,
      "RELAY_STATE": true,
    }
  }
  */
  DynamicJsonDocument  jsonBuffer(PAYLOAD_SIZE_OUT); // https://arduinojson.org/v6/assistant/

  StreamString payload;
  
  jsonBuffer["topic"] = "status";
  jsonBuffer["data"]["device_id"] = DEVICE_ID.c_str();
  jsonBuffer["data"]["device_addr"] = WiFi.localIP().toString(); // causing boot loop
  jsonBuffer["data"]["device_build"] = BUILD_DATE.c_str();
  jsonBuffer["data"]["heartbeat_value"] = HEARTBEAT_VALUE; 

  jsonBuffer["data"]["button_state"] = (bool)buttonState;
  jsonBuffer["data"]["relay_state"] = (bool)relayState; // our relay is off when high
  jsonBuffer["data"]["avg_voltage"] = (float)averageVoltage;

  serializeJson(jsonBuffer,payload);

  webSocket_Server.broadcastTXT(payload);
  webSocket_Client.sendTXT(payload);
}
void webSocketEvent_Client(WStype_t type, uint8_t * payload, size_t length) { 
  /*
    type is the response type:
    0 – WStype_ERROR
    1 – WStype_DISCONNECTED
    2 – WStype_CONNECTED
    3 – WStype_TEXT
    4 – WStype_BIN
    5 – WStype_FRAGMENT_TEXT_START
    6 – WStype_FRAGMENT_BIN_START
    7 – WStype_FRAGMENT
    8 – WStype_FRAGMENT_FIN
    9 – WStype_PING
    10- WStype_PONG - reply from ping
  */
  // payload - the data (note this is a pointer)
  if(type == WStype_CONNECTED) {
      broadcastUpdates();
  }
  if(type == WStype_TEXT)
  {   
    DynamicJsonDocument jsonBuffer(PAYLOAD_SIZE_IN);
    deserializeJson(jsonBuffer, payload);
    const char* device_id = jsonBuffer["device_id"];
    if((strcmp(device_id, DEVICE_ID.c_str()) == 0) || (strcmp(device_id, "all") == 0)) { 
      parseWebSocketMessage(jsonBuffer);
    }
  }  
    else  // event is not TEXT. Display the details in the serial monitor
  {
    Serial.print("WStype = ");   Serial.println(type);  
    Serial.print("WS payload = ");
    // since payload is a pointer we need to type cast to char
    for(int i = 0; i < length; i++) { Serial.print((char) payload[i]); }
    Serial.println();
  } 
}
void webSocketEvent_Server(byte num, WStype_t type, uint8_t * payload, size_t length) {
  /*
    type is the response type:
    0 – WStype_ERROR
    1 – WStype_DISCONNECTED
    2 – WStype_CONNECTED
    3 – WStype_TEXT
    4 – WStype_BIN
    5 – WStype_FRAGMENT_TEXT_START
    6 – WStype_FRAGMENT_BIN_START
    7 – WStype_FRAGMENT
    8 – WStype_FRAGMENT_FIN
    9 – WStype_PING
    10- WStype_PONG - reply from ping
  */
  // payload - the data (note this is a pointer)
  if(type == WStype_CONNECTED) {
      broadcastUpdates();
  }
  if(type == WStype_TEXT)
  {   
    DynamicJsonDocument jsonBuffer(PAYLOAD_SIZE_IN);
    deserializeJson(jsonBuffer, payload);
    parseWebSocketMessage(jsonBuffer);
  }  
    else  // event is not TEXT. Display the details in the serial monitor
  {
    Serial.print("WStype = ");   Serial.println(type);  
    Serial.print("WS payload = ");
    // since payload is a pointer we need to type cast to char
    for(int i = 0; i < length; i++) { Serial.print((char) payload[i]); }
    Serial.println();
  } 
}
void parseWebSocketMessage(JsonDocument& jsonBuffer) {
  const char* topic = jsonBuffer["topic"];
  if(strcmp(topic, "command") == 0) {
    const char* child_id = jsonBuffer["child"]["id"];
    const char* child_state = jsonBuffer["child"]["state"];
      if(strcmp(child_id, "relay") == 0) {
        if(strcmp(child_state, "true") == 0) {
          relayState = HIGH;
          previousLimitMillis = millis();
        }
        if(strcmp(child_state, "false") == 0) {
          relayState = LOW;
          previousLimitMillis = millis();
        }
      }
    broadcastUpdates();
  }
  if(strcmp(topic, "status") == 0) {
    broadcastUpdates();
  }
}

/* configuration file */
bool loadConfig() {
  Serial.println("loading config");
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    //Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonDocument<200> doc; // https://arduinojson.org/v6/assistant/
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    //Serial.println("Failed to parse config file");
    return false;
  }

  const char* c_ws_server_ip = doc["webserver"]["ip"];
  const char* c_ws_server_port = doc["webserver"]["port"];
  const char* c_ws_server_url = doc["webserver"]["url"];
  
  strcpy(ws_server_ip, c_ws_server_ip);
  strcpy(ws_server_port, c_ws_server_port);
  strcpy(ws_server_url, c_ws_server_url);

  Serial.print("ws_server_ip: "); Serial.println(ws_server_ip);
  Serial.print("ws_server_port: "); Serial.println(ws_server_port);
  Serial.print("ws_server_url: "); Serial.println(ws_server_url);

  return true;
}
bool saveConfig() {
  Serial.println("saving config");
  StaticJsonDocument<200> doc; // https://arduinojson.org/v6/assistant/

  doc["webserver"]["ip"] = ws_server_ip;
  doc["webserver"]["port"] = ws_server_port;
  doc["webserver"]["url"] = ws_server_url;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  serializeJson(doc, configFile);
  return true;
}

/* setup */
void setup() {
      
  DEVICE_ID += String(ESP.getChipId(), HEX);

  Serial.begin(SERIAL_BAUD_RATE, SERIAL_8N1);
  delay(500); // give the serial port time to start up
    
  setupPins(); // should be before setupWifiManager() to catch button
  initLittleFS();
  loadConfig();
  timer_heartbeat = millis();

  setupWifiManager();
  setupOTA();

  setupWebserver();
  setupWebsocket();

  // seed timers
  previousSampleMillis = millis();
  previousLimitMillis = millis();
  previousPrintMillis = millis();

  //
  // done with setup
  //
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("AP address: ");
  Serial.println(WiFi.softAPIP());
}

/* loop */
void loop() {
  //
  // handle logic
  //
  if (OtaInProgress == false) {
    monitorVoltage();
    //checkWifi();
    handleIO();
    handleHeartbeat();
    webSocket_Server.loop();
    webSocket_Client.loop();
  }
  ArduinoOTA.handle();
  
  //
  // take a breather
  //
  yield();
}
