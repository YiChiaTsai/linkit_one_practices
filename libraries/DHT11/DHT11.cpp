#include "DHT11.h"

int DHT11::read(int pin)
{
  unsigned int loopCnt;
  
  uint8_t bits[5] = {0x00, 0x00, 0x00, 0x00, 0x00}; // BUFFER TO RECEIVE
  uint8_t cnt = 7;
  uint8_t idx = 0;
  
  // from MCS to DHT11
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(20); // at least 18
  digitalWrite(pin, HIGH);
  delayMicroseconds(40); // 20~40 
  
  pinMode(pin, INPUT);
  
  // DHT11 acknowledge or timeout
  loopCnt = 10000;
  while(digitalRead(pin) == LOW){
    if(loopCnt-- == 0){
      #ifdef DHT11_DEBUG
      Serial.println("timeout 1");
      #endif
      return DHTLIB_ERROR_TIMEOUT;
    }
  }
  
  loopCnt = 10000;
  while(digitalRead(pin) == HIGH){
    if(loopCnt-- == 0){
      #ifdef DHT11_DEBUG
      Serial.println("timeout 2s");
      #endif
      return DHTLIB_ERROR_TIMEOUT;
    }
  }
  
  // read 40 bits( = 5 bytes) or timeout
  for(int i = 0; i < 40; i++){
    loopCnt = 10000;
    while(digitalRead(pin) == LOW){
      if(loopCnt-- == 0){
        #ifdef DHT11_DEBUG
        Serial.print("timeout 3, i=");
        Serial.println(i);
        #endif
        return DHTLIB_ERROR_TIMEOUT;
      }
    }
    
    unsigned long t = micros();
    
    loopCnt = 10000;
    while(digitalRead(pin) == HIGH){
      if(loopCnt-- == 0){
        #ifdef DHT11_DEBUG
        Serial.print("timeout 4, i=");
        Serial.println(i);
        #endif
        if(i == 39){ // last bit of checksum
          break;
        }
        else{
          return DHTLIB_ERROR_TIMEOUT;
        }
      }
    }
    
    if((micros() - t) > 40) // is the bit 1 or 0?
      bits[idx] |= (1 << cnt);
    
    if(cnt == 0){
      cnt = 7; // restart at MSB
      idx++;   // next byte!
    }
    else{
      cnt--;
    }
  }
  
  // write to variables
  // as bits[1] and bits[3] are allways zero they are omitted in formulas.
  humidity = bits[0]; 
  temperature = bits[2]; 
  uint8_t checksum = bits[0] + bits[1] + bits[2] + bits[3];
  
  #ifdef DHT11_DEBUG
  Serial.print(bits[0], HEX); Serial.print(", ");
  Serial.print(bits[1], HEX); Serial.print(", ");
  Serial.print(bits[2], HEX); Serial.print(", ");
  Serial.print(bits[3], HEX); Serial.print(", ");
  Serial.print(bits[4], HEX); Serial.print(" =? ");
  Serial.println(checksum, HEX);
  #endif

  if(bits[4] != checksum){
    if(abs(bits[4] - checksum) == 1)
      return DHTLIB_ERROR_CHECKSUM_NOT_SO_BAD;
    else
      return DHTLIB_ERROR_CHECKSUM;
  }
  
  return DHTLIB_OK;
}
const char * DHT11::err_str(int err_code)
{
  const char *s;
  switch(err_code){
    case 0:
      s = "Ok";
    break;
    case -1:
      s = "Checksum error";
    break;
    case -2:
      s = "Timeout";
    break;
    case -3:
      s = "Checksum error(not very bad)";
    break;
    default:
      return "Unknown";
    break;
  }
  return s;
}
