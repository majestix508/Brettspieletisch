// FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/
#include <lvgl.h>

#include <TFT_eSPI.h>

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen - Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>

#include <FastLED.h>
#include <image.h>
#include <Preferences.h>

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

Preferences preferences;

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

#define DATA_PIN 22
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 150
#define BRIGHTNESS          96
CRGB leds[NUM_LEDS];


//Table Positions and Colors
typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
}rgb_color;

// 0 = TOP
// 1 = BOTTOM
// 2 = LEFT
// 3 = RIGHT
// 4 = LEFT_TOP
// 5 = LEFT_BOTTOM
// 6 = RIGHT_TOP
// 7 = RIGHT_BOTTOM
rgb_color g_table_colors[8];

bool effect_active=false;
int effect_nr = 1;
bool active_top, active_bottom, active_left, active_right, active_left_top, active_left_bottom, active_right_top, active_right_bottom=false;

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

// Get the Touchscreen data
//void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
//  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
//  if(touchscreen.tirqTouched() && touchscreen.touched()) {
//    // Get Touchscreen points
//    TS_Point p = touchscreen.getPoint();
//    // Calibrate Touchscreen points with map function to the correct width and height
//    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
//    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
//    z = p.z;
//
//    data->state = LV_INDEV_STATE_PRESSED;
//
//    // Set the coordinates
//    data->point.x = x;
//    data->point.y = y;
//
//    // Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
//    /* Serial.print("X = ");
//    Serial.print(x);
//    Serial.print(" | Y = ");
//    Serial.print(y);
//    Serial.print(" | Pressure = ");
//    Serial.print(z);
//    Serial.println();*/
//  }
//  else {
//    data->state = LV_INDEV_STATE_RELEASED;
//  }
//}

void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();

    // Advanced Touchscreen calibration, LEARN MORE » https://RandomNerdTutorials.com/touchscreen-calibration/
    float alpha_x, beta_x, alpha_y, beta_y, delta_x, delta_y;

    // REPLACE WITH YOUR OWN CALIBRATION VALUES » https://RandomNerdTutorials.com/touchscreen-calibration/
    alpha_x = -0.000;
    beta_x = 0.090;
    delta_x = -37.107;
    alpha_y = 0.067;
    beta_y = 0.000;
    delta_y = -12.906;

    x = alpha_y * p.x + beta_y * p.y + delta_y;
    // clamp x between 0 and SCREEN_WIDTH - 1
    x = max(0, x);
    x = min(SCREEN_WIDTH - 1, x);

    y = alpha_x * p.x + beta_x * p.y + delta_x;
    // clamp y between 0 and SCREEN_HEIGHT - 1
    y = max(0, y);
    y = min(SCREEN_HEIGHT - 1, y);

    // Basic Touchscreen calibration points with map function to the correct width and height
    //x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    //y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);

    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;

    // Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
    //Serial.print("X = ");
    //Serial.print(x);
    //Serial.print(" | Y = ");
    //Serial.print(y);
    //Serial.print(" | Pressure = ");
    //Serial.print(z);
    //Serial.println();
    // Print the coordinates on the screen
    //String touch_data = "X = " + String(x) + "\nY = " + String(y) + "\nZ = " + String(z);
    //lv_label_set_text(text_label_touch, touch_data.c_str());
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void readPreferences() {
  
  preferences.begin("app", false);

  //defaults
  uint8_t def_r =0;
  uint8_t def_g =255;
  uint8_t def_b =0;

  for(int i=0;i<8;i++){
    // 0 = TOP
    // 1 = BOTTOM
    // 2 = LEFT
    // 3 = RIGHT
    // 4 = LEFT_TOP
    // 5 = LEFT_BOTTOM
    // 6 = RIGHT_TOP
    // 7 = RIGHT_BOTTOM
    
    String key;
    switch (i) {
      case 0:
        key="TOP_";
        break;
      case 1:
        key="BOTTOM_";
        break;
      case 2:
        key="LEFT_";
        break;
      case 3:
        key="RIGHT_";
        break;
      case 4:
        key="LEFT_TOP_";
        break;
      case 5:
        key="LEFT_BOTTOM_";
        break;
      case 6:
        key="RIGHT_TOP_";
        break;
      case 7:
        key="RIGHT_BOTTOM_";
        break;
    }
    String keyR, keyG, keyB;
    keyR = key+"R";
    keyG = key+"G";
    keyB = key+"B";

    g_table_colors[i].r = preferences.getUInt(keyR.c_str(),def_r);
    g_table_colors[i].g = preferences.getUInt(keyG.c_str(),def_g);
    g_table_colors[i].b = preferences.getUInt(keyB.c_str(),def_b);

    Serial.println(key);
    Serial.println(keyR);
    Serial.println(keyG);
    Serial.println(keyB);
    Serial.println(g_table_colors[i].r);
    Serial.println(g_table_colors[i].g);
    Serial.println(g_table_colors[i].b);

  }
  preferences.end();
}

void writePreferences() {
  
  //To clean the non volantile storage over time!
  //nvs_flash_erase(); // erase the NVS partition and...
  //nvs_flash_init(); // initialize the NVS partition.

  preferences.begin("app", false);
  preferences.clear();
  
  String key;
  for(int i=0;i<8;i++){
    // 0 = TOP
    // 1 = BOTTOM
    // 2 = LEFT
    // 3 = RIGHT
    // 4 = LEFT_TOP
    // 5 = LEFT_BOTTOM
    // 6 = RIGHT_TOP
    // 7 = RIGHT_BOTTOM
    
    String key;
    switch (i) {
      case 0:
        key="TOP_";
        break;
      case 1:
        key="BOTTOM_";
        break;
      case 2:
        key="LEFT_";
        break;
      case 3:
        key="RIGHT_";
        break;
      case 4:
        key="LEFT_TOP_";
        break;
      case 5:
        key="LEFT_BOTTOM_";
        break;
      case 6:
        key="RIGHT_TOP_";
        break;
      case 7:
        key="RIGHT_BOTTOM_";
        break;
    }
    String keyR, keyG, keyB;
    keyR = key+"R";
    keyG = key+"G";
    keyB = key+"B";

    preferences.putUInt(keyR.c_str(),g_table_colors[i].r);
    preferences.putUInt(keyG.c_str(),g_table_colors[i].g);
    preferences.putUInt(keyB.c_str(),g_table_colors[i].b);

    Serial.println(key);
    Serial.println(keyR);
    Serial.println(keyG);
    Serial.println(keyB);
    Serial.println(g_table_colors[i].r);
    Serial.println(g_table_colors[i].g);
    Serial.println(g_table_colors[i].b);

  }
  preferences.end();

}

/////////////////////////////////////////////////////////////////////////////////////////////

void lv_clear_screen() {
  lv_obj_clean(lv_scr_act());
}

//////////////////////////////////////////////////////////////////////////////////////
//  Color Picker

enum {SLIDER_R =0, SLIDER_G, SLIDER_B};

typedef struct {
  uint8_t slider_type;
  lv_obj_t *label;
}rgb_mixer_t;

lv_obj_t * colorPickerRect;

uint8_t cp_r,cp_g,cp_b;


void slider_callback(lv_event_t* e)
{

    lv_obj_t * slider = (lv_obj_t*)lv_event_get_target(e);
    rgb_mixer_t *user_data = (rgb_mixer_t *)lv_event_get_user_data(e);
    int32_t value = lv_slider_get_value(slider);
    lv_label_set_text_fmt(user_data->label,"%d",value);
    if (user_data->slider_type == SLIDER_R) {
      cp_r = value;
    } else if (user_data->slider_type == SLIDER_G) {
      cp_g = value;
    }else if (user_data->slider_type == SLIDER_B) {
      cp_b = value;
    }
    lv_obj_set_style_bg_color(colorPickerRect, lv_color_make(cp_r,cp_g,cp_b),LV_PART_MAIN);
}

void backBtn_Callback(lv_event_t* e) {
   lv_event_code_t code = lv_event_get_code(e);
   if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("back Clicked");
        lv_clear_screen();
        select_TablePosition();
    }   
}

void saveBtn_Callback(lv_event_t *e) {
   lv_event_code_t code = lv_event_get_code(e);
   int *user_data = (int*)lv_event_get_user_data(e);
   if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("save Clicked with data: %d", *user_data);
        //save value and go back
        lv_color_t slider_color = lv_obj_get_style_bg_color(colorPickerRect,LV_PART_MAIN);
        //LV_LOG_USER("color r: %d", slider_color.red);
        //LV_LOG_USER("color g: %d", slider_color.green);
        //LV_LOG_USER("color b: %d", slider_color.blue);

        g_table_colors[*user_data].r = slider_color.red;
        g_table_colors[*user_data].g = slider_color.green;
        g_table_colors[*user_data].b = slider_color.blue;
         
        writePreferences();

        show_position();

        lv_clear_screen();
        show_table_screen();
   }
}

void Color_slider(char * tablePos) {

    //Populate Values!
    static int color_position=0;
    if (strcmp(tablePos,"Top")==0){
        color_position=0;
    } else if (strcmp(tablePos,"Bottom")==0){
        color_position=1;
    } else if (strcmp(tablePos,"Left")==0){
        color_position=2;
    } else if (strcmp(tablePos,"Right")==0){
        color_position=3;
    } else if (strcmp(tablePos,"Left-Top")==0){
        color_position=4;
    } else if (strcmp(tablePos,"Left-Bottom")==0){
        color_position=5;
    } else if (strcmp(tablePos,"Right-Top")==0){
        color_position=6;
    } else if (strcmp(tablePos,"Right-Bottom")==0){
        color_position=7;
    }

    static rgb_mixer_t r, g, b;
     r.slider_type = SLIDER_R;
     g.slider_type = SLIDER_G;
     b.slider_type = SLIDER_B;

   /*Create sliders*/
     lv_obj_t* slider_r = lv_slider_create(lv_scr_act());
     lv_obj_t* slider_g = lv_slider_create(lv_scr_act());
     lv_obj_t* slider_b = lv_slider_create(lv_scr_act());

     //Set value range for slider
     lv_slider_set_range(slider_r,0,255);
     lv_slider_set_range(slider_g,0,255);
     lv_slider_set_range(slider_b,0,255);

 /*Align sliders*/
     lv_obj_align(slider_r,LV_ALIGN_TOP_MID,0, 40);
     lv_obj_align_to(slider_g, slider_r, LV_ALIGN_TOP_MID,0,40);
     lv_obj_align_to(slider_b, slider_g, LV_ALIGN_TOP_MID, 0, 40);

   lv_obj_t * heading = lv_label_create(lv_scr_act());
   lv_label_set_text_fmt(heading, "Color Picker for %s",tablePos);
   lv_obj_align(heading,LV_ALIGN_TOP_MID,0,10);



/* Create a base object to use it as rectangle */
     colorPickerRect = lv_obj_create(lv_scr_act());
     lv_obj_set_size(colorPickerRect, 120 , 80);
     lv_obj_align_to(colorPickerRect, slider_b, LV_ALIGN_TOP_LEFT, 0, 30);
     lv_obj_set_style_border_color(colorPickerRect,lv_color_black(),LV_PART_MAIN);
     lv_obj_set_style_border_width(colorPickerRect,3,LV_PART_MAIN);

    //Back-Button
     lv_obj_t *backBtn = lv_button_create(lv_scr_act());
     lv_obj_set_size(backBtn, 120, 30);
     lv_obj_align_to(backBtn, slider_b, LV_ALIGN_TOP_RIGHT, 0, 30);
    lv_obj_t *btnlabel = lv_label_create(backBtn);
    lv_label_set_text(btnlabel, "<< Back");
    lv_obj_center(btnlabel);
    lv_obj_add_event_cb(backBtn, backBtn_Callback, LV_EVENT_CLICKED, NULL);

    //Save-Button
    lv_obj_t *saveBtn = lv_button_create(lv_scr_act());
    lv_obj_set_size(saveBtn, 120, 30);
     lv_obj_align_to(saveBtn, backBtn, LV_ALIGN_BOTTOM_MID, 0, 40);
    lv_obj_t *btnlabelS = lv_label_create(saveBtn);
    lv_label_set_text(btnlabelS, "Save");
    lv_obj_center(btnlabelS);
    lv_obj_add_event_cb(saveBtn, saveBtn_Callback, LV_EVENT_CLICKED, &color_position);


     lv_obj_set_style_bg_color(slider_r,lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
     lv_obj_set_style_bg_color(slider_r,lv_palette_main(LV_PALETTE_RED), LV_PART_KNOB);
     lv_obj_set_style_bg_color(slider_g,lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
     lv_obj_set_style_bg_color(slider_g,lv_palette_main(LV_PALETTE_GREEN), LV_PART_KNOB);
     lv_obj_set_style_bg_color(slider_b,lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
     lv_obj_set_style_bg_color(slider_b,lv_palette_main(LV_PALETTE_BLUE), LV_PART_KNOB);

    r.label = lv_label_create(lv_scr_act());
    lv_label_set_text(r.label, "0");
    lv_obj_align_to(r.label, slider_r, LV_ALIGN_OUT_TOP_MID,0,0);

    g.label = lv_label_create(lv_scr_act());
    lv_label_set_text(g.label, "0");
    lv_obj_align_to(g.label, slider_g, LV_ALIGN_OUT_TOP_MID,0,0);

    b.label = lv_label_create(lv_scr_act());
    lv_label_set_text(b.label, "0");
    lv_obj_align_to(b.label, slider_b, LV_ALIGN_OUT_TOP_MID,0,0);

    lv_obj_add_event_cb(slider_r,slider_callback,LV_EVENT_VALUE_CHANGED, &r );
    lv_obj_add_event_cb(slider_g,slider_callback,LV_EVENT_VALUE_CHANGED, &g );
    lv_obj_add_event_cb(slider_b,slider_callback,LV_EVENT_VALUE_CHANGED, &b );


    lv_slider_set_value(slider_r,g_table_colors[color_position].r, LV_ANIM_OFF);
    lv_label_set_text_fmt(r.label,"%d",g_table_colors[color_position].r);
    lv_slider_set_value(slider_g,g_table_colors[color_position].g, LV_ANIM_OFF);
    lv_label_set_text_fmt(g.label,"%d",g_table_colors[color_position].g);
    lv_slider_set_value(slider_b,g_table_colors[color_position].b, LV_ANIM_OFF);
    lv_label_set_text_fmt(b.label,"%d",g_table_colors[color_position].b);
    lv_obj_set_style_bg_color(colorPickerRect, lv_color_make(g_table_colors[color_position].r,g_table_colors[color_position].g,g_table_colors[color_position].b),LV_PART_MAIN);

    cp_r = g_table_colors[color_position].r;
    cp_g =g_table_colors[color_position].g;
    cp_b =g_table_colors[color_position].b;

}

//////////////////////////////////////////////////////////////////////////////////////
// Screen to set color for table position
// Table Positions:
// T = Top B = BOTTOM
// L = LEFT R = RIGHT
// LT = LEFT-TOP LB = LEF-BOTTOM
// RT = RIGHT-TOP RB= RIGHT-BOTTOM

lv_obj_t * selectPos_dd; //DropDown global

void dropDownSelect_Callback(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t*)lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        LV_LOG_USER("Option: %s", buf);
    }
}

void tablePosSelect_Callback(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t*)lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        char buf[32];
        lv_dropdown_get_selected_str(selectPos_dd,buf,sizeof(buf));
        LV_LOG_USER("Select - Option: %s", buf);
        lv_clear_screen();
        Color_slider(buf);
    }

}

void select_TablePosition() {
  lv_obj_t * heading = lv_label_create(lv_scr_act());
  lv_label_set_text(heading, "Select Table Position");
  lv_obj_align(heading,LV_ALIGN_TOP_MID,0,10);

  selectPos_dd = lv_dropdown_create(lv_screen_active());
  lv_dropdown_set_options(selectPos_dd, "Top\n"
                            "Bottom\n"
                            "Left\n"
                            "Right\n"
                            "Left-Top\n"
                            "Left-Bottom\n"
                            "Right-Top\n"
                            "Right-Bottom\n");

    lv_obj_align(selectPos_dd, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_event_cb(selectPos_dd, dropDownSelect_Callback, LV_EVENT_ALL, NULL);

  //Select-Button
  lv_obj_t *selectBtn = lv_button_create(lv_scr_act());
  lv_obj_set_size(selectBtn, 300, 80);
  lv_obj_align(selectBtn, LV_ALIGN_TOP_MID, 0, 130);
  lv_obj_t *btnlabel = lv_label_create(selectBtn);
  lv_label_set_text(btnlabel, "Select");
  lv_obj_center(btnlabel);
  lv_obj_add_event_cb(selectBtn, tablePosSelect_Callback, LV_EVENT_ALL, NULL);

}


//////////////////////////////////////////////////////////////////////////////////////
// Screen to StartGame or set some values

void mainscreen_Callback(lv_event_t *e) {
   lv_event_code_t code = lv_event_get_code(e);
   char *user_data = (char *)lv_event_get_user_data(e);
   if(code == LV_EVENT_CLICKED) {
        if (strcmp(user_data,"start")==0){
        LV_LOG_USER("start Clicked");
          lv_clear_screen();
          show_table_screen();
        } else if (strcmp(user_data,"settings")==0){
        LV_LOG_USER("settings Clicked");
          lv_clear_screen();
          settings_screen();
        }
    }   
}

void main_screen(void) {

  lv_obj_t * heading = lv_label_create(lv_scr_act());
  lv_label_set_text(heading, "Stockis Brettspieletisch");
  lv_obj_align(heading,LV_ALIGN_TOP_MID,0,10);

  //Start-Button
  lv_obj_t *startBtn = lv_button_create(lv_scr_act());
  lv_obj_set_size(startBtn, 300, 80);
  lv_obj_align_to(startBtn, heading, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_t *btnlabel = lv_label_create(startBtn);
  lv_label_set_text(btnlabel, "Start");
  lv_obj_add_event_cb(startBtn, mainscreen_Callback, LV_EVENT_ALL, (void*)"start");
  lv_obj_center(btnlabel);

  //Settings-Button
  lv_obj_t *settingsBtn = lv_button_create(lv_scr_act());
  lv_obj_set_size(settingsBtn, 300, 80);
  lv_obj_align_to(settingsBtn, heading, LV_ALIGN_TOP_MID, 0, 130);
  lv_obj_t *btnlabel2 = lv_label_create(settingsBtn);
  lv_label_set_text(btnlabel2, "Settings");
  lv_obj_add_event_cb(settingsBtn, mainscreen_Callback, LV_EVENT_ALL, (void*)"settings");
  lv_obj_center(btnlabel2);

}

//////////////////////////////////////////////////////////////////////////////////////

// New screen - tab view and some buttons
static void event_handler_playerbtn(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  char *user_data = (char*)lv_event_get_user_data(e);
  if(code == LV_EVENT_CLICKED) {
    LV_LOG_USER("button: %s",user_data);
    if (strcmp(user_data,"top")==0){
      active_top = !active_top;
    } else if (strcmp(user_data,"bottom")==0){
      active_bottom = !active_bottom;
    }else if (strcmp(user_data,"left")==0){
      active_left = !active_left;
    }else if (strcmp(user_data,"right")==0){
      active_right = !active_right;
    }else if (strcmp(user_data,"left-top")==0){
      active_left_top = !active_left_top;
    }else if (strcmp(user_data,"left-bottom")==0){
      active_left_bottom = !active_left_bottom;
    }else if (strcmp(user_data,"right-top")==0){
      active_right_top = !active_right_top;
    }else if (strcmp(user_data,"right-bottom")==0){
      active_right_bottom = !active_right_bottom;
    }
    show_position();
  }
}

static void event_handler_settings(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  char* user_data = (char *)lv_event_get_user_data(e);
  if(code == LV_EVENT_CLICKED) {
    LV_LOG_USER("settings");
    if (strcmp(user_data,"color")==0){
      lv_clear_screen();
      select_TablePosition();
    } else if (strcmp(user_data,"back")==0) {
      lv_clear_screen();
      main_screen();
    }
  }
}

#define BUTTON_THICKNESS 40
//screen 320x240

void show_table_screen() {

  lv_obj_t * btn_left = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn_left, event_handler_playerbtn, LV_EVENT_ALL, (void*)"left");
  lv_obj_set_pos(btn_left,  1, 1);
  lv_obj_set_width(btn_left,BUTTON_THICKNESS);
  lv_obj_set_height(btn_left,238);
  lv_obj_add_flag(btn_left, LV_OBJ_FLAG_CHECKABLE);
  if (active_left){
    lv_obj_add_state(btn_left,LV_STATE_CHECKED);
  }

  lv_obj_t * btn_right = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn_right, event_handler_playerbtn, LV_EVENT_ALL, (void*)"right");
  lv_obj_set_pos(btn_right,  319-BUTTON_THICKNESS, 0);
  lv_obj_set_width(btn_right,BUTTON_THICKNESS);
  lv_obj_set_height(btn_right,238);
  lv_obj_add_flag(btn_right, LV_OBJ_FLAG_CHECKABLE);
  if (active_right){
    lv_obj_add_state(btn_right,LV_STATE_CHECKED);
  }

  lv_obj_t * btn_top = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn_top, event_handler_playerbtn, LV_EVENT_ALL, (void*)"top");
  lv_obj_set_pos(btn_top, 42,1);
  lv_obj_set_width(btn_top,320-40-40-4);
  lv_obj_set_height(btn_top,40);
  lv_obj_add_flag(btn_top, LV_OBJ_FLAG_CHECKABLE);
  if (active_top){
    lv_obj_add_state(btn_top,LV_STATE_CHECKED);
  }

  lv_obj_t * btn_bottom = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn_bottom, event_handler_playerbtn, LV_EVENT_ALL, (void*)"bottom");
  lv_obj_set_pos(btn_bottom, 42,240-40-2);
  lv_obj_set_width(btn_bottom,320-40-40-4);
  lv_obj_set_height(btn_bottom,40);
  lv_obj_add_flag(btn_bottom, LV_OBJ_FLAG_CHECKABLE);
  if (active_bottom){
    lv_obj_add_state(btn_bottom,LV_STATE_CHECKED);
  }

  lv_obj_t * btn_lefttop = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn_lefttop, event_handler_playerbtn, LV_EVENT_ALL, (void*)"left-top");
  lv_obj_set_pos(btn_lefttop, 42,42);
  lv_obj_set_width(btn_lefttop,40);
  lv_obj_set_height(btn_lefttop,(240-84-4)/2);
  lv_obj_add_flag(btn_lefttop, LV_OBJ_FLAG_CHECKABLE);
  if (active_left_top){
    lv_obj_add_state(btn_lefttop,LV_STATE_CHECKED);
  }

  lv_obj_t * btn_leftbottom = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn_leftbottom, event_handler_playerbtn, LV_EVENT_ALL, (void*)"left-bottom");
  lv_obj_set_pos(btn_leftbottom, 42,43 + (240-84-4)/2);
  lv_obj_set_width(btn_leftbottom,40);
  lv_obj_set_height(btn_leftbottom,(240-84-4)/2);
  lv_obj_add_flag(btn_leftbottom, LV_OBJ_FLAG_CHECKABLE);
  if (active_left_bottom){
    lv_obj_add_state(btn_leftbottom,LV_STATE_CHECKED);
  }

  lv_obj_t * btn_righttop = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn_righttop, event_handler_playerbtn, LV_EVENT_ALL, (void*)"right-top");
  lv_obj_set_pos(btn_righttop, 320-1-40-1-40,42);
  lv_obj_set_width(btn_righttop,40);
  lv_obj_set_height(btn_righttop,(240-84-4)/2);
  lv_obj_add_flag(btn_righttop, LV_OBJ_FLAG_CHECKABLE);
  if (active_right_top){
    lv_obj_add_state(btn_righttop,LV_STATE_CHECKED);
  }

  lv_obj_t * btn_rightbottom = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn_rightbottom, event_handler_playerbtn, LV_EVENT_ALL, (void*)"right-bottom");
  lv_obj_set_pos(btn_rightbottom, 320-1-40-1-40,43 + (240-84-4)/2);
  lv_obj_set_width(btn_rightbottom,40);
  lv_obj_set_height(btn_rightbottom,(240-84-4)/2);
  lv_obj_add_flag(btn_rightbottom, LV_OBJ_FLAG_CHECKABLE);
  if (active_right_bottom){
    lv_obj_add_state(btn_rightbottom,LV_STATE_CHECKED);
  }

  lv_obj_t * btn_center = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn_center, event_handler_settings, LV_EVENT_ALL, (void*)"color");
  lv_obj_set_pos(btn_center, 320/2-40, 240/2-41);
  lv_obj_set_width(btn_center,80);
  lv_obj_set_height(btn_center,40);

  lv_obj_t *color_label = lv_label_create(btn_center);
  lv_label_set_text(color_label, "Farbe");

  lv_obj_t * btn_centerB = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn_centerB, event_handler_settings, LV_EVENT_ALL, (void*)"back");
  lv_obj_set_pos(btn_centerB, 320/2-40, 240/2+1);
  lv_obj_set_width(btn_centerB,80);
  lv_obj_set_height(btn_centerB,40);

  lv_obj_t *color_label2 = lv_label_create(btn_centerB);
  lv_label_set_text(color_label2, "<< Back");

}

//////////////////////////////////////////////////////////////////////////////////////
// Settings Screen
// at the moment just a button-mask with different fastled effects

static void settings_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t*)lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_buttonmatrix_get_selected_button(obj);
        const char * txt = lv_buttonmatrix_get_button_text(obj, id);
        LV_UNUSED(txt);
        LV_LOG_USER("%s was pressed\n", txt);

        if (id == 11) { //Back
          effect_active=false;
          clear_leds();
          lv_clear_screen();
          main_screen();
        } else if (id == 10) { //Stop animation
          //Stop Animation
          LV_LOG_USER("stop action\n");

          effect_active=false;
          clear_leds();
        } else if (id == 0) {
          //start Rainbow effect
          clear_leds();
          effect_active=true;
          effect_nr=1;
        } else if (id == 1) {
          //start simple color
          clear_leds();
          effect_active=true;
          effect_nr=2;
        } else if (id == 2) {
          //start simple color-run
          clear_leds();
          effect_active=true;
          effect_nr=3;
        } else if (id == 3) {
          //start simple color-run
          clear_leds();
          effect_active=true;
          effect_nr=4;
        }
        //TODO
    }
}

static const char * btnm_map[] = {"1", "2", "3", "4", "5", "\n",
                                  "6", "7", "8", "9", "0", "\n",
                                  "Stop", "Back", ""
                                 };

void settings_screen() {

  lv_obj_t * heading = lv_label_create(lv_scr_act());
  lv_label_set_text(heading, "Settings");
  lv_obj_align(heading,LV_ALIGN_TOP_MID,0,10);

  lv_obj_t * btnm1 = lv_buttonmatrix_create(lv_screen_active());
  lv_buttonmatrix_set_map(btnm1, btnm_map);
  lv_buttonmatrix_set_button_width(btnm1, 10, 1);
  //lv_buttonmatrix_set_button_ctrl(btnm1, 10, LV_BUTTONMATRIX_CTRL_CHECKABLE);
  lv_buttonmatrix_set_button_ctrl(btnm1, 10, LV_BUTTONMATRIX_CTRL_CHECKED);
  lv_buttonmatrix_set_button_ctrl(btnm1, 11, LV_BUTTONMATRIX_CTRL_CHECKED);
  lv_obj_align(btnm1, LV_ALIGN_CENTER, 0, 30);
  lv_obj_add_event_cb(btnm1, settings_event_handler, LV_EVENT_ALL, NULL);
}

//////////////////////////////////////////////////////////////////////////////////////
// Splash-Screen

void splash_screen(void) {
  LV_IMAGE_DECLARE(stockisbrettspieletisch);
  lv_obj_t * img1 = lv_image_create(lv_screen_active());
  lv_image_set_src(img1, &stockisbrettspieletisch);
  lv_obj_align(img1, LV_ALIGN_CENTER, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////
//Fast Led stuff

void rainbow_wave(uint8_t thisSpeed, uint8_t deltaHue) {     // The fill_rainbow call doesn't support brightness levels.
 
// uint8_t thisHue = beatsin8(thisSpeed,0,255);                // A simple rainbow wave.
 uint8_t thisHue = beat8(thisSpeed,255);                     // A simple rainbow march.
  
 fill_rainbow(leds, NUM_LEDS, thisHue, deltaHue);            // Use FastLED's fill_rainbow routine.
 
} // rainbow_wave()

void pixel_run(){
  static uint8_t pos = 0;
  for(int i=0; i<NUM_LEDS;i++){
    if (pos==i){
      leds[i] = CRGB::Blue;
    } else {
      leds[i] = CRGB::Black;
    }
  }
  if (pos == NUM_LEDS-1){
    pos=0;
  }
  pos++;
}

void strip_breath(){

  static float pulseSpeed = 0.5;
  
  static uint8_t hueA = 15;  // Start hue at valueMin.
  static uint8_t satA = 230;  // Start saturation at valueMin.
  static float valueMin = 120.0;  // Pulse minimum value (Should be less then valueMax).

  static uint8_t hueB = 95;  // End hue at valueMax.
  static uint8_t satB = 255;  // End saturation at valueMax.
  static float valueMax = 255.0;  // Pulse maximum value (Should be larger then valueMin).

  static uint8_t hue = hueA;  // Do Not Edit
  static uint8_t sat = satA;  // Do Not Edit
  static float val = valueMin;  // Do Not Edit
  static uint8_t hueDelta = hueA - hueB;  // Do Not Edit
  static float delta = (valueMax - valueMin) / 2.35040238;  // Do Not Edit


  float dV = ((exp(sin(pulseSpeed * millis()/2000.0*PI)) -0.36787944) * delta);
  val = valueMin + dV;
  hue = map(val, valueMin, valueMax, hueA, hueB);  // Map hue based on current val
  sat = map(val, valueMin, valueMax, satA, satB);  // Map sat based on current val

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, sat, val);
    // You can experiment with commenting out these dim8_video lines
    // to get a different sort of look.
    leds[i].r = dim8_video(leds[i].r);
    leds[i].g = dim8_video(leds[i].g);
    leds[i].b = dim8_video(leds[i].b);
  }

}

void show_effect() {
  if (effect_nr == 1) {
    rainbow_wave(10,10);
  }else if (effect_nr == 2){
    for(int i=0; i<NUM_LEDS;i++){
      leds[i] = CRGB::Red;
    }
  } else if (effect_nr == 3) {
    pixel_run();
  } else if (effect_nr == 4) {
    strip_breath();
  }
  
  //TODO
}

void show_position() {
    CRGB l_color;

    for (int i=0; i<NUM_LEDS;i++){
      if (active_top && i>=72 && i<103){ //TOP
        l_color.r = g_table_colors[0].r;
        l_color.g = g_table_colors[0].g;
        l_color.b = g_table_colors[0].b;
        leds[i]=l_color;
      } else if (active_bottom &&  (i>=0 && i<27 || i==148 || i==149)  ){ //BOTTOM
        l_color.r = g_table_colors[1].r;
        l_color.g = g_table_colors[1].g;
        l_color.b = g_table_colors[1].b;
        leds[i]=l_color;
      } else if (active_left && i>=103 && i<148){ //LEFT
        l_color.r = g_table_colors[2].r;
        l_color.g = g_table_colors[2].g;
        l_color.b = g_table_colors[2].b;
        leds[i]=l_color;
      } else if (active_right && i>=27 && i<72) { //RIGHT
        l_color.r = g_table_colors[3].r;
        l_color.g = g_table_colors[3].g;
        l_color.b = g_table_colors[3].b;
        leds[i]=l_color;
      } else if (active_left_top && i>=103 && i<126){ // LEFT-TOP
        l_color.r = g_table_colors[4].r;
        l_color.g = g_table_colors[4].g;
        l_color.b = g_table_colors[4].b;
        leds[i]=l_color;
      } else   if (active_left_bottom && i>=126 && i<148){ //LEFT_BOTTOM
        l_color.r = g_table_colors[5].r;
        l_color.g = g_table_colors[5].g;
        l_color.b = g_table_colors[5].b;
        leds[i]=l_color;
      } else if (active_right_top && i>=50 && i<72){ //RIGHT_TOP
        l_color.r = g_table_colors[6].r;
        l_color.g = g_table_colors[6].g;
        l_color.b = g_table_colors[6].b;
        leds[i]=l_color;
      } else if (active_right_bottom && i>=27 && i<50){ //RIGHT_BOTTOM
        l_color.r = g_table_colors[7].r;
        l_color.g = g_table_colors[7].g;
        l_color.b = g_table_colors[7].b;
          leds[i]=l_color;
      } else {
        leds[i] = CRGB::Black;
      }
    }
  FastLED.show();

}

bool clearLEDS=false;

void clear_leds(){
  clearLEDS=true;
}

void intern_clear_leds() {
  //for(int i=0;i<NUM_LEDS;i++){
  //  leds[i] == CRGB::Black;
  //}
  //fill_solid(leds,NUM_LEDS,CRGB::Black);
  FastLED.clear();
  FastLED.show();
}

void fastled_main() {
    if (effect_active==true) {
      show_effect();
    }
    if (clearLEDS==true){
      intern_clear_leds();
      clearLEDS=false;
    }
}


//////////////////////////////////////////////////////////////////////////////////////

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);
  
  readPreferences();

  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
  touchscreen.setRotation(2);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
    
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // Set the callback function to read Touchscreen input
  lv_indev_set_read_cb(indev, touchscreen_read);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  // Function to draw the GUI (text, buttons and sliders)
  splash_screen();
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(3000);     // tell LVGL how much time has passed
  delay(3000);
  lv_clear_screen();
  main_screen();

}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass

  fastled_main();
  FastLED.show();  
}
