#define API_SITE_URL "graph.facebook.com"
#define API_SITE_PORT 80

#include <stdarg.h>
#include <LGPRS.h>
#include <LGPRSClient.h>
#include <HttpClient.h>

#define BAUDRATE 115200

#define PF_BUF_SIZE 256
void pf(const char *fmt, ...){
  char tmp[PF_BUF_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tmp, PF_BUF_SIZE, fmt, args);
  va_end(args);
  Serial.print(tmp);
}

bool attachGprs(int tries){
  bool flag = false;
  
  Serial.print("Attaching GPRS...");
  for(int i = 0; i < tries; i++){
    if(LGPRS.attachGPRS()){
      Serial.print("ok");
      flag = true;
      break;
    }
    delay(1000);
    Serial.print("...");
  }
  Serial.println("");
  
  return flag;
}

void setup(){
  Serial.begin(BAUDRATE);
}

int queryFacebook(){
  pf("queryFacebook\n");
  int likesCount = -1;
  
  // use HttpClient
  LGPRSClient cli;  
  HttpClient hcli(cli);
  
  // send HTTP request
  String url = String("/caveeducation");
  int err = hcli.startRequest(API_SITE_URL, API_SITE_PORT, url.c_str(), HTTP_METHOD_GET, NULL);
  hcli.finishRequest();
  
  int rscode = hcli.responseStatusCode();
  if(err == HTTP_SUCCESS && rscode > 0){ // successful
    pf("http client request succeed, rscode: %d\n", rscode);
    
    /*
    while(!hcli.endOfHeadersReached()){ // print headers
      char c = hcli.readHeader();
      Serial.print(c);
    }
    Serial.println("");
    */
    hcli.skipResponseHeaders(); // ignore headers

    Serial.println("getting the body of the response");
    char *buf = (char *) malloc(1024+1 * sizeof(char));
    int n;
    String s;
    while( (n = hcli.read((uint8_t *)buf, 1024)) > 0){
      buf[n] = '\0';
      s += buf;
    }
    free(buf);
    
    Serial.println(s);
    
    const char kwLeft[] = "\"likes\":";
    const char kwRight[] = ",";
    int iLeft = s.indexOf(kwLeft) + strlen(kwLeft);
    int iRight = s.indexOf(kwRight, iLeft);
    pf("iLeft %d, iRight %d\n", iLeft, iRight);
    likesCount = s.substring(iLeft, iRight).toInt();
  }
  else{
    pf("http client request failed, rscode: %d, err: %d\n", rscode, err);
  }
  
  return likesCount;
}

int queryFacebook_no_httpclient(){
  pf("queryFacebook_no_httpclient\n");
  int likesCount = -1;
  
  LGPRSClient cli;  
  
  while(cli.connect(API_SITE_URL, API_SITE_PORT) == 0){
    Serial.println("Reconnecting...");
    delay(1000);
  }

  Serial.println("send HTTP GET request");
  cli.println("GET /caveeducation HTTP/1.1");
  cli.println("Host: " API_SITE_URL);
  cli.println("Connection: close");
  cli.println();

  Serial.println("waiting for HTTP response");
  while(!cli.available()){
    delay(100);
  }
  
  Serial.println("getting the body of the response");
  char *buf = (char *) malloc(1024+1 * sizeof(char));
  int n;
  String s;
  while( (n = cli.read((uint8_t *)buf, 1024)) > 0){
    buf[n] = '\0';
    s += buf;
  }
  free(buf);
  
  const char kwLeft[] = "\"likes\":";
  const char kwRight[] = ",";
  int iLeft = s.indexOf(kwLeft) + strlen(kwLeft);
  int iRight = s.indexOf(kwRight, iLeft);
  if(iLeft != -1 && iRight != -1){
    likesCount = s.substring(iLeft, iRight).toInt();
  }
  
  return likesCount;
}

void loop(){
  if(Serial.available()){
    char d = Serial.read();
    switch(d){
      case 'i':
        if(attachGprs(5)){
          Serial.println("Attaching GPRS succeed.");
        }
        else{
          Serial.println("Attaching GPRS failed. Timeout.");
        }
      break;
      case 'q':{
        int likesCount = queryFacebook();
        Serial.print("Likes:");
        if(likesCount != -1){
          Serial.println(likesCount);
        }
        else{
          Serial.println(" can't get.");
        }
      }
      break;
      case 'w':{
        int likesCount = queryFacebook_no_httpclient();
        Serial.print("Likes:");
        if(likesCount != -1){
          Serial.println(likesCount);
        }
        else{
          Serial.println(" can't get.");
        }
      }
      break;
    }
  }
}
