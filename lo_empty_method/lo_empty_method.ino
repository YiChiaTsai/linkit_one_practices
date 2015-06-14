
/* this code can pass the compiler and linker,
   and sucessfully uploaded to the board.
   However, it can't run.
*/
#define BAUDRATE 115200
#define LED_PIN 13

class MyClass
{
public:
  void aMethod();
};

void aFunction(){
  MyClass x;
  x.aMethod();
}

void setup(){
  Serial.begin(BAUDRATE);
  pinMode(LED_PIN, OUTPUT);
}

void loop(){
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED high");
  delay(300);
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED low");
  delay(300);
}

