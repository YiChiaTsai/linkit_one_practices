#include <stdarg.h>
#include <LBT.h> 
#include <LBTClient.h> 

#define BAUDRATE 115200

#define PF_BUF_SIZE 256 // limited to 256 chars
void pf(const char *fmt, ...){
  char tmp[PF_BUF_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tmp, PF_BUF_SIZE, fmt, args);
  va_end(args);
  Serial.print(tmp);
}

String make_bt_address(const char *fmt, ...){
  char tmp[PF_BUF_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tmp, PF_BUF_SIZE, fmt, args);
  va_end(args);
  String rst = tmp;
  return rst;
}

void setup() {
  Serial.begin(BAUDRATE);
}

void loop() {
  if(Serial.available()){
    int c = Serial.read();
    switch(c){
      case 'b':
        Serial.print("LBTClient.begin..."); 
        if(LBTClient.begin()){
          Serial.println("succeed."); 
        }
        else{
          Serial.println("failed."); 
        }
      break;
      case 'e':
        Serial.println("LBTClient.end"); 
        LBTClient.end();
      break;
      case 's':{
        int cnt = LBTClient.scan();
        pf("LBTClient.scan cnt is %d\n", cnt);
        for(int i = 0; i < cnt; i++){
          LBTDeviceInfo info;
          if(LBTClient.getDeviceInfo(i, &info)){
            pf("%d, %02x:%02x:%02x:%02x:%02x:%02x, %s\n", i, 
              info.address.lap[0], info.address.lap[1], info.address.lap[2], 
              info.address.uap, info.address.nap[0], info.address.nap[1], 
              info.name);
          }
        }
      }
      break;
      case '0':
      case '1':{
        int idx = c - '0';
        LBTDeviceInfo info;
        if(LBTClient.getDeviceInfo(idx, &info)){
          String addr = make_bt_address("%02x:%02x:%02x:%02x:%02x:%02x", 
            info.address.lap[0], info.address.lap[1], info.address.lap[2], 
            info.address.uap, info.address.nap[0], info.address.nap[1]);
          pf("addr is %s\n", addr.c_str());
          if(LBTClient.connect(addr.c_str(), "1234")){
            pf("LBTClient.connect succeed.\n");
          }
          else{
            pf("LBTClient.connect failed.\n");
          }
        }
        else{
          pf("LBTClient.getDeviceInfo %d failed\n", idx);
        }
      }
      break;
    }
  }  
}

