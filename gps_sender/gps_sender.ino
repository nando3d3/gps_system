//
// Heltec 32 LoRa OLED (with SD Card)
//               ______________
//          Gnd |    |  USB|   | Gnd
//           5v |    | port|   |  5v
//          3v3 |    |_____|   | 3v3
//          Gnd |      ALL     |  36<-     <- : Input Only!
//           Rx |     GPIO     |  37<-
//           Tx |     3.3v     |  38<-
//          RST |   ________   |  39<-
//            0 |  |        |  |  34<-
//    (SCL)  22 |  |        |  |  35<-
// SPI MISO  19 |  |        |  |  32<-
//           23 |  |        |  |  33<-
//LoRa CS(HI)18 |  |        |  |  25  (LED)
// SPI SCK    5 |  |  OLED  |  |  26  LoRa IRQ
//OLED SCL   15 |  |        |  |  27  SPI MOSI
//            2 |  |        |  |  14  LoRa Rst
//OLED SCA    4 |  |        |  |  12
//           17 |  |________|  |  13
//OLED RST   16 |______________|  21  (SD_CS)


#include <SPI.h>
#include <mySD.h>
#include <TinyGPS++.h>
#include "heltec.h"

#define MICROSD_PIN_CHIP_SELECT   4
#define MICROSD_PIN_MOSI          13
#define MICROSD_PIN_MISO          21
#define MICROSD_PIN_SCK           22
 
#define LOG_FILE_PREFIX "gpslog" // Nome do arquivo.
#define MAX_LOG_FILES 100 // Número máximo de gpslog que pode ser feito
#define LOG_FILE_SUFFIX "csv" // Sufixo do arquivo
char logFileName[13]; // Char string para o nome do arquivo
// Data to be logged:
#define LOG_COLUMN_COUNT 4
char * log_col_names[LOG_COLUMN_COUNT] = {
 "hora", "longitude", "latitude", "altitude"
};


TinyGPSPlus tinyGPS; // tinyGPSPlus object to be used throughout
#define GPS_BAUD 9600
#define BAND    915E6 //433E6  //you can set band here directly,e.g. 868E6,915E6
String outh; //String para armazenar a hora
String outlg; //String para armazenar a longitude
String outlt; //String para armazenar a latitude
String outalt; //String para armazenar a altitude


void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1,2,17); //RX (GPS) -> 17 (ESP); TX (GPS) -> 2;
  //Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, false /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Heltec.begin(false, true, false, true, BAND);
  /*Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();
  Heltec.display->drawString(0, 10, "Iniciando...");
  Heltec.display->display();
  delay(1000);*/

  //SD
  Serial.println("Configurando SD Card");
  if (!SD.begin(MICROSD_PIN_CHIP_SELECT, MICROSD_PIN_MOSI, MICROSD_PIN_MISO, MICROSD_PIN_SCK)) {
    Serial.println("Falha na inicialização");
  }

  updateFileName(); // Toda vez que inicia, cria um novo arquivo gpslog(x+1).csv
  printHeader(); // Coloca o cabeçalho no arquivo novo
}

void loop()
{
  while (Serial2.available() > 0)
    if (tinyGPS.encode(Serial2.read()))
      {
      logGPSData();

      //Transmite os dados
      LoRa.beginPacket();
      LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
      LoRa.print(outh + "\n");
      LoRa.print(outlt + "\n");
      LoRa.print(outlg + "\n");
      LoRa.print(outalt);
      LoRa.endPacket();
      /*Heltec.display->clear();
      Heltec.display->drawString(5, 5, outh);
      Heltec.display->drawString(5, 20, outlt);
      Heltec.display->drawString(5, 35, outlg);
      Heltec.display->drawString(5, 50, outalt);
      Heltec.display->display();*/
      
      }
  if (millis() > 5000 && tinyGPS.charsProcessed() < 10)
  {
    Serial.println(F("GPS não detectado."));
    while(true);
  }
}

byte logGPSData()
{
  File logFile = SD.open(logFileName, FILE_WRITE); // Abre o arquivo

  //Configuração da hora
  String hrs = "";
  if (tinyGPS.time.hour()  <= 2) {
    hrs = hrs + (tinyGPS.time.hour()+21) + ":";
  } 
  else {
    if (tinyGPS.time.hour() < 10) hrs += "0";
    hrs = hrs + (tinyGPS.time.hour()-3) + ":";
  };
  if (tinyGPS.time.minute() < 10) hrs +=0;
  hrs = hrs + tinyGPS.time.minute() + ":";
  if (tinyGPS.time.second() < 10) hrs += 0;
  hrs = hrs + tinyGPS.time.second() + "";
  outh ="Horas: " + hrs;
    
  //Configuração da latitude, longitude e altitude
  String strLat = "";
  char buff1[12];
  float lat = tinyGPS.location.lat();
  dtostrf(lat, 5, 6, buff1);
  strLat = strLat + "LAT: " + buff1;
  outlt = strLat;

  String strLng = "";
  char buff2[12];
  float lng = tinyGPS.location.lng();
  dtostrf(lng, 5, 6, buff2);
  strLng = strLng + "LNG: " + buff2; 
  outlg = strLng;

  String strAlt = "";
  char buff3[12];
  float alt = tinyGPS.altitude.meters();
  dtostrf(alt, 5, 6, buff3);
  strAlt = strAlt + "ALT: " + buff3; 
  outalt = strAlt;
  
  if (logFile)
  { 
    logFile.print(hrs);
    logFile.print('\t');
    logFile.print(tinyGPS.location.lng(), 6);
    logFile.print('\t');
    Serial.println(outh);
    logFile.print(tinyGPS.location.lat(), 6);
    logFile.print('\t');
    Serial.println(outlt);
    logFile.print(tinyGPS.altitude.meters(), 1);
    logFile.print('\t');
    Serial.println(outlg);
    Serial.println(outalt);
    Serial.print('\n');
    logFile.println();
    logFile.close();
 
    return 1; // Retorna em caso de true
  }
 
  return 0; // Se falhar em abrir o arquivo, retorna false
}

// printHeader() - coloca o cabeçalho no arquivo
void printHeader()
{
  File logFile = SD.open(logFileName, FILE_WRITE); // Open the log file
 
  if (logFile) // If the log file opened, print our column names to the file
  {
    int i = 0;
    for (; i < LOG_COLUMN_COUNT; i++)
    {
      logFile.print(log_col_names[i]);
      if (i < LOG_COLUMN_COUNT - 1) // If it's anything but the last column
        logFile.print('\t'); // print a comma
      else // If it's the last column
        logFile.println(); // print a new line
    }
    logFile.close(); // close the file
  }
}
 
// updateFileName() - Procura os nomes dos arquivos no SD Card e cria um novo incrementando +1 no nome;
void updateFileName()
{
  int i = 0;
  for (; i < MAX_LOG_FILES; i++)
  {
    memset(logFileName, 0, strlen(logFileName)); // Limpa logFileName string
    // Set logFileName to "gpslogXX.csv":
    sprintf(logFileName, "%s%d.%s", LOG_FILE_PREFIX, i, LOG_FILE_SUFFIX);
    if (!SD.exists(logFileName)) // Se o arquivo não existe
    {
      break; // Quebra o loop ao achar o index
    }
    else // Caso contrário...
    {
      Serial.print(logFileName);
      Serial.println(" existe"); // Print a debug statement
    }
  }
  Serial.print("Nome do arquivo: ");
  Serial.println(logFileName); // Debug print the file name
}
