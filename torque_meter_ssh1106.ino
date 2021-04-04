
// Torque Meter using SSH1106 display
//
//
//   Before turning on the torque meter, make sure there is no load as it 
//   will zero the scale when it boots up.
//
//   To access the Menu, press and hold the button switch.
//   Menu options will cycle.  Release button to make selection.
//   Menu options are:
//       1. Toggle Display between torque reading and graph
//       2. Zero scale
//       3. Exit
//
//   To toggle between torque reading and graph view, press and hold down the 
//   button.  Release when the view changes.  Do not continue holding down the
//   button as it will zero the meter.
//
//   To zero the meter again, press and hold down the button until the Zero Scale
//   displays.
//
// Version 1.04
//    - update button action to have scroll through menu options

#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>  // OLED

#include <splash.h>
#include "HX711.h"

//
// Wiring Up Circuit
//
// Load Cell to HX711
//    Red to E+, Black to E-, White to A-, Green to A+
// HX711 to Arduino
//    GND to GND, DT to D2, SCK to D3, VCC to 3.3V
// SSH1106 to Arduino
//    GND to GND, VCC to 5V, SCL to A5, SDA to A4
// Battery to Switch to Arduino
//    Battery Pos to Switch, Battery Neg to GND, Switch to VIN
// Button Switch
//    Leg 1 to D4, Leg 2 to GND
//


// Button Switch
const int SWITCH_PIN = 4;

// HX711 Load Cell
#include "HX711.h"
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

HX711 scale;
float calibration_factor = 129054;  // Calibrate scale to ounce (20g = 0.705oz)
float arm_length = 29.5 / 25.4;     // arm length in inches

const int reading_delay = 10;


// SH1106 OLED Display
U8G2_SH1106_128X64_NONAME_1_HW_I2C display(U8G2_R0);

void display_setup()
{
  display.begin();
}

void display_title_screen()
{
  
  display.firstPage();
  do {
    //display.setFont(u8g2_font_helvB14_tf);
    display.setFont(u8g2_font_logisoso32_tf);
    display.drawStr(10, 36, "Torque");
    display.drawStr(10, 64, "Meter");
  } while( display.nextPage() );
  
}

void display_tare_screen()
{
  display.firstPage();
  do {
    display.setFont(u8g2_font_helvB14_tf);
    display.drawStr(20, 30, "Zeroing");
    display.drawStr(20, 50, "Scale");
  } while (display.nextPage());
}

void display_torque(float torque)
{
  display.firstPage();
  do {
    display.setFont(u8g2_font_logisoso32_tf);
    display.setCursor(30,50);
    display.print(torque);
  } while ( display.nextPage() );
}

void display_menu(int option)
{
  display.firstPage();
  do {
    display.setFont(u8g2_font_helvB14_tf);

    switch (option) {
      case 1:
        display.drawStr(10, 20, "Menu - 1");
        display.drawStr(20, 60, "Set Display");
        break;
      case 2:
        display.drawStr(10, 20, "Menu - 2");
        display.drawStr(20, 60, "Zero Scale");
        break;
      case 3:
        display.drawStr(10, 20, "Menu - 3");
        display.drawStr(20, 60, "Exit");
    }
  } while (display.nextPage());
}


// History Graph

const int     HISTORY_SIZE = 57;

const float   VALUE_MIN     = -0.5;   // min and max values to graph
const float   VALUE_MAX     = 2.5;
const int     Y_AXIS_HEIGHT = 64;     // vertical axis height

int history_data[HISTORY_SIZE];

void history_graph_setup()
{
  for (int i=0; i<HISTORY_SIZE; i++)
    history_data[i] = Y_AXIS_HEIGHT;
}

void history_graph_add(float value)
{
  static int i = 0;
  int data;

  //data = Y_AXIS_HEIGHT * ((value - VALUE_MIN) / (VALUE_MAX - VALUE_MIN));
  data = Y_AXIS_HEIGHT * ((VALUE_MAX - value) / (VALUE_MAX - VALUE_MIN));
  
  if ( i == HISTORY_SIZE ) {
    for (int j=0; j<HISTORY_SIZE-1; j++) {
      history_data[j] = history_data[j+1];
    }
    i = HISTORY_SIZE - 1;
  }

  history_data[i] = data;
  i++;
}

void graph_draw_axis(int y_start, int y_end)
{
  float tick;
  
  // Y Axis and Ticks
  display.drawLine(7, 0, 7, 100);
  for (int i = y_start; i <= y_end; i++) {
    tick = (float) i;
    tick = Y_AXIS_HEIGHT * ( (tick - VALUE_MIN) / (VALUE_MAX - VALUE_MIN));
    display.drawPixel(6, Y_AXIS_HEIGHT - tick);
  }

  // X Axis
  display.drawPixel(27, 63);
  display.drawPixel(47, 63);
  display.drawPixel(67, 63);
  display.drawPixel(87, 63);
  display.drawPixel(107, 63);
  display.drawLine(7, 62, 120, 62);
}

void history_graph_display(float reading)
{
  display.setFont(u8g2_font_helvB14_tf);
  
  display.firstPage();
  
  do {
    graph_draw_axis(VALUE_MIN, VALUE_MAX);
    
    display.setCursor(80, 20);
    display.println((float) reading, 2);
  
    for(int i=0; i < HISTORY_SIZE; i++) {
      int x = (2*i) + 7;
      int y = history_data[i];
      display.drawLine(x, y, x+2, y);
    }
  } while (display.nextPage());
}



void setup() {

  Serial.begin(38400);

  pinMode(SWITCH_PIN, INPUT_PULLUP); // Switch Pin in Input

  Serial.println("Torque Meter");

  Serial.println("... setup");
  display_setup();
  history_graph_setup();
  
  Serial.println("... title screen");
  display_title_screen();

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.println("Before setting up the scale:");
  
  if (scale.is_ready()) {
    Serial.println("read: \t\t");
    Serial.println(scale.read());
  } else {
    Serial.println("HX711 not found.");
  }

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(10));

  // Set the calibration factor
  scale.set_scale(calibration_factor);

  // Zero the scale
  display_tare_screen();
  scale.tare();

  Serial.println("Readings:");
}

const int MENU_OPTION_HOLD = 3;
const int MENU_NUM_OPTIONS = 3;  // mode, zero, exit

void loop() {
  // put your main code here, to run repeatedly:
  static int count = 0;
  count++;

  static int button_down_count = 0;
  static int button_state = 0;
  static int display_mode = 0;

  float torque;
  static int   menu_option = 0;

  // Check switch
  button_state = digitalRead(SWITCH_PIN); // Read the status of the switch

  switch(button_state) {
    case LOW:
      button_down_count++;
      Serial.print("button_down_count ");
      Serial.println(button_down_count);

      //menu_option = ((button_down_count + MENU_OPTION_HOLD - 1) / MENU_OPTION_HOLD) % (MENU_NUM_OPTIONS);

      menu_option = ((button_down_count / MENU_OPTION_HOLD) % MENU_NUM_OPTIONS) + 1;
      break;
      
    case HIGH:

      switch (menu_option) {
         case 1:
           display_mode = !(display_mode);
           break;

         case 2:
           display_tare_screen();
           scale.tare();
           delay(reading_delay);
           break;

         case 3:
           // exit without changing anything
           break;
           
         default:
           break;
      }

      button_down_count = 0;
      menu_option       = 0;
      break;
  }

  // Take a load cell reading
  Serial.print("count:\t");
  Serial.print(count);
  Serial.print("\traw: ");
  Serial.print(scale.read(),1);
  Serial.print("\t\tweight: ");
  Serial.print(scale.get_units(), 3);
  
  Serial.print("\t torque: ");
  torque = scale.get_units(5) * arm_length;
  Serial.println(torque, 3);

  // Decide whether to show button down
  // or the torque screens
  if (menu_option > 0) {
    display_menu(menu_option);
  } else {
    if (display_mode == 0) {
      display_torque(torque);
    } else {
      history_graph_add(torque);
      history_graph_display(torque);
    }
  }
  
  scale.power_down();              // put the ADC in sleep mode
  delay(reading_delay);
  scale.power_up();
}
