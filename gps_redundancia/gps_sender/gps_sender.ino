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
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define MICROSD_PIN_CHIP_SELECT   4
#define MICROSD_PIN_MOSI          13
#define MICROSD_PIN_MISO          21
#define MICROSD_PIN_SCK           22
#define I2C_SDA 33
#define I2C_SCL 32

#define LOG_FILE_PREFIX "gpslog" // Nome do arquivo.
#define MAX_LOG_FILES 100 // Número máximo de gpslog que pode ser feito
#define LOG_FILE_SUFFIX "csv" // Sufixo do arquivo
char logFileName[13]; // Char string para o nome do arquivo
// Data to be logged:
#define LOG_COLUMN_COUNT 15
char * log_col_names[LOG_COLUMN_COUNT] = {
 "hora", "latitude", "longitude", "altitude(m)", "velocidade(km/h)", "curso(°)", "satelites", "data", 
 "acelX", "acelY", "acelZ", "gyroX", "gyroY", "gyroZ", "temp"
};

TwoWire I2CBME = TwoWire (0);
Adafruit_MPU6050 mpu;

TinyGPSPlus tinyGPS; // objeto tinyGPSPlus para ser usado em tudo
#define GPS_BAUD 9600
#define BAND    920E6 //433E6  //Escolha a frequência

//Controle da taxa de dados
#define LOG_RATE 4000 //Log a cada 4 segundos
unsigned long lastLog = 0; //Variável global do último log

String outh; //String para armazenar a hora
String outlg; //String para armazenar a longitude
String outlt; //String para armazenar a latitude
String outalt; //String para armazenar a altitude
String velo; //String para armazenar a velocidade
String sat;
String curso, ax, ay, az, gx, gy, gz ,temperatura;
int testSD = 1; //String para testar SD

void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1,2,17); //RX (GPS) -> 17 (ESP); TX (GPS) -> 2;
  //Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, false /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Heltec.begin(false, true, false, true, BAND);

  //SD
  Serial.println("Configurando SD Card");
  if (!SD.begin(MICROSD_PIN_CHIP_SELECT, MICROSD_PIN_MOSI, MICROSD_PIN_MISO, MICROSD_PIN_SCK)) {
    Serial.println("Falha na inicialização");
    testSD = 0;
  }

  updateFileName(); // Toda vez que inicia, cria um novo arquivo gpslog(x+1).csv
  printHeader(); // Coloca o cabeçalho no arquivo novo

  //MPU
  Serial.println("Adafruit MPU6050 test!");
  I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
  bool status;

  status = mpu.begin(0x68, &I2CBME);
  
  if(!status){
    Serial.println("Failed to find MPU6050 chip");
  }
  Serial.println("MPU6050 Found!");

  
  
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
}

void loop()
{
  if ((lastLog + LOG_RATE) <= millis())
  { // If it's been LOG_RATE milliseconds since the last log:
    if (tinyGPS.location.isUpdated()) // If the GPS data is vaild
    {
      if (logGPSData()) // Log the GPS data
      {
        Serial.println("GPS logged."); // Print a debug message
        lastLog = millis(); // Update the lastLog variable
        if (testSD == 0) {
        Serial.println("Falha em salvar no SD!");
        }
        //Transmite os dados
        LoRa.beginPacket();
        LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
        LoRa.print(outh + "\n"
        + outlt + "\n"
        + outalt + "\n");
        LoRa.print(velo + "\n");
        LoRa.print(curso + "\n");
        LoRa.print(sat + "\n");
        LoRa.print("Data: " + String(tinyGPS.date.value()) + "\n");
        LoRa.print(ax + "\n");
        LoRa.print(ay + "\n");
        LoRa.print(az + "\n");
        LoRa.print(gx + "\n");
        LoRa.print(gy + "\n");
        LoRa.print(gz + "\n");
        LoRa.print(temperatura + "\n");
        LoRa.print("SD: " + String(testSD));
        LoRa.endPacket();
       }
      else // Se falhar em obter dados
      {
        Serial.println("Falha em obter novos dados.");
      }
    }
    else // Se os dados do GPS não são válidos
    {
      // Imprime uma mensagem de bug, indicando que talvez não haja satélites suficientes
      Serial.print("Sem dados GPS. Sats: ");
      Serial.println(tinyGPS.satellites.value());
    }
  }
 
  // Se não está logando, continua a alimentar o GPS
  while (Serial2.available())
    tinyGPS.encode(Serial2.read());

}

byte logGPSData()
{

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
  outh ="Hrs:" + hrs;
    
  //Configuração da latitude, longitude e altitude
  String strLat = "";
  char buff1[12];
  float lat = tinyGPS.location.lat();
  dtostrf(lat, 5, 6, buff1);
  strLat = strLat + "LAT:" + buff1;
  outlt = strLat;

  String strLng = "";
  char buff2[12];
  float lng = tinyGPS.location.lng();
  dtostrf(lng, 5, 6, buff2);
  strLng = strLng + "LNG:" + buff2; 
  outlg = strLng;

  String strAlt = "";
  char buff3[12];
  float alt = tinyGPS.altitude.meters();
  dtostrf(alt, 5, 6, buff3);
  strAlt = strAlt + "ALT: " + buff3; 
  outalt = strAlt;

  velo = "VEL:" + String(tinyGPS.speed.mph()*1.609, 2);
  sat = "SAT:" + String(tinyGPS.satellites.value());
  curso = "Curso:" + String(tinyGPS.course.deg(), 1) + "°";
  

  Serial.println(outh);
  Serial.println(outlt);
  Serial.println(outlg);
  Serial.println(outalt);
  //Serial.println("VEL: " + String(tinyGPS.speed.mph()*1.60934, 1) + " km/h");
  Serial.println("Curso:" + String(tinyGPS.course.deg(), 1) + "°");
  Serial.println("SAT:" + String(tinyGPS.satellites.value()));
  Serial.println("Data:" + String(tinyGPS.date.value()) + "\n");

  //Configuração dos dados do MPU
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  ax = "Ax:" + String(a.acceleration.x);
  ay = "Ay:" + String(a.acceleration.y);
  az = "Az:" + String(a.acceleration.z);
  gx = "Gx:" + String(g.gyro.x);
  gy = "Gy:" + String(g.gyro.y);
  gz = "Gz:" + String(g.gyro.z);
  temperatura = "T:" + String(temp.temperature);
  Serial.println("Dados do MPU:");
  Serial.println("Acel (m/s2):");
  Serial.println(ax);
  Serial.println(ay);
  Serial.println(az);
  Serial.println(" m/s^2");

  Serial.print("Rotação X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

  Serial.print(temperatura);
  Serial.println(" degC\n");

  File logFile = SD.open(logFileName, FILE_WRITE); // Abre o arquivo
  
  if (logFile)
  { 
    logFile.print(hrs);
    logFile.print(',');
    logFile.print(tinyGPS.location.lat(), 6);
    logFile.print(',');
    logFile.print(tinyGPS.location.lng(), 6);
    logFile.print(',');
    logFile.print(tinyGPS.altitude.meters(), 1);
    logFile.print(',');
    logFile.print(tinyGPS.speed.mph()*1.609, 2);
    logFile.print(',');
    logFile.print(tinyGPS.course.deg(), 1);
    logFile.print(',');
    logFile.print(tinyGPS.satellites.value());
    logFile.print(',');
    logFile.print(tinyGPS.date.value());
    logFile.print(a.acceleration.x);
    logFile.print(',');
    logFile.print(a.acceleration.y);
    logFile.print(',');
    logFile.print(a.acceleration.z);
    logFile.print(',');
    logFile.print(g.gyro.x);
    logFile.print(',');
    logFile.print(g.gyro.y);
    logFile.print(',');
    logFile.print(g.gyro.z);
    logFile.print(',');
    logFile.print(temp.temperature);
    logFile.println();
    logFile.close();
 
    return 1; // Retorna em caso de true
  } 
 
  return 0; // Se falhar em abrir o arquivo, retorna false
}

// printHeader() - coloca o cabeçalho no arquivo
void printHeader()
{
  File logFile = SD.open(logFileName, FILE_WRITE); // Abre o arquivo log
 
  if (logFile) // Se o arquivo abrir, as colunas são colocadas
  {
    int i = 0;
    for (; i < LOG_COLUMN_COUNT; i++)
    {
      logFile.print(log_col_names[i]);
      if (i < LOG_COLUMN_COUNT - 1) // Se não há nada além da última coluna a ser colocada
        logFile.print(','); // coloca uma vírgula
      else // If it's the last column
        logFile.println(); // coloca uma nova linha
    }
    logFile.close(); // fecha o arquivo
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
