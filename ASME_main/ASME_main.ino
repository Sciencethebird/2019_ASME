#include <math.h>
#include <PS2X_lib.h>
#include <Servo.h>

#define PI 3.14159265

#define STEP 0
#define DIR 1

#define CLOSE 0
#define OPEN 1

#define PS2_DAT        8  //14
#define PS2_CMD        9  //15
#define PS2_SEL        10  //16
#define PS2_CLK        11  //17

//************************** Pin Definition *******************************

  const int motor_LF[3] = {53, 51, 0}; // (motor left  front) IN1 IN2 ENA
  const int motor_LB[3] = {49, 47, 0}; // (motor left  back)  IN1 IN2 ENA
  const int motor_RF[3] = {52, 50, 0}; // (motor right front) IN1 IN2 ENA
  const int motor_RB[3] = {48, 46, 0}; // (motor right back)  IN1 IN2 ENA

/*
const int motor_RF[3] = {0, 0, 0}; // (motor left  front) IN1 IN2 ENA
const int motor_RB[3] = {0, 0, 0}; // (motor left  back)  IN1 IN2 ENA
const int motor_LF[3] = {52, 50, 0}; // (motor right front) IN1 IN2 ENA
const int motor_LB[3] = {48, 46, 0}; // (motor right back)  IN1 IN2 ENA
*/
const int stpr_L[2] = {44, 42};
const int stpr_R[2] = {40, 38};

//*********************** function declaration *****************************
void ps2_setup();
void motorstop();
void motorrun(int, int, int); //type, power, yaw
void motorspin(int); ///???

void stpr_rotate(const int stpr[2], int, int);
void servo_action(int); // action

float Polar_Angle(float, float);  //y, x
float Polar_Length(float, float);

int error = 0; // ps2 controller status
byte type = 0;
byte vibrate = 0;

int pos = 0; // servo and stepper motor status
bool gate_is_open = false;
bool bottom_is_open = false;

PS2X ps2x;
Servo servo_A;

void setup() {

  Serial.begin(38400);

  ps2_setup();


  //*********************PIN MODE*************************

  // dc motors
  for (int i = 0; i < 3; i++) pinMode(motor_LF[i], OUTPUT);
  for (int i = 0; i < 3; i++) pinMode(motor_LB[i], OUTPUT);
  for (int i = 0; i < 3; i++) pinMode(motor_RF[i], OUTPUT);
  for (int i = 0; i < 3; i++) pinMode(motor_RB[i], OUTPUT);

  // stepper motors
  for (int i = 0; i < 2; i++) pinMode(stpr_L[i], OUTPUT);
  for (int i = 0; i < 2; i++) pinMode(stpr_R[i], OUTPUT);

  servo_A.attach(3);

  motorstop();
}

void loop() {

  bool motorstate = 0; ///????

  float left_joystick_angle = 0;
  float left_joystick_length = 0;

  ps2x.read_gamepad(false, vibrate);  //read controller, **** args to be determined
  delay(200);

  int ly = (int)ps2x.Analog(PSS_LY);
  int lx = (int)ps2x.Analog(PSS_LX);
  int rx =  (int)ps2x.Analog(PSS_RX);


  lx = (float)map(lx, 0, 255, -200, 200);
  ly = (float)map(ly, 0, 255, 200, -200);
  rx = (float)map(rx, 0, 255, -200, 200);
  //Serial.println(rx);

  lx == 0 ? lx = 1 : 1;
  ly == 0 ? ly = 1 : 1;
  rx == 1 ? rx = 0 : 1;

  if (lx == 1 && ly == 1) {
    motorstate = 0;
  } else {
    motorstate = 1;
    left_joystick_angle  = Polar_Angle(static_cast<float>(lx), static_cast<float>(ly));
    left_joystick_length = Polar_Length(static_cast<float>(lx), static_cast<float>(ly));
    /*Serial.print("angle:");
      Serial.println(left_joystick_angle);
      Serial.print("Length:");
      Serial.println(left_joystick_length);*/

  }

  if (motorstate == 0) {
    motorspin(rx);
  } else {
    rx = map(rx, -200, 200, -25, 25);
    if (left_joystick_angle < 22.5 || left_joystick_angle >= 337.5) {
      motorstop();
      delay(5);
      motorrun(0, left_joystick_length < 200 ? left_joystick_length : 200, rx); //go right
    } else if (left_joystick_angle >= 67.5 && left_joystick_angle < 112.5) {
      motorstop();
      delay(5);
      motorrun(1, left_joystick_length < 200 ? left_joystick_length : 200, rx); //go straight
    } else if (left_joystick_angle >= 157.5 && left_joystick_angle < 202.5) {
      motorstop();
      delay(5);
      motorrun(2, left_joystick_length < 200 ? left_joystick_length : 200, rx); //go left
    } else if (left_joystick_angle >= 247.5 && left_joystick_angle < 292.5) {
      motorstop();
      delay(5);
      motorrun(3, left_joystick_length < 200 ? left_joystick_length : 200, rx); //go back
    } else if (left_joystick_angle >= 22.5 && left_joystick_angle < 67.5) {
      motorstop();
      delay(5);
      motorrun(4, left_joystick_length < 200 ? left_joystick_length : 200, rx); //go rightfront
    } else if (left_joystick_angle >= 112.5 && left_joystick_angle < 157.5) {
      motorstop();
      delay(5);
      motorrun(5, left_joystick_length < 200 ? left_joystick_length : 200, rx); //go leftfront
    } else if (left_joystick_angle >= 202.5 && left_joystick_angle < 247.5) {
      motorstop();
      delay(5);
      motorrun(6, left_joystick_length < 200 ? left_joystick_length : 200, rx); //go leftback
    } else if (left_joystick_angle >= 292.5 && left_joystick_angle < 337.5) {
      motorstop();
      delay(5);
      motorrun(7, left_joystick_length < 200 ? left_joystick_length : 200, rx); //go rightback
    }
  }

  /// servo
  if (ps2x.ButtonPressed(PSB_CIRCLE) && !ps2x.ButtonPressed(PSB_SQUARE)) {

    if (gate_is_open) {
      servo_action(CLOSE);
      gate_is_open = false;
    } else {
      servo_action(OPEN);
      gate_is_open = true;
    }
    Serial.println("Circle just pressed");
  }

  ///stepper motor
  if (ps2x.ButtonPressed(PSB_SQUARE) && !ps2x.ButtonPressed(PSB_CIRCLE)) {
    Serial.println("Square just pressed");
    stpr_action(CLOSE, 20000);
    /*
      if (bottom_is_open) {
      stpr_action(OPEN, 20000);
      bottom_is_open = false;
      } else {
      stpr_action(CLOSE, 20000);
      bottom_is_open = true;
      }*/
  }
  if (ps2x.ButtonPressed(PSB_TRIANGLE) && !ps2x.ButtonPressed(PSB_CIRCLE)) {

    Serial.println("Triangle just pressed");
    stpr_action(OPEN, 20000);
  }

}





//****************** function implementation ********************//

float Polar_Angle(float x, float y) {
  if (x > 0 && y > 0)
    return atan((float)y / (float)x) * 180 / PI;
  else if (x < 0)
    return atan((float)y / (float)x) * 180 / PI + 180;
  else
    return atan((float)y / (float)x) * 180 / PI + 360;
};
float Polar_Length(float x, float y) {
  return sqrt(x * x + y * y);
};

void motorstop() {
  for (int i = 0; i < 2; i++) digitalWrite(motor_RF[i], 0);
  for (int i = 0; i < 2; i++) digitalWrite(motor_RB[i], 0);
  for (int i = 0; i < 2; i++) digitalWrite(motor_LF[i], 0);
  for (int i = 0; i < 2; i++) digitalWrite(motor_LB[i], 0);
};

void motorrun(int type, int power , int yaw) {
  switch (type) {
    case 0:                                   //right
      analogWrite(motor_LB[2], power);
      analogWrite(motor_LF[2], power);
      analogWrite(motor_RF[2], power);
      analogWrite(motor_RB[2], power);
      digitalWrite(motor_RF[0], 0);
      digitalWrite(motor_RF[1], 1);
      digitalWrite(motor_RB[0], 1);
      digitalWrite(motor_RB[1], 0);
      digitalWrite(motor_LF[0], 1);
      digitalWrite(motor_LF[1], 0);
      digitalWrite(motor_LB[0], 0);
      digitalWrite(motor_LB[1], 1);
      break;
    case 1:                                 //front
      analogWrite(motor_LB[2], power);
      analogWrite(motor_LF[2], power);
      analogWrite(motor_RF[2], power);
      analogWrite(motor_RB[2], power);
      digitalWrite(motor_RF[0], 1);
      digitalWrite(motor_RF[1], 0);
      digitalWrite(motor_RB[0], 1);
      digitalWrite(motor_RB[1], 0);
      digitalWrite(motor_LF[0], 1);
      digitalWrite(motor_LF[1], 0);
      digitalWrite(motor_LB[0], 1);
      digitalWrite(motor_LB[1], 0);
      break;
    case 2:                                 //left
      analogWrite(motor_LB[2], power);
      analogWrite(motor_LF[2], power);
      analogWrite(motor_RF[2], power);
      analogWrite(motor_RB[2], power);
      digitalWrite(motor_RF[0], 1);
      digitalWrite(motor_RF[1], 0);
      digitalWrite(motor_RB[0], 0);
      digitalWrite(motor_RB[1], 1);
      digitalWrite(motor_LF[0], 0);
      digitalWrite(motor_LF[1], 1);
      digitalWrite(motor_LB[0], 1);
      digitalWrite(motor_LB[1], 0);
      break;
    case 3:                               //back
      analogWrite(motor_LB[2], power);
      analogWrite(motor_LF[2], power);
      analogWrite(motor_RF[2], power);
      analogWrite(motor_RB[2], power);
      digitalWrite(motor_RF[0], 0);
      digitalWrite(motor_RF[1], 1);
      digitalWrite(motor_RB[0], 0);
      digitalWrite(motor_RB[1], 1);
      digitalWrite(motor_LF[0], 0);
      digitalWrite(motor_LF[1], 1);
      digitalWrite(motor_LB[0], 0);
      digitalWrite(motor_LB[1], 1);
      break;
    case 4:                                 //rightfront
      digitalWrite(motor_RF[0], 0);
      digitalWrite(motor_RF[1], 1);
      digitalWrite(motor_RB[0], 1);
      digitalWrite(motor_RB[1], 0);
      digitalWrite(motor_LF[0], 1);
      digitalWrite(motor_LF[1], 0);
      digitalWrite(motor_LB[0], 0);
      digitalWrite(motor_LB[1], 1);
      analogWrite(motor_LB[2], 20);
      analogWrite(motor_LF[2], power);
      analogWrite(motor_RF[2], 20);
      analogWrite(motor_RB[2], power);
      break;
    case 5:                             //leftfront
      digitalWrite(motor_RF[0], 1);
      digitalWrite(motor_RF[1], 0);
      digitalWrite(motor_RB[0], 0);
      digitalWrite(motor_RB[1], 1);
      digitalWrite(motor_LF[0], 0);
      digitalWrite(motor_LF[1], 1);
      digitalWrite(motor_LB[0], 1);
      digitalWrite(motor_LB[1], 0);
      analogWrite(motor_LB[2], power);
      analogWrite(motor_LF[2], 20);
      analogWrite(motor_RF[2], power);
      analogWrite(motor_RB[2], 20);
      break;
    case 6:                           //leftback
      digitalWrite(motor_RF[0], 1);
      digitalWrite(motor_RF[1], 0);
      digitalWrite(motor_RB[0], 0);
      digitalWrite(motor_RB[1], 1);
      digitalWrite(motor_LF[0], 0);
      digitalWrite(motor_LF[1], 1);
      digitalWrite(motor_LB[0], 1);
      digitalWrite(motor_LB[1], 0);
      analogWrite(motor_LB[2], 20);
      analogWrite(motor_LF[2], power);
      analogWrite(motor_RF[2], 20);
      analogWrite(motor_RB[2], power);
      break;
    case 7:                         //rightback
      digitalWrite(motor_RF[0], 0);
      digitalWrite(motor_RF[1], 1);
      digitalWrite(motor_RB[0], 1);
      digitalWrite(motor_RB[1], 0);
      digitalWrite(motor_LF[0], 1);
      digitalWrite(motor_LF[1], 0);
      digitalWrite(motor_LB[0], 0);
      digitalWrite(motor_LB[1], 1);
      analogWrite(motor_LB[2], power);
      analogWrite(motor_LF[2], 20);
      analogWrite(motor_RF[2], power);
      analogWrite(motor_RB[2], 20);
      break;
  }
}

void motorspin(int yaw) {
  if (yaw == 0)
    motorstop();
  else if (yaw > 0) {
    digitalWrite(motor_RF[0], 0);
    digitalWrite(motor_RF[1], 1);
    digitalWrite(motor_RB[0], 0);
    digitalWrite(motor_RB[1], 1);
    digitalWrite(motor_LF[0], 1);
    digitalWrite(motor_LF[1], 0);
    digitalWrite(motor_LB[0], 1);
    digitalWrite(motor_LB[1], 0);
    analogWrite(motor_LB[2], yaw);
    analogWrite(motor_LF[2], yaw);
    analogWrite(motor_RF[2], yaw);
    analogWrite(motor_RB[2], yaw);
  }
  else {
    yaw = -yaw;
    digitalWrite(motor_RF[0], 1);
    digitalWrite(motor_RF[1], 0);
    digitalWrite(motor_RB[0], 1);
    digitalWrite(motor_RB[1], 0);
    digitalWrite(motor_LF[0], 0);
    digitalWrite(motor_LF[1], 1);
    digitalWrite(motor_LB[0], 0);
    digitalWrite(motor_LB[1], 1);
    analogWrite(motor_LB[2], yaw);
    analogWrite(motor_LF[2], yaw);
    analogWrite(motor_RF[2], yaw);
    analogWrite(motor_RB[2], yaw);
  }
}

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
  for (int i = 0; i < steps / 4; i++) {
    digitalWrite(stpr_L[STEP], HIGH);
    digitalWrite(stpr_R[STEP], HIGH);
    delayMicroseconds(100);
    digitalWrite(stpr_L[STEP], LOW);
    digitalWrite(stpr_R[STEP], LOW);
    delayMicroseconds(100);
  }
  ps2x.read_gamepad(false, vibrate);
  for (int i = 0; i < steps / 4; i++) {
    digitalWrite(stpr_L[STEP], HIGH);
    digitalWrite(stpr_R[STEP], HIGH);
    delayMicroseconds(100);
    digitalWrite(stpr_L[STEP], LOW);
    digitalWrite(stpr_R[STEP], LOW);
    delayMicroseconds(100);
  }
  ps2x.read_gamepad(false, vibrate);
  for (int i = 0; i < steps / 4; i++) {
    digitalWrite(stpr_L[STEP], HIGH);
    digitalWrite(stpr_R[STEP], HIGH);
    delayMicroseconds(100);
    digitalWrite(stpr_L[STEP], LOW);
    digitalWrite(stpr_R[STEP], LOW);
    delayMicroseconds(100);
  }
  ps2x.read_gamepad(false, vibrate);
  for (int i = 0; i < steps / 4; i++) {
    digitalWrite(stpr_L[STEP], HIGH);
    digitalWrite(stpr_R[STEP], HIGH);
    delayMicroseconds(100);
    digitalWrite(stpr_L[STEP], LOW);
    digitalWrite(stpr_R[STEP], LOW);
    delayMicroseconds(100);
  }
}


void ps2_setup() {

  delay(300);  //added delay to give wireless ps2 module some time to startup, before configuring it
  //setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
  //error = ps2x.config_gamepad(false, false);
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false);

  if (error == 0) {
    Serial.print("Found Controller, configured successful ");
    Serial.println("Try out all the buttons, X will vibrate the controller, faster as you press harder;");
    Serial.println("holding L1 or R1 will print out the analog stick values.");
    Serial.println("Note: Go to www.billporter.info for updates and to report bugs.");
  }
  else if (error == 1)
    Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");

  else if (error == 2)
    Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");

  else if (error == 3)
    Serial.println("Controller refusing to enter Pressures mode, may not support it. ");

  type = ps2x.readType();
  switch (type) {
    case 0:
      Serial.print("Unknown Controller type found ");
      break;
    case 1:
      Serial.print("DualShock Controller found ");
      break;
    case 2:
      Serial.print("GuitarHero Controller found ");
      break;
    case 3:
      Serial.print("Wireless Sony DualShock Controller found ");
      break;
  }
}


