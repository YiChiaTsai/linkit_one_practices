#define WIFI_AP "N10U" // 
#define WIFI_PASSWORD "yn0933!@" // 
#define WIFI_AUTH LWIFI_WPA 

#define MCS_SITE_URL "api.mediatek.com" 
#define MCS_SITE_PORT 80
#define DEVICEID "DT5H6II2" // 
#define DEVICEKEY "6OjAm1l0KiYu0i0m" // 

#define GPS_ID "GPS" //

#include <stdarg.h>
#include <LGPS.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <HttpClient.h>
#include <aJSON.h>
#include <SerialCommand.h>
#include <Timer.h>

#define BAUDRATE 115200

SerialCommand g_scmd;

#define PF_BUF_SIZE 256 // limited to 128 chars
void pf(const char *fmt, ...){
  char tmp[PF_BUF_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tmp, PF_BUF_SIZE, fmt, args);
  va_end(args);
  Serial.print(tmp);
}

boolean gps_write(const char *id, const char *lati, const char *longi, const char *alti){
  pf("GPS upload, id %s, latitude %s, longitude %s, altitude %s\n", id, lati, longi, alti);
  boolean flag = false;

  LWiFiClient cli;  
  HttpClient hcli(cli);
  
  aJsonObject *values = aJson.createObject();
  aJson.addStringToObject(values, "latitude", lati);
  aJson.addStringToObject(values, "longitude", longi);
  aJson.addStringToObject(values, "altitude", alti);
  
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
    //pf("http client request succeed, rscode: %d\n", rscode);
    
    /*
    while(!hcli.endOfHeadersReached()){
      char c = hcli.readHeader();
      Serial.print(c);
    }
    Serial.println("");
    */
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

// examples:
// $GPGGA,065020.000,2501.8119,N,12124.9557,E,1,9,0.99,62.3,M,15.0,M,,*6A
// $GPRMC,065020.000,A,2501.8119,N,12124.9557,E,0.000,359.11,250515,,,A*5B

// the parsing code is from https://github.com/adafruit/Adafruit-GPS-Library/

uint8_t hour, minute, seconds, year, month, day;
uint16_t milliseconds;
// Floating point latitude and longitude value in degrees.
float latitude, longitude;

// Fixed point latitude and longitude value with degrees stored in units of 1/100000 degrees,
// and minutes stored in units of 1/100000 degrees.  See pull #13 for more details:
//   https://github.com/adafruit/Adafruit-GPS-Library/pull/13
int32_t latitude_fixed, longitude_fixed;
float latitudeDegrees, longitudeDegrees;
float geoidheight, altitude;
float speed, angle, magvariation, HDOP;
char lat, lon, mag;
boolean fix;
uint8_t fixquality, satellites;

uint8_t parseHex(char c) {
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A')+10;
    // if (c > 'F')
    return 0;
}

boolean parse(char *nmea) {
  // do checksum check

  // first look if we even have one
  if (nmea[strlen(nmea)-4] == '*') {
    uint16_t sum = parseHex(nmea[strlen(nmea)-3]) * 16;
    sum += parseHex(nmea[strlen(nmea)-2]);
    
    // check checksum 
    for (uint8_t i=1; i < (strlen(nmea)-4); i++) {
      sum ^= nmea[i];
    }
    if (sum != 0) {
      // bad checksum :(
      //return false;
    }
  }
  int32_t degree;
  long minutes;
  char degreebuff[10];
  // look for a few common sentences
  if (strstr(nmea, "$GPGGA")) {
    // found GGA
    char *p = nmea;
    // get time
    p = strchr(p, ',')+1;
    float timef = atof(p);
    uint32_t time = timef;
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);

    milliseconds = fmod(timef, 1.0) * 1000;

    // parse out latitude
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      strncpy(degreebuff, p, 2);
      p += 2;
      degreebuff[2] = '\0';
      degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      minutes = 50 * atol(degreebuff) / 3;
      latitude_fixed = degree + minutes;
      latitude = degree / 100000 + minutes * 0.000006F;
      latitudeDegrees = (latitude-100*int(latitude/100))/60.0;
      latitudeDegrees += int(latitude/100);
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      if (p[0] == 'S') latitudeDegrees *= -1.0;
      if (p[0] == 'N') lat = 'N';
      else if (p[0] == 'S') lat = 'S';
      else if (p[0] == ',') lat = 0;
      else return false;
    }
    
    // parse out longitude
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      strncpy(degreebuff, p, 3);
      p += 3;
      degreebuff[3] = '\0';
      degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      minutes = 50 * atol(degreebuff) / 3;
      longitude_fixed = degree + minutes;
      longitude = degree / 100000 + minutes * 0.000006F;
      longitudeDegrees = (longitude-100*int(longitude/100))/60.0;
      longitudeDegrees += int(longitude/100);
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      if (p[0] == 'W') longitudeDegrees *= -1.0;
      if (p[0] == 'W') lon = 'W';
      else if (p[0] == 'E') lon = 'E';
      else if (p[0] == ',') lon = 0;
      else return false;
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      fixquality = atoi(p);
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      satellites = atoi(p);
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      HDOP = atof(p);
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      altitude = atof(p);
    }
    
    p = strchr(p, ',')+1;
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      geoidheight = atof(p);
    }
    return true;
  }
  
  if (strstr(nmea, "$GPRMC")) {
   // found RMC
    char *p = nmea;

    // get time
    p = strchr(p, ',')+1;
    float timef = atof(p);
    uint32_t time = timef;
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);

    milliseconds = fmod(timef, 1.0) * 1000;

    p = strchr(p, ',')+1;
    // Serial.println(p);
    if (p[0] == 'A') 
      fix = true;
    else if (p[0] == 'V')
      fix = false;
    else
      return false;

    // parse out latitude
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      strncpy(degreebuff, p, 2);
      p += 2;
      degreebuff[2] = '\0';
      long degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      long minutes = 50 * atol(degreebuff) / 3;
      latitude_fixed = degree + minutes;
      latitude = degree / 100000 + minutes * 0.000006F;
      latitudeDegrees = (latitude-100*int(latitude/100))/60.0;
      latitudeDegrees += int(latitude/100);
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      if (p[0] == 'S') latitudeDegrees *= -1.0;
      if (p[0] == 'N') lat = 'N';
      else if (p[0] == 'S') lat = 'S';
      else if (p[0] == ',') lat = 0;
      else return false;
    }
    
    // parse out longitude
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      strncpy(degreebuff, p, 3);
      p += 3;
      degreebuff[3] = '\0';
      degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      minutes = 50 * atol(degreebuff) / 3;
      longitude_fixed = degree + minutes;
      longitude = degree / 100000 + minutes * 0.000006F;
      longitudeDegrees = (longitude-100*int(longitude/100))/60.0;
      longitudeDegrees += int(longitude/100);
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      if (p[0] == 'W') longitudeDegrees *= -1.0;
      if (p[0] == 'W') lon = 'W';
      else if (p[0] == 'E') lon = 'E';
      else if (p[0] == ',') lon = 0;
      else return false;
    }
    // speed
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      speed = atof(p);
    }
    
    // angle
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      angle = atof(p);
    }
    
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      uint32_t fulldate = atof(p);
      day = fulldate / 10000;
      month = (fulldate % 10000) / 100;
      year = (fulldate % 100);
    }
    // we dont parse the remaining, yet!
    return true;
  }

  return false;
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
    Serial.print("SSID: ");
    Serial.println(LWiFi.SSID());
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

void wifi_handler(){
  const char *arg = g_scmd.next();
  if(arg){
    if(strcmp(arg, "on") == 0){
      wifi_on();
    }
    else if(strcmp(arg, "off") == 0){
      wifi_off();
    }
    else if(strcmp(arg, "scan") == 0){
      wifi_scan();
    }
    else if(strcmp(arg, "status") == 0){
      wifi_status();
    }
    else if(strcmp(arg, "connect") == 0){
      wifi_connect();
    }
    else if(strcmp(arg, "c") == 0){
      wifi_connect();
    }
    else if(strcmp(arg, "disconnect") == 0){
      wifi_disconnect();
    }
    else if(strcmp(arg, "d") == 0){
      wifi_disconnect();
    }
    else{
      pf("Unknown command and argument: wifi %s\n", arg);
    }
  }
  else{
    wifi_status();
  }
}
void gps_on(){
  pf("GPS module is turned on.\n");
  LGPS.powerOn();  
  // LGPS.powerOn(GPS_BEIDOU);
}
void gps_off(){
  pf("GPS module is turned off.\n");
  LGPS.powerOff();
}
void gps_handler(){
  const char *arg = g_scmd.next();
  if(arg && strcmp(arg, "off") == 0){
    gps_off();
  }
  else{
    gps_on();
  }
}

void gps_cb(void *context){
  gpsSentenceInfoStruct info;
  LGPS.getData(&info);
  Serial.print((char *)info.GPGGA);
  Serial.print((char *)info.GPRMC);
  
  parse((char *)info.GPGGA);
  parse((char *)info.GPRMC);
  if(fixquality == 0){
    pf("!!! GPGGA does not have fix. !!!\n");
    digitalWrite(13, LOW);
  }
  else{
    digitalWrite(13, HIGH);
  }
  
  if(fix == false){
    pf("!!! GPRMC data is not ok yet. !!!\n");
  }
  pf("\n");
}
void upload_cb(void *context){
  if(LWiFi.status() != LWIFI_STATUS_CONNECTED){
    pf("Not connected to any WiFi AP. Upload aborted.\n");
    return;
  }
  
  if(fixquality > 0){
    String lat_s;
    String lon_s;
    String alt_s;
    if(lat == 'S')
      lat_s += '-';
    lat_s += String(latitudeDegrees, 6);
    if(lon == 'W')
      lon_s += '-';
    lon_s += String(longitudeDegrees, 6);
    alt_s += String(altitude, 2);
    if(gps_write(GPS_ID, lat_s.c_str(), lon_s.c_str(), alt_s.c_str())){
      pf("GPS upload...succeed.\n");
    }
    else{
      pf("GPS upload...failed.\n");
    }
  }
  else{
    pf("GPS fix quality is not ok. Upload aborted.\n");
  }
}

Timer timer;
int8_t timer_id_gps;
int8_t timer_id_upload;
int gps_delay = 5 * 1000;
int upload_delay = 30 * 1000;

void g_handler(){
  const char *arg = g_scmd.next();
  if(arg){
    int ms = atoi(arg);
    if(ms <= 0){
      timer.stop(timer_id_gps);
      pf("Will not get GPS data.\n", ms);
    }
    else{
      pf("Will get GPS data every %d seconds.\n", ms);
      gps_delay = ms * 1000;
      timer.stop(timer_id_gps);
      timer_id_gps = timer.every(gps_delay, gps_cb, NULL);
    }
  }
  else{
    pf("Will get GPS data every %d seconds.\n", gps_delay/1000);
  }
}
void u_handler(){
  const char *arg = g_scmd.next();
  if(arg){
    int ms = atoi(arg);
    if(ms <= 0){
      timer.stop(timer_id_upload);
      pf("Will not upload GPS data.\n", ms);
    }
    else{
      pf("Will upload GPS data every %d seconds.\n", ms);
      upload_delay = ms * 1000;
      timer.stop(timer_id_upload);
      timer_id_upload = timer.every(upload_delay, upload_cb, NULL);
    }
  }
  else{
    pf("Will upload GPS data every %d seconds.\n", upload_delay/1000);
  }
}
void stop_handler(){
  timer_id_gps = timer.stop(timer_id_gps);
  timer_id_upload = timer.stop(timer_id_upload);
  wifi_disconnect();
  wifi_off();
  gps_off();
}
void start_handler(){
  gps_on();
  delay(500);
  wifi_on();
  delay(500);
  wifi_connect();

  if(LWiFi.status() == LWIFI_STATUS_CONNECTED){
    pf("Will get GPS data every %d seconds.\n", gps_delay/1000);
    timer_id_gps = timer.every(gps_delay, gps_cb, NULL);
    pf("Will upload GPS data every %d seconds.\n", upload_delay/1000);
    timer_id_upload = timer.every(upload_delay, upload_cb, NULL);
  }
  else{
    pf("Not connected to any WiFi AP. Don't start timer to get GPS and upload.\n");
  }
}

void rgeo_g_handler(){
  String lat_s;
  String lon_s;
  if(lat == 'S')
    lat_s += '-';
  lat_s += String(latitudeDegrees, 6);
  if(lon == 'W')
    lon_s += '-';
  lon_s += String(longitudeDegrees, 6);

  reverse_g_geocoding(lat_s.c_str(), lon_s.c_str());
}
boolean reverse_g_geocoding(const char *lati, const char *longi){
  // maps.googleapis.com
  // /maps/api/geocode/json?key=&latlng=
  // key=
  // latlng
  const char *key = "AIzaSyAEi59PJRk7HRqJDVXtXx7gzL4XC2SS3ek";
  pf("reverse_geocoding, latitude %s, longitude %s\n", lati, longi);
  boolean flag = false;

  LWiFiClient cli;  
  HttpClient hcli(cli);
  
  String req = String("/maps/api/geocode/json?latlng=") + lati + "," + longi + "&key=" + "AIzaSyAEi59PJRk7HRqJDVXtXx7gzL4XC2SS3ek";
  pf("request is %s\n", req.c_str());
  int err = hcli.startRequest("maps.googleapis.com", 80, req.c_str(), HTTP_METHOD_GET, NULL);
  hcli.finishRequest();
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client request succeed, rscode: %d\n", rscode);
    
    while(!hcli.endOfHeadersReached()){
      char c = hcli.readHeader();
      Serial.print(c);
    }
    Serial.println("");
    
    // hcli.skipResponseHeaders();

    int bodyLen = hcli.contentLength();
    if(bodyLen != 0){
      char *body = (char *) malloc((bodyLen+1) * sizeof(char));
      hcli.read((uint8_t *) body, bodyLen);
      body[bodyLen] = '\0';
      pf("response body len: %d\n", bodyLen);
      pf("response body:\n%s\n", body);
      free(body);
    }
    else{
      int c;
      while( (c = hcli.read()) != -1){
        Serial.print((char) c);
      }
    }
    
    /*
    aJsonObject *root = aJson.parse((char *) body);
    aJsonObject *results = aJson.getObjectItem(root, "results");
    if(strcmp(results->valuestring, "Success.") == 0){
      flag = true;
    }

    aJson.deleteItem(root);*/
    
  }
  else{
    pf("http client request failed, err: %d, rscode: %d\n", err, rscode);
  }
  
  return flag;
}
void geo_g_handler(){
  
  geocoding_g("1600+Amphitheatre+Parkway,+Mountain+View,+CA");
}
boolean geocoding_g(const char *addr){
  // maps.googleapis.com
  // /maps/api/geocode/json?latlng=&key=???
  // key=
  // latlng
  const char *key = "AIzaSyAEi59PJRk7HRqJDVXtXx7gzL4XC2SS3ek";
  pf("geocoding, addr %s\n", addr);
  boolean flag = false;

  LWiFiClient cli;  
  HttpClient hcli(cli);
  
  String req = String("/maps/api/geocode/json?address=") + addr + "&key=" + key;
  pf("request is %s\n", req.c_str());
  int err = hcli.startRequest("maps.googleapis.com", 80, req.c_str(), HTTP_METHOD_GET, NULL);
  hcli.finishRequest();
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client request succeed, rscode: %d\n", rscode);
    
    while(!hcli.endOfHeadersReached()){
      char c = hcli.readHeader();
      Serial.print(c);
    }
    Serial.println("");
    
    // hcli.skipResponseHeaders();

    int bodyLen = hcli.contentLength();
    if(bodyLen != 0){
      char *body = (char *) malloc((bodyLen+1) * sizeof(char));
      hcli.read((uint8_t *) body, bodyLen);
      body[bodyLen] = '\0';
      pf("response body len: %d\n", bodyLen);
      pf("response body:\n%s\n", body);
      free(body);
    }
    else{
      int c;
      while( (c = hcli.read()) != -1){
        Serial.print((char) c);
      }
    }
    
    /*
    aJsonObject *root = aJson.parse((char *) body);
    aJsonObject *results = aJson.getObjectItem(root, "results");
    if(strcmp(results->valuestring, "Success.") == 0){
      flag = true;
    }

    aJson.deleteItem(root);*/
    
  }
  else{
    pf("http client request failed, err: %d, rscode: %d\n", err, rscode);
  }
  
  return flag;
}

void setup(){
  Serial.begin(BAUDRATE);
  pinMode(13, OUTPUT);
  
  g_scmd.addCommand("wifi", wifi_handler);
  g_scmd.addCommand("gps", gps_handler);
  g_scmd.addCommand("g", g_handler);
  g_scmd.addCommand("u", u_handler);
  g_scmd.addCommand("stop", stop_handler);
  g_scmd.addCommand("start", start_handler);
  
  // test, not work, 'cause google geocoding api needs SSL
  g_scmd.addCommand("rgeo_g", rgeo_g_handler); // test, not work
  g_scmd.addCommand("geo_g", geo_g_handler); // test, not work

  g_scmd.setDefaultHandler(cmd_unrecognized);
  
  start_handler();
}

void loop(){
  g_scmd.readSerial();
  timer.update();
}
