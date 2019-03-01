
#include <math.h>
#include <PS2X_lib.h>
#include <Servo.h>

#define PI 3.14159265

#define OPEN 1
#define CLOSE 0

#define PS2_DAT        8   //14
#define PS2_CMD        9   //15
#define PS2_SEL        10  //16
#define PS2_CLK        11  //17

//************************** Pin Definition *******************************

const int motor_LF[3] = {51, 53, 4}; // (motor left  front) IN1 IN2 ENA
const int motor_LB[3] = {47, 49, 5}; // (motor left  back)  IN1 IN2 ENA
const int motor_RF[3] = {52, 50, 6}; // (motor right front) IN1 IN2 ENA
const int motor_RB[3] = {48, 46, 7}; // (motor right back)  IN1 IN2 ENA


const int stpr_state_out = 36;
const int servo_state_out = 38;

const int stpr_L_tweak_out[2] = {32, 33};
const int stpr_R_tweak_out[2] = {26, 27};

const int stpr_enable[2] = {22, 24};

//*********************** function declaration *****************************//
void ps2_setup();
void motorstop();
void motorrun(int, int, int); //type, power, yaw
void motorspin(int); ///???

float Polar_Angle(float, float);  //y, x
float Polar_Length(float, float);

int error = 0; // ps2 controller status
byte type = 0;
byte vibrate = 0;

bool stpr_enable_state = false;

PS2X ps2x;

//*********************** setup *****************************//
void setup() {

  Serial.begin(38400);

  ps2_setup();


  //*********************PIN MODE*************************

  // dc motors
  for (int i = 0; i < 3; i++) pinMode(motor_LF[i], OUTPUT);
  for (int i = 0; i < 3; i++) pinMode(motor_LB[i], OUTPUT);
  for (int i = 0; i < 3; i++) pinMode(motor_RF[i], OUTPUT);
  for (int i = 0; i < 3; i++) pinMode(motor_RB[i], OUTPUT);

  // Slave Signal
  
  pinMode(servo_state_out, OUTPUT);
  
  pinMode(stpr_state_out, OUTPUT);
  for (int i = 0; i < 2; i++) pinMode(stpr_enable[i], OUTPUT);
  for (int i = 0; i < 2; i++) pinMode(stpr_L_tweak_out[i], OUTPUT);
  for (int i = 0; i < 2; i++) pinMode(stpr_R_tweak_out[i], OUTPUT);

  // prevent random movemont
  motorstop();
  digitalWrite(stpr_state_out, LOW);
  digitalWrite(servo_state_out, LOW);
  for (int i = 0; i < 2; i++) digitalWrite(stpr_L_tweak_out[i], LOW);
  for (int i = 0; i < 2; i++) digitalWrite(stpr_R_tweak_out[i], LOW);
  for (int i = 0; i < 2; i++) digitalWrite(stpr_enable[i], LOW);
}

void loop() {


  bool motorstate = 0;

  float left_joystick_angle = 0;
  float left_joystick_length = 0;

  ps2x.read_gamepad(false, vibrate);
  delay(100);

  int ly = (int)ps2x.Analog(PSS_LY);
  int lx = (int)ps2x.Analog(PSS_LX);
  int rx =  (int)ps2x.Analog(PSS_RX); // rx control spin


  lx = (float)map(lx, 0, 255, -200, 200);
  ly = (float)map(ly, 0, 255, 200, -200);
  rx = (float)map(rx, 0, 255, -200, 200);

  lx == 0 ? lx = 1 : 1;
  ly == 0 ? ly = 1 : 1;
  rx == 1 ? rx = 0 : 1; // if rx == 1 then rx = 0, else do nothing

  if (ps2x.ButtonPressed(PSB_SQUARE)) return; /// if square is pressed means noise

  if (lx == 1 && ly == 1) {
    motorstate = 0;
  } else {
    motorstate = 1;
    if ( ps2x.ButtonPressed(PSB_SQUARE) ) motorstate = 0; // prevent random movement
    left_joystick_angle  = Polar_Angle(static_cast<float>(lx), static_cast<float>(ly));
    left_joystick_length = Polar_Length(static_cast<float>(lx), static_cast<float>(ly));
  }

  /// servo and stepper control signal
  if (ps2x.ButtonPressed(PSB_CIRCLE) ) {
    digitalWrite(servo_state_out, HIGH);
    delay(25);
    Serial.println("Circle just pressed");
  } else if (ps2x.ButtonPressed(PSB_TRIANGLE) ) {
    digitalWrite(stpr_state_out, HIGH);
    delay(25);
    Serial.println("Triangle just pressed");
  } else {
    digitalWrite(stpr_state_out, LOW);
    digitalWrite(servo_state_out, LOW);
    delay(25);
  }

  /// stepper enable
  if (ps2x.ButtonPressed(PSB_CROSS)) {
    if (stpr_enable_state) {
      stpr_enable_state = false;
      for (int i = 0; i < 2; i++) digitalWrite(stpr_enable[i], LOW);
    } else {
      stpr_enable_state = true; /// stops both stepper motor
      for (int i = 0; i < 2; i++) digitalWrite(stpr_enable[i], HIGH);
    }
  }

  /// stepper tweak
  if (ps2x.ButtonPressed(PSB_PAD_UP)) {
    digitalWrite(stpr_L_tweak_out[OPEN], HIGH);
    Serial.println("PSB_UP just pressed");
    delay(25);
  } else if (ps2x.ButtonPressed(PSB_PAD_DOWN)) {
    digitalWrite(stpr_L_tweak_out[CLOSE], HIGH);
    Serial.println("PSB_DOWN just pressed");
    delay(25);
  } else if (ps2x.ButtonPressed(PSB_PAD_LEFT)) {
    digitalWrite(stpr_R_tweak_out[CLOSE], HIGH);
    delay(25);
  } else if (ps2x.ButtonPressed(PSB_PAD_RIGHT)) {
    digitalWrite(stpr_R_tweak_out[OPEN], HIGH);
    delay(25);
  } else {
    for (int i = 0; i < 2; i++) digitalWrite(stpr_L_tweak_out[i], LOW);
    for (int i = 0; i < 2; i++) digitalWrite(stpr_R_tweak_out[i], LOW);
    delay(25);
  }

  /// spin motion
  if (rx > 10) {
    motorspin(200);
    delay(25);
    return;
  } else if (rx < -10) {
    motorspin(-200);
    delay(25);
    return;
  }

  /// forward backward right left
  if (motorstate == 0) {
    motorstop();
  } else {
    rx = map(rx, -200, 200, -25, 25);

    if (left_joystick_angle < 22.5 || left_joystick_angle >= 337.5) {

      motorrun(2, left_joystick_length < 250 ? left_joystick_length : 250, rx); // go right

    } else if (left_joystick_angle >= 67.5 && left_joystick_angle < 112.5) {

      motorrun(1, left_joystick_length < 250 ? left_joystick_length : 250, rx); // go straight

    } else if (left_joystick_angle >= 157.5 && left_joystick_angle < 202.5) {

      motorrun(0, left_joystick_length < 250 ? left_joystick_length : 250, rx); // go left

    } else if (left_joystick_angle >= 247.5 && left_joystick_angle < 292.5) {

      motorrun(3, left_joystick_length < 250 ? left_joystick_length : 250, rx); // go back

    } else if (left_joystick_angle >= 22.5 && left_joystick_angle < 67.5) {

      //motorrun(4, left_joystick_length < 200 ? left_joystick_length : 200, rx); // go rightfront

    } else if (left_joystick_angle >= 112.5 && left_joystick_angle < 157.5) {

      //motorrun(5, left_joystick_length < 200 ? left_joystick_length : 200, rx); // go leftfront

    } else if (left_joystick_angle >= 202.5 && left_joystick_angle < 247.5) {

      //motorrun(6, left_joystick_length < 200 ? left_joystick_length : 200, rx); // go leftback
    } else if (left_joystick_angle >= 292.5 && left_joystick_angle < 337.5) {

      //motorrun(7, left_joystick_length < 200 ? left_joystick_length : 200, rx); // go rightback
    }
    delay(50);
  }
}


//****************** function implementation ********************//

// helper functions
float Polar_Angle(float x, float y) {
  if (x > 0 && y > 0) return atan((float)y / (float)x) * 180 / PI;
  else if (x < 0) return atan((float)y / (float)x) * 180 / PI + 180;
  else return atan((float)y / (float)x) * 180 / PI + 360;
};
float Polar_Length(float x, float y) {
  return sqrt(x * x + y * y);
};

// dc motor controll
void motorstop() {
  for (int i = 0; i < 2; i++) digitalWrite(motor_RF[i], 0);
  for (int i = 0; i < 2; i++) digitalWrite(motor_RB[i], 0);
  for (int i = 0; i < 2; i++) digitalWrite(motor_LF[i], 0);
  for (int i = 0; i < 2; i++) digitalWrite(motor_LB[i], 0);
};

void motorrun(int type, int power , int yaw) {
  switch (type) {
    case 0:                                //right
      analogWrite(motor_LB[2], 200);
      analogWrite(motor_LF[2], 200);
      analogWrite(motor_RF[2], 200);
      analogWrite(motor_RB[2], 200);
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
      analogWrite(motor_LB[2], 200);
      analogWrite(motor_LF[2], 200);
      analogWrite(motor_RF[2], 200);
      analogWrite(motor_RB[2], 200);
      digitalWrite(motor_RF[0], 1);
      digitalWrite(motor_RF[1], 0);
      digitalWrite(motor_RB[0], 0);
      digitalWrite(motor_RB[1], 1);
      digitalWrite(motor_LF[0], 0);
      digitalWrite(motor_LF[1], 1);
      digitalWrite(motor_LB[0], 1);
      digitalWrite(motor_LB[1], 0);
      break;
    case 3:                                  //back
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
    case 5:                                  //leftfront
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




