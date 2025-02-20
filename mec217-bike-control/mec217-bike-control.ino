/**
 *  Original Author: D S Brennan (github.com/dsbrennan)
 *  Created: 29/08/2023
 *  Updated: 20/02/2025
 *
 *  Copyright 2023 - 2025, MIT Licence
 **/
#include <Servo.h>

// pins: do not change these values
#define SYSTEM_READY_LED_PIN 5
#define CRANK_ACTIVITY_LED_PIN 4
#define MOTOR_ACTIVITY_LED_PIN 6
#define CRANK_PIN 2
#define WHEEL_PIN 3
#define ESC_PIN 9

// safety: do not change these values
#define CRANK_PASS_ACTIVITY_DELAY 500
#define CRANK_PASS_MAXIMUM_DELAY 3000
#define WHEEL_MAXIMUM_SPEED 25.0

// bike: crank radius 0.175m, wheel radius 0.33m
#define CRANK_CIRCUMFORANCE 1.0995
#define WHEEL_CIRCUMFORANCE 2.0734

// debug
#define MESSAGE_MAXIMUM_INTERVAL 1000

// constants, can be tweaked
const int startup_time = 3000;
const int esc_initial_power = 45;
const int esc_power_limit = 140;
const float esc_step_up = 0.1;
const float esc_step_down = 0.5;

// count variables
unsigned volatile int crank_rotations_counter;
unsigned volatile int wheel_rotations_counter;
unsigned volatile long crank_interrupt_current_time;
unsigned volatile long crank_interrupt_previous_time;
unsigned volatile long wheel_interrupt_current_time;
unsigned volatile long wheel_interrupt_previous_time;
unsigned long current_loop_time;
unsigned long message_previous_time;

// output variables
float esc_power_output;
Servo esc;

/*
  System Setup
  ---------------------
*/
void setup() {
  // serial
  Serial.begin(9600);
  // set default values
  crank_rotations_counter = 0;
  wheel_rotations_counter = 0;
  crank_interrupt_current_time = 0;
  crank_interrupt_previous_time = 0;
  wheel_interrupt_current_time = 0;
  wheel_interrupt_previous_time = 0;
  current_loop_time = 0;
  message_previous_time = 0;
  // status LEDs
  pinMode(SYSTEM_READY_LED_PIN, OUTPUT);
  digitalWrite(SYSTEM_READY_LED_PIN, LOW);
  pinMode(CRANK_ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(CRANK_ACTIVITY_LED_PIN, LOW);
  pinMode(MOTOR_ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(MOTOR_ACTIVITY_LED_PIN, LOW);
  // crank sensor
  pinMode(CRANK_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CRANK_PIN), crankInterrupt, FALLING);
  // wheel sensor
  pinMode(WHEEL_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(WHEEL_PIN), wheelInterrupt, FALLING);
  // ESC
  esc_power_output = esc_initial_power;
  esc.attach(ESC_PIN);
  esc.write(0);
  // pause system untill ready
  long zero_time = millis();
  while (millis() < zero_time + startup_time) {
    delay(1000);
  }
  // activate system ready LED
  digitalWrite(SYSTEM_READY_LED_PIN, HIGH);
  Serial.println("System Ready");
}

/*
  Main Loop
  ---------------------
*/
void loop() {
  // show crank activity LED
  digitalWrite(CRANK_ACTIVITY_LED_PIN, (current_loop_time - crank_interrupt_current_time <= CRANK_PASS_ACTIVITY_DELAY ? HIGH : LOW));

  // calculate crank speed
  float crank_kmph = 0;
  float crank_rotation_time = crank_interrupt_current_time - crank_interrupt_previous_time;
  if (crank_rotation_time > 1) {
    float crank_rpm = 60.0 / (crank_rotation_time / 1000);
    crank_kmph = (crank_rpm * 60 * CRANK_CIRCUMFORANCE) / 1000;
  }

  // calculate wheel speed
  float wheel_kmph = 0;
  float wheel_rotation_time = wheel_interrupt_current_time - wheel_interrupt_previous_time;
  if (wheel_rotation_time > 1) {
    float wheel_rpm = 60.0 / (wheel_rotation_time / 1000);
    wheel_kmph = (wheel_rpm * 60 * WHEEL_CIRCUMFORANCE) / 1000;
  }

  // crank interrupt within maximum delay
  current_loop_time = millis();
  if (crank_interrupt_current_time > startup_time && crank_interrupt_previous_time > startup_time
      && current_loop_time - crank_interrupt_current_time <= CRANK_PASS_MAXIMUM_DELAY
      && wheel_kmph <= WHEEL_MAXIMUM_SPEED) {
    // power up ESC
    digitalWrite(MOTOR_ACTIVITY_LED_PIN, HIGH);
    if ((esc_power_output + esc_step_up) <= esc_power_limit) {
      esc_power_output += esc_step_up;
    } else {
      esc_power_output = esc_power_limit;
    }    
  } else {
    // power down ESC
    digitalWrite(MOTOR_ACTIVITY_LED_PIN, LOW);
    if (esc_power_output - esc_step_down >= esc_initial_power) {
      esc_power_output -= esc_step_down;
    } else {
      esc_power_output = esc_initial_power;
    }
  }
  esc.write((int)esc_power_output);

  // display message
  if (current_loop_time - message_previous_time >= MESSAGE_MAXIMUM_INTERVAL) {
    message_previous_time = current_loop_time;
    Serial.print("ESC power output: ");
    Serial.print(esc_power_output);
    Serial.print(" crank count: ");
    Serial.print(crank_rotations_counter);
    Serial.print(" wheel count: ");
    Serial.print(wheel_rotations_counter);
    Serial.print(" crank kmph: ");
    Serial.print(crank_kmph);
    Serial.print(" wheel kmph: ");
    Serial.println(wheel_kmph);
  }

  // pause the system for 0.025s
  delay(25);
}

/*
  Crank Sensor Interrupt
  ---------------------
*/
void crankInterrupt() {
  crank_rotations_counter = crank_rotations_counter + 1;
  crank_interrupt_previous_time = crank_interrupt_current_time;
  crank_interrupt_current_time = millis();
}

/*
  Wheel Sensor Interrupt
  ---------------------
*/
void wheelInterrupt() {
  wheel_rotations_counter = wheel_rotations_counter + 1;
  wheel_interrupt_previous_time = wheel_interrupt_current_time;
  wheel_interrupt_current_time = millis();
}