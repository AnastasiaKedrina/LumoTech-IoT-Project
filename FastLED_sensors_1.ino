/// @file    Noise.ino
/// @brief   Demonstrates how to use noise generation on a 2D LED matrix
/// @example Noise.ino

#include <FastLED.h>

#define NUM_LEDS 60
#define FRAMES_PER_SECOND  120
CRGB leds[NUM_LEDS];

static uint16_t x;
static uint16_t y;
static uint16_t z;

uint16_t speed = 2;  // 1-100
uint16_t scale = 1;  // 1-4000
uint16_t noise[60];

uint16_t BRIGHTNESS = 10;

//---------------------энкодер
#define CLK 2
#define DT 3
#define SW 4

int counter = 50;
int encoder_brightness_step = 20;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";
unsigned long lastButtonPress = 0;
//---------------------

String serialString = "";
int delim = 0;
int weather_num = 0;
int temperature = 0;
int move_trig, LM_state, weather_val, encoder_val = 0;

uint8_t ihue = 0;
uint8_t hue_offset = 0;
uint8_t hue = 0;
uint8_t hue_saturation = 0;
uint8_t hue_value = 0;
uint8_t color_scale = 1;

uint8_t gHue = 0;

void setup() {
  Serial.begin(9600);
  delay(3000);
  FastLED.addLeds<WS2812, 13, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  x = random16();
  y = random16();
  z = random16();

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  lastStateCLK = digitalRead(CLK);
}

void fillnoise8() {
  for (int i = 0; i < 60; i++) {
    int ioffset = scale * i;
    noise[i] = inoise8(x + ioffset, 0, z);
  }
  z += speed;

  
  for (int i = 0; i < 60; i++) {
    hue = ihue + (noise[i] >> color_scale) + hue_offset;
    hue_value = noise[i];

    if (LM_state == 0) {
      if ((temperature < 3) && (temperature > -3)) {
        hue_saturation = 50;
      } else if ((temperature < 6) && (temperature > -6)) {
        hue_saturation = 100;
      }
      if ((temperature < 10) && (temperature > -10)) {
        hue_saturation = 150;
      } else {
        hue_saturation = 255 - (noise[i] % 50);
      }
    } 
    leds[i] = CHSV(hue, hue_saturation, hue_value);
  }
}


void loop() {
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  get_encoder_value();
  set_brightness_by_encoder();

  if (Serial.available() > 0) {
    sensors_and_weather_by_serial();
  }

  set_LM_state();
  FastLED.show();
}


void set_LM_state() {
  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  if(LM_state > 1){
      hue_saturation = 255;
  }
  switch (LM_state) {
    case 8:
      juggle();
      break;
    case 7:
    // голубой
      fillnoise8();
      temperature = -1;
      weather_num = 1;
      set_weather_num();
      set_color_by_temperature();
      break;
    case 6:
      sinelon();
      break;
    case 5:
    // красный
      fillnoise8();
      temperature = 30;
      weather_num = 2;
      set_weather_num();
      set_color_by_temperature();
      break;
    case 4:
      confetti();
      break;
    case 3:
    // синий
      fillnoise8();
      temperature = -30;
      weather_num = 1;
      set_weather_num();
      set_color_by_temperature();
      break;
    case 2:
      rainbow();
      break;
    case 1:
    // белый
      ihue = 0;
      hue_saturation = 0;
      fillnoise8();
      break;
    case 0:
    // цвета зависят от погоды
      fillnoise8();
      set_weather_num();
      set_color_by_temperature();
      break;
    default:
      fillnoise8();
      set_weather_num();
      set_color_by_temperature();
      break;
  }
}

void set_weather_num() {
  switch (weather_num) {
    case 2:
      // Rain, Snow
      speed = 4;
      scale = 150;
      color_scale = 5;
      hue_offset = 0;
      break;

    case 1:
      // Clouds
      speed = 3;
      scale = 50;
      color_scale = 2;
      hue_offset = -20;
      break;

    case 0:
      // Clear
      speed = 1;
      scale = 10;
      color_scale = 1;
      hue_offset = -50;
      break;

    default:
      speed = 4;
      scale = 150;
      color_scale = 5;
      hue_offset = 0;
      break;
  }
}

void set_color_by_temperature() {
  // 0 желтый
  // 40 красный
  // 50 розовый
  // 60 фиолетовый
  // 110 синий
  // 130 голубой
  // 220 зеленый
  if (temperature >= 0) {
    // для теплой погоды - теплые цвета
    ihue = temperature + 60;
  } else {
    // для холодной погоды - холодные цвета
    ihue = temperature * 2 + 230;
  }
}

void sensors_and_weather_by_serial() {
  // на вход строка из LM_state, temperature, weather_num
  char separator = ',';
  serialString = Serial.readString();
  delim = 0;

  delim = serialString.indexOf(',');
  LM_state = serialString.substring(0, delim).toInt();
  serialString = serialString.substring(delim + 1, serialString.length());

  delim = serialString.indexOf(separator);
  temperature = serialString.substring(0, delim).toFloat();
  serialString = serialString.substring(delim + 1, serialString.length());

  delim = serialString.indexOf(separator);
  weather_num = serialString.substring(0, delim).toInt();
  serialString = serialString.substring(delim + 1, serialString.length());

  // Serial.print(LM_state);
  // Serial.print(separator);
  // Serial.print(temperature);
  // Serial.print(separator);
  // Serial.println(weather_num);
}

void get_encoder_value() {
  currentStateCLK = digitalRead(CLK);
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
    if (digitalRead(DT) != currentStateCLK) {
      counter = counter - encoder_brightness_step;
    } else {
      counter = counter + encoder_brightness_step;
    }
    // Serial.print("Counter: ");
    // Serial.println(counter);
  }
  lastStateCLK = currentStateCLK;
  int btnState = digitalRead(SW);

  if (btnState == LOW) {
    if (millis() - lastButtonPress > 50) {
      // Serial.println("Button pressed!");
      LM_state += 1;
      if (LM_state>8){ LM_state = 0;}
    }
    lastButtonPress = millis();
  }
}

void set_brightness_by_encoder() {
  if (counter > 255) {
    counter = 255;
  } else if (counter < 0) {
    counter = 0;
  }
  BRIGHTNESS = counter;
  FastLED.setBrightness(BRIGHTNESS);
}





void rainbow() 
{
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  fadeToBlackBy( leds, NUM_LEDS, 1);
  int pos = beatsin16( 13, 0, NUM_LEDS-1);
  leds[pos] += CHSV( gHue, 255, 192);
}


void juggle() {
  fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 20;
  }
}
