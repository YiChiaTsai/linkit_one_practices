#define BAUDRATE 115200
#define LED_PIN 13

void setup() {
  Serial.begin(BAUDRATE);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  int c;
  if( (c = Serial.read()) != -1){
    switch(c){
      case '0':
        digitalWrite(LED_PIN, LOW);
      break;
      case '1':
        digitalWrite(LED_PIN, HIGH);
      break;
      case 'r':
        Serial.println(digitalRead(LED_PIN));
      break;
    }
  }
}

