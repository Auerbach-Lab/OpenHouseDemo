#include <Arduino.h>
#include <limits.h>
#include <ezButton.h>

#define LED_RED_PIN 48
#define LED_GREEN_PIN 50
#define BUTTON_PIN 52

unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long scheduledMillis = 0;
unsigned long lastDebounceTime = 0;  
unsigned long debounceDelay = 50;    
int reactionMillis = 0;
int redState = LOW;
int greenState = LOW;
char buffer[64];

int buttonState = LOW;
int lastState = LOW;

void onRelease() {
  reactionMillis = millis() - scheduledMillis;
  Serial.print("reaction: ");
  Serial.println(reactionMillis);
  if (reactionMillis >= 0) {
    greenState = HIGH;
  } else {
    redState = HIGH;
  } 
}

void onPress() {
  scheduledMillis = millis() + random(2000,3000);
  Serial.print("scheduled: ");
  Serial.println(scheduledMillis);
  redState = LOW;
  greenState = LOW;
}

void playTone() {

}


void setup() {
  Serial.begin(9600);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);
  currentMillis = millis();
  if(reading != lastState) {
    lastDebounceTime = currentMillis;
  }
  if ((currentMillis - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        onRelease();
      } else {
        onPress();
      }
    }
  }
  if((currentMillis > scheduledMillis) && (currentMillis < (scheduledMillis + 300))) {
    playTone();
  }

  lastState = reading;
  digitalWrite(LED_RED_PIN, redState);
  digitalWrite(LED_GREEN_PIN, greenState);
}