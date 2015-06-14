#include <Servo.h> 
 
Servo myservo; 

void setup() { 
  Serial.begin(115200);
  myservo.attach(9); 
} 
 
void loop() { 
  if(Serial.available()){
      char c = Serial.read();
      if('0' <= c && c <= '9'){
        int val = (c - '0') * 20;
        myservo.write(val);
        Serial.println(val);
      }
  }
} 
