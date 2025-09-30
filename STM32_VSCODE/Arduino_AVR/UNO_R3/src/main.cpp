#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
int led = 8;
LiquidCrystal_I2C lcd(0x27,16,2); 

void setup() {
  pinMode(8, OUTPUT);
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("HELLO WORLD");
}

void loop() {
  Serial.println ("Hello");
  digitalWrite (led,HIGH);
  delay(500);
  digitalWrite(led,LOW);
  delay(500);

  lcd.setCursor(0,0);
  lcd.print("HELLO WORLD!");
}