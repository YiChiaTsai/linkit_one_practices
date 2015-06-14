#include <stdarg.h>
#include <DHT11.h>
#define SERIAL_BAUDRATE 115200

#define DELAY_TIME 3000 // milliseconds

#define DHT11PIN 6
DHT11 dht11;

#define PF_BUF_SIZE 256 // limited to 256 chars
void pf(const char *fmt, ...){
  char tmp[PF_BUF_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tmp, PF_BUF_SIZE, fmt, args);
  va_end(args);
  Serial.print(tmp);
}

void setup(){
  Serial.begin(SERIAL_BAUDRATE);
}

void loop(){
  int chk = dht11.read(DHT11PIN);
  
  if(chk == 0){
    pf("Humidity %d%%, Temperature %d C\n", dht11.humidity, dht11. temperature);
  }
  else{
    Serial.println(dht11.err_str(chk));
  }
  
  delay(DELAY_TIME);
}

