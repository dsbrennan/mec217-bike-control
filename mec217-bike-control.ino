/**
 *  Original Author: D S Brennan (github.com/dsbrennan)
 *  Created: 29/08/2023
 *
 *  Copyright 2023, MIT Licence
 **/
#include <Servo.h>
//Pin setup
const int SYSTEM_READY_LED_PIN = 5;
const int MOTOR_ACTIVITY_LED_PIN = 6;
const int CRANK_ACTIVITY_LED_PIN = 4;
const int CRANK_PIN = 2;
const int RPM_PIN = 3;
const int ESC_PIN = 9;
//Constants
const int startup_time = 10000;
const int crank_pass_maximum_delay = 1500;
const int wheel_rotation_time = 30000;
const int esc_initial_power = 55;
const int esc_power_limit = 140;
const int esc_step_up = 1;
const int esc_step_down = 5;
const float wheel_circumforance = 2.0;
const float wheel_maximum_speed = 25.0;
//Variables
unsigned volatile long last_crank_pass;
unsigned volatile long last_rpm_pass;
unsigned volatile long last_rpm_pass_2;
unsigned volatile int wheel_rotations_count;
unsigned long current_loop_time;
unsigned long esc_activation_time;
signed int timer_activation_count;
signed int timer_deactivation_count;
unsigned int esc_power_output;
Servo esc;

//Setup System
void setup() {
  Serial.begin(9600);
  //Setup Status LEDs
  pinMode(SYSTEM_READY_LED_PIN, OUTPUT);
  digitalWrite(SYSTEM_READY_LED_PIN, LOW);
  pinMode(CRANK_ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(CRANK_ACTIVITY_LED_PIN, LOW);
  pinMode(MOTOR_ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(MOTOR_ACTIVITY_LED_PIN, LOW);
  //Setup Crank Sensor
  pinMode(CRANK_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CRANK_PIN), crank, FALLING);
  //Setup RPM Sensor
  pinMode(RPM_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RPM_PIN), rpm, FALLING);
  //Setup Timer
  timer_activation_count = -1;
  timer_deactivation_count = -1;
  //Setup ESC
  esc_power_output = esc_initial_power;
  esc.attach(ESC_PIN);
  esc.write(0);
  //Pause System untill ready
  long zero_time = millis();
  while (millis() < zero_time + startup_time){
    delay(1000);
  }
  //Activate System ready LED
  digitalWrite(SYSTEM_READY_LED_PIN, HIGH);
}

//Main function loop
void loop() {
  //Show Crank LED
  digitalWrite(CRANK_ACTIVITY_LED_PIN, ((digitalRead(CRANK_PIN) == 1) ? LOW : HIGH));
  //Calculate RPM
  float rpm_pass_delta = last_rpm_pass - last_rpm_pass_2;
  float rpm_value = (float)60000.0 / rpm_pass_delta;
  float rph_value = rpm_value * 60;
  float rph_meter = rph_value * wheel_circumforance;
  float kmph = rph_meter / 1000;
  Serial.print("Wheel spinning at: ");
  Serial.println(kmph);
  //Last crank pass within time limit
  current_loop_time = millis();
  if(current_loop_time - last_crank_pass <= crank_pass_maximum_delay && kmph <= wheel_maximum_speed){
    //Record first activation time
    if(esc_activation_time < startup_time){
      Serial.print("ESC First Activation: ");
      Serial.println(current_loop_time);
      esc_activation_time = current_loop_time;
      timer_activation_count = wheel_rotations_count;
    }
    //Power Up ESC
    digitalWrite(MOTOR_ACTIVITY_LED_PIN, HIGH);
    if(esc_power_output <= esc_power_limit && (esc_power_output + esc_step_up) <= esc_power_limit){
      Serial.print("ESC Power Output (up): ");
      Serial.println(esc_power_output);
      esc.write(esc_power_output);
      esc_power_output += esc_step_up;
    }else{
      Serial.print("ESC Power Output (Limited): ");
      Serial.println(esc_power_limit);
      esc.write(esc_power_limit);
    }
  }else{
    //Power Down ESC
    digitalWrite(MOTOR_ACTIVITY_LED_PIN, LOW);
    if(esc_power_output - esc_step_down > esc_initial_power){
      esc_power_output -= esc_step_down;
      Serial.print("ESC Power Output (down): ");
      Serial.println(esc_power_output);
      esc.write(esc_power_output);
    }else{
      esc_power_output = esc_initial_power;
      Serial.println("ESC Power STOP");
      esc.write(0);
    }
  }
  //Output wheel rotation
  if(esc_activation_time > startup_time){
    if(esc_activation_time + wheel_rotation_time <= current_loop_time){
      if(timer_deactivation_count < timer_activation_count){
        timer_deactivation_count = wheel_rotations_count;
        Serial.print("Total Wheel Rotations within Time: ");
        Serial.println(timer_deactivation_count - timer_activation_count);
      }
    }else{
      Serial.print("Current Wheel Rotations: ");
      Serial.print(wheel_rotations_count);
      Serial.print(" (");
      Serial.print(((esc_activation_time + wheel_rotation_time) - current_loop_time)/1000);
      Serial.println("s) left");
    }
  }
  //Pause the system for 0.25s
  delay(250);
}

//Crank sensor interrupt
void crank(){
  last_crank_pass = millis();
}

//Wheel RPM sensor interrupt
void rpm(){
  wheel_rotations_count = wheel_rotations_count + 1;
  last_rpm_pass_2 = last_rpm_pass;
  last_rpm_pass = millis();
}

