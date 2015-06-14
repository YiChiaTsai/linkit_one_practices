#define PHONE_NUMBER "0912345678"
#define SMS_MESSAGE "Hi, How are you? Sent from LinkIt ONE"

#include <LGSM.h>

#define LED_PIN 13

bool isSmsReady(int tries){
  bool flag = false;
  
  Serial.print("Waiting SMS to be ready...");
  for(int i = 0; i < tries; i++){
    if(LSMS.ready()){
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

bool sendSms(const char *msg){
  bool flag = false;
  
  Serial.print("Begin to send SMS...");
  if(LSMS.beginSMS(PHONE_NUMBER)){ // step 1
    Serial.println("ok");
    
    LSMS.print(msg); // step 2
    
    Serial.print("Finish sending SMS...");
    if(LSMS.endSMS()){ // step 3
      Serial.println("ok");
      flag = true;
    }
    else{
      Serial.println("fail");
    }
  }
  else{
    Serial.println("fail");
  }
  
  return flag;
}

void setup(){
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
}

void loop(){
  if(Serial.available()){
    char c = Serial.read();
    switch(c){
      case 'i':
        if(isSmsReady(5)){
          Serial.println("SMS is ready.");
        }
        else{
          Serial.println("SMS is NOT ready. Timeout.");
        }
      break;
      case 's':
        if(sendSms(SMS_MESSAGE)){
          Serial.println("Sending SMS succeed.");
        }
        else{
          Serial.println("Sending SMS failed.");
        }
      break;
      case '0': // send 0 to turn off LED
        if(sendSms("0")){
          Serial.println("Sending SMS succeed.");
        }
        else{
          Serial.println("Sending SMS failed.");
        }
      break;
      case '1': // send 1 to turn off LED
        if(sendSms("1")){
          Serial.println("Sending SMS succeed.");
        }
        else{
          Serial.println("Sending SMS failed.");
        }
      break;
    }
  }
  
  if(LSMS.available()){
    Serial.println("Receiving new SMS.");
    
    char buf[20];
    LSMS.remoteNumber(buf, 20);
    Serial.print("Number: ");
    Serial.println(buf);
    
    Serial.print("Content: ");
    
    int c;
    while( (c = LSMS.read()) > 0){
      Serial.print((char) c);
      if((char) c == '0'){
        digitalWrite(LED_PIN, LOW);
      }
      else if((char) c == '1'){
        digitalWrite(LED_PIN, HIGH);
      }
    }
    Serial.println("");
    
    LSMS.flush(); // delete SMS
  }
}

