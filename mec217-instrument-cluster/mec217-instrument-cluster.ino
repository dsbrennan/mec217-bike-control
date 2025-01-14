#include "Arduino_GigaDisplay_GFX.h"
#include <math.h>

// colours
#define colour_black 0x0000
#define colour_white 0xFFFF
#define colour_red 0xD800
#define colour_green 0x06E0
#define colour_grey 0x8C71

// display
#define screen_rotate 3
#define screen_width 800
#define screen_height 480
#define char_width 6
#define char_height 8

// cluster layout
#define dial_x_position 400 // screen width / 2
#define dial_y_position 280 // screen height / 2 + 30
#define dial_start_degree -120
#define dial_end_degree 120
#define dial_radius 240 // screen height / 2
#define dial_label_start 0
#define dial_label_end 50
#define dial_label_step 5
#define dial_label_multiplier 0.80
#define dial_ticker_step 4.8 // dial end degree - dial start degree / dial label end - dail label start 
#define dial_ticker_outside_multiplier 0.95
#define dial_ticker_inside_multiplier 0.90
#define dial_canvas_arm_x_origin 240 // canvas width / 2
#define dial_canvas_arm_y_origin 240 // canvas height / 2 * 3
#define dial_arm_inside_multiplier 0.505 // ratio of dial_radius to rotation_counter_radius
#define dial_crank_arm_outside_multiplier 0.6
#define dial_wheel_arm_outside_multiplier 0.74
#define rotation_counter_radius 120 //screen_height / 4
#define rotation_counter_text_scale 15

// visual output variables
GigaDisplay_GFX display;
GFXcanvas1 canvas(screen_height, (screen_height/4)*3);// height will depend upon dial radius and start and end degree

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
  display.setRotation(screen_rotate);
  display.fillScreen(colour_black);
  //setup instrument cluster
  //ic - arc
  drawArc(dial_x_position, dial_y_position, dial_start_degree, dial_end_degree, dial_radius, 3, colour_white);
  //ic - tickers
  for (double i = dial_start_degree; i <= dial_end_degree; i+= dial_ticker_step){
    float angle = i < 0 ? -i : i;
    float radians_angle = (angle < 90 ? 90 - angle : angle - 90) * (M_PI / 180.0);
    float y_increment = dial_radius * sin(radians_angle);
    float x_increment = dial_radius * cos(radians_angle);
    display.drawLine(
      i < 0 ? dial_x_position - (dial_ticker_outside_multiplier * x_increment) : dial_x_position + (dial_ticker_outside_multiplier * x_increment),
      angle < 90 ? dial_y_position - (dial_ticker_outside_multiplier * y_increment) : dial_y_position + (dial_ticker_outside_multiplier * y_increment), 
      i < 0 ? dial_x_position - (dial_ticker_inside_multiplier * x_increment) : dial_x_position + (dial_ticker_inside_multiplier * x_increment),
      angle < 90 ? dial_y_position - (dial_ticker_inside_multiplier * y_increment) : dial_y_position + (dial_ticker_inside_multiplier * y_increment), 
      colour_white
    );
  }
  //ic - labels
  int dial_label_degrees = (dial_end_degree - dial_start_degree) / ((dial_label_end - dial_label_start) / dial_label_step);
  char numerical_text[16];
  for (int i = 0; i <= ((dial_label_end - dial_label_start) / dial_label_step); i++) {
    float label_angle = dial_start_degree + (i * dial_label_degrees);
    float angle = label_angle < 0 ? -label_angle : label_angle;
    float radians_angle =  (angle < 90 ? 90 - angle : angle - 90) * (M_PI / 180.0);
    float y_increment = dial_label_multiplier * dial_radius * sin(radians_angle);
    float x_increment = dial_label_multiplier * dial_radius * cos(radians_angle);
    itoa(dial_label_start + (i * dial_label_step), numerical_text, 10);
    displayCenteredText(
      label_angle < 0 ? dial_x_position - x_increment : dial_x_position + x_increment,
      angle < 90 ? dial_y_position - y_increment : dial_y_position + y_increment,
      numerical_text, colour_white, colour_black, 2
    );
  }
  //ic - counter 
  display.fillCircle(dial_x_position, dial_y_position, rotation_counter_radius, colour_grey);
  displayCenteredText(
    dial_x_position, dial_y_position + 5, "0", colour_white, colour_grey, rotation_counter_text_scale
  );
}


void loop() {
  //canvas section
  //clear old arms
  canvas.drawLine(previous_crank_arm_inside_x, previous_crank_arm_inside_y, previous_crank_arm_outside_x, previous_crank_arm_outside_y, 0);
  display.drawLine(
    dial_x_position - (canvas.width() / 2) + previous_crank_arm_inside_x,
    dial_y_position - ((canvas.height() / 3) * 2) + previous_crank_arm_inside_y,
    dial_x_position - (canvas.width() / 2) + previous_crank_arm_outside_x,
    dial_y_position - ((canvas.height() / 3) * 2) + previous_crank_arm_outside_y,
    colour_black
  );
  canvas.drawLine(previous_wheel_arm_inside_x, previous_wheel_arm_inside_y, previous_wheel_arm_outside_x, previous_wheel_arm_outside_y, 0);
  display.drawLine(
    dial_x_position - (canvas.width() / 2) + previous_wheel_arm_inside_x,
    dial_y_position - ((canvas.height() / 3) * 2) + previous_wheel_arm_inside_y,
    dial_x_position - (canvas.width() / 2) + previous_wheel_arm_outside_x,
    dial_y_position - ((canvas.height() / 3) * 2) + previous_wheel_arm_outside_y,
    colour_black
  );
  
  //calculate crank speed arm
  float crank_degree = dial_start_degree + (crank_speed * dial_ticker_step);
  float angle = crank_degree < 0 ? -crank_degree : crank_degree;
  float radians_angle = (angle < 90 ? 90 - angle : angle - 90) * (M_PI / 180.0);
  float y_increment = dial_radius * sin(radians_angle);
  float x_increment = dial_radius * cos(radians_angle);
  previous_crank_arm_inside_x = crank_degree < 0 ? dial_canvas_arm_x_origin - (x_increment * dial_arm_inside_multiplier) : dial_canvas_arm_x_origin + (x_increment * dial_arm_inside_multiplier);
  previous_crank_arm_inside_y = angle < 90 ? dial_canvas_arm_y_origin - (y_increment * dial_arm_inside_multiplier) : dial_canvas_arm_y_origin + (y_increment * dial_arm_inside_multiplier);
  previous_crank_arm_outside_x = crank_degree < 0 ? dial_canvas_arm_x_origin - (x_increment * dial_crank_arm_outside_multiplier) : dial_canvas_arm_x_origin + (x_increment * dial_crank_arm_outside_multiplier);
  previous_crank_arm_outside_y = angle < 90 ? dial_canvas_arm_y_origin - (y_increment * dial_crank_arm_outside_multiplier) : dial_canvas_arm_y_origin + (y_increment * dial_crank_arm_outside_multiplier);
  canvas.drawLine(previous_crank_arm_inside_x, previous_crank_arm_inside_y, previous_crank_arm_outside_x, previous_crank_arm_outside_y, 1);
  //calcualte wheel speed arm
  float wheel_degree = dial_start_degree + (wheel_speed * dial_ticker_step);
  angle = wheel_degree < 0 ? -wheel_degree : wheel_degree;
  radians_angle = (angle < 90 ? 90 - angle : angle - 90) * (M_PI / 180.0);
  y_increment = dial_radius * sin(radians_angle);
  x_increment = dial_radius * cos(radians_angle);
  previous_wheel_arm_inside_x = wheel_degree < 0 ? dial_canvas_arm_x_origin - (x_increment * dial_arm_inside_multiplier) : dial_canvas_arm_x_origin + (x_increment * dial_arm_inside_multiplier);
  previous_wheel_arm_inside_y = angle < 90 ? dial_canvas_arm_y_origin - (y_increment * dial_arm_inside_multiplier) : dial_canvas_arm_y_origin + (y_increment * dial_arm_inside_multiplier);
  previous_wheel_arm_outside_x = wheel_degree < 0 ? dial_canvas_arm_x_origin - (x_increment * dial_wheel_arm_outside_multiplier) : dial_canvas_arm_x_origin + (x_increment * dial_wheel_arm_outside_multiplier);
  previous_wheel_arm_outside_y = angle < 90 ? dial_canvas_arm_y_origin - (y_increment * dial_wheel_arm_outside_multiplier) : dial_canvas_arm_y_origin + (y_increment * dial_wheel_arm_outside_multiplier);
  canvas.drawLine(previous_wheel_arm_inside_x, previous_wheel_arm_inside_y, previous_wheel_arm_outside_x, previous_wheel_arm_outside_y, 1);

  //display section
  //write canvas to screen
  display.drawBitmap(dial_x_position - (canvas.width() / 2), dial_y_position - ((canvas.height() / 3) * 2), canvas.getBuffer(), canvas.width(), canvas.height(), colour_green);
  //current number of rotations
  char numerical_text[16];
  itoa(rotation_counter, numerical_text, 10);
  display.fillCircle(dial_x_position, dial_y_position, rotation_counter_radius, colour_red);
  displayCenteredText(
    dial_x_position, dial_y_position + 5, numerical_text, colour_white, colour_red, rotation_counter_text_scale
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
      x - ((modulo_count * size * char_width) / 2) - ((half_count - i) * size * char_width) + (size / 2),
      y - (size * char_height) / 2,
      text[i], text_colour, background_colour, size
    );
  }
}
