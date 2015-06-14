#define BAUDRATE 19200

void setup(){
  Serial.begin(BAUDRATE);
  Serial1.begin(BAUDRATE);
}

void loop(){
  int c;
  if( (c = Serial.read()) != -1){
    Serial1.write((uint8_t) c);
  }

  if( (c = Serial1.read()) != -1){
    Serial.write((uint8_t) c);
  }  
}

