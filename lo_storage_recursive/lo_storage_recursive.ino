#include <LSD.h>
#define BAUDRATE 19200

void setup(){
  Serial.begin(BAUDRATE);
  LSD.begin();
}
void printSpace(int n){
  int i;
  for(i = 0; i < n; i++){
    Serial.print(' ');
  }
}
void printD(const char *path, int n_space){
  LFile f = LSD.open(path, FILE_READ);
  printSpace(n_space);
  Serial.print(path);
  Serial.print(" - ");
  Serial.println(f.name());
  
  if(f.isDirectory()){
    LFile subf;

    while(subf = f.openNextFile(FILE_READ)){
      String p = String(path) + "\\" + subf.name();
      printD(p.c_str(), n_space + 4);
      //Serial.println(subf.name());
      subf.close();
    }
  }

  f.close();
}
void loop(){
  Serial.println("---------------------");
  printD("", 0);
  
  delay(5000);
}


