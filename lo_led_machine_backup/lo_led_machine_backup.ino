#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <HttpClient.h>
//#include <b64.h>
#include <LDateTime.h>

#define WIFI_AP "__SSID__"
#define WIFI_PASSWORD "__PASSWORD__"
#define WIFI_AUTH LWIFI_WPA // LWIFI_OPEN, LWIFI_WPA, LWIFI_WEP
#define per 50
#define per1 3
#define DEVICEID "__DeviceId__" // 
#define DEVICEKEY "__DeviceKey__" // 
#define SITE_URL "api.mediatek.com"

#define LED_PIN 13

unsigned int rtc;
unsigned int lrtc;
unsigned int rtc1;
unsigned int lrtc1;

char port[4]="   ";
char connection_info[21]="                    ";
char ip[21]="              ";             
int portnum;
int val = 0;
String tcpdata = String(DEVICEID) + "," + String(DEVICEKEY) + ",0";

LWiFiClient c;
LWiFiClient c2;
HttpClient http(c2);

void setup(){
  Serial.begin(115200);
  LTask.begin();
  LWiFi.begin();
  
  // comment out this line when Serial is not present, ie. 
  // run without connecting to PC
  while(!Serial) 
      delay(1000); 

  Serial.println("Connecting to AP");
  while(LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)) <= 0){
    delay(1000);
  }
  
  Serial.println("calling connection");

  while(!c2.connect(SITE_URL, 80)){
    Serial.println("Re-Connecting to WebSite");
    delay(1000);
  }
  delay(100);

  pinMode(LED_PIN, OUTPUT);
  getconnectInfo();
  connectTCP();
}

void getconnectInfo(){
  //calling RESTful API to get TCP socket connection
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
    Serial.println("waiting HTTP response: ");
    Serial.println(errorcount);
    errorcount += 1;
    if(errorcount > 10){
      c2.stop();
      return;
    }
    delay(100);
  }
  int err = http.skipResponseHeaders();

  int bodyLen = http.contentLength();
  Serial.print("Content length is: ");
  Serial.println(bodyLen);
  Serial.println();
  char c;
  int ipcount = 0;
  int count = 0;
  int separater = 0;
  while(c2){
    int v = c2.read();
    if(v != -1){
      c = v;
      Serial.print(c);
      connection_info[ipcount]=c;
      if(c==',')
      separater=ipcount;
      ipcount++;    
    }
    else{
      Serial.println("no more content, disconnect");
      c2.stop();
    }
  }
  
  Serial.print("The connection info: ");
  Serial.println(connection_info);
  int i;
  for(i=0; i<separater ;i++){
    ip[i]=connection_info[i];
  }
  int j=0;
  separater++;
  for(i=separater; i<21 && j<5; i++){
    port[j]=connection_info[i];
    j++;
  }
  Serial.println("The TCP Socket connection instructions:");
  Serial.print("IP: ");
  Serial.println(ip);
  Serial.print("Port: ");
  Serial.println(port);
  portnum = atoi (port);
  Serial.println(portnum);

} //getconnectInfo

void uploadstatus(){
  //calling RESTful API to upload datapoint to MCS to report LED status
  Serial.println("\nuploadstatus calling connection");
  LWiFiClient c2;  

  while(!c2.connect(SITE_URL, 80)){
    Serial.println("Re-Connecting to WebSite");
    delay(1000);
  }
  delay(100);
  
  String upload_led;
  if(digitalRead(LED_PIN) == 1)
    upload_led = "LED,,1";
  else
    upload_led = "LED,,0";

  int thislength = upload_led.length();
  HttpClient http(c2);
  c2.print("POST /mcs/v2/devices/");
  c2.print(DEVICEID);  
  c2.println("/datapoints.csv HTTP/1.1");
  
  c2.print("Host: ");
  c2.println(SITE_URL);
  c2.print("deviceKey: ");
  c2.println(DEVICEKEY);
  c2.print("Content-Length: ");
  c2.println(thislength);
  c2.println("Content-Type: text/csv");
  c2.println("Connection: close");
  c2.println();
  
  c2.println(upload_led);
  
  delay(500);

  int errorcount = 0;
  while(!c2.available()){
    Serial.print("waiting HTTP response: ");
    Serial.println(errorcount);
    errorcount += 1;
    if(errorcount > 10){
      c2.stop();
      return;
    }
    delay(100);
  }
  int err = http.skipResponseHeaders();

  int bodyLen = http.contentLength();
  Serial.print("xyz Content length is: ");
  Serial.println(bodyLen);
  Serial.println();
  while(c2){
    int v = c2.read();
    if(v != -1){
      Serial.print(char(v));
    }
    else{
      Serial.println("no more content, disconnect");
      c2.stop();
    }
  }
} // uploadstatus

//establish TCP connection with TCP Server with designate IP and Port
void connectTCP(){
  c.stop();
  Serial.print("Connecting to TCP: ");
  Serial.print(ip);Serial.print("  ");
  Serial.println(portnum);
  while(0 == c.connect(ip, portnum)){
    Serial.println("Re-Connecting to TCP");    
    delay(1000);
  }  
  Serial.println("send TCP connect");
  c.println(tcpdata);
  c.println();
  Serial.println("waiting TCP response:");
} //connectTCP

void heartBeat(){
  Serial.println("send TCP heartBeat");
  c.println(tcpdata);
  c.println();
} //heartBeat

void loop(){
  //Check for TCP socket command from MCS Server 
  String tcpcmd = "";
  while(c.available()){
      int v = c.read();
      if(v != -1){
        Serial.print((char)v);
        tcpcmd += (char)v;
        if(tcpcmd.substring(52).equals("1")){
          digitalWrite(LED_PIN, HIGH);
          Serial.println("LED switched to ON ");
          tcpcmd = "";
        }
        else if(tcpcmd.substring(52).equals("0")){  
          digitalWrite(LED_PIN, LOW);
          Serial.println("LED switched to OFF");
          tcpcmd = "";
        }
      }
   }

  LDateTime.getRtc(&rtc);
  if((rtc - lrtc) >= per){
    heartBeat();
    lrtc = rtc;
  }
  //Check for report datapoint status interval
  LDateTime.getRtc(&rtc1);
  if ((rtc1 - lrtc1) >= per1) {
    uploadstatus();
    lrtc1 = rtc1;
  }
}
