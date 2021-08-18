// voltage and current thresholds

#define MIN_VOLTAGE       10.8
#define E2_VOLTAGE        11.2
#define E3_VOLTAGE        11.6
#define E4_VOLTAGE        12.2
#define E5_VOLTAGE        12.6
#define MAX_VOLTAGE       14.1

#define CHARGE_CURRENT    0.21
#define MIN_CURRENT       0.01
#define CURRENT_MARGIN    0.03


// led brightnesses

#define LED_R_BRIGHTNESS  255
#define LED_G_BRIGHTNESS  255
#define LED_B_BRIGHTNESS  255
#define LED_Y_BRIGHTNESS  255


// measurement configuration and calibration

#define MEAS_AVG_COUNT    5

#define VOLT_CAL_BH       (12.67/541)
#define VOLT_CAL_BL       (12.98/530)

#define AMP_CAL_BH        ((0.218-0.178)/(61-22))
#define AMP_CAL_BL        ((0.205-0.076)/(187-64))


// pin config

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


// makro to set leds

#define setLights(r, g, b, y) { \
  analogWrite(PIN_LED_R, LED_R_BRIGHTNESS * r); \
  analogWrite(PIN_LED_G, LED_G_BRIGHTNESS * g); \
  analogWrite(PIN_LED_B, LED_B_BRIGHTNESS * b); \
  analogWrite(PIN_LED_Y, LED_Y_BRIGHTNESS * y); \
}

// setup pins, communication and ADC
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

// mainloop counter
uint16_t i = 0;

// generic loop variable
uint8_t j = 0;

// increment with ~200 Hz when supply is (not) connected,
// stay at zero otherwise
uint16_t noSupplyCounter = 0;
uint16_t supplyCounter = 0;

// high/low side current source setpoints
uint8_t bhCurrSp = 0;
uint8_t blCurrSp = 0;

// read averaged value of analog input pin
float analogReadAverage(uint8_t pin) {
  float res = 0;
  uint8_t i = 0;
  for(i=0; i<MEAS_AVG_COUNT; i++) {
    res += analogRead(pin);
  }
  return res/MEAS_AVG_COUNT;
}

// return high side battery voltage
float measureVoltH() {
  return analogReadAverage(PIN_VOLT_BH)*VOLT_CAL_BH
      - measureCurrH() * 0.1;
}

// return low side battery voltage
float measureVoltL() {
  return analogReadAverage(PIN_VOLT_BL)*VOLT_CAL_BL
      - measureCurrL() * 0.1;
}

// return high side battery charge current (cannot measure discharging)
float measureCurrH() {
  return analogReadAverage(PIN_CURR_BH)*AMP_CAL_BH;
}

// return high side battery charge current (cannot measure discharging)
float measureCurrL() {
  return analogReadAverage(PIN_CURR_BL)*AMP_CAL_BL;
}


// mainloop
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
      setLights(0, 1, 0, 0);
    }
    else if (measureCurrH() < CHARGE_CURRENT - CURRENT_MARGIN
      && measureCurrL() < CHARGE_CURRENT - CURRENT_MARGIN)
    {
      // L2 Konstantspannungsladung
      setLights(0, 0, 1, 0);
    }
    else {
      // L1 Konstantstromladung
      setLights(0, 0, 1, 1);
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
      setLights(0, 1, 0, 0);
    }
    else if(measureVoltH() > E4_VOLTAGE && measureVoltL() > E4_VOLTAGE)
    {
      // E4
      setLights(0, 1, 0, 1);
    }
    else if(measureVoltH() > E3_VOLTAGE && measureVoltL() > E3_VOLTAGE)
    {
      // E3
      setLights(0, 0, 0, 1);
    }
    else if(measureVoltH() > E2_VOLTAGE && measureVoltL() > E2_VOLTAGE)
    {
      // E2
      setLights(1, 0, 0, 1);
    }
    else if(measureVoltH() > MIN_VOLTAGE && measureVoltL() > MIN_VOLTAGE)
    {
      // E1
      setLights(1, 0, 0, 0);
    }

    // only shut down if no AC/DC was present within the last 10s
    else if(noSupplyCounter > 2000) {
      setLights(0, 0, 0, 0);

      for(j=0; j<3; j++) {
        delay(300);
        setLights(1, 0, 0, 0);
        delay(300);
        setLights(0, 0, 0, 0);
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


// send debug information via serial
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
