// ===== RX480E → ESP32 =====
// A = 18, B = 19. (C = 22, D = 23 available later)
// Outputs: A-Single=33, A-Double=25, B-Single=26, B-Double=27

// Input Buttons (from the RX480E)
#define PIN_A 18
#define PIN_B 19

// Output pins
#define OUT_A_SINGLE 33
#define OUT_A_DOUBLE 25
#define OUT_B_SINGLE 26
#define OUT_B_DOUBLE 27

// Timing 
const uint32_t DOUBLE_MS   = 450;    // max gap for double
const uint32_t DEBOUNCE_US = 9000;   // ignore edges <9ms apart (RF train filter)

// Button state structure
struct Btn {
  bool pendingSingle = false;
  uint32_t singleDeadline = 0;
  bool toggleSingle = false;
  bool toggleDouble = false;
};

Btn A, B;

// Interupt Service Routines (ISR) flags + timestamps
volatile bool pulseA = false;
volatile bool pulseB = false;
volatile uint32_t lastA_us = 0;
volatile uint32_t lastB_us = 0;

// Debounced rising-edge interrupts
void IRAM_ATTR isrA() {
  uint32_t now = micros();
  if (now - lastA_us >= DEBOUNCE_US) {
    lastA_us = now;
    pulseA = true;
  }
}
void IRAM_ATTR isrB() {
  uint32_t now = micros();
  if (now - lastB_us >= DEBOUNCE_US) {
    lastB_us = now;
    pulseB = true;
  }
}

// Click Logic
void doSingle(Btn& b, const char* name, int outPin) {
  b.toggleSingle = !b.toggleSingle;
  digitalWrite(outPin, b.toggleSingle ? HIGH : LOW);
  Serial.printf("%s SINGLE -> %s\n", name, b.toggleSingle ? "ON" : "OFF");
}

void doDouble(Btn& b, const char* name, int outPin) {
  b.toggleDouble = !b.toggleDouble;
  digitalWrite(outPin, b.toggleDouble ? HIGH : LOW);
  Serial.printf("%s DOUBLE -> %s\n", name, b.toggleDouble ? "ON" : "OFF");
}

void processBtn(Btn& b, volatile bool& pulseFlag, const char* name, int outSingle, int outDouble) {
  uint32_t now = millis();

  if (pulseFlag) {
    pulseFlag = false;

    // Second click → DOUBLE
    if (b.pendingSingle && now <= b.singleDeadline) {
      b.pendingSingle = false;
      doDouble(b, name, outDouble);
      return;
    }

    // First click → wait for possible double
    b.pendingSingle = true;
    b.singleDeadline = now + DOUBLE_MS;
  }

  // If we waited too long → SINGLE
  if (b.pendingSingle && now > b.singleDeadline) {
    b.pendingSingle = false;
    doSingle(b, name, outSingle);
  }
}


void setup() {
  Serial.begin(115200);

  // Inputs (internal pulldown avoids ghost pulses)
  pinMode(PIN_A, INPUT_PULLDOWN);
  pinMode(PIN_B, INPUT_PULLDOWN);

  // Outputs
  pinMode(OUT_A_SINGLE, OUTPUT);
  pinMode(OUT_A_DOUBLE, OUTPUT);
  pinMode(OUT_B_SINGLE, OUTPUT);
  pinMode(OUT_B_DOUBLE, OUTPUT);

  // Rising-edge only
  attachInterrupt(digitalPinToInterrupt(PIN_A), isrA, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_B), isrB, RISING);

  Serial.println("Ready: SINGLE + DOUBLE + TOGGLES");
}

void loop() {
  processBtn(A, pulseA, "A", OUT_A_SINGLE, OUT_A_DOUBLE);
  processBtn(B, pulseB, "B", OUT_B_SINGLE, OUT_B_DOUBLE);
}