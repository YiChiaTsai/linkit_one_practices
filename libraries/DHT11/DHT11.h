#ifndef DHT11_h 
#define DHT11_h

#if defined(ARDUINO) && (ARDUINO >= 100) 
  #include <Arduino.h> 
#else 
  #include <WProgram.h> 
#endif

#define DHT11LIB_VERSION "0.4.1"

// #define DHT11_DEBUG

#define DHTLIB_OK 0 
#define DHTLIB_ERROR_CHECKSUM -1 
#define DHTLIB_ERROR_TIMEOUT -2
#define DHTLIB_ERROR_CHECKSUM_NOT_SO_BAD -3

class DHT11 
{ 
public: 
  int read(int pin); 
  int humidity; 
  int temperature;
  const char *err_str(int err_code);
};


#endif
