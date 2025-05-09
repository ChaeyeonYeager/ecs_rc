#include <Arduino.h>
#include "PinChangeInterrupt.h"

// — 수신기 채널 입력 핀 정의 —
// A0: CH3 → 모드 전환 스위치
// A1: CH2 → 밝기 조절 스틱
const int pinModeSwitch   = A0;
const int pinBrightness   = A1;

// — PWM 신호 측정 변수 —
// modePulseWidth: CH3 펄스 폭 (μs 단위)
// brightPulseWidth: CH2 펄스 폭 (μs 단위)
volatile int           modePulseWidth    = 1500;
volatile unsigned long modeStartMicros   = 0;
volatile bool          newModePulse      = false;

volatile int           brightPulseWidth  = 1500;
volatile unsigned long brightStartMicros = 0;
volatile bool          newBrightPulse    = false;

// — RGB LED 핀 정의 (common-cathode) —
// R, G, B 각각 아노드에 220Ω 저항 후 연결
const int pinR = 9;
const int pinG = 10;
const int pinB = 11;

// — RGB 순환 모드 변수 —
unsigned long lastCycleMs   = 0;
int          cycleIndex     = 0;                // 0: R, 1: G, 2: B
const unsigned long cycleInterval = 1000;       // 1초 간격

void setup() {
  // 수신신호용 핀 설정
  pinMode(pinModeSwitch, INPUT_PULLUP);
  pinMode(pinBrightness, INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(pinModeSwitch),   isrModeSwitch,   CHANGE);
  attachPCINT(digitalPinToPCINT(pinBrightness),   isrBrightness,   CHANGE);

  // RGB LED 핀 출력 설정
  pinMode(pinR, OUTPUT);
  pinMode(pinG, OUTPUT);
  pinMode(pinB, OUTPUT);

  Serial.begin(9600);
}

// CH3 (모드 스위치) ISR
void isrModeSwitch() {
  if (digitalRead(pinModeSwitch) == HIGH) {
    modeStartMicros = micros();
  } else if (modeStartMicros && !newModePulse) {
    modePulseWidth = micros() - modeStartMicros;
    modeStartMicros = 0;
    newModePulse   = true;
  }
}

// CH2 (밝기 조절) ISR
void isrBrightness() {
  if (digitalRead(pinBrightness) == HIGH) {
    brightStartMicros = micros();
  } else if (brightStartMicros && !newBrightPulse) {
    brightPulseWidth = micros() - brightStartMicros;
    brightStartMicros = 0;
    newBrightPulse   = true;
  }
}

void loop() {
  // 최근 측정된 펄스 폭 읽기
  int pwmMode   = modePulseWidth;    // CH3
  int pwmBright = brightPulseWidth;  // CH2

  // CH2(1000~2000μs) → 밝기(0~255)
  int brightness = map(pwmBright, 1000, 2000, 0, 255);
  brightness = constrain(brightness, 0, 255);

  // 모드 분기
  if (pwmMode < 1200) {
    // 모드0: LED 완전 OFF
    analogWrite(pinR, 0);
    analogWrite(pinG, 0);
    analogWrite(pinB, 0);

  } else if (pwmMode < 1700) {
    // 모드1: 흰색 모드 (세 채널 동일 밝기)
    analogWrite(pinR, brightness);
    analogWrite(pinG, brightness);
    analogWrite(pinB, brightness);

  } else {
    // 모드2: RGB 순환 모드
    unsigned long now = millis();
    if (now - lastCycleMs > cycleInterval) {
      lastCycleMs = now;
      cycleIndex = (cycleIndex + 1) % 3;
    }
    // 현재 색상만 밝기 조절, 나머지 채널은 0
    analogWrite(pinR, (cycleIndex == 0) ? brightness : 0);
    analogWrite(pinG, (cycleIndex == 1) ? brightness : 0);
    analogWrite(pinB, (cycleIndex == 2) ? brightness : 0);
  }

  // 디버그: 펄스 폭 변화 시 시리얼 출력
  if (newBrightPulse) {
    Serial.print("CH2 PWM: "); Serial.println(pwmBright);
    newBrightPulse = false;
  }
  if (newModePulse) {
    Serial.print("CH3 PWM: "); Serial.println(pwmMode);
    newModePulse = false;
  }
}
