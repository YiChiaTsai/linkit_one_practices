#include <LBattery.h>
#define BAUDRATE 19200

void setup(){
  Serial.begin(BAUDRATE);
}

void loop(){
  Serial.print("Battery level is ");
  Serial.println(LBattery.level());
  Serial.print("Charging: ");
  Serial.println(LBattery.isCharging() ? "yes" : "no");
  delay(5000);
}


