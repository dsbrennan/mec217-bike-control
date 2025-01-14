#include "Arduino_GigaDisplay_GFX.h"
#include <math.h>

// colours
#define COLOUR_BLACK 0x0000
#define COLOUR_WHITE 0xFFFF
#define COLOUR_RED 0xD800
#define COLOUR_GREEN 0x06E0
#define COLOUR_GRAY 0x8C71

// display
#define SCREEN_ROTATE 3
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

// cluster layout
#define DIAL_X_POSITION 400 // SCREEN_WIDTH / 2
#define DIAL_Y_POSITION 280 // SCREEN_HEIGHT / 2 + padding (30 in this case)
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

// visual output variables
GigaDisplay_GFX display;
GFXcanvas1 canvas(SCREEN_HEIGHT, (SCREEN_HEIGHT/4)*3);// height will depend upon dial radius and start and end degree

// speed variables
unsigned int rotation_counter;
float crank_speed;
float wheel_speed;

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
  //setup serial
  Serial.begin(9600);
  delay(3000);
  //set variable defailt values
  rotation_counter = 0;
  crank_speed = 0.0;
  wheel_speed = 0.0;
  //setup blank horizontal screen
  display.begin();  // Init display library
  display.setRotation(SCREEN_ROTATE);
  display.fillScreen(COLOUR_BLACK);
  //setup instrument cluster
  //ic - arc
  drawArc(DIAL_X_POSITION, DIAL_Y_POSITION, DIAL_START_DEGREE, DIAL_END_DEGREE, DIAL_RADIUS, 3, COLOUR_WHITE);
  //ic - tickers
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
  //ic - labels
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
  //ic - counter 
  display.fillCircle(DIAL_X_POSITION, DIAL_Y_POSITION, DIAL_ROTATION_COUNTER_RADIUS, COLOUR_GRAY);
  displayCenteredText(
    DIAL_X_POSITION, DIAL_Y_POSITION + 5, "0", COLOUR_WHITE, COLOUR_GRAY, DIAL_ROTATION_COUNTER_TEXT_SCALE
  );
}


void loop() {
  //canvas section
  //clear old arms
  canvas.drawLine(previous_crank_arm_inside_x, previous_crank_arm_inside_y, previous_crank_arm_outside_x, previous_crank_arm_outside_y, 0);
  display.drawLine(
    DIAL_X_POSITION - (canvas.width() / 2) + previous_crank_arm_inside_x,
    DIAL_Y_POSITION - ((canvas.height() / 3) * 2) + previous_crank_arm_inside_y,
    DIAL_X_POSITION - (canvas.width() / 2) + previous_crank_arm_outside_x,
    DIAL_Y_POSITION - ((canvas.height() / 3) * 2) + previous_crank_arm_outside_y,
    COLOUR_BLACK
  );
  canvas.drawLine(previous_wheel_arm_inside_x, previous_wheel_arm_inside_y, previous_wheel_arm_outside_x, previous_wheel_arm_outside_y, 0);
  display.drawLine(
    DIAL_X_POSITION - (canvas.width() / 2) + previous_wheel_arm_inside_x,
    DIAL_Y_POSITION - ((canvas.height() / 3) * 2) + previous_wheel_arm_inside_y,
    DIAL_X_POSITION - (canvas.width() / 2) + previous_wheel_arm_outside_x,
    DIAL_Y_POSITION - ((canvas.height() / 3) * 2) + previous_wheel_arm_outside_y,
    COLOUR_BLACK
  );
  
  //calculate crank speed arm
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
  //calcualte wheel speed arm
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

  //display section
  //write canvas to screen
  display.drawBitmap(DIAL_X_POSITION - (canvas.width() / 2), DIAL_Y_POSITION - ((canvas.height() / 3) * 2), canvas.getBuffer(), canvas.width(), canvas.height(), COLOUR_GREEN);
  //current number of rotations
  char numerical_text[16];
  itoa(rotation_counter, numerical_text, 10);
  display.fillCircle(DIAL_X_POSITION, DIAL_Y_POSITION, DIAL_ROTATION_COUNTER_RADIUS, COLOUR_RED);
  displayCenteredText(
    DIAL_X_POSITION, DIAL_Y_POSITION + 5, numerical_text, COLOUR_WHITE, COLOUR_RED, DIAL_ROTATION_COUNTER_TEXT_SCALE
  );


  //simulate
  rotation_counter++;
  crank_speed+=0.25;
  wheel_speed+=0.5;
  //sleep
  delay(50);
}

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
