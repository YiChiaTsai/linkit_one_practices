#include <LSD.h>
#include <LAudio.h>
#define BAUDRATE 19200

LFile dir;
unsigned char vol = 6;
String get_next(){
  LFile f = dir.openNextFile(FILE_READ);
  String song = String("\\mp3\\") + f.name();
  while(!song.endsWith(String(".mp3"))){
    f.close();
    f = dir.openNextFile(FILE_READ);
    song = String("\\mp3\\") + f.name();
  }
  f.close();
  return song;
}
void setup(){
  Serial.begin(BAUDRATE);
  
  LAudio.begin();
  LAudio.setVolume(vol);
  
  LSD.begin();
  dir = LSD.open("mp3", FILE_READ);
  String song = get_next();
  LAudio.playFile(storageSD, (char *)song.c_str());
  Serial.println(song.c_str());
}
void vol_inc(unsigned char n){
  vol += n;
  if(vol > 6){
    vol = 6;
  }
  if(vol < 0){
    vol = 0;
  }
  LAudio.setVolume(vol);
}
void loop(){
  String song;
  while(Serial.available()){
    char c = Serial.read();
    switch(c){
      case 'p':
        song = get_next();
        LAudio.playFile(storageSD, (char *)song.c_str());
        Serial.println(song.c_str());
      break;
      case 's':
        LAudio.stop();
      break;
      case 'r':
        LAudio.stop();
        dir.rewindDirectory();
      break;
      case '+':
        vol_inc(1);
      break;
      case '-':
        vol_inc(-1);
      break;
    }
  }
}


