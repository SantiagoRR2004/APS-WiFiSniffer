//#include <Arduino.h>
#include <M5Stack.h>

// put function declarations here:


void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.print("Hello world!");
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
