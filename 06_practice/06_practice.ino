int led = 7;

int brightness = 0;

int setperiod_value = 10000;
int period;

unsigned long startTime;


// setup
void setup() {

  pinMode(led, OUTPUT);

  set_period(setperiod_value);

  startTime = micros();
}


// loop
void loop() {

  unsigned long now = micros();

  // 1초 주기
  unsigned long elapsed =
      (now - startTime) % 1000000UL;


  // ----------------------------
  // 밝기 계산
  // ----------------------------

  if (elapsed < 500000UL) {

    // 0 → 100
    brightness =
        (elapsed * 100UL) / 500000UL;

  }
  else {

    // 100 → 0
    brightness =
        ((1000000UL - elapsed) * 100UL)
        / 500000UL;
  }


  // PWM 한 주기 출력
  pwm_output(brightness);
}


// --------------------------------
// PWM period 설정
// --------------------------------
void set_period(int p) {

  if (p < 100)
    p = 100;

  if (p > 10000)
    p = 10000;

  period = p;
}


// --------------------------------
// PWM 출력
// --------------------------------
void pwm_output(int duty) {

  if (duty <= 0) {

    digitalWrite(led, LOW);

    delayMicroseconds(period);

    return;
  }


  if (duty >= 100) {

    digitalWrite(led, HIGH);

    delayMicroseconds(period);

    return;
  }


  unsigned long highTime =
      ((unsigned long)period * duty) / 100;

  unsigned long lowTime =
      period - highTime;


  digitalWrite(led, HIGH);
  delayMicroseconds(highTime);

  digitalWrite(led, LOW);
  delayMicroseconds(lowTime);
}
