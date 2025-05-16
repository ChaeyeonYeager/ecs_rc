//202001929 김채연
#include <Arduino.h>
#include "PinChangeInterrupt.h"

// ────────────────────────────────────────────────────────────────────────────
// ISR(Interrupt Service Routine) 프로토타입 선언
//  각 수신기 채널 입력 변화 시 호출되어 펄스 폭을 측정합니다.
void isrPowerSwitch();
void isrColorControl();
void isrBrightness();

// ────────────────────────────────────────────────────────────────────────────
// 수신기 채널 입력 핀 정의 (아두이노 아날로그 핀 A0~A2를 PCINT로 사용)
const int pinPowerSwitch = A2;  // 전원 ON/OFF 채널
const int pinColorCtrl   = A0;  // 색상(hue) 조절 채널
const int pinBrightness  = A1;  // 밝기 조절 채널

// ────────────────────────────────────────────────────────────────────────────
// PWM 신호 저장 변수들
volatile int           powerPulseWidth   = 1500; // 전원 채널 펄스 폭(µs), 기본 1500
volatile unsigned long powerStartMicros  = 0;    // 전원 채널 HIGH 시작 시간
volatile bool          newPowerPulse     = false;// 새로운 값 기록 플래그

volatile int           colorPulseWidth   = 1500; // 색상 채널 펄스 폭
volatile unsigned long colorStartMicros  = 0;    // 색상 채널 HIGH 시작 시간
volatile bool          newColorPulse     = false;// 새로운 값 기록 플래그

volatile int           brightPulseWidth  = 1500; // 밝기 채널 펄스 폭
volatile unsigned long brightStartMicros = 0;    // 밝기 채널 HIGH 시작 시간
volatile bool          newBrightPulse    = false;// 새로운 값 기록 플래그

// ────────────────────────────────────────────────────────────────────────────
// 출력 핀 정의
const int pinR         = 9;   // RGB 빨강 채널 (PWM 지원)
const int pinG         = 10;  // RGB 초록 채널 (PWM 지원)
const int pinB         = 11;  // RGB 파랑 채널 (PWM 지원)
const int pinBrightLED = 6;   // 단색 밝기 LED (PWM 지원)
const int pinPowerLED  = 13;  // 전원 상태 표시 LED (디지털)

// ────────────────────────────────────────────────────────────────────────────
void setup() {
  // ★ 수신기 채널 입력 설정 및 PCINT(핀 변경 인터럽트) 연결 ★
  pinMode(pinPowerSwitch, INPUT_PULLUP);
  // A2 핀 상태 변화시 isrPowerSwitch() 호출
  attachPCINT(digitalPinToPCINT(pinPowerSwitch), isrPowerSwitch, CHANGE);

  pinMode(pinColorCtrl, INPUT_PULLUP);
  // A0 핀 상태 변화시 isrColorControl() 호출
  attachPCINT(digitalPinToPCINT(pinColorCtrl), isrColorControl, CHANGE);

  pinMode(pinBrightness, INPUT_PULLUP);
  // A1 핀 상태 변화시 isrBrightness() 호출
  attachPCINT(digitalPinToPCINT(pinBrightness), isrBrightness, CHANGE);

  // ★ LED 출력 핀 모드 설정 ★
  pinMode(pinR, OUTPUT);
  pinMode(pinG, OUTPUT);
  pinMode(pinB, OUTPUT);
  pinMode(pinBrightLED, OUTPUT);
  pinMode(pinPowerLED, OUTPUT);

  // 시리얼 통신 시작 (디버깅용)
  Serial.begin(9600);
}

void loop() {
  // ── 1) PWM 신호 값을 로컬 변수로 복사 ──────────────────────────────────
  int pwmPower  = powerPulseWidth;   // 전원 스위치 채널 PWM 값
  int pwmColor  = colorPulseWidth;   // 색상 조절 채널 PWM 값
  int pwmBright = brightPulseWidth;  // 밝기 조절 채널 PWM 값

  // ── 2) 밝기 값 매핑: 리모컨 PWM(1000~2000µs) → 0~255 범위로 변환 ──────
  int brightness = map(pwmBright, 1000, 2000, 0, 255);
  brightness = constrain(brightness, 0, 255); // 단색 밝기 LED에만 사용

  // ── 3) 색상 값 매핑: hue 값으로 변환 (0~255) ───────────────────────────
  int hue = map(pwmColor, 1000, 2000, 0, 255);
  hue = constrain(hue, 0, 255); // RGB 색상에만 사용

  // ── 4) HSV → RGB 변환 (단순 3구간 분할 방식) ───────────────────────────
  int r = 0, g = 0, b = 0;
  if (hue < 85) {
    r = 255 - hue * 3;
    g = hue * 3;
    b = 0;
  } else if (hue < 170) {
    int h2 = hue - 85;
    r = 0;
    g = 255 - h2 * 3;
    b = h2 * 3;
  } else {
    int h2 = hue - 170;
    r = h2 * 3;
    g = 0;
    b = 255 - h2 * 3;
  }

  // ── 5) 전원 스위치: 전원 상태 LED만 제어 (13번 핀) ──────────────────────
  if (pwmPower < 1500) {
    digitalWrite(pinPowerLED, LOW); // 전원 OFF 표시
  } else {
    digitalWrite(pinPowerLED, HIGH); // 전원 ON 표시
  }

  // ── 6) 색상 조절 스틱 → RGB LED 제어 (밝기 적용 없이 색상만) ────────────
  analogWrite(pinR, r); // 빨강 채널
  analogWrite(pinG, g); // 초록 채널
  analogWrite(pinB, b); // 파랑 채널

  // ── 7) 밝기 조절 스틱 → 단색 LED 제어 (6번 핀) ─────────────────────────
  analogWrite(pinBrightLED, brightness); // 밝기 반영

  // ── 8) 디버깅 출력: 새로운 값 들어왔을 때만 한 번 출력 ─────────────────
  if (newPowerPulse) {
    Serial.print("Power PWM: ");
    Serial.println(pwmPower);
    newPowerPulse = false;
  }

  if (newColorPulse) {
    Serial.print("Color PWM: ");
    Serial.println(pwmColor);
    newColorPulse = false;
  }

  if (newBrightPulse) {
    Serial.print("Bright PWM: ");
    Serial.println(pwmBright);
    newBrightPulse = false;
  }
}

// ────────────────────────────────────────────────────────────────────────────
// ISR 정의: PCINT 인터럽트 발생 시 호출되어 'HIGH→LOW' 구간의 길이(펄스 폭)를 측정

// 전원 스위치 채널 ISR
void isrPowerSwitch() {
  if (digitalRead(pinPowerSwitch) == HIGH) {
    // HIGH 엣지: 펄스 시작 지점 저장
    powerStartMicros = micros();
  } else {
    // LOW 엣지: 현재 시간 - 시작 시간 = 펄스 폭 계산
    powerPulseWidth = micros() - powerStartMicros;
    newPowerPulse = true;  // loop()에서 처리할 수 있도록 플래그 설정
  }
}

// 색상 조절 채널 ISR
void isrColorControl() {
  if (digitalRead(pinColorCtrl) == HIGH) {
    colorStartMicros = micros();
  } else if (colorStartMicros) {
    colorPulseWidth = micros() - colorStartMicros;
    newColorPulse = true;
  }
}

// 밝기 조절 채널 ISR
void isrBrightness() {
  if (digitalRead(pinBrightness) == HIGH) {
    brightStartMicros = micros();
  } else if (brightStartMicros) {
    brightPulseWidth = micros() - brightStartMicros;
    newBrightPulse = true;
  }
}
