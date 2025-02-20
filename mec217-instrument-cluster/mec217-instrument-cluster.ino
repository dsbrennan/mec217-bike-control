/**
 *  Original Author: D S Brennan (github.com/dsbrennan)
 *  Created: 13/01/2025
 *
 *  Copyright 2025, MIT Licence
 **/
#include "Arduino_GigaDisplay_GFX.h"
#include "Arduino_GigaDisplayTouch.h"
#include <math.h>

// pins
#define CRANK_PIN 2
#define WHEEL_PIN 3

// colours
#define COLOUR_BLACK 0x0000
#define COLOUR_WHITE 0xFFFF
#define COLOUR_RED 0xD800
#define COLOUR_GREEN 0x06E0
#define COLOUR_GRAY 0x8C71

// display
#define SCREEN_ROTATE 1
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

// cluster layout
#define DIAL_X_POSITION 400 // SCREEN_WIDTH / 2
#define DIAL_Y_POSITION 270 // SCREEN_HEIGHT / 2 + padding (30 in this case)
#define DIAL_START_DEGREE -120
#define DIAL_END_DEGREE 120
#define DIAL_RADIUS 240 // SCREEN_HEIGHT / 2
#define DIAL_LABEL_START 0
#define DIAL_LABEL_END 50
#define DIAL_LABEL_STEP 5
#define DIAL_LABEL_MULTIPLIER 0.80
#define DIAL_TICKER_STEP 4.8 // (DIAL_END_DEGREE - DIAL_START_DEGREE) / (DIAL_LABEL_END - DIAL_LABEL_START)
#define DIAL_TICKER_OUTSIDE_MULTIPLIER 0.95
#define DIAL_TICKER_INSIDE_MULTIPLIER 0.90
#define DIAL_CANVAS_ARM_X_ORIGIN 240 // CANVAS_WIDTH / 2
#define DIAL_CANVAS_ARM_Y_ORIGIN 240 // CANVAS_HEIGHT / 2 * 3
#define DIAL_ARM_INSIDE_MULTIPLIER 0.505 // ratio of DIAL_RADIUS to DIAL_ROTATION_COUNTER_RADIUS
#define DIAL_CRANK_ARM_OUTSIDE_MULTIPLIER 0.6
#define DIAL_WHEEL_ARM_OUTSIDE_MULTIPLIER 0.74
#define DIAL_ROTATION_COUNTER_RADIUS 120 //SCREEN_HEIGHT / 4
#define DIAL_ROTATION_COUNTER_TEXT_SCALE 15
#define DIAL_COUNTDOWN_X_POSITION 100 // SCREEN_WIDTH / 8
#define DIAL_COUNTDOWN_Y_POSITION 96 // SCREEN_HEIGHT /  5
#define DIAL_COUNTDOWN_RADIUS 80 // SCREEN_HEIGHT / 6
#define DIAL_COUNTDOWN_TEXT_SCALE 9
#define DIAL_STATUS_Y_POSITION 440 // SCREEN_HEIGHT / 4 * 3
#define DIAL_STATUS_TEXT_SIZE 3

// control system
#define CRANK_PASS_MAXIMUM_DELAY 1500 // crank_pass_maximum_delay in student code
#define ROTATION_TIME_LIMIT 30000 // wheel_rotation_time in student code
#define CRANK_CIRCUMFORANCE 0.5 // not in student code
#define WHEEL_CIRCUMFORANCE 2.0 // wheel_circumforance in student code
#define TOUCH_RESET_MINIMUM_DELAY 500 // not in student code
#define TOUCH_RESET_MAXIMUM_DELAY 1000 // not in student code

// visual output variables
GigaDisplay_GFX display;
GFXcanvas1 canvas(SCREEN_HEIGHT, (SCREEN_HEIGHT/4)*3);// height will depend upon dial radius and start and end degree
Arduino_GigaDisplayTouch touchDetector;

// control variables
unsigned volatile long wheel_rotation_counter;
unsigned volatile long touch_interrupt_time;
unsigned volatile long crank_interrupt_current_time;
unsigned volatile long crank_interrupt_previous_time;
unsigned volatile long wheel_interrupt_current_time;
unsigned volatile long wheel_interrupt_previous_time;
unsigned long current_loop_time;
unsigned long timer_activation_time;
signed int timer_activation_count;
signed int timer_deactivation_count;

// previous arm positions
uint16_t previous_crank_arm_inside_x = 0;
uint16_t previous_crank_arm_inside_y = 0;
uint16_t previous_crank_arm_outside_x = 0;
uint16_t previous_crank_arm_outside_y = 0;
uint16_t previous_wheel_arm_inside_x = 0;
uint16_t previous_wheel_arm_inside_y = 0;
uint16_t previous_wheel_arm_outside_x = 0;
uint16_t previous_wheel_arm_outside_y = 0;

void setup() {
  /*
    System setup
  */
  // setup serial
  Serial.begin(9600);
  delay(3000);
  // set counting default values
  wheel_rotation_counter = 0;
  touch_interrupt_time = 0;
  crank_interrupt_current_time = 0;
  crank_interrupt_previous_time = 0;
  wheel_interrupt_current_time = 0;
  wheel_interrupt_previous_time = 0;
  current_loop_time = 0;
  timer_activation_time = 0;
  timer_activation_count = -1;
  timer_deactivation_count = -1;
  // setup crank sensor
  pinMode(CRANK_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CRANK_PIN), crankInterrupt, FALLING);
  // setup wheel sensor
  pinMode(WHEEL_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(WHEEL_PIN), wheelInterrupt, FALLING);
  // setup touch
  touchDetector.begin();
  touchDetector.onDetect(touchInterrupt);
  //setup blank horizontal screen
  display.begin();  // Init display library
  display.setRotation(SCREEN_ROTATE);
  display.fillScreen(COLOUR_BLACK);
  
  /*
    Setup Instrument Cluster
    -----------------------
    The intrument cluster displays the number of rotations within the time period, the current crank speed (how fast you are peddling),
    and the wheel speed (how fast the back wheel of the bike is going after your gearbox increases the wheels speed).
  */
  // arc
  drawArc(DIAL_X_POSITION, DIAL_Y_POSITION, DIAL_START_DEGREE, DIAL_END_DEGREE, DIAL_RADIUS, 3, COLOUR_WHITE);
  // tickers
  for (double i = DIAL_START_DEGREE; i <= DIAL_END_DEGREE; i+= DIAL_TICKER_STEP){
    float angle = i < 0 ? -i : i;
    float radians_angle = (angle < 90 ? 90 - angle : angle - 90) * (M_PI / 180.0);
    float y_increment = DIAL_RADIUS * sin(radians_angle);
    float x_increment = DIAL_RADIUS * cos(radians_angle);
    display.drawLine(
      i < 0 ? DIAL_X_POSITION - (DIAL_TICKER_OUTSIDE_MULTIPLIER * x_increment) : DIAL_X_POSITION + (DIAL_TICKER_OUTSIDE_MULTIPLIER * x_increment),
      angle < 90 ? DIAL_Y_POSITION - (DIAL_TICKER_OUTSIDE_MULTIPLIER * y_increment) : DIAL_Y_POSITION + (DIAL_TICKER_OUTSIDE_MULTIPLIER * y_increment), 
      i < 0 ? DIAL_X_POSITION - (DIAL_TICKER_INSIDE_MULTIPLIER * x_increment) : DIAL_X_POSITION + (DIAL_TICKER_INSIDE_MULTIPLIER * x_increment),
      angle < 90 ? DIAL_Y_POSITION - (DIAL_TICKER_INSIDE_MULTIPLIER * y_increment) : DIAL_Y_POSITION + (DIAL_TICKER_INSIDE_MULTIPLIER * y_increment), 
      COLOUR_WHITE
    );
  }
  // labels
  int dial_label_degrees = (DIAL_END_DEGREE - DIAL_START_DEGREE) / ((DIAL_LABEL_END - DIAL_LABEL_START) / DIAL_LABEL_STEP);
  char numerical_text[16];
  for (int i = 0; i <= ((DIAL_LABEL_END - DIAL_LABEL_START) / DIAL_LABEL_STEP); i++) {
    float label_angle = DIAL_START_DEGREE + (i * dial_label_degrees);
    float angle = label_angle < 0 ? -label_angle : label_angle;
    float radians_angle =  (angle < 90 ? 90 - angle : angle - 90) * (M_PI / 180.0);
    float y_increment = DIAL_LABEL_MULTIPLIER * DIAL_RADIUS * sin(radians_angle);
    float x_increment = DIAL_LABEL_MULTIPLIER * DIAL_RADIUS * cos(radians_angle);
    itoa(DIAL_LABEL_START + (i * DIAL_LABEL_STEP), numerical_text, 10);
    displayCenteredText(
      label_angle < 0 ? DIAL_X_POSITION - x_increment : DIAL_X_POSITION + x_increment,
      angle < 90 ? DIAL_Y_POSITION - y_increment : DIAL_Y_POSITION + y_increment,
      numerical_text, COLOUR_WHITE, COLOUR_BLACK, 2
    );
  }
  // counters 
  display.fillCircle(DIAL_X_POSITION, DIAL_Y_POSITION, DIAL_ROTATION_COUNTER_RADIUS, COLOUR_GRAY);
  displayCenteredText(
    DIAL_X_POSITION, DIAL_Y_POSITION + 5, "0", COLOUR_WHITE, COLOUR_GRAY, DIAL_ROTATION_COUNTER_TEXT_SCALE
  );
  itoa(ROTATION_TIME_LIMIT/1000, numerical_text, 10);
  display.fillCircle(DIAL_COUNTDOWN_X_POSITION, DIAL_COUNTDOWN_Y_POSITION, DIAL_COUNTDOWN_RADIUS, COLOUR_GRAY);
  displayCenteredText(
    DIAL_COUNTDOWN_X_POSITION,  DIAL_COUNTDOWN_Y_POSITION + 5, numerical_text, COLOUR_WHITE, COLOUR_GRAY, DIAL_COUNTDOWN_TEXT_SCALE
  );
  // system status
  displayCenteredText(DIAL_X_POSITION, DIAL_STATUS_Y_POSITION, "system ready", COLOUR_WHITE, COLOUR_BLACK, DIAL_STATUS_TEXT_SIZE);
}


void loop() {
  /*
    Control System Monitoring
    -------------------------
    Monitor the control system by mirroring key calculations
  */
  current_loop_time = millis();
  if(
    touch_interrupt_time > 0 && timer_activation_count >= 0
    && current_loop_time - touch_interrupt_time > TOUCH_RESET_MINIMUM_DELAY 
    && current_loop_time - touch_interrupt_time < TOUCH_RESET_MAXIMUM_DELAY
  ){
    // reset system
    wheel_rotation_counter = 0;
    crank_interrupt_current_time = 0;
    crank_interrupt_previous_time = 0;
    wheel_interrupt_current_time = 0;
    wheel_interrupt_previous_time = 0;
    timer_activation_time = 0;
    timer_activation_count = -1;
    timer_deactivation_count = -1;
    Serial.println("system reset");
  }
  if(
    crank_interrupt_current_time > 0 && crank_interrupt_previous_time > 0
    && (crank_interrupt_current_time - crank_interrupt_previous_time > 0)
    && current_loop_time - crank_interrupt_current_time <= CRANK_PASS_MAXIMUM_DELAY
  ){
    // start timer
    if (timer_activation_count < 0 && timer_deactivation_count < 0){
      Serial.println("starting timer");
      timer_activation_time = current_loop_time;
      timer_activation_count = wheel_rotation_counter;
    }
  }
  // end timer
  if (timer_activation_time > 0 && current_loop_time >= timer_activation_time + ROTATION_TIME_LIMIT){
    if (timer_deactivation_count < 0){
      Serial.println("ending timer");
      timer_deactivation_count = wheel_rotation_counter;
    }
  }
  // crank speed
  float crank_speed = 0.0;
  if(crank_interrupt_current_time > 0 && crank_interrupt_previous_time > 0){
    float crank_pass_delta = crank_interrupt_current_time - crank_interrupt_previous_time;
    float rpm_value = (float)60000.0 / crank_pass_delta;
    float rph_value = rpm_value * 60;
    float rph_meter = rph_value * CRANK_CIRCUMFORANCE;
    crank_speed = rph_meter / 1000;
  }
  // wheel speed
  float wheel_speed = 0.0;
  if(wheel_interrupt_current_time > 0 && wheel_interrupt_previous_time > 0){
    float wheel_pass_delta = wheel_interrupt_current_time - wheel_interrupt_previous_time;
    float rpm_value = (float)60000.0 / wheel_pass_delta;
    float rph_value = rpm_value * 60;
    float rph_meter = rph_value * WHEEL_CIRCUMFORANCE;
    wheel_speed = rph_meter / 1000;
  }

  /*
    Clear previous displayed speed arms
    -----------------------------------
    As the previously displayed arms (crank arm and wheel arm) have been writen to both the
    offscreen canvas and the display, they need to be reset on both output. The canvas needs
    the value reset to 0 and the display needs the value set to the same background colour as
    the display.
  */
  // crank arm
  canvas.drawLine(previous_crank_arm_inside_x, previous_crank_arm_inside_y, previous_crank_arm_outside_x, previous_crank_arm_outside_y, 0);
  display.drawLine(
    DIAL_X_POSITION - (canvas.width() / 2) + previous_crank_arm_inside_x,
    DIAL_Y_POSITION - ((canvas.height() / 3) * 2) + previous_crank_arm_inside_y,
    DIAL_X_POSITION - (canvas.width() / 2) + previous_crank_arm_outside_x,
    DIAL_Y_POSITION - ((canvas.height() / 3) * 2) + previous_crank_arm_outside_y,
    COLOUR_BLACK
  );
  // wheel arm
  canvas.drawLine(previous_wheel_arm_inside_x, previous_wheel_arm_inside_y, previous_wheel_arm_outside_x, previous_wheel_arm_outside_y, 0);
  display.drawLine(
    DIAL_X_POSITION - (canvas.width() / 2) + previous_wheel_arm_inside_x,
    DIAL_Y_POSITION - ((canvas.height() / 3) * 2) + previous_wheel_arm_inside_y,
    DIAL_X_POSITION - (canvas.width() / 2) + previous_wheel_arm_outside_x,
    DIAL_Y_POSITION - ((canvas.height() / 3) * 2) + previous_wheel_arm_outside_y,
    COLOUR_BLACK
  );
  
  /*
    Calculate new arm positions
    ---------------------------
    Once the new positions of the arms (crank arm and wheel arm) have been calculated
    they need to be written to the offscreen canvas. As the canvas is a 2 colour
    canvas (background and foreground), the values of the pixels need to only be 
    set to 1 as the colour for the foreground is decided when writing to the display. 
  */
  // crank arm
  float crank_degree = DIAL_START_DEGREE + (crank_speed * DIAL_TICKER_STEP);
  float angle = crank_degree < 0 ? -crank_degree : crank_degree;
  float radians_angle = (angle < 90 ? 90 - angle : angle - 90) * (M_PI / 180.0);
  float y_increment = DIAL_RADIUS * sin(radians_angle);
  float x_increment = DIAL_RADIUS * cos(radians_angle);
  previous_crank_arm_inside_x = crank_degree < 0 ? DIAL_CANVAS_ARM_X_ORIGIN - (x_increment * DIAL_ARM_INSIDE_MULTIPLIER) : DIAL_CANVAS_ARM_X_ORIGIN + (x_increment * DIAL_ARM_INSIDE_MULTIPLIER);
  previous_crank_arm_inside_y = angle < 90 ? DIAL_CANVAS_ARM_Y_ORIGIN - (y_increment * DIAL_ARM_INSIDE_MULTIPLIER) : DIAL_CANVAS_ARM_Y_ORIGIN + (y_increment * DIAL_ARM_INSIDE_MULTIPLIER);
  previous_crank_arm_outside_x = crank_degree < 0 ? DIAL_CANVAS_ARM_X_ORIGIN - (x_increment * DIAL_CRANK_ARM_OUTSIDE_MULTIPLIER) : DIAL_CANVAS_ARM_X_ORIGIN + (x_increment * DIAL_CRANK_ARM_OUTSIDE_MULTIPLIER);
  previous_crank_arm_outside_y = angle < 90 ? DIAL_CANVAS_ARM_Y_ORIGIN - (y_increment * DIAL_CRANK_ARM_OUTSIDE_MULTIPLIER) : DIAL_CANVAS_ARM_Y_ORIGIN + (y_increment * DIAL_CRANK_ARM_OUTSIDE_MULTIPLIER);
  canvas.drawLine(previous_crank_arm_inside_x, previous_crank_arm_inside_y, previous_crank_arm_outside_x, previous_crank_arm_outside_y, 1);
  // wheel arm
  float wheel_degree = DIAL_START_DEGREE + (wheel_speed * DIAL_TICKER_STEP);
  angle = wheel_degree < 0 ? -wheel_degree : wheel_degree;
  radians_angle = (angle < 90 ? 90 - angle : angle - 90) * (M_PI / 180.0);
  y_increment = DIAL_RADIUS * sin(radians_angle);
  x_increment = DIAL_RADIUS * cos(radians_angle);
  previous_wheel_arm_inside_x = wheel_degree < 0 ? DIAL_CANVAS_ARM_X_ORIGIN - (x_increment * DIAL_ARM_INSIDE_MULTIPLIER) : DIAL_CANVAS_ARM_X_ORIGIN + (x_increment * DIAL_ARM_INSIDE_MULTIPLIER);
  previous_wheel_arm_inside_y = angle < 90 ? DIAL_CANVAS_ARM_Y_ORIGIN - (y_increment * DIAL_ARM_INSIDE_MULTIPLIER) : DIAL_CANVAS_ARM_Y_ORIGIN + (y_increment * DIAL_ARM_INSIDE_MULTIPLIER);
  previous_wheel_arm_outside_x = wheel_degree < 0 ? DIAL_CANVAS_ARM_X_ORIGIN - (x_increment * DIAL_WHEEL_ARM_OUTSIDE_MULTIPLIER) : DIAL_CANVAS_ARM_X_ORIGIN + (x_increment * DIAL_WHEEL_ARM_OUTSIDE_MULTIPLIER);
  previous_wheel_arm_outside_y = angle < 90 ? DIAL_CANVAS_ARM_Y_ORIGIN - (y_increment * DIAL_WHEEL_ARM_OUTSIDE_MULTIPLIER) : DIAL_CANVAS_ARM_Y_ORIGIN + (y_increment * DIAL_WHEEL_ARM_OUTSIDE_MULTIPLIER);
  canvas.drawLine(previous_wheel_arm_inside_x, previous_wheel_arm_inside_y, previous_wheel_arm_outside_x, previous_wheel_arm_outside_y, 1);

  /*
    Write graphical components to display
    -------------------------------------
    Write the offscreen canvas to the display and re write the counter to the display
    with the latest value.
  */
  // canvas -> display
  display.drawBitmap(DIAL_X_POSITION - (canvas.width() / 2), DIAL_Y_POSITION - ((canvas.height() / 3) * 2), canvas.getBuffer(), canvas.width(), canvas.height(), COLOUR_GREEN);
  // number of rotations
  char numerical_text[16];
  bool timer_active = timer_activation_time > 0 && current_loop_time < timer_activation_time + ROTATION_TIME_LIMIT;
  itoa((timer_active ? wheel_rotation_counter : timer_deactivation_count) - timer_activation_count, numerical_text, 10);
  display.fillCircle(DIAL_X_POSITION, DIAL_Y_POSITION, DIAL_ROTATION_COUNTER_RADIUS, timer_active ? COLOUR_RED : COLOUR_GRAY);
  displayCenteredText(
    DIAL_X_POSITION, DIAL_Y_POSITION + 5, numerical_text, COLOUR_WHITE, timer_active ? COLOUR_RED : COLOUR_GRAY, DIAL_ROTATION_COUNTER_TEXT_SCALE
  );
  // number of seconds left
  itoa((timer_active ? (ROTATION_TIME_LIMIT - (current_loop_time - timer_activation_time)) : ROTATION_TIME_LIMIT) / 1000, numerical_text, 10);
  display.fillCircle(DIAL_COUNTDOWN_X_POSITION, DIAL_COUNTDOWN_Y_POSITION, DIAL_COUNTDOWN_RADIUS, timer_active ? COLOUR_GREEN : COLOUR_GRAY);
  displayCenteredText(
    DIAL_COUNTDOWN_X_POSITION,  DIAL_COUNTDOWN_Y_POSITION + 5, numerical_text, COLOUR_WHITE, timer_active ? COLOUR_GREEN : COLOUR_GRAY, DIAL_COUNTDOWN_TEXT_SCALE
  );
  // system status
  char status[12];
  strcpy(status, timer_active ? "  counting  " : (timer_deactivation_count - timer_activation_count > 0 ? "  complete  " : "system ready"));
  displayCenteredText(DIAL_X_POSITION, DIAL_STATUS_Y_POSITION, status, COLOUR_WHITE, COLOUR_BLACK, DIAL_STATUS_TEXT_SIZE);

  // simulate
  if (current_loop_time - touch_interrupt_time > 13000){
    crank_interrupt_previous_time = current_loop_time - 1000;
    crank_interrupt_current_time = current_loop_time;
    wheel_rotation_counter = (current_loop_time - 13000) / 2000;
    float acceleration = 10000.0 - (((current_loop_time - touch_interrupt_time - 13000.0)/1000.0)*500.0);
    wheel_interrupt_previous_time = current_loop_time - (acceleration > 290 ? acceleration : 290);
    wheel_interrupt_current_time = current_loop_time;
  }

  // sleep thread
  delay(50);
}

/*
  Crank Sensor Interrupt
  ---------------------
*/
void crankInterrupt(){
  crank_interrupt_previous_time = crank_interrupt_current_time;
  crank_interrupt_current_time = millis();
}

/*
  Wheel Sensor Interrupt
  ---------------------
*/
void wheelInterrupt(){
  wheel_rotation_counter = wheel_rotation_counter + 1;
  wheel_interrupt_previous_time = wheel_interrupt_current_time;
  wheel_interrupt_current_time = millis();
}

/*
  Touch Interrupt
  --------------
*/
void touchInterrupt(uint8_t contacts, GDTpoint_t* points){
  if(contacts >= 3){
    touch_interrupt_time = millis();
  }
}

/*
  Draw Arc
  --------
  Draw an arced line around a point (x, y) from the start angle (start_angle) 
  to the end angle (end_angle) with a given radius (radius) and thickness (thickness).
*/
void drawArc(uint16_t x, uint16_t y, int16_t start_angle, int16_t end_angle, uint16_t radius, uint16_t thickness, uint16_t colour) {
  //ensure angle is between -180 to 180
  if (start_angle < -180 || start_angle > end_angle) {
    return;
  }
  if (end_angle > 180 || end_angle < start_angle) {
    return;
  }
  //draw arc
  float step = 0.2;
  for (float i = start_angle; i < end_angle; i += step) {
    float angle = i < 0 ? -i : i;
    float radians_angle = (angle < 90 ? 90 - angle : angle - 90) * (M_PI / 180.0);
    float y_increment = radius * sin(radians_angle);
    float x_increment = radius * cos(radians_angle);
    if (thickness == 1) {
      display.drawPixel(i < 0 ? x - x_increment : x + x_increment, angle < 90 ? y - y_increment : y + y_increment, colour);
    } else {
      display.fillCircle(i < 0 ? x - x_increment : x + x_increment, angle < 90 ? y - y_increment : y + y_increment, thickness / 2.0, colour);
    }
  }
}

/*
  Display Centered Text
  ---------------------
  Draw text centered around a point (x, y)
*/
void displayCenteredText(uint16_t x, uint16_t y, char text[], uint16_t text_colour, uint16_t background_colour, uint8_t size) {
  int item_count = strlen(text);
  int half_count = item_count / 2;
  int modulo_count = item_count % 2;
  for (int i = 0; i < item_count; i++) {
    display.drawChar(
      x - ((modulo_count * size * CHAR_WIDTH) / 2) - ((half_count - i) * size * CHAR_WIDTH) + (size / 2),
      y - (size * CHAR_HEIGHT) / 2,
      text[i], text_colour, background_colour, size
    );
  }
}
