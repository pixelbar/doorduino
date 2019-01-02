#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <Entropy.h>
#include <OneWire.h>
#include "ds1961.h"
#include "hexutil.c"

#define PIN_LED_GREEN 11
#define PIN_LED_RED   12
#define PIN_1WIRE     10

#define OFF    0
#define GREEN  1
#define RED    2
#define YELLOW (GREEN | RED)

OneWire ds(PIN_1WIRE);
DS1961  sha(&ds);

#define LOGO_HEIGHT   16
#define LOGO_WIDTH 16
static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
B00000000, B00110000 };

byte addr[8];

/*

  5V -----\/\/\-----+
          R=4k7     |
  10 ---------------+------------o   o-----1wire----+
                                                    |
                                                    |
  12 -----\/\/\------------------o   o------>|------+
          R=220                             red     |
                                                    |
  13 -----\/\/\------------------o   o------>|------+
          R=220                            green    |
                                                    |
 GND ----------------------------o   o--------------+

*/


void led (byte color) {
  digitalWrite(PIN_LED_GREEN, color & GREEN);
  digitalWrite(PIN_LED_RED,   color & RED);
}

void setup () {
  Serial.begin(9600);
  Serial.println("RESET");
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED,   OUTPUT);
  Entropy.Initialize();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  
  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  drawlogo();
  
  delay(2000);
}

void drawlogo(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}

void drawtext(String str) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  display.println(str);
  display.display();
}

void loop () {
  byte secret[8];
  int i;

  for (i = 0; i < 8; i++) {
    secret[i] = Entropy.random(256);
  }
  
  while (1) { /* yo dawg */
    ds.reset_search();
    if (ds.search(addr)) {
      if (OneWire::crc8(addr, 7) != addr[7]) return;
      delay(100);

      String first = hexdump(addr, 8);
  
      if (!sha.WriteSecret(addr, secret)) {
        Serial.println("ERROR");
        drawtext("error!");
        led(RED);
        delay(2000);
      } else {
        led(GREEN);
        String sec = hexdump(secret, 8);
        drawtext(first + "\n\n" + sec);
        Serial.println(first + "\n\n" + sec);
        delay(15000);
      }
      
      break;
    }

    if(millis() % 300 > 120) {
      drawtext("Reading.. \n\nPut iButton on reader");
    } else {
      drawtext("Reading... \n\nPut iButton on reader");
    }
    
    led(millis() % 150 > 120 ? OFF : RED);
  }
}

String hexdump(byte* string, int size) {
  String str = "";
  for (int i = 0; i < size; i++) {
    str = str + String(string[i] >> 4, HEX);
    str = str + String(string[i] & 0xF, HEX);
  }
  return str;
}
