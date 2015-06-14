#define WIFI_AP "N10U" //
#define WIFI_PASSWORD "yn0933!@" //
#define WIFI_AUTH LWIFI_WPA // LWIFI_OPEN, LWIFI_WPA, LWIFI_WEP

#define MCS_SITE_URL "api.mediatek.com"
#define MCS_SITE_PORT 80
#define DEVICEID "D7B0jyyc" // 
#define DEVICEKEY "VhCjl4QEhhuq046M" // 

#define TEMP_ID "TEMP" // 
#define HUMIDITY_ID "HUMIDITY" // 

#define DHT11PIN 6 //

#define MEASURE_PERIOD 3 // unit: seconds
#define UPLOAD_PERIOD 60 // unit: seconds

#include <stdarg.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <HttpClient.h>
#include <aJSON.h>
#include <LDateTime.h>
#include <DHT11.h>
#include <Timer.h>

#define BAUDRATE 115200

#define SAMPLE_COUNT 10
DHT11 dht11;
int g_temp[SAMPLE_COUNT];
int g_humidity[SAMPLE_COUNT];
int g_idx = SAMPLE_COUNT; // this is the invalid initial value

unsigned int measure_t_old;
unsigned int upload_t_old;

#define PF_BUF_SIZE 256 // limited to 256 chars
void pf(const char *fmt, ...){
  char tmp[PF_BUF_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tmp, PF_BUF_SIZE, fmt, args);
  va_end(args);
  Serial.print(tmp);
}

// upload data to data channel
boolean upload(int t, int h){
  if(LWiFi.status() != LWIFI_STATUS_CONNECTED){
    pf("WiFi AP is not connected.\n");
  }

  //pf("upload, t %d, h %d\n", t, h);
  boolean flag = false;

  LWiFiClient cli;  
  HttpClient hcli(cli);
  
  // for temperature
  aJsonObject *values_t = aJson.createObject();
  aJson.addNumberToObject(values_t, "value", t);
  aJsonObject *dps_t = aJson.createObject();
  aJson.addStringToObject(dps_t, "dataChnId", TEMP_ID);
  aJson.addItemToObject(dps_t, "values", values_t);
  
  // for humidity
  aJsonObject *values_h = aJson.createObject();
  aJson.addNumberToObject(values_h, "value", h);
  aJsonObject *dps_h = aJson.createObject();
  aJson.addStringToObject(dps_h, "dataChnId", HUMIDITY_ID);
  aJson.addItemToObject(dps_h, "values", values_h);
  
  // put them in the array
  aJsonObject *dps = aJson.createArray();
  aJson.addItemToArray(dps, dps_t);
  aJson.addItemToArray(dps, dps_h);
  
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
    //pf("http client request succeed, rscode: %d\n", rscode);
    
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

Timer timer_measure;
// measure periodically
void measure(void *context){
  int chk = dht11.read(DHT11PIN);
  if(chk == 0){
    pf("Humidity %d%%, Temperature %d C\n", dht11.humidity, dht11.temperature);
    
    if(g_idx == SAMPLE_COUNT){ // the very first readings
      for(int i = 0; i < SAMPLE_COUNT; i++){
        g_temp[i] = dht11.temperature;
        g_humidity[i] = dht11.humidity;
      }
      g_idx = 0;
    }
    else{ // 
      g_temp[g_idx] = dht11.temperature;
      g_humidity[g_idx] = dht11.humidity;
      g_idx++;
      if(g_idx >= SAMPLE_COUNT){
        g_idx = 0;
      }
    }
  }
  else{
    pf("DHT11 error: %s\n", dht11.err_str(chk));
  }
}

Timer timer_upload;
// upload periodically
void upload_helper(void *context){
  int temp_avg = 0;
  int humidity_avg = 0;
  for(int i = 0; i < SAMPLE_COUNT; i++){
    temp_avg += g_temp[i];
    humidity_avg += g_humidity[i];
  }
  temp_avg /= SAMPLE_COUNT;
  humidity_avg /= SAMPLE_COUNT;
  pf("Average: Humidity %d%%, Temperature %d C\n", humidity_avg, temp_avg);
  
  pf("Uploading temperature and humidity...");
  if(upload(temp_avg, humidity_avg)){
    pf("succeed.\n");
  }
  else{
    pf("failed.\n");
  }
}

void setup(){
  Serial.begin(BAUDRATE);
  
  LWiFi.begin();
  delay(100);
  
  pf("Connecting to WiFi AP...\n");
  while(LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)) <= 0){
    Serial.println("failed, retrying...");
    delay(500);
  }
  Serial.println("succeed.");
  
  timer_measure.every(MEASURE_PERIOD * 1000, measure, (void *)NULL);
  timer_upload.every(UPLOAD_PERIOD * 1000, upload_helper, (void *)NULL);
}

void loop(){
  timer_measure.update();
  timer_upload.update();
}
