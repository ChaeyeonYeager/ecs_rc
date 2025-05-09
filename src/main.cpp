#include <Arduino.h>
#include "PinChangeInterrupt.h"

// -- 수신기 채널 입력 핀 정의 --
const int pinRC3 = A0;  // CH3 입력 (모드 스위치)
const int pinRC2 = A1;  // CH2 입력 (밝기 조절)

// -- 신호 측정용 변수 --
volatile int           nRC3PulseWidth    = 1500;
volatile unsigned long ulRC3StartHigh    = 0;
volatile bool          bNewRC3Pulse      = false;

volatile int           nRC2PulseWidth    = 1500;
volatile unsigned long ulRC2StartHigh    = 0;
volatile bool          bNewRC2Pulse      = false;

// -- LED 출력 핀 (PWM) --
const int redPin   = 9;
const int greenPin = 10;
const int bluePin  = 11;

// -- 순환 모드 관련 변수 --
unsigned long lastCycleTime = 0;
int          cycleIndex     = 0;
const unsigned long cycleInterval = 1000; // 1초

void setup() {
  // 수신신호용
  pinMode(pinRC3, INPUT_PULLUP);
  pinMode(pinRC2, INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(pinRC3), pwmRC3, CHANGE);
  attachPCINT(digitalPinToPCINT(pinRC2), pwmRC2, CHANGE);

  // LED 출력용
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  Serial.begin(9600);
}

// CH3(PWM) ISR
void pwmRC3() {
  if (digitalRead(pinRC3) == HIGH) {
    ulRC3StartHigh = micros();
  } else {
    if (ulRC3StartHigh && !bNewRC3Pulse) {
      nRC3PulseWidth = micros() - ulRC3StartHigh;
      ulRC3StartHigh = 0;
      bNewRC3Pulse = true;
    }
  }
}

// CH2(PWM) ISR
void pwmRC2() {
  if (digitalRead(pinRC2) == HIGH) {
    ulRC2StartHigh = micros();
  } else {
    if (ulRC2StartHigh && !bNewRC2Pulse) {
      nRC2PulseWidth = micros() - ulRC2StartHigh;
      ulRC2StartHigh = 0;
      bNewRC2Pulse = true;
    }
  }
}

void loop() {
  // 최신 신호값 읽기
  int ch3 = nRC3PulseWidth;  // 모드 스위치
  int ch2 = nRC2PulseWidth;  // 밝기 조절

  // CH2(1000~2000μs) → 밝기(0~255)
  int brightness = map(ch2, 1000, 2000, 0, 255);
  brightness = constrain(brightness, 0, 255);

  // 모드 분기
  if (ch3 < 1200) {
    // 모드0: LED OFF
    analogWrite(redPin,   0);
    analogWrite(greenPin, 0);
    analogWrite(bluePin,  0);

  } else if (ch3 < 1700) {
    // 모드1: 흰색(밝기 제어)
    analogWrite(redPin,   brightness);
    analogWrite(greenPin, brightness);
    analogWrite(bluePin,  brightness);

  } else {
    // 모드2: RGB 순환
    unsigned long now = millis();
    if (now - lastCycleTime > cycleInterval) {
      lastCycleTime = now;
      cycleIndex = (cycleIndex + 1) % 3;
    }
    switch (cycleIndex) {
      case 0:
        analogWrite(redPin,   brightness);
        analogWrite(greenPin, 0);
        analogWrite(bluePin,  0);
        break;
      case 1:
        analogWrite(redPin,   0);
        analogWrite(greenPin, brightness);
        analogWrite(bluePin,  0);
        break;
      case 2:
        analogWrite(redPin,   0);
        analogWrite(greenPin, 0);
        analogWrite(bluePin,  brightness);
        break;
    }
  }

  // 디버그용 시리얼 출력 (값이 바뀔 때만)
  if (bNewRC2Pulse) {
    Serial.print("CH2 PWM = "); Serial.println(ch2);
    bNewRC2Pulse = false;
  }
  if (bNewRC3Pulse) {
    Serial.print("CH3 PWM = "); Serial.println(ch3);
    bNewRC3Pulse = false;
  }
}
