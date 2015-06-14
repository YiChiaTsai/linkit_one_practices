#define WIFI_AP "N10U" // 
#define WIFI_PASSWORD "yn0933!@" // 
#define WIFI_AUTH LWIFI_WPA // LWIFI_OPEN, LWIFI_WPA, LWIFI_WEP

#define MCS_SITE_URL "api.mediatek.com" // MediaTek Cloud Sandbox
#define MCS_SITE_PORT 80
#define DEVICEID "DLfLSBiB" // 
#define DEVICEKEY "6qQfHuFRFLFPfTPj" // 

#define LED_PWM_ID "LED_PWM"
#define Analog0_ID "Analog0"

#include <stdarg.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <HttpClient.h>
#include <aJSON.h>
#include <LDateTime.h>

#define BAUDRATE 115200

#define LED_PIN 9
#define Analog_PIN A0

#define IP_STRING_LEN ((3 * 4 + 3) + 1)

LWiFiClient g_client;

#define PF_BUF_SIZE 256 // limited to 128 chars
void pf(const char *fmt, ...){
  char tmp[PF_BUF_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tmp, PF_BUF_SIZE, fmt, args);
  va_end(args);
  Serial.print(tmp);
}

void setup(){
  Serial.begin(BAUDRATE);
  pinMode(LED_PIN, OUTPUT);
}

boolean int_write(const char *id, int v, int v2=-1){
  pf("switch_write, id %s, value %d\n", id, v);
  boolean flag = false;

  LWiFiClient cli;  
  HttpClient hcli(cli);
  
  aJsonObject *values = aJson.createObject();
  aJson.addNumberToObject(values, "value", v);
  
  if(v2 != -1){
    aJson.addNumberToObject(values, "period", v2);
  }
  
  aJsonObject *dps_d = aJson.createObject();
  aJson.addStringToObject(dps_d, "dataChnId", id);
  aJson.addItemToObject(dps_d, "values", values);
  
  aJsonObject *dps = aJson.createArray();
  aJson.addItemToArray(dps, dps_d);
  
  aJsonObject *root = aJson.createObject();
  aJson.addItemToObject(root, "datapoints", dps);
  
  char *data = aJson.print(root);
  const int dataLength = strlen(data);
  pf("dataLength:%d, data:\n%s\n", dataLength, data);
  
  String req = String("/mcs/v2/devices/") + DEVICEID + "/datapoints";
  int err = hcli.startRequest(MCS_SITE_URL, MCS_SITE_PORT, req.c_str(), HTTP_METHOD_POST, NULL);
  hcli.sendHeader("Content-Type", "application/json");
  hcli.sendHeader("deviceKey", DEVICEKEY);
  hcli.sendHeader(HTTP_HEADER_CONTENT_LENGTH, dataLength);
  hcli.write((uint8_t *) data, dataLength);
  hcli.finishRequest();
  
  free(data);
  aJson.deleteItem(root);
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client request succeed, rscode: %d\n", rscode);
    
    while(!hcli.endOfHeadersReached()){
      char c = hcli.readHeader();
      Serial.print(c);
    }
    Serial.println("");
    //hcli.skipResponseHeaders();

    int bodyLen = hcli.contentLength();
    char *body = (char *) malloc((bodyLen+1) * sizeof(char));
    hcli.read((uint8_t *) body, bodyLen);
    body[bodyLen] = '\0';
    pf("response body len: %d\n", bodyLen);
    pf("response body:\n%s\n", body);
    
    aJsonObject *root = aJson.parse((char *) body);
    aJsonObject *results = aJson.getObjectItem(root, "results");
    if(strcmp(results->valuestring, "Success.") == 0){
      flag = true;
    }

    aJson.deleteItem(root);
    free(body);
  }
  else{
    pf("http client request failed, err: %d, rscode: %d\n", err, rscode);
  }
  
  return flag;
}

int int_read(const char *id, boolean is_string){
  pf("int_read, id %s\n", id);
  int result = -1;

  LWiFiClient cli;  
  HttpClient hcli(cli);
  
  String req = String("/mcs/v2/devices/") + DEVICEID + "/datachannels/" + id + "/datapoints";
  int err = hcli.startRequest(MCS_SITE_URL, MCS_SITE_PORT, req.c_str(), HTTP_METHOD_GET, NULL);
  hcli.sendHeader("Content-Type", "application/json");
  hcli.sendHeader("deviceKey", DEVICEKEY);
  hcli.finishRequest();
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client request succeed, rscode: %d\n", rscode);
    
    while(!hcli.endOfHeadersReached()){
      char c = hcli.readHeader();
      Serial.print(c);
    }
    Serial.println("");
    //hcli.skipResponseHeaders();
    
    int bodyLen = hcli.contentLength();
    char *body = (char *) malloc((bodyLen+1) * sizeof(char));
    hcli.read((uint8_t *) body, bodyLen);
    body[bodyLen] = '\0';
    pf("response body len: %d\n", bodyLen);
    pf("response body:\n%s\n", body);
    
    aJsonObject *root = aJson.parse((char *) body);
    aJsonObject *dataChannels = aJson.getObjectItem(root, "dataChannels");
    aJsonObject *dataChannels0 = aJson.getArrayItem(dataChannels, 0);
    aJsonObject *dataPoints = aJson.getObjectItem(dataChannels0, "dataPoints");
    aJsonObject *dataPoints0 = aJson.getArrayItem(dataPoints, 0);
    aJsonObject *values = aJson.getObjectItem(dataPoints0, "values");
    aJsonObject *value = aJson.getObjectItem(values, "value");
    
    if(is_string){
      result = atoi(value->valuestring);
    }
    else{
      result = value->valueint;
    }
    
    aJson.deleteItem(root);
    free(body);
  }
  else{
    pf("http client request failed, err: %d, rscode: %d\n", err, rscode);
  }
  
  return result;
}

#define HEARTBEAT_PERIOD 100 // seconds
void heartBeat(Client &cli){
  static unsigned int rtc_old;
  unsigned int rtc;
  LDateTime.getRtc(&rtc);
  if((rtc - rtc_old) >= HEARTBEAT_PERIOD){
    rtc_old = rtc;
    Serial.println("send TCP heartBeat");
    
    // Heartbeat format:
    // deviceId, deviceKey, timestamp
    String hearbeat_data = String(DEVICEID) + "," + String(DEVICEKEY) + ",0";
    cli.println(hearbeat_data);
    cli.println();
  }
}

boolean getIpPort(char *ip_out, int *port_out){
  // pf("getIpPort\n");
  boolean flag = false;
  
  LWiFiClient cli;
  HttpClient hcli(cli);
  
  String req = String("/mcs/v2/devices/") + DEVICEID + "/connections";
  int err = hcli.startRequest(MCS_SITE_URL, MCS_SITE_PORT, req.c_str(), HTTP_METHOD_GET, NULL);
  hcli.sendHeader("Content-Type", "application/json");
  hcli.sendHeader("deviceKey", DEVICEKEY);
  hcli.finishRequest();
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    // pf("http client req succeed, rscode: %d\n", rscode);
    
    hcli.skipResponseHeaders();
    int bodyLen = hcli.contentLength();
    // pf("response body len: %d\n", bodyLen);
    
    char *info = (char *) malloc((bodyLen+1) * sizeof(char));
    hcli.read((uint8_t *) info, bodyLen);
    info[bodyLen] = '\0';
    
    // pf("response body:\n%s\n", info);
    
    aJsonObject *root = aJson.parse(info);
    aJsonObject *ip = aJson.getObjectItem(root, "ip");
    aJsonObject *port = aJson.getObjectItem(root, "port");
    
    strncpy(ip_out, ip->valuestring, IP_STRING_LEN);
    *port_out = atoi(port->valuestring);
    
    aJson.deleteItem(root);
    free(info);
    
    flag = true;
  }
  else{
    pf("http client req failed, err: %d, rscode: %d\n", err, rscode);
  }
  
  hcli.stop();
  return flag;
} 

void setLedStatus(int v){
  v = constrain(v, 0, 255);
  analogWrite(LED_PIN, v);
}

int A0Status;

void loop(){
  if(Serial.available()){
    char d = Serial.read();
    switch(d){
      case 'b':{
        Serial.println("Turn on WiFi module");
        LWiFi.begin();
      }
      break;
      case 'e':{
        Serial.println("Turn off WiFi module");
        LWiFi.end();
      }
      break;
      case 'c':{
        Serial.print("Connecting WiFi AP...");
        if(LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)) > 0){
          Serial.println("succeed");
        }
        else{
          Serial.println("failed");
        }
      }
      break;
      case 'd':{
        LWiFi.disconnect();
        Serial.println("Disconnected from WiFi AP");
      }
      break;
      case 'm':{
        g_client.stop();
        pf("Trying to make permanently connection to the server...\n");
        char ip[IP_STRING_LEN];
        int port;
        if(getIpPort(ip, &port)){
          pf("Successfully got the server's ip %s and port %d\n", ip, port);
          
          pf("Permanently connecting to the server: ip %s, port %d...", ip, port);
          if(g_client.connect(ip, port)){
            pf("succeed.\n");
          }
          else{
            pf("failed.\n");
          }
        }
        else{
          pf("Failed to get the server's ip and port.\n");
        }
      }
      break;
      case '1':{
        int v = int_read(LED_PWM_ID, true);
        pf("result is %d\n", v);
        setLedStatus(v);
      }
      break;
      case '3':
        pf("result is %d\n", int_read(Analog0_ID, false));
      break;
      case '4':
        if(int_write(Analog0_ID, analogRead(Analog_PIN))){
          pf("int_write succeed\n");
        }
        else{
          pf("int_write failed\n");
        }
      break;
    }
  }

  String tcpcmd;
  while(g_client.available()){
    int c = g_client.read();
    tcpcmd += (char) c;
  }
  if(tcpcmd.length()){
    pf("Server sent TCP command: %s\n", tcpcmd.c_str());
    
    // The command Format:
    // deviceId, deviceKey, timestamp, dataChnId, commandValue
    // for example:
    // D0pxIEKU,Pw0Cl0i0f6oN1Zk1,1430900112156,LED_CONTROL,0
    
    int idx = tcpcmd.indexOf(String(LED_PWM_ID));
    pf("idx = %d\n", idx);
    if(idx != -1){
      idx += strlen(LED_PWM_ID) + 1; // +1 for comma
      int idx2 = tcpcmd.indexOf(String(","), idx);
      int v = tcpcmd.substring(idx, idx2).toInt();
      setLedStatus(v);
    }
  }

  if(g_client.connected()){
    heartBeat(g_client);
  }
  
  int A0Status_new = analogRead(Analog_PIN);
  if(abs(A0Status - A0Status_new) > 20){
    A0Status = A0Status_new;
    if(LWiFi.status() == LWIFI_STATUS_CONNECTED){
      int_write(Analog0_ID, A0Status);
    }
  }
}
