#define AH 3
#define BH 5
#define CH 9

#define AL 4
#define BL 6
#define CL 10

#define FAULT_PIN 13

#define HALL_A 2
#define HALL_B 7
#define HALL_C 8

#define DIR_PIN 12
#define POT_PIN A0


byte lastHall = 255;
byte activeStep = 0; 
volatile bool faultState = false;

unsigned long lastHallTime = 0; 
unsigned long lastPITime = 0;

float measureRpm = 200;
float targetRpm;

float integral = 0;
float error = 0;
float correction = 0;

float Kp = 2;
float Ki = 1.4;

int pwmDuty = 0;
const int PWM_MIN = 40;
const int PWM_MAX = 255;

byte seenMask = 0;
unsigned long lastSeenPrintTime = 0;

const int POLE_PAIRS = 1;
const unsigned long PI_INTERVAL_MS = 1;  


void overCurrent() {
  allOff();
  faultState = true;
  activeStep = 0;
}

void allOff() {
  analogWrite(AH, 0);  digitalWrite(AL, LOW);
  analogWrite(BH, 0);  digitalWrite(BL, LOW);
  analogWrite(CH, 0);  digitalWrite(CL, LOW);
}

void setStep(byte step, int duty) {
  allOff();
  delayMicroseconds(10);
  if (faultState) { activeStep = 0; return; }

  switch (step) {
    case 1: analogWrite(AH, duty); digitalWrite(BL, HIGH); break; // AH BL
    case 2: analogWrite(AH, duty); digitalWrite(CL, HIGH); break; // AH CL
    case 3: analogWrite(BH, duty); digitalWrite(CL, HIGH); break; // BH CL
    case 4: analogWrite(BH, duty); digitalWrite(AL, HIGH); break; // BH AL
    case 5: analogWrite(CH, duty); digitalWrite(AL, HIGH); break; // CH AL
    case 6: analogWrite(CH, duty); digitalWrite(BL, HIGH); break; // CH BL
  }
  activeStep = step;
}


void updateDuty(int duty) {
  if (activeStep == 0 || faultState) return;
  switch (activeStep) {
    case 1: case 2: analogWrite(AH, duty); break;
    case 3: case 4: analogWrite(BH, duty); break;
    case 5: case 6: analogWrite(CH, duty); break;
  }
}

byte readHall() {
  byte hall = 0;
  if (digitalRead(HALL_A)) hall |= 1;
  if (digitalRead(HALL_B)) hall |= 2;
  if (digitalRead(HALL_C)) hall |= 4;
  return hall;
}

byte hallToStep(byte hall, bool ccw) {
  if (!ccw) {
    switch (hall) {
      case 0b100: return 1;
      case 0b110: return 2;
      case 0b010: return 3;
      case 0b011: return 4;
      case 0b001: return 5;
      case 0b101: return 6;
      default: return 0;
    }
  } else {
    switch (hall) {
      case 0b110: return 1;
      case 0b010: return 2;
      case 0b011: return 3;
      case 0b001: return 4;
      case 0b101: return 5;
      case 0b100: return 6;
      default: return 0;
    }
  }
}

void startKick(bool ccw) {
  if (faultState) { allOff(); return; }

  if(!ccw) {
    setStep(5,255); delay(100);
    setStep(6,255); delay(100);
    setStep(1,255); delay(100);
    setStep(2,255); delay(100);
    setStep(3,255); delay(100);
    setStep(4,255);
  }
  else {
    setStep(4,255); delay(100);
    setStep(3,255); delay(100);
    setStep(2,255); delay(100);
    setStep(1,255); delay(100);
    setStep(6,255); delay(100);
    setStep(5,255);
  }
}

void setup() {
  pinMode(AH, OUTPUT); pinMode(AL, OUTPUT);
  pinMode(BH, OUTPUT); pinMode(BL, OUTPUT);
  pinMode(CH, OUTPUT); pinMode(CL, OUTPUT);

  pinMode(HALL_A, INPUT);
  pinMode(HALL_B, INPUT);
  pinMode(HALL_C, INPUT);

  pinMode(DIR_PIN, INPUT_PULLUP);
  pinMode(POT_PIN, INPUT);
  pinMode(FAULT_PIN, INPUT_PULLUP);

  Serial.begin(9600);

  bool ccw = (digitalRead(DIR_PIN) == LOW);

  if (digitalRead(FAULT_PIN) == LOW) {
    startKick(ccw);
    lastHall = readHall();
    lastHallTime = millis();
    lastPITime = millis();
  }
}

void loop() {

  if (digitalRead(FAULT_PIN) == HIGH) {
    overCurrent();
  }

  if (faultState) {
    allOff();
    return;
  }

  bool ccw = (digitalRead(DIR_PIN) == LOW);

  int potValue = analogRead(POT_PIN);
  targetRpm = map(potValue, 0, 1023, 200, 500); 

  byte hall = readHall();
  unsigned long now = millis();

  if (hall != lastHall) {
    unsigned long period = now - lastHallTime;
    lastHallTime = now;
    lastHall = hall;  

    if (period > 0) {
      measureRpm = (60000.0 / (period * 6.0)) / POLE_PAIRS;
    }

    byte step = hallToStep(hall, ccw);
    if (step != 0) {
      setStep(step, pwmDuty);   
    } else {
      allOff();
      activeStep = 0;
    }
  }


  unsigned long piNow = millis();
  if ((piNow - lastPITime) >= PI_INTERVAL_MS) {
    dt_update(piNow);
  }
}

void dt_update(unsigned long piNow) {
  float dt = (piNow - lastPITime) / 1000.0;
  lastPITime = piNow;

  error = targetRpm - measureRpm;
  integral += error * dt;

  float integralMax = PWM_MAX / max(Ki, 0.0001);
  if (integral > integralMax)  integral = integralMax;
  if (integral < -integralMax) integral = -integralMax;

  correction = Kp * error + Ki * integral;

  pwmDuty = constrain((int)correction, 0, PWM_MAX);

  updateDuty(pwmDuty);
}

