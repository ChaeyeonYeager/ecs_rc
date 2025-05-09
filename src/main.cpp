#include <Arduino.h>
#include "PinChangeInterrupt.h"

// — ISR 프로토타입 —
void isrPowerSwitch();
void isrColorControl();
void isrBrightness();

// — 수신기 채널 입력 핀 정의 —
// A2: CH5(AUX1) ← SwA 스위치 (전원 ON/OFF)
// A0: CH3 ← 스로틀 스틱 (색상 제어, 서서히 변화)
// A1: CH2 ← 엘리베이터 스틱 (밝기 조절)
const int pinPowerSwitch = A2;
const int pinColorCtrl   = A0;
const int pinBrightness  = A1;

// — PWM 신호 저장 변수 —
volatile int           powerPulseWidth   = 1500;
volatile unsigned long powerStartMicros  = 0;
volatile bool          newPowerPulse     = false;

volatile int           colorPulseWidth   = 1500;
volatile unsigned long colorStartMicros  = 0;
volatile bool          newColorPulse     = false;

volatile int           brightPulseWidth  = 1500;
volatile unsigned long brightStartMicros = 0;
volatile bool          newBrightPulse    = false;

// — RGB LED 핀 (common-cathode) —
const int pinR = 9;
const int pinG = 10;
const int pinB = 11;

void setup() {
  // SwA (CH5) ISR 등록 → 전원 ON/OFF
  pinMode(pinPowerSwitch, INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(pinPowerSwitch), isrPowerSwitch, CHANGE);

  // CH3 (색상 제어) ISR 등록 → hue 제어
  pinMode(pinColorCtrl, INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(pinColorCtrl), isrColorControl, CHANGE);

  // CH2 (밝기) ISR 등록
  pinMode(pinBrightness, INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(pinBrightness), isrBrightness, CHANGE);

  // RGB LED 핀 출력 설정
  pinMode(pinR, OUTPUT);
  pinMode(pinG, OUTPUT);
  pinMode(pinB, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  // ISR에서 갱신된 PWM 폭 읽기
  int pwmPower  = powerPulseWidth;   // SwA
  int pwmColor  = colorPulseWidth;   // CH3
  int pwmBright = brightPulseWidth;  // CH2

  // CH2 → 밝기(0~255)
  int brightness = map(pwmBright, 1000, 2000, 0, 255);
  brightness = constrain(brightness, 0, 255);

  // 전원 OFF
  if (pwmPower < 1500) {
    analogWrite(pinR, 0);
    analogWrite(pinG, 0);
    analogWrite(pinB, 0);
    return;
  }

  // CH3 → hue (0~255)
  int hue = map(pwmColor, 1000, 2000, 0, 255);
  hue = constrain(hue, 0, 255);

  // HSV → RGB 변환 (V=255)
  int r, g, b;
  if (hue < 85) {
    // 0..84 : red→green
    r = 255 - hue * 3;
    g = hue * 3;
    b = 0;
  } else if (hue < 170) {
    // 85..169 : green→blue
    int h2 = hue - 85;
    r = 0;
    g = 255 - h2 * 3;
    b = h2 * 3;
  } else {
    // 170..255 : blue→red
    int h2 = hue - 170;
    r = h2 * 3;
    g = 0;
    b = 255 - h2 * 3;
  }

  // 밝기(brightness) 적용
  r = (r * brightness) / 255;
  g = (g * brightness) / 255;
  b = (b * brightness) / 255;

  // LED 출력
  analogWrite(pinR, r);
  analogWrite(pinG, g);
  analogWrite(pinB, b);

  // (선택) 시리얼 디버그
  if (newPowerPulse) {
    Serial.print("Power PWM: ");  Serial.println(pwmPower);
    newPowerPulse = false;
  }
  if (newColorPulse) {
    Serial.print("Color PWM: ");  Serial.println(pwmColor);
    newColorPulse = false;
  }
  if (newBrightPulse) {
    Serial.print("Bright PWM: "); Serial.println(pwmBright);
    newBrightPulse = false;
  }
}

// — ISR 정의 —

// SwA (CH5) → 전원 제어
void isrPowerSwitch() {
  if (digitalRead(pinPowerSwitch) == HIGH) {
    powerStartMicros = micros();
  } else {
    powerPulseWidth   = micros() - powerStartMicros;
    powerStartMicros  = 0;
    newPowerPulse     = true;
  }
}

// CH3 → 색상 제어 (hue)
void isrColorControl() {
  if (digitalRead(pinColorCtrl) == HIGH) {
    colorStartMicros = micros();
  } else if (colorStartMicros && !newColorPulse) {
    colorPulseWidth  = micros() - colorStartMicros;
    colorStartMicros = 0;
    newColorPulse    = true;
  }
}

// CH2 → 밝기
void isrBrightness() {
  if (digitalRead(pinBrightness) == HIGH) {
    brightStartMicros = micros();
  } else if (brightStartMicros && !newBrightPulse) {
    brightPulseWidth  = micros() - brightStartMicros;
    brightStartMicros = 0;
    newBrightPulse    = true;
  }
} 