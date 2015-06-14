#include <SoftwareSerial.h>
#define BAUDRATE 19200

SoftwareSerial Serial_s(2, 3); // RX, TX

void setup(){
  Serial.begin(BAUDRATE);
  Serial_s.begin(BAUDRATE);
}

void loop(){
  int c;
  if( (c = Serial.read()) != -1){
    Serial_s.write((uint8_t) c);
  }

  if( (c = Serial_s.read()) != -1){
    Serial.write((uint8_t) c);
  }  
}

