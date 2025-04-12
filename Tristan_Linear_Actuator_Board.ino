/// The FLGT_COMP and BLU_RVN pins correspond to GPIO pins 1 and 2
int FLGT_COMP = 1;
int BLU_RVN = 2;

/// The MOTOR1 and MOTOR2 pins correspond to GPIO pins 38 and 39
int MOTOR1 = 38;
int MOTOR2 = 39;

/// Relevant threshold voltages for the Flight Computer/Fluctus and Blue Raven
float FCThreshold = 0.70;
float BRThreshold = 0.10;
int numChargeFired = 0;

/// -1 for reverse, 0 for stop, 1 for extend
int actuateStatus = 0;
int milliSecondCount = 0;

void setup() {
  /// Designate the external connector pins as analog inputs
  pinMode(FLGT_COMP, INPUT);
  pinMode(BLU_RVN, INPUT);
  
  /// Designate the MOTOR pins as digital outputs
  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);
  stopMotors();

  // Serial.begin(115200);
  // while(!Serial) {
  //   delay(100);
  // }

  // Initialize BLE
  setupBLE();
}

void loop() {
  connectionsCheck();

  /// Read the analog values off of each of the input pins
  float FLGT_COMP_SIG = analogReadMilliVolts(FLGT_COMP) / 1000.0;
  float BLU_RVN_SIG = analogReadMilliVolts(BLU_RVN) / 1000.0;

  // Serial.println("Flight Comp: " + String(FLGT_COMP_SIG) + "V\t" + "Blue Raven : " + String(BLU_RVN_SIG) + "V\t" + String(numChargeFired));

  /// To reduce the impact of noise, only actuate the servos once either/both inputs indicates more than 10 times in a row that a charge has been fired
  if (FLGT_COMP_SIG > FCThreshold || BLU_RVN_SIG < BRThreshold) {
    numChargeFired += 1;

    if (numChargeFired >= 10) {
      extendMotors();
    }
  } else {
    numChargeFired = 0;
  }

  float secondsSince = (millis() - milliSecondCount)/1000.0;

  /// Rules to check if it's time to stop extending or retracting the actuators
  if (actuateStatus == 1 && secondsSince > 15) {
    retractMotors();
  } else if (actuateStatus == -1 && secondsSince > 8) {
    stopMotors();
  }
}

/// Functions to extend, stop, and retract the linear actuators
void extendMotors() {
  actuateStatus = 1;
  milliSecondCount = millis();
  
  digitalWrite(MOTOR1, 1);
  digitalWrite(MOTOR2, 0);
}

void stopMotors() {
  actuateStatus = 0;
  
  digitalWrite(MOTOR1, 0);
  digitalWrite(MOTOR2, 0);
}

void retractMotors() {
  actuateStatus = -1;
  milliSecondCount = millis();
  
  digitalWrite(MOTOR1, 0);
  digitalWrite(MOTOR2, 1);

  disarm();
  updateArmedCharacteristic();
}
