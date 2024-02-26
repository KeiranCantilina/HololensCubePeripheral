//Libraries
#include <Vector_datatype.h>
#include <quaternion_type.h>
#include <vector_type.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include <Adafruit_Sensor.h>
#include <Wire.h>

// Tare Button Stuff
struct Button {
    const uint8_t PIN;
    uint32_t numberKeyPresses;
    bool pressed;
};
Button button1 = {6, 0, false};


// BLE UUIDs
#define SERVICE_UUID        "56507bcc-dc2f-44b6-8d75-ab321779368c"
#define CHARACTERISTIC_UUID "470d57b4-95d2-439b-a6cb-b1e68eb55352"

// IMU Define
#define BNO085
//#define DEBUG
bool IMUConnected;

#ifdef BNO085

// Adafruit BNO085 specific libraries
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

// BLE and Misc global vars
int led = LED_BUILTIN;
bool deviceConnected = false;
BLECharacteristic *pCharacteristic;
BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
byte packetArray[16];
//const char ManufacturerData[] = "PUDGIES!";
float rX, rY, rZ, rW;



// Interrupt Function
// void ARDUINO_ISR_ATTR isr() {
//     button1.numberKeyPresses += 1;
//     button1.pressed = true;
// }


// BNO085 vars
#ifdef BNO085
Adafruit_BNO08x  bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;
quat_t tare = {0,0,0,1};
quat_t dataQuat = {0,0,0,1};


// Set reports function
void setReports(void) {
  Serial.println("Setting desired reports");
  if (! bno08x.enableReport(SH2_GAME_ROTATION_VECTOR)) {
    Serial.println("Could not enable game vector");
  }
}


// Set Tare Function
void setTare(void){

  Serial.println("Setting Tare");
  delay(10);

  if (bno08x.wasReset()) {
    Serial.print("sensor was reset ");
    setReports();
  }
  if (! bno08x.getSensorEvent(&sensorValue)) {
    return;
  }
  
  // Get current orientation and stuff data into quaternion
  switch (sensorValue.sensorId) {
    case SH2_GAME_ROTATION_VECTOR:
      tare.set(0,sensorValue.un.gameRotationVector.i);
      tare.set(1,sensorValue.un.gameRotationVector.j);
      tare.set(2,sensorValue.un.gameRotationVector.k);
      tare.set(3,sensorValue.un.gameRotationVector.real);
      break;
  }

  // Tare quaternion is conjugate of data quaternion
  tare = tare.conj();
}
#endif


//Setup BLE callbacks onConnect and onDisconnect
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


// Function for getting orientation as a byte array
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

        // Old untared data method
        // rW = sensorValue.un.gameRotationVector.real;
        // rX = sensorValue.un.gameRotationVector.i;
        // rY = sensorValue.un.gameRotationVector.j;
        // rZ = sensorValue.un.gameRotationVector.k;

        // Get data from IMU and stuff into quaternion
        dataQuat.set(0,sensorValue.un.gameRotationVector.i);
        dataQuat.set(1,sensorValue.un.gameRotationVector.j);
        dataQuat.set(2,sensorValue.un.gameRotationVector.k);
        dataQuat.set(3,sensorValue.un.gameRotationVector.real);

        // Multiply data by tare quaternion
        dataQuat = dataQuat * tare;

        // Break apart quaternion into float components
        rX = dataQuat.get(0);
        rY = dataQuat.get(1);
        rZ = dataQuat.get(2);
        rW = dataQuat.get(3);

        // Debug print
        Serial.print("Orientation Rotation Vector   x: ");
        Serial.print(rX, 5);
        Serial.print(" y: ");
        Serial.print(rY, 5);
        Serial.print(" z: ");
        Serial.print(rZ, 5);
        Serial.print(" w: ");
        Serial.println(rW, 5);
        break;
    }
  #endif
  
  // Hardcoded debug data
  #ifdef DEBUG
  rX = -0.1286683;
  rY = -0.1286683;
  rZ = -0.2573365;
  rW = -0.9490347;
  #endif

  // Stuffing quaternion data floats into a byte array
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

  // For some reason this works better, instead of making PpacketArray a global variable
  memcpy(packetArray, PpacketArray, sizeof(packetArray));

  Serial.println(packetArray[1], HEX);
  Serial.println(PpacketArray[1], HEX);
}


// Setup runs once after power on
void setup() {
  IMUConnected = false;

  // Some boards work best if we also make a serial connection
  Serial.begin(115200);
  delay(1000);
  
  // set LED to be an output pin
  pinMode(led, OUTPUT);

  // Setup interrupt pins
  //pinMode(button1.PIN, INPUT_PULLUP);
  //attachInterrupt(button1.PIN, isr, FALLING);

  // Test printing to serial in setup
  Serial.println("Setup Step 1 Complete!");

  #ifdef BNO085

  // Begin BNO085 SPI comms
  if (!bno08x.begin_SPI(BNO08X_CS, BNO08X_INT)) {
    Serial.println("Failed to find BNO08x chip");
    while (1) { delay(10); }
  }
  Serial.println("BNO08x Found!");

  // ID prints
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

  // Set report settings
  setReports();
  IMUConnected = true;

  // Tare IMU
  //setTare();

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
  pCharacteristic->addDescriptor(&pCharacteristicDescriptor);

  // Start Service
  pService->start();
  
  // Create advertiser
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);

  // Nickname of device
  oAdvertisementData.setShortName("Cubii");
  //oAdvertisementData.setManufacturerData(ManufacturerData);
  pAdvertising->setAdvertisementData(oAdvertisementData);

  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);

  // Start Advertising
  BLEDevice::startAdvertising();
  Serial.println("Setup Step 2 Complete!");
}


// To run continuously after setup()
void loop() {
  
  #ifndef DEBUG
  if(IMUConnected){
    // Check if we need to tare
    // if (button1.pressed) {
    //     Serial.println("Tare");
    //     setTare();
    //     button1.pressed = false;
    // }

    // Grab latest orientation data from IMU
    GetOrientation();

    // Serial.print("Sending Value: ");
    // Serial.println(packetArray[1], HEX);

    // if not empty data byte array, do stuff
    if(packetArray[1] != NULL){
      
      // Send data and notify
      //Serial.print("Sending Value: ");
      //Serial.println(packetArray[1], HEX);
      pCharacteristic->setValue(packetArray, sizeof(packetArray));
      pCharacteristic->notify();
    }
    else{
      Serial.println("Packet Array Null!");
      //Serial.println(packetArray[1], HEX);
    }
    // else{
    //   IMUConnected = false;
    //   Serial.println("MPU Disconnected! Error State :(");
    // }
  }
  #endif

  #ifdef DEBUG
  GetOrientation();
  Serial.print("Sending Value: ");
  Serial.println(packetArray[1], HEX);
  pCharacteristic->setValue(packetArray, sizeof(packetArray));
  pCharacteristic->notify();

  // Debug LED blinking
  // for (int i = 0; i<1000; i++){
  //   digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  //   delay(500);                // wait for a half second
  //   digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  //   delay(500);                // wait for a half second
  #endif

  delay(10);
}

