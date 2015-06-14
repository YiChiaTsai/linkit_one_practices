#include <Stepper.h>
#define SERIAL_BAUDRATE 115200

#define IN1_PIN 8
#define IN2_PIN 9
#define IN3_PIN 10
#define IN4_PIN 11
#define STEPS 1024   // how many steps per one turn

Stepper motor(STEPS, IN1_PIN, IN2_PIN, IN3_PIN, IN4_PIN);

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  
  motor.setSpeed(40); // rotation speed
}
void loop() {
  if(Serial.available()) {
    int steps = Serial.parseInt(); 
    Serial.println(steps);
    motor.step(steps); 
  }
}

