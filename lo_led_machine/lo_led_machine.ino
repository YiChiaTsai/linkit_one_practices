#define WIFI_AP "N10U"
#define WIFI_PASSWORD "yn0933!@"
#define WIFI_AUTH LWIFI_WPA // LWIFI_OPEN, LWIFI_WPA, LWIFI_WEP

#define MCS_SITE_URL "api.mediatek.com" // MediaTek Cloud Sandbox
#define MCS_SITE_PORT 80
#define DEVICEID "DvapQx4H" // 
#define DEVICEKEY "cA6hNMBBEu6N5ehS" // 

#define LED_ID "LED" //
#define LED_CONTROL_ID "LED_CONTROL" //
#define AGE_ID "AGE" //
#define TEMP_ID "TEMP" //

#include <stdarg.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <HttpClient.h>
#include <aJSON.h>
#include <LDateTime.h>

#define BAUDRATE 115200
#define LED_PIN 13

// 2^32 = 4 294 967 296
// timestamp of 2015.05.06 16:20:00 is about 1 430 900 112 156
// which is the microseconds from 1970.01.01 00:00:00

  /*
  // note: 2^32= 4 294 967 296 is smaller than 1 429 080 238 462 (unix time in milliseconds)
  unsigned int t;
  LDateTime.getRtc(&t); // 1429080238462,  3468826296
  t *= 1000;
  aJson.addNumberToObject(sd0, "timestamp", (double) t);*/
  

char g_ip[16]; // example: 123.456.789.123           
int g_port;
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

boolean led_upload(const char *id, int v, boolean is_str){
  pf("led_upload, v is %d\n", v);
  
  LWiFiClient cli;  
  HttpClient hcli(cli);
  int err = 0;
  boolean flag = false;
  
  aJsonObject *sd0 = aJson.createObject();
  aJson.addStringToObject(sd0, "dataChnId", id);
  
  /*
  // note: 2^32= 4 294 967 296 is smaller than 1 429 080 238 462 (unix time in milliseconds)
  unsigned int t;
  LDateTime.getRtc(&t); // 1429080238462,  3468826296
  t *= 1000;
  aJson.addNumberToObject(sd0, "timestamp", (double) t);*/
  
  aJsonObject *values = aJson.createObject();
  if(is_str){
    aJson.addStringToObject(values, "value", String(v).c_str());
  }
  else{
    aJson.addNumberToObject(values, "value", v);
  }
  
  aJson.addItemToObject(sd0, "values", values);
  
  aJsonObject *dps = aJson.createArray();
  aJson.addItemToArray(dps, sd0);
  
  aJsonObject *root = aJson.createObject();
  aJson.addItemToObject(root, "datapoints", dps);
  
  char *data = aJson.print(root);
  const int dataLength = strlen(data);
  pf("---len=%d---\n%s\n", dataLength, data);
  
  String req = String("/mcs/v2/devices/") + DEVICEID + "/datapoints";
  err = hcli.startRequest(MCS_SITE_URL, MCS_SITE_PORT, req.c_str(), HTTP_METHOD_POST, NULL);
  hcli.sendHeader("Content-Type", "application/json");
  hcli.sendHeader("deviceKey", DEVICEKEY);
  hcli.sendHeader(HTTP_HEADER_CONTENT_LENGTH, dataLength);
  hcli.write((uint8_t *) data, dataLength);
  hcli.finishRequest();
  
  free(data);
  aJson.deleteItem(root);
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client req succeed, rscode: %d\n", rscode);
    
    while(!hcli.endOfHeadersReached()){
      char c = hcli.readHeader();
      Serial.print(c);
    }
    Serial.println("");
    //hcli.skipResponseHeaders();
    int bodyLen = hcli.contentLength();
    
    
    char *info = (char *) malloc((bodyLen+1) * sizeof(char));
    hcli.read((uint8_t *) info, bodyLen);
    info[bodyLen] = '\0';
    pf("response body len: %d\n%s\n", bodyLen, info);
    free(info);
  }
  return flag;
}

boolean dp_read(const char *id){
  pf("dp_read, id %s\n", id);
  
  LWiFiClient cli;  
  HttpClient hcli(cli);
  int err = 0;
  boolean flag = false;
  
  // String req = String("/mcs/v2/devices/") + DEVICEID + "/datachannels/" + id + "/datapoints?start=1429081300731&end=1429084300731&limit=5";
  String req = String("/mcs/v2/devices/") + DEVICEID + "/datachannels/" + id + "/datapoints";
  err = hcli.startRequest(MCS_SITE_URL, MCS_SITE_PORT, req.c_str(), HTTP_METHOD_GET, NULL);
  hcli.sendHeader("Content-Type", "application/json");
  hcli.sendHeader("deviceKey", DEVICEKEY);
  hcli.finishRequest();
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client req succeed, rscode: %d\n", rscode);
    
    while(!hcli.endOfHeadersReached()){
      char c = hcli.readHeader();
      Serial.print(c);
    }
    Serial.println("");
    //hcli.skipResponseHeaders();
    int bodyLen = hcli.contentLength();
    
    char *info = (char *) malloc((bodyLen+1) * sizeof(char));
    hcli.read((uint8_t *) info, bodyLen);
    info[bodyLen] = '\0';
    pf("response body len: %d\n", bodyLen);
    Serial.println(info);
    free(info);
  }
  return flag;
}

// void per(unsigned int duration, unsigned int *time_old, void(*func)()){
  // unsigned int t;
  // LDateTime.getRtc(&t);
  
  // if((t - *time_old) >= duration){
    // *time_old = t;
    // func();
  // }
// }

boolean makeConnection(const char *ip, int port){
  g_client.stop();
  
  pf("makeConnection ip %s, port %d ...", ip, port);
  if(g_client.connect(ip, port)){
    Serial.println("succeed");
    return true;
  }
  else{
    Serial.println("failed");
  }
  return false;
}

#define HEARTBEAT_PERIOD 100
void heartBeat(Client &cli){
  Serial.println("send TCP heartBeat");
  
  String hearbeat_data = String(DEVICEID) + "," + String(DEVICEKEY) + ",0";
  cli.println(hearbeat_data);
  cli.println();
}

boolean getIpPort(){
  pf("in getIpPort...\n");
  LWiFiClient cli;
  HttpClient hcli(cli);
  int err = 0;
  boolean flag = false;
  
  String req = String("/mcs/v2/devices/") + DEVICEID + "/connections";
  err = hcli.startRequest(MCS_SITE_URL, MCS_SITE_PORT, req.c_str(), HTTP_METHOD_GET, NULL);
  hcli.sendHeader("Content-Type", "application/json");
  hcli.sendHeader("deviceKey", DEVICEKEY);
  hcli.finishRequest();
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client req succeed, rscode: %d\n", rscode);
    
    hcli.skipResponseHeaders();
    int bodyLen = hcli.contentLength();
    pf("response body len: %d\n", bodyLen);
    
    char *info = (char *) malloc((bodyLen+1) * sizeof(char));
    hcli.read((uint8_t *) info, bodyLen);
    info[bodyLen] = '\0';
    
    pf("---content---\n%s\n", info);
    
    aJsonObject *jsonobj = aJson.parse(info);
    pf("root type %d\n", jsonobj->type);
    aJsonObject *ipj = aJson.getObjectItem(jsonobj, "ip");
    aJsonObject *portj = aJson.getObjectItem(jsonobj, "port");
    pf("ip %s, type %d, port %s, type %d\n", ipj->valuestring, ipj->type, portj->valuestring, portj->type);
    
    strncpy(g_ip, ipj->valuestring, 20);
    g_port = atoi(portj->valuestring);
    pf("ip is %s, port is %d\n", g_ip, g_port);
    
    aJson.deleteItem(jsonobj);
    
    free(info);
    
    flag = true;
  }
  else{
    pf("http client req failed, err: %d, rscode: %d\n", err, rscode);
  }
  
  hcli.stop();
  return flag;
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
          Serial.println(" succeed");
        }
        else{
          Serial.println(" failed");
        }
      }
      break;
      case 'd':{
        LWiFi.disconnect();
        Serial.println("Disconnected from WiFi AP");
      }
      break;
      case 'q':
        Serial.println("Trying to get server's ip and port...");
        if(getIpPort()){
          pf("    succeed. ip %s, port %d\n", g_ip, g_port);
        }
        else{
          pf("    failed.\n");
        }
      break;
      case 'm':
        if(getIpPort()){
          pf("makeconnection %d\n", makeConnection(g_ip, g_port));
          heartBeat(g_client);
        }
      break;
      case 'h':
        heartBeat(g_client);
      break;
      case 'u':
        led_upload(LED_ID, digitalRead(LED_PIN), false);
      break;
      case 'a':
        led_upload(AGE_ID, 300, true);
      break;
      case '2':
        dp_read(LED_CONTROL_ID);
      break;
      case '3':
        dp_read(AGE_ID);
      break;
      case '4':
        dp_read(AGE_ID);
      break;
    }
  }

  if(g_client.available()){
    int guessLen = 4 + strlen(DEVICEID) + strlen(DEVICEKEY) + 13 + 100;
    char *data = (char *) malloc(guessLen * sizeof(char));
    // warning: is this a bug: LWiFiClient.read(buf,len) doesn't read the first byte  !!!
    data[0] = g_client.read();
    int realLen = 1 + g_client.read((uint8_t *)(data+1), guessLen-1);
    data[realLen] = '\0';
    pf("response real len %d, guessLen %d\n", realLen, guessLen);
    pf("%s\n", data);
    
    if(strstr(data, LED_CONTROL_ID) != NULL){
      digitalWrite(LED_PIN, data[realLen-1] == '1' ? HIGH : LOW);
    }
    else if(strstr(data, AGE_ID) != NULL){
      const char *sub = strrchr(data, ',');
      pf("age is %s\n", sub+1);
    }
    else if(strstr(data, TEMP_ID) != NULL){
      const char *sub = strrchr(data, ',');
      pf("temp is %s\n", sub+1);
    }
    free(data);
  }

  // per(UPLOADSTATUS_PER, &time_uploadStatus, uploadStatus);
  // per(HEARTBEAT_PER, &time_heartBeat, heartBeat);
}
