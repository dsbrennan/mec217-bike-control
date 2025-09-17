/**
 *  Original Author: D S Brennan (github.com/dsbrennan)
 *  Created: 20/02/2025
 *
 *  Copyright 2025, MIT Licence
 **/
#include <Servo.h>

// pins
#define SYSTEM_READY_LED_PIN 5
#define CRANK_ACTIVITY_LED_PIN 4
#define MOTOR_ACTIVITY_LED_PIN 6
#define ESC_PIN 9

// delays
#define DELAY_UNIT 5000

// esc constants
const int esc_neutral_power = 45;
const int esc_forward_power = 180;
const int esc_reverse_power = 0;

// output variables
Servo esc;

/*
  System Setup
  ------------
*/
void setup() {
  // serial
  Serial.begin(9600);

  // status LEDs
  pinMode(SYSTEM_READY_LED_PIN, OUTPUT);
  digitalWrite(SYSTEM_READY_LED_PIN, LOW);
  pinMode(CRANK_ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(CRANK_ACTIVITY_LED_PIN, LOW);
  pinMode(MOTOR_ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(MOTOR_ACTIVITY_LED_PIN, LOW);

  // attached ESC
  esc.attach(ESC_PIN);
  esc.write(esc_neutral_power);
  delay(DELAY_UNIT);
  Serial.println("Ready");
  delay(DELAY_UNIT);

  // turn on esc

  Serial.println("Turn on ESC: hold down the SET button and power button untill it beeps");
  delay(5 * DELAY_UNIT);

  // set neutral position
  digitalWrite(SYSTEM_READY_LED_PIN, HIGH);
  Serial.println("Set neutral: press the set button once, the motor should beep once");
  esc.write(esc_neutral_power);
  delay(DELAY_UNIT);

  // set full power position
  digitalWrite(CRANK_ACTIVITY_LED_PIN, HIGH);
  Serial.println("Set full: press the set button once, the motor should beep twice");
  esc.write(esc_forward_power);
  delay(DELAY_UNIT);

  // set full reverse position
  digitalWrite(MOTOR_ACTIVITY_LED_PIN, HIGH);
  Serial.println("Set reverse: press the set button once, the motor should beep thrice");
  esc.write(esc_reverse_power);
  delay(DELAY_UNIT);

  // turn off esc
  Serial.println("ESC calibrated: turn off the ESC");
}

/*
  Main Loop
  ---------
*/
void loop() {
  // turn off status LEDs
  digitalWrite(SYSTEM_READY_LED_PIN, LOW);
  digitalWrite(CRANK_ACTIVITY_LED_PIN, LOW);
  digitalWrite(MOTOR_ACTIVITY_LED_PIN, LOW);
}
