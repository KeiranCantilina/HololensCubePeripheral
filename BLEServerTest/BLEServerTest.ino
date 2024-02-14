#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>


// BLE UUIDs
#define SERVICE_UUID        "56507bcc-dc2f-44b6-8d75-ab321779368c"
#define CHARACTERISTIC_UUID "470d57b4-95d2-439b-a6cb-b1e68eb55352"

// IMU Define
#define BNO085
//#define DEBUG
bool MPUConnected;

#ifdef BNO085
  #include <Adafruit_BNO08x.h>
  #include <sh2.h>
  #include <sh2_SensorValue.h>
  #include <sh2_err.h>
  #include <sh2_hal.h>
  #include <sh2_util.h>
  #include <shtp.h>

  // Pinout
  #define BNO08X_CS 10 // Chip Select active low
  #define BNO08X_INT 9 // Interrupt active low
  #define BNO08X_RESET 5 // Reset active low
#endif

// BLE and Misc vars
int led = LED_BUILTIN;
bool deviceConnected = false;
BLECharacteristic *pCharacteristic;
BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
float quaternionArray[4];
byte packetArray[16];
//const char ManufacturerData[] = "PUDGIES!";

// Float/Byte string conversion union
// typedef union
// {
//   float number;
//   uint8_t bytes[4];
// } FLOATUNION_t;
// FLOATUNION_t rX, rY, rZ, rW;
float rX, rY, rZ, rW;

// BNO085 vars
#ifdef BNO085
Adafruit_BNO08x  bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

void setReports(void) {
  Serial.println("Setting desired reports");
  if (! bno08x.enableReport(SH2_GAME_ROTATION_VECTOR)) {
    Serial.println("Could not enable game vector");
  }
}
#endif

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Connected!");
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Disconnected!");
  }
};


// Function for getting orientation as a byte array sneakily disguised as a 16-byte string
void GetOrientation(){

  #ifdef BNO085
    delay(10);

    if (bno08x.wasReset()) {
      Serial.print("sensor was reset ");
      setReports();
    }
    
    //Serial.println("Getting sensor data...");

    if (! bno08x.getSensorEvent(&sensorValue)) {
      return;
    }
    
    switch (sensorValue.sensorId) {
      case SH2_GAME_ROTATION_VECTOR:
        rW = sensorValue.un.gameRotationVector.real;
        rX = sensorValue.un.gameRotationVector.i;
        rY = sensorValue.un.gameRotationVector.j;
        rZ = sensorValue.un.gameRotationVector.k;
        Serial.print("Orientation Rotation Vector   w: ");
        Serial.print(rW, 5);
        Serial.print(" x: ");
        Serial.print(rX, 5);
        Serial.print(" y: ");
        Serial.print(rY, 5);
        Serial.print(" z: ");
        Serial.println(rZ, 5);
        break;
    }
  #endif
  

  // Stuffing quaternion data into a float array
  
  #ifdef DEBUG
  rX = -0.1286683;
  rY = -0.1286683;
  rZ = -0.2573365;
  rW = -0.9490347;
  // quaternionArray[0] = (float)-0.1286683;
  // quaternionArray[1] = (float)-0.1286683;
  // quaternionArray[2] = (float)-0.2573365;
  // quaternionArray[3] = (float)-0.9490347;
  quaternionArray[0] = (float)rX;
  quaternionArray[1] = (float)rY;
  quaternionArray[2] = (float)rZ;
  quaternionArray[3] = (float)rW;
  #endif
  #ifndef DEBUG
  quaternionArray[0] = (float)rX;
  quaternionArray[1] = (float)rY;
  quaternionArray[2] = (float)rZ;
  quaternionArray[3] = (float)rW;

  byte PpacketArray[16] = {
  ((uint8_t*)&rX)[0],
  ((uint8_t*)&rX)[1],
  ((uint8_t*)&rX)[2],
  ((uint8_t*)&rX)[3],
  ((uint8_t*)&rY)[0],
  ((uint8_t*)&rY)[1],
  ((uint8_t*)&rY)[2],
  ((uint8_t*)&rY)[3],
  ((uint8_t*)&rZ)[0],
  ((uint8_t*)&rZ)[1],
  ((uint8_t*)&rZ)[2],
  ((uint8_t*)&rZ)[3],
  ((uint8_t*)&rW)[0],
  ((uint8_t*)&rW)[1],
  ((uint8_t*)&rW)[2],
  ((uint8_t*)&rW)[3]
  };
  memcpy(packetArray, &PpacketArray, sizeof(packetArray));
  #endif

  Serial.println(quaternionArray[0]);
  // Dirty convert quaternion float array to char array (LITTLE ENDIAN)
  //Serial.println("Stuffing quaternion array into string");
  // char quaternionChars[sizeof(quaternionArray)];
  // memcpy(quaternionChars, &quaternionArray, sizeof(quaternionArray));
  // Serial.print("Char array: ");
  // Serial.println(quaternionChars);
  // return quaternionChars;
  //return quaternionArray;
}



void setup() {
  MPUConnected = false;
  // Some boards work best if we also make a serial connection
  Serial.begin(115200);
  delay(1000);
  
  // set LED to be an output pin
  pinMode(led, OUTPUT);

  // Test printing to serial in setup
  Serial.println("Setup Step 1 Complete!");

#ifdef BNO085
  if (!bno08x.begin_SPI(BNO08X_CS, BNO08X_INT)) {
    Serial.println("Failed to find BNO08x chip");
    while (1) { delay(10); }
  }
  Serial.println("BNO08x Found!");

  for (int n = 0; n < bno08x.prodIds.numEntries; n++) {
    Serial.print("Part ");
    Serial.print(bno08x.prodIds.entry[n].swPartNumber);
    Serial.print(": Version :");
    Serial.print(bno08x.prodIds.entry[n].swVersionMajor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionMinor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionPatch);
    Serial.print(" Build ");
    Serial.println(bno08x.prodIds.entry[n].swBuildNumber);
  }

  setReports();
  MPUConnected = true;
  Serial.println("Reading events");
  delay(100);
#endif



  // BLE Setup
  BLEDevice::init("Cubii");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID), 30, 0);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  BLEDescriptor pCharacteristicDescriptor(BLEUUID((uint16_t)0x2902));
  //pCharacteristicDescriptor.setValue("Test Data");
  pCharacteristic->addDescriptor(&pCharacteristicDescriptor);

  // pCharacteristic->setValue("Hello World says Keiran");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);

  oAdvertisementData.setShortName("Cubii");
  //oAdvertisementData.setManufacturerData(ManufacturerData);
  pAdvertising->setAdvertisementData(oAdvertisementData);

  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Setup Step 2 Complete!");
}

void loop() {
  // Say hi!
  //Serial.println("Running");
#ifndef DEBUG
  if(MPUConnected){
    //float[] result = GetOrientation();
    GetOrientation();
    Serial.print("Sending Value: ");
    Serial.println(packetArray[1], HEX);
    // if empty string, break
    if(quaternionArray != NULL){
      //do stuff
      
      pCharacteristic->setValue(packetArray, sizeof(packetArray));
      pCharacteristic->notify();
    }
    // else{
    //   MPUConnected = false;
    //   Serial.println("MPU Disconnected! Error State :(");
    // }
  }
#endif
#ifdef DEBUG
  //String result = GetOrientation();
  GetOrientation();
  Serial.print("Sending Value: ");
  Serial.println(packetArray[1], HEX);
  //pCharacteristic->setValue(result);
  
  pCharacteristic->setValue(&packetArray, sizeof(packetArray));
  pCharacteristic->notify();
#endif

  delay(10);
  // for (int i = 0; i<1000; i++){
  //   digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  //   delay(500);                // wait for a half second
  //   digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  //   delay(500);                // wait for a half second
  
  
  //   pCharacteristic->setValue(String(i));
  //   pCharacteristic->notify();
  // }
    
}

