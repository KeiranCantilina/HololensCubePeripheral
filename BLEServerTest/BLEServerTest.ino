#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "56507bcc-dc2f-44b6-8d75-ab321779368c"
#define CHARACTERISTIC_UUID "470d57b4-95d2-439b-a6cb-b1e68eb55352"

int led = LED_BUILTIN;
bool deviceConnected = false;
BLECharacteristic *pCharacteristic;

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

void setup() {
  // Some boards work best if we also make a serial connection
  Serial.begin(115200);
  delay(1000);
  
  // set LED to be an output pin
  pinMode(led, OUTPUT);

  // Test printing to serial in setup
  Serial.println("Setup Step 1 Complete!");

  /// Added here

  BLEDevice::init("AardappelSap_BLETest");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  BLEDescriptor pCharacteristicDescriptor(BLEUUID((uint16_t)0x2903));
  pCharacteristicDescriptor.setValue("Test Data");
  pCharacteristic->addDescriptor(&pCharacteristicDescriptor);

  pCharacteristic->setValue("Hello World says Keiran");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Setup Step 2 Complete!");
}

void loop() {
  // Say hi!
  Serial.println("Running");
  for (int i = 0; i<1000; i++){
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(500);                // wait for a half second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(500);                // wait for a half second
  
  
    pCharacteristic->setValue(String(i));
    pCharacteristic->notify();
  }
    
}

