#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "adeff3c9-7d59-4470-a847-da82025400e2"
#define Nav_UUID            "716e54c6-edd1-4732-bde1-aade233caeaa"
#define Batt_UUID           "31da16da-b080-4934-b09d-60b877186aea"

uint8_t BattSense = 34;

uint8_t PWMFront = 16;
uint8_t FrontIn1 = 17;
uint8_t FrontIn2 = 18;

uint8_t PWMRear = 19;
uint8_t RearIn1 = 21;
uint8_t RearIn2 = 22;

uint8_t lights = 4;

uint8_t ENMotorDriver = 23;

class BattInfoCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic) {
      float VBAT = (127.0f/100.0f) * 3.30f * float(analogRead(BattSense)) / 4095.0f;  // LiPo battery
      char BattLevel[10];
      sprintf(BattLevel,"%4.0f",calculateBattLevel(VBAT, 3.2f, 4.2f));
      pCharacteristic->setValue(BattLevel);
      pCharacteristic->notify();
    }
    
    //Asymmetric sigmoidal approximation
    float calculateBattLevel(float voltage, float minVoltage, float maxVoltage) {
      float result = 101 - (101.0f / pow(1.0f + pow(1.33f * (voltage - minVoltage)/(maxVoltage - minVoltage) ,4.5f), 3.0f));
      return result >= 100.0f ? 100.0f : result;
    }
};

class NavigationCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
   
      //Forward
      if(value[0] == '1' && value[1] == '0'){
        digitalWrite(RearIn1, HIGH);
        digitalWrite(RearIn2, LOW);
        Serial.println("Forward");
      }
      
      //Backward
      if(value[0] == '0' && value[1] == '1'){
        digitalWrite(RearIn1, LOW);
        digitalWrite(RearIn2, HIGH);
        Serial.println("Backward");
      }

      //Rear Motor Stall
      if(value[0] == '0' && value[1] == '0'){
        digitalWrite(RearIn1, LOW);
        digitalWrite(RearIn2, LOW);
        Serial.println("Rear Stall");
      }

      //Left
      if(value[2] == '0' && value[3] == '1'){
        digitalWrite(FrontIn1, HIGH);
        digitalWrite(FrontIn2, LOW);
        Serial.println("Left");
      }
      
      //Right
      if(value[2] == '1' && value[3] == '0'){
        digitalWrite(FrontIn1, LOW);
        digitalWrite(FrontIn2, HIGH);
        Serial.println("Right");
      }

      //Front Motor Stall
      if(value[2] == '0' && value[3] == '0'){
        digitalWrite(FrontIn1, LOW);
        digitalWrite(FrontIn2, LOW);
        Serial.println("Front Stall");
      }

      //Headlights on
      if(value[4] == '1'){
        digitalWrite(lights, HIGH);
      }

      //Headlights off
      if(value[4] == '0'){
        digitalWrite(lights, LOW);
      }
      
      //Rear Motor Accleration
      String acc = "";
      acc += value[5];
      acc += value[6];
      acc += value[7];
      ledcWrite(1, acc.toInt());
      Serial.println(acc);

    }    
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
      digitalWrite(ENMotorDriver, HIGH);
      //Set BLE connection interval for connected device
      pServer->updateConnParams(param->connect.remote_bda, 0x0014,0x0028, 0x01F3, 0x0C80);
      BLEDevice::setPower(ESP_PWR_LVL_P7);
      Serial.println("connected");
    }
    
    void onDisconnect(BLEServer* pServer) {
      digitalWrite(ENMotorDriver, LOW);
      Serial.println("disconnected");
    }
};

void setup() {
  Serial.begin(115200);

  BLEDevice::init("Rover");
  BLEDevice::setPower(ESP_PWR_LVL_P7);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *navCharacteristic = pService->createCharacteristic(
                                         Nav_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  navCharacteristic->setCallbacks(new NavigationCallbacks());

  BLECharacteristic *battCharacteristic = pService->createCharacteristic(
                                         Batt_UUID,
                                         BLECharacteristic::PROPERTY_READ
                                       );
  battCharacteristic->setCallbacks(new BattInfoCallbacks());
  
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  pAdvertising->start();

  ledcAttachPin(PWMRear, 1); // assign Motor PWM pins to channels
  ledcAttachPin(PWMFront, 2);
  ledcSetup(1, 12000, 8); // 12 kHz PWM, 8-bit resolution
  ledcSetup(2, 12000, 8);
  ledcWrite(2, 255);

  pinMode(RearIn1, OUTPUT);
  pinMode(RearIn2, OUTPUT);
  pinMode(FrontIn1, OUTPUT);
  pinMode(FrontIn2, OUTPUT);
  pinMode(lights, OUTPUT);
  pinMode(ENMotorDriver, OUTPUT);
}

void loop() {
  delay(2000);
}
