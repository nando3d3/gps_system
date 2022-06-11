 
#include <SPI.h>
#include <SD.h>
#include <TinyGPS++.h>
 
#define ARDUINO_USD_CS 8 // uSD card CS pin (pin 8 on Duinopeak GPS Logger Shield)
 
/////////////////////////
// Log File Defintions //
/////////////////////////
// Keep in mind, the SD library has max file name lengths of 8.3 - 8 char prefix,
// and a 3 char suffix.
// Our log files are called "gpslogXX.csv, so "gpslog99.csv" is our max file.
#define LOG_FILE_PREFIX "gps2log" // Name of the log file.
#define MAX_LOG_FILES 100 // Number of log files that can be made
#define LOG_FILE_SUFFIX "csv" // Suffix of the log file
char logFileName[13]; // Char string to store the log file name
// Data to be logged:
#define LOG_COLUMN_COUNT 8
char * log_col_names[LOG_COLUMN_COUNT] = {
  "hora", "latitude", "longitude", "altitude(m)", "velocidade(km/h)", "curso(°)", "date", "satelites"
}; // log_col_names is printed at the top of the file.
 
//////////////////////
// Log Rate Control //
//////////////////////
#define LOG_RATE 1000 // Log every 5 seconds
unsigned long lastLog = 0; // Global var to keep of last time we logged
 
/////////////////////////
// TinyGPS Definitions //
/////////////////////////
TinyGPSPlus tinyGPS; // tinyGPSPlus object to be used throughout
#define GPS_BAUD 9600 // GPS module's default baud rate
 
////////////////////////////////////////////////
// Arduino GPS Shield Serial Port Definitions //
////////////////////////////////////////////////
// If you're using an Arduino Uno, Mega, RedBoard, or any board that uses the
// 0/1 UART for programming/Serial monitor-ing, use SoftwareSerial:
#include <SoftwareSerial.h>
#define ARDUINO_GPS_RX 3 // GPS TX, Arduino RX pin
#define ARDUINO_GPS_TX 2 // GPS RX, Arduino TX pin
SoftwareSerial ssGPS(ARDUINO_GPS_TX, ARDUINO_GPS_RX); // Create a SoftwareSerial
 
// Set gpsPort to either ssGPS if using SoftwareSerial or Serial1 if using an
// Arduino with a dedicated hardware serial port
#define gpsPort ssGPS  // Alternatively, use Serial1 on the Leonardo
 
// Define the serial monitor port. On the Uno, Mega, and Leonardo this is 'Serial'
//  on other boards this may be 'SerialUSB'
#define SerialMonitor Serial

String outh; //String para armazenar a hora
String outlg; //String para armazenar a longitude
String outlt; //String para armazenar a latitude
String outalt; //String para armazenar a altitude
String velo; //String para armazenar a velocidade
String sat;
String curso;

void setup()
{
  SerialMonitor.begin(9600);
  gpsPort.begin(GPS_BAUD);
 
  SerialMonitor.println("Configurando SD Card");
  // see if the card is present and can be initialized:
  if (!SD.begin(ARDUINO_USD_CS))
  {
    SerialMonitor.println("Falha na inicialização");
  }
  updateFileName(); // Toda vez que inicia, cria um novo arquivo gpslog(x+1).csv
  printHeader(); // Coloca o cabeçalho no arquivo novo
}
 
void loop()
{
  if ((lastLog + LOG_RATE) <= millis())
  { // If it's been LOG_RATE milliseconds since the last log:
    if (tinyGPS.location.isUpdated()) // If the GPS data is vaild
    {
      if (logGPSData()) // Log the GPS data
      {
        SerialMonitor.println("GPS logged."); // Print a debug message
        lastLog = millis(); // Update the lastLog variable
      }
      else // If we failed to log GPS
      { // Print an error, don't update lastLog
        SerialMonitor.println("Falha em obter novos dados.");
      }
    }
    else // If GPS data isn't valid
    {
      // Print a debug message. Maybe we don't have enough satellites yet.
      SerialMonitor.print("Sem dados GPS. Sats: ");
      SerialMonitor.println(tinyGPS.satellites.value());
    }
  }
 
  // If we're not logging, continue to "feed" the tinyGPS object:
  while (gpsPort.available())
    tinyGPS.encode(gpsPort.read());
}
 
byte logGPSData()
{
  File logFile = SD.open(logFileName, FILE_WRITE); // Open the log file

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

  //Configuração de latitude, longitude e altitude
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
    //Salva longitude, latitude, altitude, velocidade (km/h), curso (em graus)
    // data, horas e números de satélites.

    logFile.print(hrs);
    logFile.print(',');
    logFile.print(tinyGPS.location.lat(), 6);
    logFile.print(',');
    logFile.print(tinyGPS.location.lng(), 6);
    logFile.print(',');
    logFile.print(tinyGPS.altitude.meters(), 1);
    logFile.print(',');
    logFile.print(tinyGPS.speed.mph()*1.60934, 1);
    logFile.print(',');
    logFile.print(tinyGPS.course.deg(), 1);
    logFile.print(',');
    logFile.print(tinyGPS.date.value());
    logFile.print(',');
    logFile.print(tinyGPS.satellites.value());
    logFile.println();
    logFile.close();

    Serial.println(outh);
    Serial.println(outlt);
    Serial.println(outlg);
    Serial.println(outalt + "\n");

    return 1; // Return success
  }
 
  return 0; // If we failed to open the file, return fail
}
 
// printHeader() - prints our eight column names to the top of our log file
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
        logFile.print(','); // print a comma
      else // If it's the last column
        logFile.println(); // print a new line
    }
    logFile.close(); // close the file
  }
}
 
// updateFileName() - Looks through the log files already present on a card,
// and creates a new file with an incremented file index.
void updateFileName()
{
  int i = 0;
  for (; i < MAX_LOG_FILES; i++)
  {
    memset(logFileName, 0, strlen(logFileName)); // Clear logFileName string
    // Set logFileName to "gpslogXX.csv":
    sprintf(logFileName, "%s%d.%s", LOG_FILE_PREFIX, i, LOG_FILE_SUFFIX);
    if (!SD.exists(logFileName)) // If a file doesn't exist
    {
      break; // Break out of this loop. We found our index
    }
    else // Otherwise:
    {
      SerialMonitor.print(logFileName);
      SerialMonitor.println(" exists"); // Print a debug statement
    }
  }
  SerialMonitor.print("File name: ");
  SerialMonitor.println(logFileName); // Debug print the file name
}
