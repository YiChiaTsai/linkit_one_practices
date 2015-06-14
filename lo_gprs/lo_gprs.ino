#include <LGPRS.h>
#include <LGPRSClient.h>
#include <HttpClient.h>

LGPRSClient cli; 

void setup(){
  Serial.begin(115200);
}

bool attachGprs(int tries){
  bool flag = false;
  
  Serial.print("Attaching GPRS...");
  for(int i = 0; i < tries; i++){
    if(LGPRS.attachGPRS()){
      Serial.print("ok");
      flag = true;
      break;
    }
    delay(1000);
    Serial.print("...");
  }
  Serial.println("");
  
  return flag;
}

void loop(){
  if(Serial.available()){
    char c = Serial.read();
    switch(c){
      case 'i':
        if(attachGprs(5)){
          Serial.println("Attaching GPRS succeed.");
        }
        else{
          Serial.println("Attaching GPRS failed. Timeout.");
        }
      break;
      case 't':{
        IPAddress a;
        char hostname[] = "www.google.com";
        Serial.print("Test: trying to resolve the IP address of ");
        Serial.println(hostname);
        if(LGPRS.hostByName(hostname, a) == 1){
          Serial.print("Test succeed. IP address is ");
          Serial.println(a);
        }
        else{
          Serial.println("Test failed.");
        }
      }
      break;
      case 'c':{
        Serial.print("Connecting...");
        if(cli.connect("www.google.com", 80)){
          Serial.println("succeed");
        }
        else{
          Serial.println("failed");
        }
      }
      break;
      case 'd':{
        Serial.print("Disconnected");
        cli.stop();
      }
      break;
      case 's':{
        HttpClient hcli(cli);
        if(hcli.get("www.google.com", 80, "/") == 0){
          Serial.println("Sending request succeed");
        }
        else{
          Serial.println("Sending request failed");
        }
      }
      case 'w':{
        cli.println("GET / HTTP/1.1"); 
        cli.println("Host: www.google.com:80");
        cli.println(); 
      }
    }
  }
  
  if(cli.available()){
    int c;
    while( (c = cli.read()) >= 0){
      Serial.print((char) c);
    }
    Serial.println("");
  }
}

