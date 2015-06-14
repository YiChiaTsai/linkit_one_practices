#include <stdarg.h>
#include <LWiFi.h> 
#include <LWiFiClient.h> 

#define BAUDRATE 115200

#define WIFI_AP "N10U"
#define WIFI_PASSWD "yn0933!@"
#define URL "www.mediatek.com" 

LWiFiClient cli;

#define PF_BUF_SIZE 128 // limited to 128 chars
void pf(const char *fmt, ...){
    char tmp[PF_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tmp, PF_BUF_SIZE, fmt, args);
    va_end(args);
    Serial.print(tmp);
}

const char * const wifi_status_str(LWifiStatus ws){
    switch(ws){
      case LWIFI_STATUS_DISABLED:
        return "WiFi module status: disabled";
      break;
      case LWIFI_STATUS_DISCONNECTED:
        return "WiFi module status: disconnected";
      break;
      case LWIFI_STATUS_CONNECTED:
        return "WiFi module status: connected";
      break;
    }
    return "WiFi status: error, no such status";
}

void setup(){
  Serial.begin(BAUDRATE);
}

void loop(){
  if(Serial.available()){
    char d = Serial.read();
    switch(d){
      case 'l':{
        int i;
        int n_ap = LWiFi.scanNetworks();
        pf("Number of WiFi networks found: %d\n", n_ap);
        for(i = 0; i < n_ap; i++){
            pf("%d, %s, RSSI: %d\n", i, LWiFi.SSID(i), LWiFi.RSSI(i));
        }
      }
      break;
      case 's':{
        LWifiStatus ws = LWiFi.status();
        Serial.println(wifi_status_str(ws));
        
        uint8_t ma[VM_WLAN_WNDRV_MAC_ADDRESS_LEN] = {0};
        LWiFi.macAddress(ma);
        Serial.print("MAC address: ");
        int i;
        for(i = 0; i < VM_WLAN_WNDRV_MAC_ADDRESS_LEN-1; i++){
            pf("%02X:", ma[i]);
        }
        pf("%02X\n", ma[i]);
        
        if(ws == LWIFI_STATUS_CONNECTED){
          IPAddress ipa = LWiFi.localIP();
          Serial.print("localIP: ");
          ipa.printTo(Serial);
          Serial.println("");
          
          Serial.print("subnetMask: ");
          ipa = LWiFi.subnetMask();
          ipa.printTo(Serial);
          Serial.println("");
          
          Serial.print("gatewayIP: ");
          ipa = LWiFi.gatewayIP();
          ipa.printTo(Serial);
          Serial.println("");
        }
      }
      break;
      case 'b':{
        Serial.println("LWiFi.begin() turn on WiFi module");
        LWiFi.begin();
      }
      break;
      case 'e':{
        Serial.println("LWiFi.end() turn off WiFi module");
        LWiFi.end();
      }
      break;
      case 'c':{
        Serial.print("Connecting...");
        if(LWiFi.connect(WIFI_AP, LWiFiLoginInfo(LWIFI_WPA, WIFI_PASSWD)) > 0){
          Serial.println(" succeed");
        }
        else{
          Serial.println(" failed");
        }
      }
      break;
      case 'd':{
        LWiFi.disconnect();
        Serial.println("Disconnected");
      }
      break;
      case 'g':{
        Serial.println("send request...");
        cli.connect(URL, 80);
        cli.println("GET / HTTP/1.1"); 
        cli.println("Host: " URL); 
        cli.println("Connection: close");
        cli.println();
      }
      break;
    }
  }
  
  if(cli){
    int d_len = cli.available();
    pf("There are %d bytes data\n", d_len);
    
  }
  while(cli){
    int d;
    d = cli.read();
    if(d < 0)
      break;
    Serial.print((char) d); 
  }
}


