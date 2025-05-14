// ###################################################################
// v2 - 30/04/2021 15:52
// ###################################################################

#include <Arduino.h>
#include <Wire.h>
#include <Tone32.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <WiFi.h>

// ###################################################################
// 5 lines to be completed :
// ###################################################################

#define AIO_USERNAME  "" // related to your ADAFRUIT account
#define AIO_KEY  ""           // related to your ADAFRUIT account
#define AIO_FEED "CO2"   // for example : "/feeds/co2"

#define wifi_ssid "your's"      // your domestic WIFI id
#define wifi_pwd "your's"      // your domestic WIFI password

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883

// ###################################################################
// Depends of your pinout
// ###################################################################
int ledR =5;
int ledO =18;
int ledV =19;
int ledB =23;

// ###################################################################
// A buzzer
// ###################################################################
int PIN_BUZZER = 16;
#define BUZZER_PIN 32
#define BUZZER_CHANNEL 0

// ###################################################################
// Connection ESP32 / S8 sensor
// ###################################################################
#define TXD2 21
#define RXD2 22

byte CO2req[] = {0xFE, 0X04, 0X00, 0X03, 0X00, 0X01, 0XD5, 0XC5};
byte Response[20];
uint16_t crc_02;
int ASCII_WERT;
int int01, int02, int03;
unsigned long ReadCRC;      // CRC Control Return Code 

// Ausselesen ABC-Periode / FirmWare / SensorID 
byte ABCreq[] = {0xFE, 0X03, 0X00, 0X1F, 0X00, 0X01, 0XA1, 0XC3};   // 1f in Hex -> 31 dezimal
byte FWreq[] = {0xFE, 0x04, 0x00, 0x1C, 0x00, 0x01, 0xE4, 0x03};    // FW      ->       334       1 / 78
byte ID_Hi[] = {0xFE, 0x04, 0x00, 0x1D, 0x00, 0x01, 0xB5, 0xC3};    // Sensor ID hi    1821       7 / 29
byte ID_Lo[] = {0xFE, 0x04, 0x00, 0x1E, 0x00, 0x01, 0x45, 0xC3};    // Sensor ID lo   49124     191 / 228 e.g. in Hex 071dbfe4
//byte Tstreq[] = {0xFE, 0x03, 0x00, 0x01, 0x00, 0x01, 0xE4, 0x03};   // Read the holding register
//byte Tstreq[] = {0xFE, 0x04, 0x00, 0x01, 0x00, 0x01, 0xE4, 0x03}; // Read the input Register
byte Tstreq[] = {0xFE, 0x44, 0X00, 0X01, 0X02, 0X9F, 0X25};       // undocumented function 44
int Test_len = 7;               // length of Testrequest in case of function 44 only 7 otherwise 8


WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
boolean MQTT_connect();
boolean MQTT_connect() {  int8_t ret; if (mqtt.connected()) {    return true; }  uint8_t retries = 3;  while ((ret = mqtt.connect()) != 0) { mqtt.disconnect(); delay(2000);  retries--;if (retries == 0) { return false; }} return true;}
Adafruit_MQTT_Publish Co2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME AIO_FEED);

void send_Request (byte * Request, int Re_len)
{
  while (!Serial1.available())
  {
    Serial1.write(Request, Re_len);   // Send request to S8-Sensor
    delay(50);
  }
}

void read_Response (int RS_len)
{
  int01 = 0;
  while (Serial1.available() < 7 ) 
  {
    int01++;
    if (int01 > 10)
    {
      while (Serial1.available())
        Serial1.read();
      break;
    }
    delay(50);
  }
  for (int02 = 0; int02 < RS_len; int02++)    // Empfangsbytes
  {
    Response[int02] = Serial1.read();
  }
}

unsigned short int ModBus_CRC(unsigned char * buf, int len)
{
  unsigned short int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned short int)buf[pos];   // XOR byte into least sig. byte of crc
    for (int i = 8; i != 0; i--) {         // Loop over each bit
      if ((crc & 0x0001) != 0) {           // If the LSB is set
        crc >>= 1;                         // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;  
}

unsigned long get_Value(int RS_len)
/* Extract the Value from the response (byte 3 and 4)  
    and check the CRC if the response is compleat and correct */
{
  /* Loop only for test and developtment purpose */
//  for (int i=0; i<RS_len-2; i++)
//  {
//    int03 = Response[i];
//    Serial.printf("Wert[%i] : %i / ", i, int03);
//  }

// Check the CRC //
  ReadCRC = (uint16_t)Response[RS_len-1] * 256 + (uint16_t)Response[RS_len-2];
  if (ModBus_CRC(Response, RS_len-2) == ReadCRC) {
    // Read the Value //
    unsigned long val = (uint16_t)Response[3] * 256 + (uint16_t)Response[4];
    return val * 1;       // S8 = 1. K-30 3% = 3, K-33 ICB = 10
  }
  else {
    Serial.print("Error");
    return 99;
  }
}

void setup() {
  pinMode(ledR, OUTPUT);
  pinMode(ledO, OUTPUT);
  pinMode(ledV, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode (PIN_BUZZER, OUTPUT);
  int i;
  for(i=0;i<3;i++)
  {
  digitalWrite(ledO,HIGH);
  digitalWrite(ledV,HIGH);
  digitalWrite(ledR,HIGH); 
  digitalWrite(ledB,HIGH); 
  delay(500);
  digitalWrite(ledO,LOW);
  digitalWrite(ledV,LOW);
  digitalWrite(ledR,LOW);
  digitalWrite(ledB,LOW);  
  delay(500);
  }
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);      // UART to Sensair CO2 Sensor
  
  Serial.println();
  Serial.print("ABC-Tage: ");  
  send_Request(ABCreq, 8);                     // Request ABC-Information from the Sensor
  read_Response(7);
  Serial.printf("%02ld", get_Value(7));
  Serial.println("");

  Serial.print("Sensor ID : ");                // 071dbfe4
  send_Request(ID_Hi, 8);
  read_Response(7);
  Serial.printf("%02x%02x", Response[3], Response[4]);
  send_Request(ID_Lo, 8);
  read_Response(7);
  Serial.printf("%02x%02x", Response[3], Response[4]);
  Serial.println("");

  Serial.print("FirmWare  : ");
  send_Request(FWreq, 8);                // Send Request for Firmwar to the Sensor
  read_Response(7);                   // get Response (Firmware 334) from the Sensor
  Serial.printf("%02d%02d", Response[3], Response[4]);
  Serial.println("");

 // Read all Register 
  for (int i=1; i<=31; i++)
  {
    Tstreq[3] = i;
    crc_02 = ModBus_CRC(Tstreq,Test_len-2);     // calculate CRC for request
    Tstreq[Test_len-1] = (uint8_t)((crc_02 >> 8) & 0xFF);
    Tstreq[Test_len-2] = (uint8_t)(crc_02 & 0xFF);
    delay(100);
    Serial.print("Registerwert : ");
    Serial.printf("%02x", Tstreq[0]);
    Serial.printf("%02x", Tstreq[1]);
    Serial.printf("%02x", Tstreq[2]);
    Serial.print(" ");
    Serial.printf("%02x", Tstreq[3]);
    Serial.print(" ");
    Serial.printf("%02x", Tstreq[Test_len-3]);
    Serial.printf("%02x", Tstreq[Test_len-2]);
    Serial.printf("%02x", Tstreq[Test_len-1]);

    send_Request(Tstreq, Test_len);
    read_Response(7);
    // Check if CRC is OK 
    ReadCRC = (uint16_t)Response[6] * 256 + (uint16_t)Response[5];
    if (ModBus_CRC(Response, Test_len-2) == ReadCRC) {
      Serial.printf(" -> (d) %03d %03d", Response[3], Response[4]);
      Serial.printf(" -> (x) %02x %02x", Response[3], Response[4]);
      Serial.printf(" ->(Wert) %05d", (uint16_t)Response[3] * 256 + (uint16_t)Response[4]);
      ASCII_WERT = (int)Response[3];
      Serial.print(" -> (t) ");
      if ((ASCII_WERT <= 128) and (ASCII_WERT >= 47)) 
      {
        Serial.print((char)Response[3]);
        Serial.print(" ");
        ASCII_WERT = (int)Response[4];
        if ((ASCII_WERT <= 128) and (ASCII_WERT >= 47)) 
        {
          Serial.print((char)Response[4]);
        }
        else
        {
          Serial.print(" ");
        }
      }
      else 
      {
        Serial.print("   ");
      }
      if ((Response[1] == 68) and (Tstreq[3] == 8))
         Serial.print("   CO2");      
      Serial.println("  Ende ");
    }
    else 
      Serial.print("CRC-Error");
  }
  WiFi.disconnect();
  delay(3000);
  Serial.println("Wifi : started");
  WiFi.begin(wifi_ssid,wifi_pwd);
  while ((!(WiFi.status() == WL_CONNECTED))){
    delay(300);
    Serial.println("Wifi connection in progress");
  }
  Serial.println("Wifi : connected");
}  

void loop() {

  // Check if the wifi connection is still available
  if ( !(WiFi.status() == WL_CONNECTED) )
  {
      Serial.println("Wifi : a problem occurs");
      WiFi.disconnect();
      delay(3000);
      Serial.println("Wifi : re-started");
      WiFi.begin(wifi_ssid,wifi_pwd);
      while ((!(WiFi.status() == WL_CONNECTED))){
        delay(300);
        Serial.print("Wifi connection in progress");
      }
      Serial.println("Wifi re-connected");
  }
  send_Request(CO2req, 8);               // send request for CO2-Data to the Sensor
  read_Response(7);                      // receive the response from the Sensor
  unsigned long CO2 = get_Value(7);
  int valeur = (int)CO2;
  delay(6000);
  String CO2s = "CO2: " + String(CO2);
  Serial.println(CO2s);
  if (CO2 < 450){
    digitalWrite(ledB,HIGH);
    delay(1000);
    digitalWrite(ledB,LOW);
    delay(200);
    digitalWrite(ledB,HIGH);
    digitalWrite(ledV,LOW);
    digitalWrite(ledO,LOW);
    digitalWrite(ledR,LOW);
    digitalWrite(PIN_BUZZER,LOW);
  }
  else if (CO2 < 600){
    digitalWrite(ledB,HIGH);
    digitalWrite(ledV,LOW);
    digitalWrite(ledO,LOW);
    digitalWrite(ledR,LOW);
    digitalWrite(PIN_BUZZER,LOW);
  }
  else if (CO2 < 800){
    digitalWrite(ledB,LOW);
    digitalWrite(ledV,HIGH);    
    digitalWrite(ledO,LOW);
    digitalWrite(ledR,LOW);
    digitalWrite(PIN_BUZZER,LOW);
  }
  else if (CO2 < 1000){
    digitalWrite(ledB,LOW);
    digitalWrite(ledV,LOW);
    digitalWrite(ledO,HIGH);
    digitalWrite(ledR,LOW);
    digitalWrite(PIN_BUZZER,LOW);
  }
  else if  (CO2 >= 1000){
    tone(BUZZER_PIN, NOTE_C4, 500, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_B4, 500, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    digitalWrite(ledR,HIGH);
    digitalWrite(ledO,LOW);
    digitalWrite(ledV,LOW);
    digitalWrite(ledB,LOW);
    delay(1000);
    digitalWrite(ledR,LOW);
    delay(200);
    digitalWrite(ledR,HIGH);    
  }
      
  if (MQTT_connect()) {
      if (Co2.publish(valeur)) {
        Serial.println("Web publication : ok");
      }
      else { 
        Serial.println("Web publication : error");
      }
    }
  else {
    Serial.println("An error occurs with the ADAFRUIT publication");
  }
}
