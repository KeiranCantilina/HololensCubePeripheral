#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>


// BLE UUIDs
#define SERVICE_UUID        "56507bcc-dc2f-44b6-8d75-ab321779368c"
#define CHARACTERISTIC_UUID "470d57b4-95d2-439b-a6cb-b1e68eb55352"

// IMU Define
//#define BNO085
#define DEBUG
bool MPUConnected = false;
#ifdef MPU6050
  #include <Adafruit_MPU6050.h>
#endif
#ifdef BNO085
  #include <Adafruit_BNO08x.h>
  #include <sh2.h>
  #include <sh2_SensorValue.h>
  #include <sh2_err.h>
  #include <sh2_hal.h>
  #include <sh2_util.h>
  #include <shtp.h>
  #define BNO08X_CS 10 // Change these
  #define BNO08X_INT 9
  #define BNO08X_RESET 5
#endif

// BLE and Misc vars
int led = LED_BUILTIN;
bool deviceConnected = false;
BLECharacteristic *pCharacteristic;
BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
//const char ManufacturerData[] = "PUDGIES!";

// Float/Byte string conversion union
typedef union
{
  float number;
  uint8_t bytes[4];
} FLOATUNION_t;
FLOATUNION_t rX, rY, rZ, rW;

// MPU6050 Vars
#ifdef MPU6050
Adafruit_MPU6050 mpu;

float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, accAngleZ;
float gyroAngleX = 0;
float gyroAngleY = 0;
float gyroAngleZ = 0;
float roll, pitch, yaw;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;
#endif

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
String GetOrientation(){
  
  #ifdef  MPU6050
  sensors_event_t a, g;
  mpu.getEvent(&a, &g);
  AccX = a.acceleration.x - AccErrorX;
  AccY = a.acceleration.y - AccErrorY;
  AccZ = a.acceleration.z - AccErrorZ;
  GyroX = g.gyro.x - GyroErrorX;
  GyroY = g.gyro.y - GyroErrorY;
  GyroZ = g.gyro.z - GyroErrorZ;
  previousTime = currentTime;        // Previous time is stored before the actual time read
  currentTime = millis();            // Current time actual time read
  elapsedTime = (currentTime - previousTime) / 1000; // Divide by 1000 to get seconds

  // THIS ALL NEEDS TO BE REDONE 
  /*
  // Roll and pitch calculations
  accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI); // AccErrorX ~(0.58) See the calculate_IMU_error()custom function for more details
  accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI); // AccErrorY ~(-1.58)
  

  // Currently the raw values are in radians per seconds, rad/s, so we need to multiply by sendonds (s) to get the angle in degrees
  gyroAngleX = gyroAngleX + GyroX * elapsedTime; // deg/s * s = deg
  gyroAngleY = gyroAngleY + GyroY * elapsedTime;
  yaw =  yaw + GyroZ * elapsedTime;

  */

  // Complementary filter - combine acceleromter and gyro angle values
  roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
  pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;

  // Print the values on the serial monitor
  Serial.print(roll);
  Serial.print("/");
  Serial.print(pitch);
  Serial.print("/");
  Serial.println(yaw);
  
  #endif

  #ifdef BNO085
    delay(10);

    if (bno08x.wasReset()) {
      Serial.print("sensor was reset ");
      setReports();
    }
    
    if (! bno08x.getSensorEvent(&sensorValue)) {
      return "";
    }
    
    switch (sensorValue.sensorId) {
      case SH2_ROTATION_VECTOR:
        rW.number = sensorValue.un.gameRotationVector.real;
        rX.number = sensorValue.un.gameRotationVector.i;
        rY.number = sensorValue.un.gameRotationVector.j;
        rZ.number = sensorValue.un.gameRotationVector.k;
        Serial.print("Orientation Rotation Vector - w: ");
        Serial.print(rW.number, 5);
        Serial.print(" x: ");
        Serial.print(rX.number, 5);
        Serial.print(" y: ");
        Serial.print(rY.number, 5);
        Serial.print(" z: ");
        Serial.println(rZ.number, 5);
        break;
    }
  #endif
  

  // Stuffing quaternion data into a float array
  float quaternionArray[4] = {rX.number, rY.number, rZ.number, rW.number};
  #ifdef DEBUG
  quaternionArray[0] = (float)0.1234;
  quaternionArray[1] = (float)0.1234;
  quaternionArray[2] = (float)0.1234;
  quaternionArray[3] = (float)0.1234;
  #endif
  
  // Dirty convert quaternion float array to char array (LITTLE ENDIAN)
  char quaternionChars[sizeof(quaternionArray)];
  memcpy(quaternionChars, &quaternionArray, sizeof(quaternionArray));
  return quaternionChars;
}

#ifdef MPU6050
void calculate_IMU_error() {
  // We can call this funtion in the setup section to calculate the accelerometer and gyro data error. From here we will get the error values used in the above equations printed on the Serial Monitor.
  // Note that we should place the IMU flat in order to get the proper values, so that we then can the correct values
  // Read accelerometer values 200 times
  while (c < 200) {
    sensors_event_t a, g;
    mpu.getEvent(&a, &g);
    AccX = a.acceleration.x;
    AccY = a.acceleration.y;
    AccZ = a.acceleration.z;

    // Sum all readings
    AccErrorX = AccErrorX + AccX;
    AccErrorY = AccErrorY + AccY;
    AccErrorZ = AccErrorZ + AccZ;
    c++;
  }

  //Divide the sum by 200 to get the error value
  AccErrorX = AccErrorX / 200;
  AccErrorY = AccErrorY / 200;
  AccErrorZ = AccErrorZ / 200;
  c = 0;

  // Read gyro values 200 times
  while (c < 200) {
    GyroX = g.gyro.x;
    GyroY = g.gyro.y;
    GyroZ = g.gyro.z;
    // Sum all readings
    GyroErrorX = GyroErrorX + GyroX;
    GyroErrorY = GyroErrorY + GyroY;
    GyroErrorZ = GyroErrorZ + GyroZ;
    c++;
  }
  //Divide the sum by 200 to get the error value
  GyroErrorX = GyroErrorX / 200;
  GyroErrorY = GyroErrorY / 200;
  GyroErrorZ = GyroErrorZ / 200;
  // Print the error values on the Serial Monitor
  Serial.print("AccErrorX: ");
  Serial.println(AccErrorX);
  Serial.print("AccErrorY: ");
  Serial.println(AccErrorY);
  Serial.print("GyroErrorX: ");
  Serial.println(GyroErrorX);
  Serial.print("GyroErrorY: ");
  Serial.println(GyroErrorY);
  Serial.print("GyroErrorZ: ");
  Serial.println(GyroErrorZ);
}
#endif

void setup() {
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

  Serial.println("Reading events");
  delay(100);
#endif

#ifdef MPU6050
  // MPU6050 Setup
  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    MPUConnected = false;
  }
  else{
    Serial.println("MPU6050 Found!");
    MPUConnected = true;

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

    mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
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

    // Calibration
    calculate_IMU_error();

    // Spaceeee
    Serial.println("");
    delay(100);
  }
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

  pCharacteristic->setValue("Hello World says Keiran");
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
  Serial.println("Running");
#ifndef DEBUG
  if(MPUConnected){
    String result = GetOrientation();
    // if empty string, break
    if(result != ""){
      //do stuff
      pCharacteristic->setValue(result);
      pCharacteristic->notify();
    }
  }
#endif
#ifdef DEBUG
  String result = GetOrientation();
  pCharacteristic->setValue(result);
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

