#include <Arduino.h>
#include <stdint.h>

#define LED_RED_PIN 48
#define LED_GREEN_PIN 50
#define BUTTON_PIN 52
#define TONE_PIN 2 TIOA0
#define DEBOUNCE_DELAY 50

static Tc *chTC = TC0;
static uint32_t chNo = 0;

unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long scheduledMillis = 0;
unsigned long lastDebounceTime = 0;  

int reactionMillis = 0;
int redState = LOW;
int greenState = LOW;
int buttonState = LOW;
int lastState = LOW;
int trial = 0;

void onRelease() {
  reactionMillis = millis() - scheduledMillis;
  Serial.print("player=1 ");
  Serial.print("trial=");
  Serial.print(trial);
  Serial.print(" reaction=");
  Serial.println(reactionMillis);
  if (reactionMillis >= 0) {
    greenState = HIGH;
  } else {
    redState = HIGH;
  } 
}

void onPress() {
  scheduledMillis = millis() + random(2000,3000);
  trial++;
  Serial.print("player=1 ");
  Serial.print("trial=");
  Serial.print(trial);
  Serial.print(" scheduled=");
  Serial.println(scheduledMillis);
  redState = LOW;
  greenState = LOW;
}

void configureToneTimer() {
  // Configure TONE_PIN pin as timer output
  pmc_enable_periph_clk( ID_PIOB ) ;
  int result = PIO_Configure( PIOB,
			      PIO_PERIPH_B,
			      PIO_PB25B_TIOA0,
			      PIO_DEFAULT);
  // Serial.println(result);

  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(ID_TC0);
  TC_Configure(chTC, chNo,
	       TC_CMR_TCCLKS_TIMER_CLOCK4 |
	       TC_CMR_WAVE |         // Waveform mode
	       TC_CMR_WAVSEL_UP_RC | // Counter running up and reset when equals to RC
	       TC_CMR_ACPA_SET |     // RA compare sets TIOA
	       TC_CMR_ACPC_CLEAR );  // RC compare clears TIOA
  chTC->TC_CHANNEL[chNo].TC_IER=TC_IER_CPCS;  // RC compare interrupt
  chTC->TC_CHANNEL[chNo].TC_IDR=~TC_IER_CPCS;
}

void setFrequencytone(uint32_t frequency)
{
  if(frequency < 0 || frequency > 100001) {
    TC_Stop(chTC, chNo);
    return;
  }
  const uint32_t rc = VARIANT_MCK / 128 / frequency; 
  //const uint32_t ra = rc >> 1; // 50% duty cycle 
  const uint32_t ra = rc >> 2; // 20% duty cycle
  TC_Stop(chTC, chNo);
  TC_SetRA(chTC, chNo, ra);
  TC_SetRC(chTC, chNo, rc);
  TC_Start(chTC, chNo);
}

void playTone() {
  
}

void sendStatistics() {

}

void setup() {
  Serial.begin(9600);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  configureToneTimer();
  //setFrequencytone(1000); // example with 1000 Hz 50% cycle duty
  sendStatistics();
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);
  currentMillis = millis();
  if(reading != lastState) {
    lastDebounceTime = currentMillis;
  }
  if ((currentMillis - lastDebounceTime) > DEBOUNCE_DELAY) {
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