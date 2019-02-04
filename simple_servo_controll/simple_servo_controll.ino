#include<Servo.h>

#define cw HIGH
#define ccw LOW
#define OPEN 1
#define CLOSE 0

Servo servo1;
int pos = 0;

const int motorIn1 = 6;
const int motorIn2 = 7;
const int motorIn3 = 8;
const int motorIn4 = 9;

const int motorIn5 = 10;
const int motorIn6 = 11;
const int motorIn7 = 12;
const int motorIn8 = 13;

const int stprA_step = 50;
const int stprA_dir = 51;

const int stprB_step = 48;
const int stprB_dir = 49;


void stpr_rotate(int stpr_dir, int stpr_step, int dir, int steps) {
//two stepper motor together
  digitalWrite(stpr_dir, dir);
  digitalWrite(stpr_dir - 2, dir);
  for (int i = 0; i < steps; i++) {
    digitalWrite(stpr_step, HIGH);
    digitalWrite(stpr_step - 2, HIGH);
    delayMicroseconds(100);
    digitalWrite(stpr_step, LOW);
    digitalWrite(stpr_step - 2, LOW);
    delayMicroseconds(100);
  }
}

void gate_servo(int action) {
  if (action == OPEN)
    for (pos = 0; pos <= 90; pos += 1) {
      servo1.write(pos);
      delay(15);
    }
  if (action == CLOSE)
    for (pos = 90; pos >= 0; pos -= 1) {
      servo1.write(pos);
      delay(15);
    }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(motorIn1, OUTPUT);
  pinMode(motorIn2, OUTPUT);
  pinMode(motorIn3, OUTPUT);
  pinMode(motorIn4, OUTPUT);

  pinMode(motorIn5, OUTPUT);
  pinMode(motorIn6, OUTPUT);
  pinMode(motorIn7, OUTPUT);
  pinMode(motorIn8, OUTPUT);

  pinMode(stprA_step, OUTPUT);
  pinMode(stprA_dir,  OUTPUT);

  pinMode(stprB_step, OUTPUT);
  pinMode(stprB_dir,  OUTPUT);
  servo1.attach(3);
}

void loop() {

  for (int i = 0; i < 256; i++) {

    analogWrite(motorIn3, 200);
    analogWrite(motorIn4, 0);

    analogWrite(motorIn1, 200);
    analogWrite(motorIn2, 0);

    analogWrite(motorIn5, 200);
    analogWrite(motorIn6, 0);

    analogWrite(motorIn7, 200);
    analogWrite(motorIn8, 0);

    delay(10);
  }

  stpr_rotate(stprA_dir, stprA_step, cw, 20000);
  stpr_rotate(stprA_dir, stprA_step, ccw, 20000);

  gate_servo(OPEN);

}
