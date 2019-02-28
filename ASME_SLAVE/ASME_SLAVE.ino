#include <math.h>
#include <Servo.h>

#define STEP 0
#define DIR 1

#define LEFT 0
#define RIGHT 1

#define CLOSE 0
#define OPEN 1

//************************** Pin Definition *******************************

const int stpr_L[2] = {4, 5};
const int stpr_R[2] = {6, 7};

const int stpr_state_in = 9;
const int servo_state_in = 10;

const int stpr_L_tweak_in[2] = {8, 11};
const int stpr_R_tweak_in[2] = {12, 13};

//*********************** function declaration *****************************

void servo_action(int action);
void stpr_action(int dir, int steps);
void stpr_tweak(int dir, int motor);


//*********************** parameters *****************************
int pos = 0; // servo and stepper motor status
bool gate_is_open = false;
bool bottom_is_open = false;
bool stpr_L_tweak[2] = {false, false};
bool stpr_R_tweak[2] = {false, false};
Servo servo_A;

void setup() {

  Serial.begin(38400);
  pinMode(stpr_state_in, INPUT);
  pinMode(servo_state_in, INPUT);

  for (int i = 0; i < 2; i++) pinMode(stpr_L[i], OUTPUT);
  for (int i = 0; i < 2; i++) pinMode(stpr_R[i], OUTPUT);
  
  for (int i = 0; i < 2; i++) pinMode(stpr_L_tweak_in[i], INPUT);
  for (int i = 0; i < 2; i++) pinMode(stpr_R_tweak_in[i], INPUT);

  //*********************PIN MODE*************************
  servo_A.attach(3);
  servo_A.write(0);
}


void loop() {

  int servo_state = digitalRead(servo_state_in);
  int stpr_state = digitalRead(stpr_state_in);

  int stpr_L_tweak_state[2] = {digitalRead(stpr_L_tweak_in[CLOSE]), digitalRead(stpr_L_tweak_in[OPEN]) };
  int stpr_R_tweak_state[2] = {digitalRead(stpr_R_tweak_in[CLOSE]), digitalRead(stpr_R_tweak_in[OPEN]) };

  //Serial.println(stpr_L_tweak_state[0]);
  //Serial.println(stpr_L_tweak_state[1]);

  /// stepper tweaking
  if (stpr_L_tweak_state[CLOSE]) {
    stpr_tweak(CLOSE, LEFT);
  } else if (stpr_L_tweak_state[OPEN]) {
    stpr_tweak(OPEN, LEFT);
  }

  if (stpr_R_tweak_state[CLOSE]) {
    stpr_tweak(CLOSE, RIGHT);
  } else if (stpr_R_tweak_state[OPEN]) {
    stpr_tweak(OPEN, RIGHT);
  }

  /// servo open/close
  if (servo_state == HIGH) {

    if (gate_is_open) {
      servo_action(CLOSE);
      gate_is_open = false;
    } else {
      servo_action(OPEN);
      gate_is_open = true;
    }
    //Serial.println("Circle just pressed");
    delay(50);
  }

  /// stepper motor

  if (stpr_state == HIGH) {

    if (bottom_is_open) {
      stpr_action(CLOSE, 21000);
      bottom_is_open = false;
    } else {
      stpr_action(OPEN, 21000);
      bottom_is_open = true;
    }
    //Serial.println("Triangle just pressed");
    delay(50);
  }
}



//****************** function implementation ********************//

void servo_action(int action) {
  if (action == OPEN)
    for (int pos = 0; pos <= 90; pos += 1) {
      servo_A.write(pos);
      delay(10);
    }
  if (action == CLOSE)
    for (int pos = 90; pos >= 0; pos -= 1) {
      servo_A.write(pos);
      delay(10);
    }
}

void stpr_action(int dir, int steps) {
  //two stepper motors move together
  digitalWrite(stpr_L[DIR], dir);
  digitalWrite(stpr_R[DIR], dir);

  for (int i = 0; i < steps; i++) {
    digitalWrite(stpr_L[STEP], HIGH);
    digitalWrite(stpr_R[STEP], HIGH);
    delayMicroseconds(100);
    digitalWrite(stpr_L[STEP], LOW);
    digitalWrite(stpr_R[STEP], LOW);
    delayMicroseconds(100);
  }
}

void stpr_tweak(int dir, int motor) {
  //two stepper motors move together
  digitalWrite(stpr_L[DIR], dir);
  digitalWrite(stpr_R[DIR], dir);

  if (motor == LEFT) {
    for (int i = 0; i < 100; i++) {
      digitalWrite(stpr_L[STEP], HIGH);
      delayMicroseconds(100);
      digitalWrite(stpr_L[STEP], LOW);
      delayMicroseconds(100);
    }
  } else if (motor == RIGHT) {
    for (int i = 0; i < 100; i++) {
      digitalWrite(stpr_R[STEP], HIGH);
      delayMicroseconds(100);
      digitalWrite(stpr_R[STEP], LOW);
      delayMicroseconds(100);
    }
  }
}

