#include <M5StickCPlus.h>

void setup() {
  // put your setup code here, to run once:
  M5.begin();
  
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(MAGENTA);
  M5.Lcd.setRotation(1);
  M5.Lcd.print("Hello world!");
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
