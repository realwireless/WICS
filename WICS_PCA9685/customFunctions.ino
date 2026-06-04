// ===============================================
// nowRail Custom Functions - WICS PCA9685 v 1.02
// ===============================================
#include <Wire.h>
#include <vector>

// Forward declarations to bridge memory pools across sheets
extern std::vector<MyLedConfig> customLeds;
extern std::vector<MyServoConfig> customServos;

// Accessory Command Received callback engine from nowRail
void nowAccComRec(int accNum, byte accInst) {
  
  // 1. Update activity states for custom LED effect tracking routines
  for (auto& led : customLeds) {
    if (led.dcc == accNum) {
      led.isActive = (accInst == led.logic);
      Serial.printf("Custom Effect Event: Address %d -> Port %d changed state. Active: %s (Effect %d)\n", 
                    led.dcc, led.port, led.isActive ? "TRUE" : "FALSE", led.effect);
    }
  }

  // 2. Adjust travel destinations for moving physical layout items (Servos)
  for (auto& s : customServos) {
    if (s.dcc == accNum) {
      if (accInst == 1) {
        s.targetPulse = s.angle1;
      } else {
        s.targetPulse = s.angle0;
      }
      s.lastPosUpdate = millis();
      Serial.printf("Custom Driver Event: Servo Point %d tracking destination path -> %d\n", s.dcc, (int)s.targetPulse);
    }
  }
}

// Not used today
// Control panel response to Accessory commands (Kept clean and empty)
void nowPanelUpdate(int accNum, byte accInst) {
  // Catch custom tracking hooks here if required in the future
}
