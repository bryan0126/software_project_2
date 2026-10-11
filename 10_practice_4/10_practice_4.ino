#include <Servo.h>
#include <math.h>

#define PIN_SERVO 10
#define PIN_TRIG 12
#define PIN_ECHO 13

const int CLOSED_ANGLE = 0; // 서보암 장착 방향에 맞춰 조절
const int OPEN_ANGLE = 120;
const unsigned long MOVING_TIME = 1500; // ms, 두 함수의 이동 시간 동일
const unsigned long SAMPLE_INTERVAL = 60;
const unsigned long SERVO_INTERVAL = 20;
const unsigned long CLOSE_DELAY = 1000;
const float DETECT_DISTANCE = 200.0f; // mm
const float RELEASE_DISTANCE = 250.0f;

// 시리얼 모니터 57600 baud: 1 = Sigmoid, 2 = Smoothstep.
// 이동 중 함수 변경은 다음 이동부터 적용된다.
// 같은 물체를 접근/제거하여 두 함수의 실제 동작을 비교한다.
// 플로터에는 동일 경과 시간의 두 예상 각도와 선택된 명령 각도를 출력.
// actual은 측정 각도가 아니라 서보에 보낸 명령 각도이다.
Servo myServo;
int selectedMode = 1;
int movingMode = 1;
float currentAngle = CLOSED_ANGLE;
float startAngle = CLOSED_ANGLE;
float stopAngle = CLOSED_ANGLE;
bool moving = false;
bool gateOpen = false;
bool carPresent = false;
unsigned long moveStartTime = 0;
unsigned long lastSamplingTime = 0;
unsigned long lastServoTime = 0;
unsigned long lastPresenceTime = 0;

float sigmoid(float t) {
  // Logistic 함수의 [-6, 6] 구간을 0~1로 정규화한다.
  // Smoothstep보다 중앙 구간에서 빠르게 움직인다.
  const float low = 1.0f / (1.0f + exp(6.0f));
  const float high = 1.0f / (1.0f + exp(-6.0f));
  float value = 1.0f / (1.0f + exp(-12.0f * (t - 0.5f)));
  return (value - low) / (high - low);
}

float smoothstep(float t) {
  // 양 끝에서 속도가 0인 3차 다항식. 지수 함수 연산이 필요 없다.
  return t * t * (3.0f - 2.0f * t);
}

float USS_measure() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  // 기존 초음파 실습의 음속 346m/s 사용. 최대 12ms 대기, 반환 단위 mm.
  return pulseIn(PIN_ECHO, HIGH, 12000UL) * (0.001f * 0.5f * 346.0f);
}

void startMove(bool open, unsigned long now) {
  gateOpen = open;
  startAngle = currentAngle; // 닫히던 중 재감지해도 각도가 튀지 않도록 함
  stopAngle = open ? OPEN_ANGLE : CLOSED_ANGLE;
  movingMode = selectedMode;
  moveStartTime = now;
  moving = true;
}

void updateServo(unsigned long now) {
  if (!moving || now - lastServoTime < SERVO_INTERVAL) return;
  lastServoTime = now;
  unsigned long elapsed = now - moveStartTime;
  float t = elapsed >= MOVING_TIME ? 1.0f : (float)elapsed / MOVING_TIME;
  float sigmoidAngle = startAngle + (stopAngle - startAngle) * sigmoid(t);
  float smoothstepAngle = startAngle + (stopAngle - startAngle) * smoothstep(t);
  currentAngle = movingMode == 1 ? sigmoidAngle : smoothstepAngle;
  if (elapsed >= MOVING_TIME) {
    currentAngle = stopAngle;
    moving = false;
  }
  myServo.write((int)(currentAngle + 0.5f));
  Serial.print("sigmoid:"); Serial.print(sigmoidAngle);
  Serial.print(",smoothstep:"); Serial.print(smoothstepAngle);
  Serial.print(",actual:"); Serial.println(currentAngle);
}

void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  Serial.begin(57600);
  myServo.attach(PIN_SERVO);
  myServo.write(CLOSED_ANGLE);
}

void loop() {
  while (Serial.available() > 0) {
    char command = Serial.read();
    if (command == '1' || command == '2') selectedMode = command - '0';
  }
  unsigned long now = millis();
  updateServo(now);
  if (now - lastSamplingTime >= SAMPLE_INTERVAL) {
    lastSamplingTime = now;
    float distance = USS_measure();
    now = millis(); // pulseIn 대기 시간 반영
    // 무응답(0)은 미감지 처리. 1초 지연으로 짧은 감지 누락을 허용한다.
    if (distance > 0.0f && distance <= DETECT_DISTANCE) {
      carPresent = true;
    } else if (distance == 0.0f || distance >= RELEASE_DISTANCE) {
      carPresent = false;
    }
    // 200~250mm 사이에서는 이전 상태를 유지하여 경계에서 떨림 방지.
    if (carPresent) {
      lastPresenceTime = now;
      if (!gateOpen) startMove(true, now);
    }
  }
  // 완전히 열린 뒤 미감지 상태가 1초 지속되면 닫는다.
  // 닫는 중 차량이 재감지되면 현재 각도에서 다시 연다.
  if (gateOpen && !moving && !carPresent && now - lastPresenceTime >= CLOSE_DELAY) {
    startMove(false, now);
  }
}
