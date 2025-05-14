#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <Wire.h>
#include <Tone32.h>

#define BUZZER_PIN 32
#define BUZZER_CHANNEL 0
#define TXD2 21
#define RXD2 22

int ledR =5;
int ledO =18;
int ledV =19;
int ledB =23;

// ###########################################################################
// SENSOR S8 
// ###########################################################################
byte CO2req[] = {0xFE, 0X04, 0X00, 0X03, 0X00, 0X01, 0XD5, 0XC5};
byte Response[20];
uint16_t crc_02;
int ASCII_WERT;
int int01, int02, int03;
unsigned long ReadCRC;        // CRC Control Return Code 

// Ausselesen ABC-Periode / FirmWare / SensorID 
byte ABCreq[] = {0xFE, 0X03, 0X00, 0X1F, 0X00, 0X01, 0XA1, 0XC3};       // 1f in Hex -> 31 dezimal
byte FWreq[] = {0xFE, 0x04, 0x00, 0x1C, 0x00, 0x01, 0xE4, 0x03};        // FW      ->       334       1 / 78
byte ID_Hi[] = {0xFE, 0x04, 0x00, 0x1D, 0x00, 0x01, 0xB5, 0xC3};        // Sensor ID hi    1821       7 / 29
byte ID_Lo[] = {0xFE, 0x04, 0x00, 0x1E, 0x00, 0x01, 0x45, 0xC3};        // Sensor ID lo   49124     191 / 228 e.g. in Hex 071dbfe4
//byte Tstreq[] = {0xFE, 0x03, 0x00, 0x01, 0x00, 0x01, 0xE4, 0x03};     // Read the holding register
//byte Tstreq[] = {0xFE, 0x04, 0x00, 0x01, 0x00, 0x01, 0xE4, 0x03};     // Read the input Register
byte Tstreq[] = {0xFE, 0x44, 0X00, 0X01, 0X02, 0X9F, 0X25};             // undocumented function 44
int Test_len = 7;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
String txValue;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);
        Serial.println();
        Serial.println("*********");
      }
    }
};
               // length of Testrequest in case of function 44 only 7 otherwise 8
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
// ###########################################################################
// SENSOR S8 
// ###########################################################################


void setup() {
  
  pinMode(ledR, OUTPUT);
  pinMode(ledO, OUTPUT);
  pinMode(ledV, OUTPUT); 
  pinMode(ledB, OUTPUT);  

  delay(1000);
  int i;
  for(i=0;i<3;i++)
  {
      digitalWrite(ledB,HIGH);
      delay(250);
      digitalWrite(ledV,HIGH);
      delay(250);
      digitalWrite(ledO,HIGH); 
      delay(250);
      digitalWrite(ledR,HIGH); 
      delay(250);
      digitalWrite(ledR,LOW);
      delay(250);
      digitalWrite(ledO,LOW);
      delay(250);
      digitalWrite(ledV,LOW);
      delay(250);
      digitalWrite(ledB,LOW);  
      delay(250);
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
  
  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  /*pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                  */
                  
   pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,BLECharacteristic::PROPERTY_NOTIFY);
                    // BLEStringCharacteristic txString(uuidOfTxString, BLERead | BLENotify | BLEBroadcast, 20); 
   pTxCharacteristic->addDescriptor(new BLE2902());
  
   BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,BLECharacteristic::PROPERTY_WRITE);
  
   pRxCharacteristic->setCallbacks(new MyCallbacks());

    // Start the service
    pService->start();
  
    // Start advertising
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");
}

void loop() {

  send_Request(CO2req, 8);               // send request for CO2-Data to the Sensor
  read_Response(7);                      // receive the response from the Sensor
  unsigned long CO2 = get_Value(7);
  int valeur = (int)CO2;
  delay(6000);
  String CO2s = "concentration:" + String(CO2); // To be compatible with FIZZIQ
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
    digitalWrite(BUZZER_PIN,LOW);
  }
  else if (CO2 < 600){
    digitalWrite(ledB,HIGH);
    digitalWrite(ledV,LOW);
    digitalWrite(ledO,LOW);
    digitalWrite(ledR,LOW);
    digitalWrite(BUZZER_PIN,LOW);
  }
  else if (CO2 < 800){
    digitalWrite(ledB,LOW);
    digitalWrite(ledV,HIGH);    
    digitalWrite(ledO,LOW);
    digitalWrite(ledR,LOW);
    digitalWrite(BUZZER_PIN,LOW);
  }
  else if (CO2 < 1000){
    digitalWrite(ledB,LOW);
    digitalWrite(ledV,LOW);
    digitalWrite(ledO,HIGH);
    digitalWrite(ledR,LOW);
    digitalWrite(BUZZER_PIN,LOW);
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
  if (deviceConnected) {
    
    String txValue=CO2s;
    
    delay(1000);
    char txString[txValue.length()+1]; // tableau de char de la taille du String param+1 (caractÃ¨re de fin de ligne)
    CO2s.toCharArray(txString,txValue.length()+1); // rÃ©cupÃ¨re le param dans le tableau de char  
    Serial.println(txString); // affiche le tableau de char
  
    pTxCharacteristic->setValue(txString);
    pTxCharacteristic->notify();
  
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
          delay(500); // give the bluetooth stack the chance to get things ready
          pServer->startAdvertising(); // restart advertising
          Serial.println("start advertising");
          oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
    }
  }
}
Chat

Nouvelle conversation



#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <Wire.h>
#include <Tone32.h>

#define BUZZER_PIN 32
#define BUZZER_CHANNEL 0
#define TXD2 21
#define RXD2 22

int ledR =5;
int ledO =18;
int ledV =19;
int ledB =23;

// ###########################################################################
// SENSOR S8 
// ###########################################################################
byte CO2req[] = {0xFE, 0X04, 0X00, 0X03, 0X00, 0X01, 0XD5, 0XC5};
byte Response[20];
uint16_t crc_02;
int ASCII_WERT;
int int01, int02, int03;
unsigned long ReadCRC;        // CRC Control Return Code 

// Ausselesen ABC-Periode / FirmWare / SensorID 
byte ABCreq[] = {0xFE, 0X03, 0X00, 0X1F, 0X00, 0X01, 0XA1, 0XC3};       // 1f in Hex -> 31 dezimal
byte FWreq[] = {0xFE, 0x04, 0x00, 0x1C, 0x00, 0x01, 0xE4, 0x03};        // FW      ->       334       1 / 78
byte ID_Hi[] = {0xFE, 0x04, 0x00, 0x1D, 0x00, 0x01, 0xB5, 0xC3};        // Sensor ID hi    1821       7 / 29
byte ID_Lo[] = {0xFE, 0x04, 0x00, 0x1E, 0x00, 0x01, 0x45, 0xC3};        // Sensor ID lo   49124     191 / 228 e.g. in Hex 071dbfe4
//byte Tstreq[] = {0xFE, 0x03, 0x00, 0x01, 0x00, 0x01, 0xE4, 0x03};     // Read the holding register
//byte Tstreq[] = {0xFE, 0x04, 0x00, 0x01, 0x00, 0x01, 0xE4, 0x03};     // Read the input Register
byte Tstreq[] = {0xFE, 0x44, 0X00, 0X01, 0X02, 0X9F, 0X25};             // undocumented function 44
int Test_len = 7;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
String txValue;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);
        Serial.println();
        Serial.println("*********");
      }
    }
};
               // length of Testrequest in case of function 44 only 7 otherwise 8
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
// ###########################################################################
// SENSOR S8 
// ###########################################################################


void setup() {
  
  pinMode(ledR, OUTPUT);
  pinMode(ledO, OUTPUT);
  pinMode(ledV, OUTPUT); 
  pinMode(ledB, OUTPUT);  

  delay(1000);
  int i;
  for(i=0;i<3;i++)
  {
      digitalWrite(ledB,HIGH);
      delay(250);
      digitalWrite(ledV,HIGH);
      delay(250);
      digitalWrite(ledO,HIGH); 
      delay(250);
      digitalWrite(ledR,HIGH); 
      delay(250);
      digitalWrite(ledR,LOW);
      delay(250);
      digitalWrite(ledO,LOW);
      delay(250);
      digitalWrite(ledV,LOW);
      delay(250);
      digitalWrite(ledB,LOW);  
      delay(250);
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
  
  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  /*pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                  */
                  
   pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,BLECharacteristic::PROPERTY_NOTIFY);
                    // BLEStringCharacteristic txString(uuidOfTxString, BLERead | BLENotify | BLEBroadcast, 20); 
   pTxCharacteristic->addDescriptor(new BLE2902());
  
   BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,BLECharacteristic::PROPERTY_WRITE);
  
   pRxCharacteristic->setCallbacks(new MyCallbacks());

    // Start the service
    pService->start();
  
    // Start advertising
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");
}

void loop() {

  send_Request(CO2req, 8);               // send request for CO2-Data to the Sensor
  read_Response(7);                      // receive the response from the Sensor
  unsigned long CO2 = get_Value(7);
  int valeur = (int)CO2;
  delay(6000);
  String CO2s = "concentration:" + String(CO2); // To be compatible with FIZZIQ
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
    digitalWrite(BUZZER_PIN,LOW);
  }
  else if (CO2 < 600){
    digitalWrite(ledB,HIGH);
    digitalWrite(ledV,LOW);
    digitalWrite(ledO,LOW);
    digitalWrite(ledR,LOW);
    digitalWrite(BUZZER_PIN,LOW);
  }
  else if (CO2 < 800){
    digitalWrite(ledB,LOW);
    digitalWrite(ledV,HIGH);    
    digitalWrite(ledO,LOW);
    digitalWrite(ledR,LOW);
    digitalWrite(BUZZER_PIN,LOW);
  }
  else if (CO2 < 1000){
    digitalWrite(ledB,LOW);
    digitalWrite(ledV,LOW);
    digitalWrite(ledO,HIGH);
    digitalWrite(ledR,LOW);
    digitalWrite(BUZZER_PIN,LOW);
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
  if (deviceConnected) {
    
    String txValue=CO2s;
    
    delay(1000);
    char txString[txValue.length()+1]; // tableau de char de la taille du String param+1 (caractÃ¨re de fin de ligne)
    CO2s.toCharArray(txString,txValue.length()+1); // rÃ©cupÃ¨re le param dans le tableau de char  
    Serial.println(txString); // affiche le tableau de char
  
    pTxCharacteristic->setValue(txString);
    pTxCharacteristic->notify();
  
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
          delay(500); // give the bluetooth stack the chance to get things ready
          pServer->startAdvertising(); // restart advertising
          Serial.println("start advertising");
          oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
    }
  }
}
