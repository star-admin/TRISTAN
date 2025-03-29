#include <Wire.h>
#include <Adafruit_INA260.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h> 

// Global variables for BLE
BLEServer *pServer = NULL;
BLECharacteristic *pCurrentCharacteristic = NULL;
BLECharacteristic *pBusVoltageCharacteristic = NULL;
BLECharacteristic *pContVoltageCharacteristic = NULL;
BLECharacteristic *pPowerCharacteristic = NULL;
BLECharacteristic *pArmedCharacteristic = NULL;
BLEService *pService = NULL;
BLEAdvertising *pAdvertising = NULL;

Preferences preferences;

// Replace macros with variables
uint8_t ARM_VALUE = 0x01;
uint8_t DISARM_VALUE = 0x00;

// Create an INA260 instance
#define NUM_LEDS 1
#define LED_PIN 5
#define SDA 6
#define SCL 7
#define SWITCH_EN 4
#define CONT_ADC 3
#define CONT_EN 10
Adafruit_INA260 ina260;

bool deviceArmed = false;     // Initial device state
bool deviceConnected = false;  // Set connection flag
bool oldDeviceConnected = false; // For detecting connection state changes

#define SERVICE_UUID "8fc39c4d-8122-45c9-9971-d74f0a238f38"
#define CURRENT_CHARACTERISTIC_UUID "e98d3cd5-d888-4f07-80ad-279b8f7d16a2"
#define BUS_VOLTAGE_CHARACTERISTIC_UUID "71f99001-b3e0-433c-947f-b55ac1dd8d8c"
#define CONT_VOLTAGE_CHARACTERISTIC_UUID "b8e28bee-cdac-44c3-b472-da95392612bb"
#define POWER_CHARACTERISTIC_UUID "08f68bcd-8e15-4142-a76c-654f547ccaf6"
#define ARMED_CHARACTERISTIC_UUID "ca2e52de-27b2-422f-8cd9-4683328c4898"

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
// Function to update the armed characteristic with current state
void updateArmedCharacteristic() {
  if (pArmedCharacteristic != NULL) {
    pArmedCharacteristic->setValue(deviceArmed ? "arm" : "off");
    pArmedCharacteristic->notify();
    Serial.println("[DEBUG] Armed characteristic updated and notified");
  }
}
// Improved server callbacks
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("[DEBUG] Client connected");

    // Send the current arming state to the client after a short delay
    // to ensure all characteristics are discovered
    delay(200);
    updateArmedCharacteristic();
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("[DEBUG] Client disconnected");
    // We'll handle reconnection in the main loop
  }
};

// Improved armed characteristic callbacks
class ArmedCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() == 0) {
      return; // No data received
    }
    
    Serial.print("[DEBUG] Received value: ");
    Serial.println(value.c_str());

    if (value == "arm") {
      deviceArmed = true;
      digitalWrite(SWITCH_EN, HIGH);
      digitalWrite(CONT_EN, LOW);
      Serial.println("[DEBUG] Arming command processed");
      
      // Update LED
      strip.setPixelColor(0, strip.Color(0, 255, 0)); // Green
      strip.show();

      // Save arming state to flash
      preferences.putBool("armed", true);
      Serial.println("[DEBUG] Arming state saved to flash");
    } 
    else if (value == "off") {
      deviceArmed = false;
      digitalWrite(SWITCH_EN, LOW);
      digitalWrite(CONT_EN, HIGH);
      Serial.println("[DEBUG] Disarming command processed");
      
      // Update LED
      strip.setPixelColor(0, strip.Color(255, 0, 0)); // Red
      strip.show();

      // Save arming state to flash
      preferences.putBool("armed", false);
      Serial.println("[DEBUG] Arming state saved to flash");
    }
  }
};



// Function to setup BLE
void setupBLE() {
  // Create BLE Device
  BLEDevice::init("STAR BLE Switch 4");

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  pService = pServer->createService(SERVICE_UUID);

  // Create Characteristics
  pCurrentCharacteristic = pService->createCharacteristic(
    CURRENT_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCurrentCharacteristic->addDescriptor(new BLE2902());

  pBusVoltageCharacteristic = pService->createCharacteristic(
    BUS_VOLTAGE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pBusVoltageCharacteristic->addDescriptor(new BLE2902());

  pContVoltageCharacteristic = pService->createCharacteristic(
    CONT_VOLTAGE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pContVoltageCharacteristic->addDescriptor(new BLE2902());

  pPowerCharacteristic = pService->createCharacteristic(
    POWER_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pPowerCharacteristic->addDescriptor(new BLE2902());

  pArmedCharacteristic = pService->createCharacteristic(
    ARMED_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | 
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY);
  pArmedCharacteristic->addDescriptor(new BLE2902());
  pArmedCharacteristic->setCallbacks(new ArmedCharacteristicCallbacks());

  // Start BLE Services
  pService->start();

  // Start Advertising
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

  // Optimize connection parameters
  /*BLEDevice::setMTU(512); // Increase MTU size
  pAdvertising->setMinInterval(400); // 400*0.625ms = 250ms
  pAdvertising->setMaxInterval(800); // 800*0.625ms = 500ms
  pAdvertising->setMinPreferred(0x06); // Default
  pAdvertising->setMaxPreferred(0x12);*/
  // ^^ these are working values, not optimized
  BLEDevice::setMTU(128); // Reduce MTU size to save power during communication

pAdvertising->setMinInterval(1600); // 1600 * 0.625ms = 1000ms (1 second)
pAdvertising->setMaxInterval(3200); // 3200 * 0.625ms = 2000ms (2 seconds)

pAdvertising->setMinPreferred(0x20); // Suggest a larger connection interval (0x20 = 32 * 1.25ms = 40ms)
pAdvertising->setMaxPreferred(0x40); // Suggest a larger connection interval (0x40 = 64 * 1.25ms = 80ms)

  pAdvertising->start();
  Serial.println("[DEBUG] BLE setup complete - advertising started");
}

void setup() {
  // Start the serial communication
  Serial.begin(115200);
  
  // Initialize LED
  strip.begin();
  strip.show();
  delay(500);
  
  // Initialize GPIO pins
  pinMode(CONT_EN, OUTPUT);
  pinMode(SWITCH_EN, OUTPUT);
  digitalWrite(CONT_EN, LOW);
  digitalWrite(SWITCH_EN, LOW);
  
  // Load preferences
  preferences.begin("ble-switch", false); // Open namespace "ble-switch" in read/write mode
  deviceArmed = preferences.getBool("armed", false); // Load arming state from flash (default: false)
  Serial.print("[DEBUG] Loaded arming state from flash: ");
  Serial.println(deviceArmed ? "ARMED" : "DISARMED");
  
  // Set initial pin states based on loaded arming state
  digitalWrite(SWITCH_EN, deviceArmed ? HIGH : LOW);
  digitalWrite(CONT_EN, deviceArmed ? LOW : HIGH);
  
  // Set initial LED color based on armed state
  strip.setPixelColor(0, deviceArmed ? strip.Color(0, 255, 0) : strip.Color(255, 0, 0));
  strip.show();
  
  // Initialize I2C
  Wire.begin(SDA, SCL);

  // Initialize INA260
 /* if (!ina260.begin()) {
    Serial.println("Failed to find INA260 chip");
    while (1) { delay(10); }
  } */
  ina260.begin();
  Serial.println("INA260 Found!");
  
  // Initialize BLE
  setupBLE();
}

void loop() {
  // Handle reconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give the Bluetooth stack time to get ready
    Serial.println("[DEBUG] Restarting advertising after disconnection");
    
    // Restart advertising to facilitate reconnection
    pAdvertising->stop();
    delay(100);
    pAdvertising->start();
    
    oldDeviceConnected = deviceConnected;
  }
  
  // Connection just established
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("[DEBUG] Connection established");
    oldDeviceConnected = deviceConnected;
  }
  
  // Only update characteristics if connected
  if (deviceConnected) {
    // Update current characteristic
    float currentValue = ina260.readCurrent();
    uint8_t currentBytes[4];
    memcpy(currentBytes, &currentValue, sizeof(currentValue));
    pCurrentCharacteristic->setValue(currentBytes, sizeof(currentBytes));
    pCurrentCharacteristic->notify();

    // Update bus voltage characteristic
    float busVoltageValue = ina260.readBusVoltage();
    uint8_t busVoltageBytes[4];
    memcpy(busVoltageBytes, &busVoltageValue, sizeof(busVoltageValue));
    pBusVoltageCharacteristic->setValue(busVoltageBytes, sizeof(busVoltageBytes));
    pBusVoltageCharacteristic->notify();

    // Update power characteristic
    float powerValue = ina260.readPower();
    uint8_t powerBytes[4];
    memcpy(powerBytes, &powerValue, sizeof(powerValue));
    pPowerCharacteristic->setValue(powerBytes, sizeof(powerBytes));
    pPowerCharacteristic->notify();

    // Update continuity voltage characteristic
    int adcValue = analogRead(CONT_ADC);
    uint8_t contVoltageBytes[4];
    memcpy(contVoltageBytes, &adcValue, sizeof(adcValue));
    pContVoltageCharacteristic->setValue(contVoltageBytes, sizeof(contVoltageBytes));
    pContVoltageCharacteristic->notify();
  }

  // Add a short delay to avoid flooding with notifications
  delay(200);
}