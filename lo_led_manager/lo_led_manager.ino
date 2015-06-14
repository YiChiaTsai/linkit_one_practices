#define WIFI_AP "N10U" //
#define WIFI_PASSWORD "yn0933!@" //
#define WIFI_AUTH LWIFI_WPA // LWIFI_OPEN, LWIFI_WPA, LWIFI_WEP

#define MCS_SITE_URL "api.mediatek.com"
#define MCS_SITE_PORT 80
#define DEVICEID "D0pxIEKU" // 
#define DEVICEKEY "Pw0Cl0i0f6oN1Zk1" // 

#define LED_ID "LED" // 
#define LED_CONTROL_ID "LED_CONTROL" // 

#include <stdarg.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <HttpClient.h>
#include <aJSON.h>
#include <LDateTime.h>

#define BAUDRATE 115200
#define LED_PIN 13

#define IP_STRING_LEN ((3 * 4 + 3) + 1)

LWiFiClient g_client;

#define PF_BUF_SIZE 256 // limited to 256 chars
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

boolean switch_write(const char *id, int v){
  pf("switch_write, id %s, value %d\n", id, v);
  boolean flag = false;

  LWiFiClient cli;  
  HttpClient hcli(cli);
  
  aJsonObject *values = aJson.createObject();
  aJson.addNumberToObject(values, "value", v);
  
  aJsonObject *dps_d = aJson.createObject();
  aJson.addStringToObject(dps_d, "dataChnId", id);
  aJson.addItemToObject(dps_d, "values", values);
  
  aJsonObject *dps = aJson.createArray();
  aJson.addItemToArray(dps, dps_d);
  
  aJsonObject *root = aJson.createObject();
  aJson.addItemToObject(root, "datapoints", dps);
  
  char *data = aJson.print(root);
  const int dataLength = strlen(data);
  //pf("dataLength:%d, data:\n%s\n", dataLength, data);
  
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
    
    // while(!hcli.endOfHeadersReached()){
      // char c = hcli.readHeader();
      // Serial.print(c);
    // }
    // Serial.println("");
    hcli.skipResponseHeaders();

    int bodyLen = hcli.contentLength();
    char *body = (char *) malloc((bodyLen+1) * sizeof(char));
    hcli.read((uint8_t *) body, bodyLen);
    body[bodyLen] = '\0';
    //pf("response body len: %d\n", bodyLen);
    //pf("response body:\n%s\n", body);
    
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

int switch_read(const char *id){
  pf("switch_read, id %s\n", id);
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
    
    // while(!hcli.endOfHeadersReached()){
      // char c = hcli.readHeader();
      // Serial.print(c);
    // }
    // Serial.println("");
    hcli.skipResponseHeaders();
    
    int bodyLen = hcli.contentLength();
    char *body = (char *) malloc((bodyLen+1) * sizeof(char));
    hcli.read((uint8_t *) body, bodyLen);
    body[bodyLen] = '\0';
    //pf("response body len: %d\n", bodyLen);
    //pf("response body:\n%s\n", body);
    
    aJsonObject *root = aJson.parse((char *) body);
    aJsonObject *dataChannels = aJson.getObjectItem(root, "dataChannels");
    aJsonObject *dataChannels0 = aJson.getArrayItem(dataChannels, 0);
    aJsonObject *dataPoints = aJson.getObjectItem(dataChannels0, "dataPoints");
    aJsonObject *dataPoints0 = aJson.getArrayItem(dataPoints, 0);
    aJsonObject *values = aJson.getObjectItem(dataPoints0, "values");
    aJsonObject *value = aJson.getObjectItem(values, "value");
    
    result = value->valueint;
    
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
  if(!cli.connected())
    return;
  
  static unsigned int rtc_old;
  unsigned int rtc;
  LDateTime.getRtc(&rtc);
  if((rtc - rtc_old) >= HEARTBEAT_PERIOD){
    rtc_old = rtc;
    Serial.println("send TCP heartBeat");
    
    // Heartbeat format:
    // deviceId, deviceKey, timestamp
    String data = String(DEVICEID) + "," + String(DEVICEKEY) + ",0";
    cli.println(data);
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

void setLedStatus(boolean status){
  pf("Turn LED %s\n", status == LOW ? "Off" : "On");
  digitalWrite(LED_PIN, status);
  
  if(switch_write(LED_ID, status)){
    pf("Upload LED status...succeed.\n");
  }
  else{
    pf("Upload LED status...failed.\n");
  }
}

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
      case 'g':{
        char ip[IP_STRING_LEN];
        int port;
        if(getIpPort(ip, &port)){
          pf("Successfully got the server's ip %s and port %d\n", ip, port);
        }
        else{
          pf("Failed to get the server's ip and port.\n");
        }
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
      case 'i':{
        boolean status = switch_read(LED_CONTROL_ID);
        pf("result is %d\n", status);
        setLedStatus(status);
      }
      break;
      // the followings are for debug
      case '1':
        pf("result is %d\n", switch_read(LED_CONTROL_ID));
      break;
      case '2':
        if(switch_write(LED_CONTROL_ID, digitalRead(LED_PIN))){
          pf("switch_write succeed\n");
        }
        else{
          pf("switch_write failed\n");
        }
      break;
      case '3':
        pf("result is %d\n", switch_read(LED_ID));
      break;
      case '4':
        if(switch_write(LED_ID, digitalRead(LED_PIN))){
          pf("switch_write succeed\n");
        }
        else{
          pf("switch_write failed\n");
        }
      break;
      case '5':
        if(switch_write(LED_ID, 0)){
          pf("switch_write succeed\n");
        }
        else{
          pf("switch_write failed\n");
        }
      break;
      case '6':
        if(switch_write(LED_ID, 1)){
          pf("switch_write succeed\n");
        }
        else{
          pf("switch_write failed\n");
        }
      break;
      case '7':{
        unsigned long x = 1431950112767; // 01 4D 66 E0 B7 FF
        Serial.println("---");
        Serial.println(x);     // 1726003199  66 E0 B7 FF
        pf("===%u\n", x);    // same
        pf("+++%lu\n", x);    // same
      }
      break;
    }
  }

  String tcpcmd;
  while(g_client.available()){
    char d[100+1];
    int cnt = g_client.read((uint8_t *) d, 100+1);
    d[cnt] = '\0';
    tcpcmd += d;
  }
  if(tcpcmd.length()){
    pf("Server sent TCP command: %s\n", tcpcmd.c_str());
    
    // The command format:
    // deviceId, deviceKey, timestamp, dataChnId, commandValue
    // for example:
    // D0pxIEKU,Pw0Cl0i0f6oN1Zk1,1430900112156,LED_CONTROL,0
    int idx = tcpcmd.indexOf(String(LED_CONTROL_ID));
    if(idx != -1){
      idx += strlen(LED_CONTROL_ID) + 1; // +1 for comma
      boolean status = tcpcmd[idx] == '0' ? LOW : HIGH;
      setLedStatus(status);
    }
  }

  heartBeat(g_client);
}
