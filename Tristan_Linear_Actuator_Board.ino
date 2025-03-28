/// The FLGT_COMP and BLU_RVN pins correspond to GPIO pins 1 and 2
int FLGT_COMP = 1;
int BLU_RVN = 2;

/// The MOTOR1 and MOTOR2 pins correspond to GPIO pins 38 and 39
int MOTOR1 = 38;
int MOTOR2 = 39;

/// Relevant threshold voltages for the Flight Computer/Fluctus and Blue Raven
float FCThreshold = 0.0;
float BRThreshold = 0.10;

int milliSecondCount = 0;
int LEDValue = 0;


void setup() {
  /// Designate the external connector pins as analog inputs
  pinMode(FLGT_COMP, INPUT);
  pinMode(BLU_RVN, INPUT);
  
  /// Designate the MOTOR pins as digital outputs
  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);

  Serial.begin(9600);
  while(!Serial) {
    delay(100);
  }

  stopMotors();
}

void loop() {
  
  /// Read the analog values off of each of the input pins
  float FLGT_COMP_SIG = analogReadMilliVolts(FLGT_COMP) / 1000.0;
  float BLU_RVN_SIG = analogReadMilliVolts(BLU_RVN) / 1000.0;

  Serial.println("Flight Comp: " + String(FLGT_COMP_SIG) + "V");
  Serial.println("Blue Raven : " + String(BLU_RVN_SIG) + "V");

  /// Alternate the GPIO pins between high and low after 5 seconds
  if (millis() - milliSecondCount > 5000) {
    milliSecondCount = millis();
    LEDValue = 1 - LEDValue;

    digitalWrite(MOTOR1, LEDValue);
    digitalWrite(MOTOR2, 1 - LEDValue);
  }
}

/// Functions to extend, stop, and retract the linear actuators
void extendMotors() {
  digitalWrite(MOTOR1, 1);
  digitalWrite(MOTOR2, 0);
}

void stopMotors() {
  digitalWrite(MOTOR1, 0);
  digitalWrite(MOTOR2, 0);
}

void retractMotors() {
  digitalWrite(MOTOR1, 0);
  digitalWrite(MOTOR2, 1);
}
