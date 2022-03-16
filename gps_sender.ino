#include <TinyGPS++.h>
#include "heltec.h"
#define BAND    915E6 //433E6  //you can set band here directly,e.g. 868E6,915E6
TinyGPSPlus gps;
String outh;
String outlg;
String outlt;
String outalt;
int x;

void setup()
{
  Serial2.begin(9600, SERIAL_8N1,2,17);
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();
  Heltec.display->drawString(0, 10, "Iniciando...");
  Heltec.display->display();
  delay(1000);
}

//Enviar pacote
void sendPacket(){
  LoRa.beginPacket();
  //LoRa.print("Hora: ");
  //LoRa.print(outh);
  //LoRa.print("LAT: ");
  //LoRa.print(outlt);
  LoRa.endPacket();
}

void loop()
{
  while (Serial2.available() > 0)
    if (gps.encode(Serial2.read()))
      {
      displayInfo();
      LoRa.beginPacket();
      LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
      LoRa.print(outh + "\n");
      LoRa.print(outlt + "\n");
      LoRa.print(outlg + "\n");
      LoRa.print(outalt);
      LoRa.endPacket();
      Heltec.display->clear();
      //Heltec.display->drawStringMaxWidth(0, 10, 128, out);
      Heltec.display->drawString(5, 5, outh);
      Heltec.display->drawString(5, 20, outlt);
      Heltec.display->drawString(5, 35, outlg);
      Heltec.display->drawString(5, 50, outalt);
      //displayInfo();
      Heltec.display->display();

      //sendPacket(); //Enviar dados

      //Imprimir no serial
      if (gps.time.hour() < 10) Serial.print(F("0"));
      Serial.print(gps.time.hour());
      Serial.print(F(":"));
      if (gps.time.minute() < 10) Serial.print(F("0"));
      Serial.print(gps.time.minute());
      Serial.print(F(":"));
      if (gps.time.second() < 10) Serial.print(F("0"));
      Serial.print(gps.time.second());
      }
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("GPS não detectado."));
    while(true);
  }
}



void displayInfo()
{
    /*String str = "";
    char buff[12];
    str = str + "Hora: " + (gps.time.hour()-3) + ":";
    str = str + gps.time.minute() + ":";
    str = str + gps.time.second() + " ";        
    float lat = gps.location.lat();
    dtostrf(lat, 5, 6, buff);
    str = str + " " + "LAT: " + buff + " "; 
    float lng = gps.location.lng();
    dtostrf(lng, 5, 6, buff);              
    str = str + "LNG: " + buff + " ";
    str = str + "ALT: " + gps.altitude.meters() + " ";
    str = str + x;           
    out = str;
    x = x + 1;*/
    String hrs = "";
    hrs = hrs + "Hora: ";
    if (gps.time.hour()  <= 2) {
      hrs = hrs + (gps.time.hour()+21) + ":";
    } 
    else {
      if (gps.time.hour() < 10) hrs += "0";
      hrs = hrs + (gps.time.hour()-3) + ":";
    };
    if (gps.time.minute() < 10) hrs +=0;
    hrs = hrs + gps.time.minute() + ":";
    if (gps.time.second() < 10) hrs += 0;
    hrs = hrs + gps.time.second() + "";
    outh = hrs;
    

    String strLat = "";
    char buff1[12];
    float lat = gps.location.lat();
    dtostrf(lat, 5, 6, buff1);
    strLat = strLat + "LAT: " + buff1;
    outlt = strLat;

    String strLng = "";
    char buff2[12];
    float lng = gps.location.lng();
    dtostrf(lng, 5, 6, buff2);
    strLng = strLng + "LNG: " + buff2; 
    outlg = strLng;

    String strAlt = "";
    char buff3[12];
    float alt = gps.altitude.meters();
    dtostrf(alt, 5, 6, buff3);
    strAlt = strAlt + "ALT: " + buff3; 
    outalt = strAlt;
    //Heltec.display->drawString(30,5, "Hora: " + (String)(gps.time.hour()-3));
}
