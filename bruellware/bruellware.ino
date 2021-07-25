#define MIN_VOLTAGE       10.8
#define E2_VOLTAGE        11.2
#define E3_VOLTAGE        11.6
#define E4_VOLTAGE        12.2
#define E5_VOLTAGE        12.6
#define MAX_VOLTAGE       14.1

#define CHARGE_CURRENT    0.21
#define MIN_CURRENT       0.01
#define CURRENT_MARGIN    0.03

#define PIN_AC_INPUT      2
#define PIN_POWER_SWITCH  8

#define PIN_RELAIS_AMP    4
#define PIN_SYS_POWER_ON  7
#define PIN_PWM_BH        3
#define PIN_PWM_BL        5

#define PIN_CURR_BH       A5
#define PIN_VOLT_BH       A4
#define PIN_CURR_BL       A0
#define PIN_VOLT_BL       A1

#define PIN_LED_R         9
#define PIN_LED_G         10
#define PIN_LED_B         11
#define PIN_LED_Y         12


#define MEAS_AVG_COUNT    5

#define VOLT_CAL_BH       (12.67/541)
#define VOLT_CAL_BL       (12.98/530)

#define AMP_CAL_BH        ((0.218-0.178)/(61-22))
#define AMP_CAL_BL        ((0.205-0.076)/(187-64))


void setup() {
  uint8_t i = 0;

  pinMode(PIN_SYS_POWER_ON, OUTPUT);
  digitalWrite(PIN_SYS_POWER_ON, HIGH);

  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_RELAIS_AMP, OUTPUT);
  pinMode(PIN_POWER_SWITCH, INPUT_PULLUP);

  analogReference(INTERNAL);
  analogWrite(PIN_PWM_BH, 0);
  analogWrite(PIN_PWM_BL, 0);

  for(i=0; i<50; i++) {
    analogRead(PIN_VOLT_BL);
    analogRead(PIN_VOLT_BH);
    analogRead(PIN_CURR_BH);
    analogRead(PIN_CURR_BH);
  }
}

uint16_t i = 0;
uint8_t j = 0;

uint16_t noSupplyCounter = 0;
uint16_t supplyCounter = 0;

uint8_t bhCurrSp = 0;
uint8_t blCurrSp = 0;

float analogReadAverage(uint8_t pin) {
  float res = 0;
  uint8_t i = 0;
  for(i=0; i<MEAS_AVG_COUNT; i++) {
    res += analogRead(pin);
  }
  return res/MEAS_AVG_COUNT;
}

float measureVoltH() {
  return analogReadAverage(PIN_VOLT_BH)*VOLT_CAL_BH
      - measureCurrH() * 0.1;
}

float measureVoltL() {
  return analogReadAverage(PIN_VOLT_BL)*VOLT_CAL_BL
      - measureCurrL() * 0.1;
}

float measureCurrH() {
  return analogReadAverage(PIN_CURR_BH)*AMP_CAL_BH;
}

float measureCurrL() {
  return analogReadAverage(PIN_CURR_BL)*AMP_CAL_BL;
}

void loop() {
  // If neither AC power nor power switch active, turn device off
  if(!digitalRead(PIN_AC_INPUT) && digitalRead(PIN_POWER_SWITCH)) {
    digitalWrite(PIN_SYS_POWER_ON, LOW);
    delay(5000);
  }

  // If power switch is turned off, turn amp off, else turn it on
  if(digitalRead(PIN_POWER_SWITCH)) {
    digitalWrite(PIN_RELAIS_AMP, LOW);
  }
  else {
    digitalWrite(PIN_RELAIS_AMP, HIGH);
  }

  // charging or...
  if(digitalRead(PIN_AC_INPUT)) {

    // update counters
    noSupplyCounter = 0;
    if(supplyCounter < 10000) {
      supplyCounter++;
    }

    if(measureVoltH() < MAX_VOLTAGE && measureCurrH() < CHARGE_CURRENT) {
      if(bhCurrSp < 255) {
        bhCurrSp += 1;
      }
    }
    else {
      if(bhCurrSp > 0) {
        bhCurrSp -= 1;
      }
    }

    if(measureVoltL() < MAX_VOLTAGE && measureCurrL() < CHARGE_CURRENT) {
      if(blCurrSp < 255) {
        blCurrSp += 1;
      }
    }
    else {
      if(blCurrSp > 0) {
        blCurrSp -= 1;
      }
    }

    if(measureCurrH() < MIN_CURRENT + CURRENT_MARGIN
      && measureCurrL() < MIN_CURRENT + CURRENT_MARGIN)
    {
      // L3 erhaltungsladung
      analogWrite(PIN_LED_R, 0);
      analogWrite(PIN_LED_G, 255);
      analogWrite(PIN_LED_B, 0);
      analogWrite(PIN_LED_Y, 0);
    }
    else if (measureCurrH() < CHARGE_CURRENT - CURRENT_MARGIN
      && measureCurrL() < CHARGE_CURRENT - CURRENT_MARGIN)
    {
      // L2 Konstantspannungsladung
      analogWrite(PIN_LED_R, 0);
      analogWrite(PIN_LED_G, 0);
      analogWrite(PIN_LED_B, 255);
      analogWrite(PIN_LED_Y, 0);
    }
    else {
      // L1 Konstantstromladung
      analogWrite(PIN_LED_R, 0);
      analogWrite(PIN_LED_G, 0);
      analogWrite(PIN_LED_B, 255);
      analogWrite(PIN_LED_Y, 255);
    }
  }
  // ..discharging operation.
  else {
    blCurrSp = 0;
    bhCurrSp = 0;

    // If AC/DC was connected and now disconnected, force shutdown
    if(supplyCounter > 200) {
      digitalWrite(PIN_SYS_POWER_ON, LOW);
      delay(5000);
    }

    // update counters
    supplyCounter = 0;
    if(noSupplyCounter < 10000) {
      noSupplyCounter++;
    }

    if(measureVoltH() > E5_VOLTAGE && measureVoltL() > E5_VOLTAGE)
    {
      // E5
      analogWrite(PIN_LED_R, 0);
      analogWrite(PIN_LED_G, 255);
      analogWrite(PIN_LED_B, 0);
      analogWrite(PIN_LED_Y, 0);
    }
    else if(measureVoltH() > E4_VOLTAGE && measureVoltL() > E4_VOLTAGE)
    {
      // E4
      analogWrite(PIN_LED_R, 0);
      analogWrite(PIN_LED_G, 255);
      analogWrite(PIN_LED_B, 0);
      analogWrite(PIN_LED_Y, 255);
    }
    else if(measureVoltH() > E3_VOLTAGE && measureVoltL() > E3_VOLTAGE)
    {
      // E3
      analogWrite(PIN_LED_R, 0);
      analogWrite(PIN_LED_G, 0);
      analogWrite(PIN_LED_B, 0);
      analogWrite(PIN_LED_Y, 255);
    }
    else if(measureVoltH() > E2_VOLTAGE && measureVoltL() > E2_VOLTAGE)
    {
      // E2
      analogWrite(PIN_LED_R, 255);
      analogWrite(PIN_LED_G, 0);
      analogWrite(PIN_LED_B, 0);
      analogWrite(PIN_LED_Y, 255);
    }
    else if(measureVoltH() > MIN_VOLTAGE && measureVoltL() > MIN_VOLTAGE)
    {
      // E1
      analogWrite(PIN_LED_R, 255);
      analogWrite(PIN_LED_G, 0);
      analogWrite(PIN_LED_B, 0);
      analogWrite(PIN_LED_Y, 0);
    }

    // only shut down if no AC/DC was present within the last 10s
    else if(noSupplyCounter > 2000) {
      analogWrite(PIN_LED_R, 0);
      analogWrite(PIN_LED_G, 0);
      analogWrite(PIN_LED_B, 0);

      for(j=0; j<3; j++) {
        delay(300);
        analogWrite(PIN_LED_R, 255);
        delay(300);
        analogWrite(PIN_LED_R, 0);
      }

      digitalWrite(PIN_SYS_POWER_ON, LOW);
      delay(5000);
    }
  }

  analogWrite(PIN_PWM_BH, bhCurrSp);
  analogWrite(PIN_PWM_BL, blCurrSp);

  if(i % 100 == 0) {
    printAll();
  }

  delay(5);
  i++;
}


void printAll() {
  Serial.print("--------------------------------\n");
  Serial.print("AC in: ");
  Serial.print(digitalRead(PIN_AC_INPUT));
  Serial.print("\nPower on: ");
  Serial.print(digitalRead(PIN_POWER_SWITCH));

  Serial.print("\n\nCurrent (H): ");
  Serial.print(measureCurrH());
  Serial.print("A (");
  Serial.print(analogRead(PIN_CURR_BH));
  Serial.print(")\nPWM Setpoint: ");
  Serial.print(bhCurrSp);
  Serial.print("\nVoltage (H): ");
  Serial.print(measureVoltH());
  Serial.print("V (");
  Serial.print(analogRead(PIN_VOLT_BH));

  Serial.print(")\n\nCurrent (L): ");
  Serial.print(measureCurrL());
  Serial.print("A (");
  Serial.print(analogRead(PIN_CURR_BL));
  Serial.print(")\nPWM Setpoint: ");
  Serial.print(blCurrSp);
  Serial.print("\nVoltage (L): ");
  Serial.print(measureVoltL());
  Serial.print("V (");
  Serial.print(analogRead(PIN_VOLT_BL));
  Serial.print(")\n");
}
