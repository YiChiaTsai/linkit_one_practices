#include "settings.h"
#include <stdarg.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <HttpClient.h>
#include <LDateTime.h>

#define BAUDRATE 115200
#define LED_PIN 13

char ip[21]; // example: 123.456.789.123           
char port[5]; // example: 80, 544, 1023
int portnum;

String tcpdata = String(DEVICEID) + "," + String(DEVICEKEY) + ",0";

LWiFiClient c; // regularly retrieve info from MCS

#define PF_BUF_SIZE 128 // limited to 128 chars
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
  LWiFi.begin();
  
  while(!Serial) // comment out when Serial is not present, ie. 
      delay(1000); // when run without connecting to PC

  Serial.println("Connecting to WiFi AP");
  while(LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)) <= 0){
    delay(1000);
  }
  
  getconnectInfo();
  connectTCP();
}

//calling RESTful API to get TCP socket connection
void getconnectInfo(){
  LWiFiClient c2;
  HttpClient http(c2);
    
  pf("getconnectInfo: client trying to connect\n");
  while(!c2.connect(SITE_URL, SITE_PORT)){
    pf("getconnectInfo: reconnecting...\n");
    delay(1000);
  }
  delay(100);

  c2.print("GET /mcs/v2/devices/");
  c2.print(DEVICEID);
  c2.println("/connections.csv HTTP/1.1");
  c2.print("Host: ");
  c2.println(SITE_URL);
  c2.print("deviceKey: ");
  c2.println(DEVICEKEY);
  c2.println("Connection: close");
  c2.println();
  delay(500);

  int errorcount = 0;
  while(!c2.available()){
    pf("waiting HTTP response, errorcount: %d\n", errorcount);
    errorcount++;
    if(errorcount > 10){
      pf("getconnectInfo tried too many times, aborted");
      c2.stop();
      return;
    }
    delay(100);
  }
  
  int err = http.skipResponseHeaders();
  pf("Content length is: %d\n\n", http.contentLength());
  
  char connection_info[21];
  int idx = 0;
  int separater = 0;
  while(c2){
    char v = c2.read();
    if(v != -1){
      connection_info[idx] = v;
      if(v == ',')
        separater = idx;
      idx++;    
    }
    else{
      Serial.println("no more content, disconnect");
      c2.stop();
    }
  }
  connection_info[idx] = '\0';
  pf("The connection info: %s\n", connection_info);
  
  int i;
  for(i = 0; i < separater; i++){
    ip[i] = connection_info[i];
  }
  ip[i] = '\0';
  
  int j = 0;
  for(i = separater+1; i<21 && j<5; i++){
    port[j] = connection_info[i];
    j++;
  }
  port[j] = '\0';
  
  pf("The TCP Socket connection socket:\n");
  portnum = atoi(port);
  pf("IP: %s, Port: %s, portnum: %d\n\n", ip, port, portnum);
} //getconnectInfo

//establish TCP connection with TCP Server with designate IP and Port
void connectTCP(){
  c.stop();
  
  pf("Connecting to TCP: %s %d\n", ip, portnum);
  while(0 == c.connect(ip, portnum)){
    pf("Reconnecting to TCP: %s %d\n", ip, portnum);
    delay(1000);
  }  
  pf("send TCP connect\n");
  c.println(tcpdata);
  c.println();
  pf("waiting TCP response:\n");
} //connectTCP

#define UPLOADSTATUS_PER 3
unsigned int time_uploadStatus;
//calling RESTful API to upload datapoint to MCS to report LED status
void uploadStatus(){
  Serial.println("uploadStatus calling connection");
  LWiFiClient c2;  

  while(!c2.connect(SITE_URL, SITE_PORT)){
    pf("uploadStatus: reconnecting to the website");
    delay(1000);
  }
  delay(100);

  HttpClient http(c2);
  c2.print("POST /mcs/v2/devices/");
  c2.print(DEVICEID);  
  c2.println("/datapoints.csv HTTP/1.1");
  c2.print("Host: ");
  c2.println(SITE_URL);
  c2.print("deviceKey: ");
  c2.println(DEVICEKEY);
  
  String led_status_data(LED_ID);
  if(digitalRead(LED_PIN) == 1){
    led_status_data += ",,1";
    pf("upload LED status ON\n");
  }
  else{
    led_status_data += ",,0";
    pf("upload LED status OFF\n");
  }

  c2.print("Content-Length: ");
  c2.println(led_status_data.length());
  c2.println("Content-Type: text/csv");
  c2.println("Connection: close");
  c2.println();
  c2.println(led_status_data);
  delay(500);

  int errorcount = 0;
  while(!c2.available()){
    pf("uploadStatus: waiting HTTP response, errorcount: %d\n", errorcount);
    errorcount += 1;
    if(errorcount > 10){
      c2.stop();
      return;
    }
    delay(100);
  }
  
  int err = http.skipResponseHeaders();
  pf("uploadStatus: Content length is: %d\n\n", http.contentLength());

  while(c2){
    int v = c2.read();
    if(v != -1){
      Serial.print(char(v));
    }
    else{
      pf("no more content, disconnect");
      c2.stop();
    }
  }
  pf("\n");
} // uploadStatus

#define HEARTBEAT_PER 50
unsigned int time_heartBeat;
void heartBeat(){
  Serial.println("send TCP heartBeat");
  c.println(tcpdata);
  c.println();
} //heartBeat

void per(unsigned int duration, unsigned int *time_old, void(*func)()){
  unsigned int t;
  LDateTime.getRtc(&t);
  
  if((t - *time_old) >= duration){
    *time_old = t;
    func();
  }
}

void loop(){
  String tcpcmd = "";
  while(c.available()){ //Check for TCP socket command from MCS Server 
      int v = c.read();
      if(v != -1){
        Serial.print((char)v);
        tcpcmd += (char)v;
        if(tcpcmd.substring(52).equals("1")){
          digitalWrite(LED_PIN, HIGH);
          pf("\nLED status set to ON\n");
          tcpcmd = "";
        }
        else if(tcpcmd.substring(52).equals("0")){  
          digitalWrite(LED_PIN, LOW);
          pf("\nLED status set to OFF\n");
          tcpcmd = "";
        }
      }
  }
  
  per(UPLOADSTATUS_PER, &time_uploadStatus, uploadStatus);
  per(HEARTBEAT_PER, &time_heartBeat, heartBeat);
}
