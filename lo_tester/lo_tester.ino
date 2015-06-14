//
#define WIFI_AP "N10U"
#define WIFI_PASSWORD "yn0933!@"
#define WIFI_AUTH LWIFI_WPA // LWIFI_OPEN, LWIFI_WPA, LWIFI_WEP

#define API_SITE_URL "api.mediatek.com"
#define API_SITE_PORT 80

#define DEVICEID "D040kFld"
#define DEVICEKEY "yv9LG4m070HWgGb1"
//

// related to MCS API
#define HTTP_HEADER_DEVICEKEY "deviceKey"
//

#include <stdarg.h>
#include <LDateTime.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <HttpClient.h>
#include <aJSON.h>
#include <SerialCommand.h>

#include "tester_defs.h"

#define BAUDRATE 115200
#define LED_PIN 13

#define IP_STRING_LEN ((3 * 4 + 3) + 1)

// the index must match the enum values in DC_ID_ENUM
DC_ID_TYPE_STRUCT g_dc[DC_ID_MAX] = {
  {"SWITCH_C", DC_TYPE_SWITCH},
  {"SWITCH", DC_TYPE_SWITCH},
  {"CATEGORY_C", DC_TYPE_CATEGORY},
  {"CATEGORY", DC_TYPE_CATEGORY},
  {"INTEGER_C", DC_TYPE_INTEGER},
  {"INTEGER", DC_TYPE_INTEGER},
  {"FLOAT_C", DC_TYPE_FLOAT},
  {"FLOAT", DC_TYPE_FLOAT},
  {"HEX_C", DC_TYPE_HEX},
  {"HEX", DC_TYPE_HEX},
  {"STRING_C", DC_TYPE_STRING},
  {"STRING", DC_TYPE_STRING},
  {"GPS_C", DC_TYPE_GPS},
  {"GPS", DC_TYPE_GPS},
  {"GPIO_C", DC_TYPE_GPIO}, 
  {"GPIO", DC_TYPE_GPIO},
  {"PWM_C", DC_TYPE_PWM},
  {"PWM", DC_TYPE_PWM},
};

SerialCommand g_scmd;
LWiFiClient g_client;

#define PF_BUF_SIZE 128 // limited to 128 chars
void pf(const char *fmt, ...){
  char tmp[PF_BUF_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tmp, PF_BUF_SIZE, fmt, args);
  va_end(args);
  Serial.print(tmp);
}

boolean read_dc(const char *id){
  pf("read_dc, id %s\n", id);
  boolean result = false;

  LWiFiClient cli;  
  HttpClient hcli(cli);
  
  String req = String("/mcs/v2/devices/") + DEVICEID + "/datachannels/" + id + "/datapoints";
  int err = hcli.startRequest(API_SITE_URL, API_SITE_PORT, req.c_str(), HTTP_METHOD_GET, NULL);
  hcli.sendHeader(HTTP_HEADER_CONTENT_TYPE, "application/json");
  hcli.sendHeader(HTTP_HEADER_DEVICEKEY, DEVICEKEY);
  hcli.finishRequest();
  
  int rscode = hcli.responseStatusCode();
  
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client request succeed, rscode: %d\n", rscode);
    
    hcli.skipResponseHeaders();
    
    int body_len = hcli.contentLength();
    char *body;
    if(body_len > 0){
      body = (char *) malloc((body_len+1) * sizeof(char));
      hcli.read((uint8_t *) body, body_len);
      body[body_len] = '\0';
    }
    else{
      String s;
      while(hcli.connected()){
        int c;
        if(hcli.available() && (c = hcli.read() != -1)){
          pf("%c", (char) c);
        }
      }
      body_len = s.length();
      body = (char *) malloc((body_len+1) * sizeof(char));
      strcpy(body, s.c_str());
      body[body_len] = '\0';
    }
    pf("response body len: %d, content:\n", body_len);
    Serial.println(body);
    
    aJsonObject *root = aJson.parse((char *) body);
    aJsonObject *dataChannels = aJson.getObjectItem(root, "dataChannels");
    aJsonObject *dataChannels0 = aJson.getArrayItem(dataChannels, 0);
    aJsonObject *dataPoints = aJson.getObjectItem(dataChannels0, "dataPoints");
    aJsonObject *dataPoints0 = aJson.getArrayItem(dataPoints, 0);
    aJsonObject *values = aJson.getObjectItem(dataPoints0, "values");
    aJsonObject *value = aJson.getObjectItem(values, "value");
    
    pf("value is ");
    switch(value->type){
      case aJson_NULL:
        pf("null\n");
      break;
      case aJson_Boolean:
        pf("%s\n", value->valuebool ? "true" : "false");
      break;
      case aJson_Int:
        pf("%d\n", value->valueint);
      break;
      case aJson_Float:
        pf("%f\n", value->valuefloat);
      break;
      case aJson_String:
        pf("%s\n", value->valuestring);
      break;
      default:
        pf("(error, unknown JSON type %d)\n", value->type);
      break;
    }
    pf("\n");
    
    aJson.deleteItem(root);
    free(body);
    
    result = true;
  }
  else{
    pf("http client request failed, err: %d, rscode: %d\n", err, rscode);
  }
  
  return result;
}
boolean check_dc_type(DC_TYPE dct){
  return (DC_TYPE_START <= dct && dct < DC_TYPE_MAX);
}
boolean write_dc(const char *id, const char *v, DC_TYPE dct){
  pf("write_dc, id %s, value %s, dc_type %d\n", id, v, dct);
  boolean flag = false;
  if(!check_dc_type(dct)){
    pf("Error: wrong data channel type: %d\n", dct);
    return flag;
  }

  LWiFiClient cli;  
  HttpClient hcli(cli);
  
  aJsonObject *item;
  switch(dct){
    case DC_TYPE_SWITCH:{
      int x = atoi(v);
      if(v == NULL)
        x = 0;
      item = aJson.createItem(x);
    }
    break;
    case DC_TYPE_CATEGORY:
      item = aJson.createItem(v);
    break;
    case DC_TYPE_INTEGER:
      item = aJson.createItem(atoi(v));
    break;
    case DC_TYPE_FLOAT:
      item = aJson.createItem(v);
      //item = aJson.createItem(atof(v));
    break;
    case DC_TYPE_HEX:
      item = aJson.createItem(v);
    break;
    case DC_TYPE_STRING:
      item = aJson.createItem(v);
    break;
    case DC_TYPE_GPS: // todo
      item = aJson.createItem(v);
    break;
    case DC_TYPE_GPIO:
      item = aJson.createItem(atoi(v));
    break;
    case DC_TYPE_PWM: // todo
      item = aJson.createItem(v);
    break;
  }
  
  aJsonObject *values = aJson.createObject();
  aJson.addItemToObject(values, "value", item);
  
  aJsonObject *dps_d = aJson.createObject();
  aJson.addStringToObject(dps_d, "dataChnId", id);
  aJson.addItemToObject(dps_d, "values", values);
  
  aJsonObject *dps = aJson.createArray();
  aJson.addItemToArray(dps, dps_d);
  
  aJsonObject *root = aJson.createObject();
  aJson.addItemToObject(root, "datapoints", dps);
  
  char *body = aJson.print(root);
  const int body_len = strlen(body);
  //pf("dataLength:%d, body:\n%s\n", body_len, body);
  
  String req = String("/mcs/v2/devices/") + DEVICEID + "/datapoints";
  int err = hcli.startRequest(API_SITE_URL, API_SITE_PORT, req.c_str(), HTTP_METHOD_POST, NULL);
  hcli.sendHeader(HTTP_HEADER_CONTENT_TYPE, "application/json");
  hcli.sendHeader(HTTP_HEADER_DEVICEKEY, DEVICEKEY);
  hcli.sendHeader(HTTP_HEADER_CONTENT_LENGTH, body_len);
  
  hcli.write((uint8_t *) body, body_len);
  hcli.finishRequest();
  
  free(body);
  aJson.deleteItem(root);
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client request succeed, rscode: %d\n", rscode);
    
    hcli.skipResponseHeaders();

    int body_len = hcli.contentLength();
    if(body_len > 0){
      char *body = (char *) malloc((body_len+1) * sizeof(char));
      hcli.read((uint8_t *) body, body_len);
      body[body_len] = '\0';
      pf("response body len: %d, content:\n", body_len);
      Serial.println(body);
      free(body);
    }
    else{
      while(hcli.connected()){
        int c;
        if(hcli.available() && (c = hcli.read() != -1)){
          pf("%c", (char) c);
        }
      }
    }
    pf("\n");

    /*
    aJsonObject *root = aJson.parse((char *) body);
    aJsonObject *results = aJson.getObjectItem(root, "results");
    if(strcmp(results->valuestring, "Success.") == 0){
      flag = true;
    }

    aJson.deleteItem(root);
    free(body);*/
  }
  else{
    pf("http client request failed, err: %d, rscode: %d\n", err, rscode);
  }
  
  return flag;
}

void wifi_on(){
  pf("WiFi module is turned on.\n");
  LWiFi.begin();
}

void wifi_off(){
  pf("WiFi module is turned off.\n");
  LWiFi.end();
}

void wifi_scan(){
  int num_ap = LWiFi.scanNetworks();
  pf("Found %d WiFi networks.\n", num_ap);
  for(int i = 0; i < num_ap; i++){
    pf("%d, SSID: %s, RSSI: %d\n", i, LWiFi.SSID(i), LWiFi.RSSI(i));
  }
  pf("\n");
}

const char * const wifi_status_str(LWifiStatus ws){
  switch(ws){
    case LWIFI_STATUS_DISABLED:
      return "WiFi module status: disabled.";
    break;
    case LWIFI_STATUS_DISCONNECTED:
      return "WiFi module status: disconnected.";
    break;
    case LWIFI_STATUS_CONNECTED:
      return "WiFi module status: connected.";
    break;
  }
  return "WiFi module status: error, unknown status.";
}

void wifi_status(){
  uint8_t ma[VM_WLAN_WNDRV_MAC_ADDRESS_LEN] = {0};
  LWiFi.macAddress(ma);
  Serial.print("WiFi module's MAC address: ");
  int i;
  for(i = 0; i < VM_WLAN_WNDRV_MAC_ADDRESS_LEN-1; i++){
      pf("%02X:", ma[i]);
  }
  pf("%02X\n", ma[i]);
  
  LWifiStatus ws = LWiFi.status();
  pf("%s\n", wifi_status_str(ws));
  
  if(ws == LWIFI_STATUS_CONNECTED){
    Serial.print("Local IP: ");
    LWiFi.localIP().printTo(Serial);
    Serial.println("");
    
    Serial.print("Subnet mask: ");
    LWiFi.subnetMask().printTo(Serial);
    Serial.println("");
    
    Serial.print("Gateway IP: ");
    LWiFi.gatewayIP().printTo(Serial);
    Serial.println("");
    
    pf("RSSI: %d\n", LWiFi.RSSI());
  }
  pf("\n");
}

void wifi_connect(){
  pf("Connecting WiFi AP...");
  if(LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)) > 0){
    pf("succeed.\n");
  }
  else{
    pf("failed.\n");
  }
}

void wifi_disconnect(){
  pf("Disconnected from WiFi AP.\n");
  LWiFi.disconnect();
}

void cmd_unrecognized(const char *command){
  pf("Unknown command: %s\n", command);
}

void cmd_help(){
  for(int i = DC_ID_START; i < DC_ID_MAX; i++){
    pf("%2d %s\n", i, g_dc[i].id);
  }
  pf("\n");
}

boolean check_dc_id(int x){
  return (DC_ID_START <= x && x < DC_ID_MAX);
}

void cmd_r(){
  if(LWiFi.status() != LWIFI_STATUS_CONNECTED){
    pf("Not connect to any WiFi AP.\n");
    return;
  }
  
  const char *arg = g_scmd.next();
  int argv = atoi(arg);
  if(arg && check_dc_id(argv)){
    read_dc(g_dc[atoi(arg)].id);
  }
  else{
    pf("Invalid data channel ID: %s\n", arg);
  }
}

void cmd_w(){
  if(LWiFi.status() != LWIFI_STATUS_CONNECTED){
    pf("Not connect to any WiFi AP.\n");
    return;
  }
  
  int id_enum = -1;
  const char *id_enum_s = g_scmd.next();
  if(id_enum_s){
    id_enum = atoi(id_enum_s);
    
    if(check_dc_id(id_enum)){
      char *val_s = g_scmd.next();
      
      if(val_s){
        write_dc(g_dc[id_enum].id, val_s, g_dc[id_enum].type);
      }
      else{
        pf("Invalid value argument: %s\n", val_s);
      }
    }
    else{
      pf("Invalid id argument: %d\n", id_enum);
    }
  }
  else{
    pf("Invalid id argument: %s\n", id_enum_s);
  }
}

boolean get_ip_port(char *ip_out, int *port_out){
  // pf("get_ip_port\n");
  boolean flag = false;
  
  LWiFiClient cli;
  HttpClient hcli(cli);
  
  String req = String("/mcs/v2/devices/") + DEVICEID + "/connections";
  int err = hcli.startRequest(API_SITE_URL, API_SITE_PORT, req.c_str(), HTTP_METHOD_GET, NULL);
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
    
    pf("get_ip_port client status %d\n", hcli.connected());
  }
  
  hcli.stop();
  return flag;
} 

void make_connection(){
  g_client.stop();
  pf("Trying to make permanent connection to the server...\n");
  char ip[IP_STRING_LEN];
  int port;
  if(get_ip_port(ip, &port)){
    pf("Successfully got the server's ip %s and port %d\n", ip, port);
    
    pf("Permanently connecting to the server with ip %s, port %d...", ip, port);
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

void setup(){
  Serial.begin(BAUDRATE);

  g_scmd.addCommand("h", cmd_help);
  g_scmd.addCommand("r", cmd_r);
  g_scmd.addCommand("w", cmd_w);
  
  g_scmd.addCommand("b", wifi_on);
  g_scmd.addCommand("e", wifi_off);
  g_scmd.addCommand("l", wifi_scan);
  g_scmd.addCommand("s", wifi_status);
  g_scmd.addCommand("c", wifi_connect);
  g_scmd.addCommand("d", wifi_disconnect);
  
  g_scmd.addCommand("m", make_connection);
  
  g_scmd.setDefaultHandler(cmd_unrecognized);
}

#define HEARTBEAT_PERIOD 100 // seconds
void heart_beat(Client &cli){
  if(!cli.connected())
    return;
  
  static unsigned int rtc_old;
  unsigned int rtc;
  LDateTime.getRtc(&rtc);
  if((rtc - rtc_old) >= HEARTBEAT_PERIOD){
    rtc_old = rtc;
    Serial.println("send TCP heartBeat");
    
    // Heartbeat format: deviceId, deviceKey, timestamp
    String data = String(DEVICEID) + "," + String(DEVICEKEY) + ",0";
    cli.println(data);
    cli.println();
  }
}

#define SERVER_COMMAND_LEN_GUESS 100
void process_server_command(Client &cli){
  if(!cli.connected())
    return;
  
  String cmd;
  while(cli.available()){
    // note: bug, read(buffer, len) does not read the first byte, will read one more byte past end
    char d[SERVER_COMMAND_LEN_GUESS+1];
    int cnt = cli.read((uint8_t *) d, SERVER_COMMAND_LEN_GUESS+1);
    d[cnt] = '\0';
    cmd += d;
  }
  if(cmd.length()){
    pf("Server sent command: %s\n", cmd.c_str());
    
    // The command format:
    // deviceId, deviceKey, timestamp, dataChnId, commandValue
    // for example:
    // D0pxIEKU,Pw0Cl0i0f6oN1Zk1,1430900112156,LED_CONTROL,0
    /*
    int idx = cmd.indexOf(String(g_dc[DC_ID_SWITCH_C].id));
    if(idx != -1){
      idx += strlen(g_dc[DC_ID_SWITCH_C].id) + 1; // +1 for comma
      boolean status = cmd[idx] == '0' ? LOW : HIGH;
    }*/
  }
}
void loop(){
  g_scmd.readSerial();

  process_server_command(g_client);

  heart_beat(g_client);
}
