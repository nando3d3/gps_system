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
//MPU
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// definir um nome para manipilar melhor 
Adafruit_MPU6050 mpu;

#define MICROSD_PIN_CHIP_SELECT   4
#define MICROSD_PIN_MOSI          13
#define MICROSD_PIN_MISO          21
#define MICROSD_PIN_SCK           22
 
#define LOG_FILE_PREFIX "gpslog" // Nome do arquivo.
#define MAX_LOG_FILES 100 // Número máximo de gpslog que pode ser feito
#define LOG_FILE_SUFFIX "csv" // Sufixo do arquivo
char logFileName[13]; // Char string para o nome do arquivo
// Data to be logged:
#define LOG_COLUMN_COUNT 11
char * log_col_names[LOG_COLUMN_COUNT] = {
 "hora", "longitude", "latitude", "altitude", "acelX", "acelY", "acelZ", "gyroX", "gyroY", "gyroZ", "temp"
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
  // --- Fim da inicialização do SD ---

  //MPU
  Serial.println("Adafruit MPU6050 test!");

  // inicializar MPU 
  if (!mpu.begin(//Testar outras portas além da 21 e 22)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  
  // faixa de medição do acelerômetro
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }

  // faixa de medição do giroscópio
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  // faixa de medição do giroscópio 
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  //Fim da configuração do MPU
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
      LoRa.print(a.acceleration.x);
      LoRa.print(a.acceleration.y);
      LoRa.print(a.acceleration.z);
      LoRa.print(g.gyro.x);
      LoRa.print(g.gyro.y);
      LoRa.print(g.gyro.z);
      LoRa.print(temp.temperature);
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

  //MPU
  /* Modo que vai ser visualizado o dados */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  //Configuração do acelerômetro
  
  
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
    logFile.print(a.acceleration.x);
    logFile.print('\t');
    logFile.print(a.acceleration.y);
    logFile.print('\t');
    logFile.print(a.acceleration.z);
    logFile.print('\t');
    logFile.print(g.gyro.x);
    logFile.print('\t');
    logFile.print(g.gyro.y);
    logFile.print('\t');
    logFile.print(g.gyro.z);
    logFile.print('\t');
    logFile.print(temp.temperature);
    Serial.println(outlg);
    Serial.println(outalt);
    Serial.println(a.acceleration.x);
    Serial.println(a.acceleration.y);
    Serial.println(a.acceleration.z);
    Serial.println(g.gyro.x);
    Serial.println(g.gyro.y);
    Serial.println(g.gyro.z);
    Serial.println(temp.temperature);
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
