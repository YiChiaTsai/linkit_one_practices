#include <LWiFi.h> 
#include <LWiFiClient.h> 

#define BAUDRATE 19200

#define WIFI_NAME "N10U"
#define WIFI_PASSWD "yn0933!@"
#define URL "www.mediatek.com" 

LWiFiClient cli;

void setup(){
  Serial.begin(BAUDRATE);
  LWiFi.begin();
}

void loop(){
  if(Serial.available()){
    char d = Serial.read();
    switch(d){
      case 'c':{
        if(LWiFi.connectWPA(WIFI_NAME, WIFI_PASSWD) <= 0){
          Serial.println("Connect ok");
        }
        else{
          Serial.println("Connect fail");
        }
      }
      break;
      case 'g':{
        Serial.println("Connecting to website...");
        cli.connect(URL, 80);
        cli.println("GET / HTTP/1.1"); 
        cli.println("Host: " URL); 
        cli.println("Connection: close");
        cli.println();
      }
      break;
    }
  }
  while(cli.available()){
    int x;
    x = cli.read();
    if(x < 0)
      break;
    Serial.print((char) x); 
  }
}


