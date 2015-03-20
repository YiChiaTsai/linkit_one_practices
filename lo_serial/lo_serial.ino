#define BAUDRATE 19200
#define LED_PIN 13
void setup(){
  Serial.begin(BAUDRATE);
  pinMode(LED_PIN, OUTPUT);
}

void loop(){
  int d;
  while(Serial.available()){
    d = Serial.read();
    if(d == '0'){
      digitalWrite(LED_PIN, LOW);
    }
    else if(d == '1'){
      digitalWrite(LED_PIN, HIGH);
    }
  }
}


