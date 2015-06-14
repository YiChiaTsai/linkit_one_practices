#define WIFI_AP "N10U"
#define WIFI_PASSWORD "yn0933!@"
#define WIFI_AUTH LWIFI_WPA // LWIFI_OPEN, LWIFI_WPA, LWIFI_WEP

#define API_SITE_URL "api.openweathermap.org"
#define API_SITE_PORT 80
#define API_KEY "c1f407e5b2b71e4f35f5b94a6d4f109b"

#include <stdarg.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <HttpClient.h>
#include <aJSON.h>

#define BAUDRATE 115200
#define LED_PIN 13

#define PF_BUF_SIZE 256
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
}

int hexToDec(String hexString){
  int decValue = 0;
  int nextInt;
  
  for(int i = 0; i < hexString.length(); i++){
    nextInt = int(hexString.charAt(i));
    if('0' <= nextInt && nextInt <= '9') 
      nextInt = map(nextInt, '0', '9', 0, 9);
    if('a' <= nextInt && nextInt <= 'f') 
      nextInt = map(nextInt, 'a', 'f', 10, 15);
    if('A' <= nextInt && nextInt <= 'F') 
      nextInt = map(nextInt, 'A', 'F', 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return decValue;
}

void queryWeather(const char *city){
  pf("queryWeather of city: %s\n", city);
  
  LWiFiClient cli;  
  HttpClient hcli(cli);
  
  String req = String("/data/2.5/weather?q=") + city + "&units=metric";
  int err = hcli.startRequest(API_SITE_URL, API_SITE_PORT, req.c_str(), HTTP_METHOD_GET, NULL);
  hcli.sendHeader("x-api-key", API_KEY);
  hcli.finishRequest();
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client req succeed, rscode: %d\n", rscode);
    
    /*
    while(!hcli.endOfHeadersReached()){ // print headers
      char c = hcli.readHeader();
      Serial.print(c);
    }
    Serial.println("");
    */
    hcli.skipResponseHeaders();
    
    String data;
    while(1){ // read body of response
      String s = hcli.readStringUntil('\n');
      s.trim();
      int len = hexToDec(s);
      if(len == 0){
        hcli.stop();
        break;
      }
      
      char *buf = (char *) malloc((len+1) * sizeof(char));
      int tmp = hcli.read((uint8_t *) buf, len);
      buf[tmp] = '\0';
      data += buf;
      free(buf);
    }
    
    if(data[0] == '{'){ // analyze JSON
      aJsonObject *root = aJson.parse((char *) data.c_str());
      aJsonObject *name = aJson.getObjectItem(root, "name");
      aJsonObject *coord = aJson.getObjectItem(root, "coord");
      aJsonObject *lon = aJson.getObjectItem(coord, "lon");
      aJsonObject *lat = aJson.getObjectItem(coord, "lat");

      aJsonObject *main = aJson.getObjectItem(root, "main");
      aJsonObject *temp = aJson.getObjectItem(main, "temp");
      aJsonObject *humidity = aJson.getObjectItem(main, "humidity");
      
      aJsonObject *clouds = aJson.getObjectItem(root, "clouds");
      aJsonObject *clouds_all = aJson.getObjectItem(clouds, "all");
      
      Serial.print("City name: ");
      Serial.print(name->valuestring);
      Serial.print(", longitude: ");
      Serial.print(lon->valuefloat);
      Serial.print(", latitude: ");
      Serial.print(lat->valuefloat);
      Serial.println("");
      
      Serial.print("Temperature: ");
      Serial.print(temp->valuefloat);
      Serial.print(", Humidity: ");
      Serial.print(humidity->valueint);
      Serial.print("%, Clouds: ");
      Serial.print(clouds_all->valueint);
      Serial.println("%");
      
      aJson.deleteItem(root);
    }
    else{ // can't get weather data
      Serial.println(data);
    }
  }
  else{
    pf("http client request failed, rscode: %d, err: %d\n", rscode, err);
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
      case 't':{
        queryWeather("Taipei,Taiwan");
      }
      break;
      case 'n':{
        queryWeather("Newyork,US");
      }
      break;
      case 's':{
        queryWeather("Sydney,AU");
      }
      break;
      case 'o':{
        queryWeather("Osaka,JP");
      }
      break;
      case 'l':{
        queryWeather("London,UK");
      }
      break;
    }
  }
}
