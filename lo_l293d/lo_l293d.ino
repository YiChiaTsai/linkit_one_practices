
// chip's pins to Arduino's pins
#define E12_PIN 3
#define A1_PIN 4
#define A2_PIN 5
#define E34_PIN 9 
#define A3_PIN 6
#define A4_PIN 7

#define MOTOR_RIGHT E34_PIN
#define MOTOR_RIGHT_FORWARD A3_PIN
#define MOTOR_RIGHT_BACKWARD A4_PIN
#define MOTOR_LEFT E12_PIN
#define MOTOR_LEFT_FORWARD A1_PIN
#define MOTOR_LEFT_BACKWARD A2_PIN

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_RIGHT_FORWARD, OUTPUT);
  pinMode(MOTOR_RIGHT_BACKWARD, OUTPUT);
  pinMode(MOTOR_LEFT_FORWARD, OUTPUT);
  pinMode(MOTOR_LEFT_BACKWARD, OUTPUT);
}
void loop() {
  forward(); delay(3000); stop(); delay(1000);
  backward(); delay(3000); stop(); delay(1000);
  rotateRight(); delay(3000); stop(); delay(1000);
  rotateLeft(); delay(3000); stop(); delay(1000);
  turnRight(); delay(3000); stop(); delay(1000);
  turnLeft(); delay(3000); stop(); delay(1000);
  moveAndTurnRight(); delay(3000); stop(); delay(1000);
  moveAndTurnLeft(); delay(3000); stop(); delay(1000);
}
void forward(){
  setMotorRight(255, false);
  setMotorLeft(255, false);
}
void backward(){
  setMotorRight(255, true);
  setMotorLeft(255, true);
}
void rotateRight(){ 
  setMotorRight(255, true);
  setMotorLeft(255, false);
}
void rotateLeft(){ 
  setMotorRight(255, false);
  setMotorLeft(255, true);
}
void turnRight(){ 
  setMotorRight(0, false);
  setMotorLeft(255, false);
}
void turnLeft(){ 
  setMotorRight(255, false);
  setMotorLeft(0, false);
}
void moveAndTurnRight(){ 
  setMotorRight(127, false);
  setMotorLeft(255, false);
}
void moveAndTurnLeft(){ 
  setMotorRight(255, false);
  setMotorLeft(127, false);
}
void stop(){ 
  setMotorRight(0, false);
  setMotorLeft(0, false);
}

void setMotorRight(int speed, boolean reverse) {
  analogWrite(MOTOR_RIGHT, speed);
  digitalWrite(MOTOR_RIGHT_FORWARD, !reverse);
  digitalWrite(MOTOR_RIGHT_BACKWARD, reverse);
}
void setMotorLeft(int speed, boolean reverse) {
  analogWrite(MOTOR_LEFT, speed);
  digitalWrite(MOTOR_LEFT_FORWARD, !reverse);
  digitalWrite(MOTOR_LEFT_BACKWARD, reverse);
}

