// #include <LSD.h>
#include <LFlash.h>
#define BAUDRATE 19200
#define LED_PIN 13

void setup(){
  Serial.begin(BAUDRATE);
  Serial.setTimeout(1000);
  pinMode(LED_PIN, OUTPUT);
  if(LFlash.begin()){
    Serial.println("Flash ok");
    digitalWrite(LED_PIN, HIGH);
  }
  else{
    Serial.println("Flash fail");
    digitalWrite(LED_PIN, LOW);
  }
}

void loop(){
  char d[64+1+1];
  int len;
  LFile f;
  while(Serial.available()){
    len = Serial.readBytesUntil('\n', d, 64);
    d[len] = '\n';
    d[len+1] = '\0';
    Serial.print(d);
    f = LFlash.open("data.txt", FILE_WRITE);
    f.write(d);
    f.close();
  }
}


