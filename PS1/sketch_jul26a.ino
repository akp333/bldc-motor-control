#define AH 3
#define AL 4
#define BH 5
#define BL 6
#define CH 9
#define CL 10

#define FAULT_PIN 13

#define HALL_A 2
#define HALL_B 7
#define HALL_C 8

#define DIR_PIN 12

#define POT_PIN A0


byte lastHall = 255;

volatile bool faultState = false;

// RPM variables
unsigned long lastHallTime = 0;
int hallPeriod;

void overCurrent()
{

  allOff();
  faultState = true;

}


void allOff() {
  digitalWrite(AH, LOW); digitalWrite(AL, LOW);
  digitalWrite(BH, LOW); digitalWrite(BL, LOW);
  digitalWrite(CH, LOW); digitalWrite(CL, LOW);
}


void setStep(byte step) {

  allOff();

  if(faultState){
    allOff();
    return;
  }

  switch(step)
  {
    case 1:      // AH BL
      digitalWrite(AH, HIGH);
      digitalWrite(BL, HIGH);
      break;

    case 2:      // AH CL
      digitalWrite(AH, HIGH);
      digitalWrite(CL, HIGH);
      break;

    case 3:      // BH CL
      digitalWrite(BH, HIGH);
      digitalWrite(CL, HIGH);
      break;

    case 4:      // BH AL
      digitalWrite(BH, HIGH);
      digitalWrite(AL, HIGH);
      break;

    case 5:      // CH AL
      digitalWrite(CH, HIGH);
      digitalWrite(AL, HIGH);
      break;

    case 6:      // CH BL
      digitalWrite(CH, HIGH);
      digitalWrite(BL, HIGH);
      break;
  }
}

void startMotor(bool ccw){

  if(faultState){
    allOff();
    return;
  }

  if(!ccw)
  {

    setStep(6); delay(100);
    setStep(5); delay(100);
    setStep(1); delay(100);
    setStep(2); delay(100);
    setStep(3); delay(100);
    setStep(4); delay(100);

  }

  else
  {

    setStep(4); delay(100);
    setStep(3); delay(100);
    setStep(2); delay(100);
    setStep(1); delay(100);
    setStep(6); delay(100);
    setStep(5); delay(100);   

  }

}


void setup()
{
  pinMode(AH, OUTPUT);
  pinMode(AL, OUTPUT);
  pinMode(BH, OUTPUT);
  pinMode(BL, OUTPUT);
  pinMode(CH, OUTPUT);
  pinMode(CL, OUTPUT);


  pinMode(HALL_A, INPUT);
  pinMode(HALL_B, INPUT);
  pinMode(HALL_C, INPUT);


  pinMode(DIR_PIN, INPUT_PULLUP);

  pinMode(POT_PIN, INPUT);

  pinMode(FAULT_PIN, INPUT_PULLUP);

  Serial.begin(9600);

  lastHall = 255;

  bool ccw = (digitalRead(DIR_PIN)==LOW);

  if(digitalRead(FAULT_PIN)==LOW)
  startMotor(ccw);

}



void loop()
{ 

  if(digitalRead(FAULT_PIN)==HIGH) {overCurrent();}

  if(faultState) 
  {
    allOff();
    return;
  }

  bool ccw = (digitalRead(DIR_PIN)==LOW);


  int potValue = analogRead(POT_PIN);

  hallPeriod = map(potValue, 0, 1023, 25, 50);


  if(digitalRead(FAULT_PIN)==LOW){

  byte hall = 0;

  if(digitalRead(HALL_A)) hall |= 1;
  if(digitalRead(HALL_B)) hall |= 2;
  if(digitalRead(HALL_C)) hall |= 4;


  if(hall != lastHall)
  {

    unsigned long now = millis();

    lastHall = hall;

    if(now-lastHallTime>hallPeriod){

    if(!ccw)
    {
      // CW
      switch(hall)
      {
        case 0b100: setStep(1); break;
        case 0b110: setStep(2); break;
        case 0b010: setStep(3); break;
        case 0b011: setStep(4); break;
        case 0b001: setStep(5); break;
        case 0b101: setStep(6); break;

        default:
        allOff();
        break;
      }
    }


    else
    {
      switch(hall)
      {
        case 0b110: setStep(1); break;
        case 0b010: setStep(2); break;
        case 0b011: setStep(3); break;
        case 0b001: setStep(4); break;
        case 0b101: setStep(5); break;
        case 0b100: setStep(6); break;

        default:
        allOff();
        break;
      }
    }

    lastHallTime=now;

    }

  }

  }
}
