#include <Arduino.h>
#include <stdint.h>

/* Tone generation for DUE
 see http://arduino.cc/forum/index.php/topic,136500.msg1029238.html#msg1029238 and 
 http://asf.atmel.com/docs/latest/sam3a/html/group__sam__drivers__pmc__group.html and
 http://asf.atmel.com/docs/latest/sam.drivers.usart.usart_synchronous_example.sam3u_ek/html/sam_pio_quickstart.html
 and http://ko7m.blogspot.com.au/2015/01/arduino-due-timers-part-1.html
 
 The Atmel Power Management Controller (pmc) optimizes power consumption by controlling
 all system and user peripheral clocks, including enabling/disabling clock inputs to 
 many peripherals and the Cortex-M Processor.
 VARIANT_MCK = 84000000 (Master ClocK freq) 

 Table showing relationship between the Timer/Counter, its channels, the IRQ to use, what 
 IRQ function must be called and the power management ID for that peripheral. 

 https://github.com/ivanseidel/DueTimer/blob/master/TimerCounter.md
 https://ww1.microchip.com/downloads/en/devicedoc/atmel-11057-32-bit-cortex-m3-microcontroller-sam3x-sam3a_datasheet.pdf
 https://github.com/arduino/ArduinoCore-sam/blob/master/variants/arduino_due_x/variant.cpp


    TC   Ch  ExClk  I/O A  I/O B  NVIC_IRQ  IRQ_handler  PMC_id
T0  TC0  0   TCLK0  TIOA0  TIOB0  TC0_IRQn  TC0_Handler  ID_TC0 << tone pin 2 is TIOA0
T1  TC0  1   TCLK1  TIOA1  TIOB1  TC1_IRQn  TC1_Handler  ID_TC1
T2  TC0  2   TCLK2  TIOA2  TIOB2  TC2_IRQn  TC2_Handler  ID_TC2
T3  TC1  0   TCLK3  TIOA3  TIOB3  TC3_IRQn  TC3_Handler  ID_TC3 
T4  TC1  1   TCLK4  TIOA4  TIOB4  TC4_IRQn  TC4_Handler  ID_TC4
T5  TC1  2   TCLK5  TIOA5  TIOB5  TC5_IRQn  TC5_Handler  ID_TC5
T6  TC2  0   TCLK6  TIOA6  TIOB6  TC6_IRQn  TC6_Handler  ID_TC6
T7  TC2  1   TCLK7  TIOA7  TIOB7  TC7_IRQn  TC7_Handler  ID_TC7 << tone pin 3 is TIOa7
T8  TC2  2   TCLK8  TIOA8  TIOB8  TC8_IRQn  TC8_Handler  ID_TC8 
  */

#define LED_RED_PIN 48
#define LED_GREEN_PIN 50
#define BUTTON_PIN 52
#define DEBOUNCE_DELAY 50

static Tc *chTC_2 = TC0;          //tone pin 2 (TIOA0) is on TC0 Ch0
static uint32_t chNo_2 = 0;       //tone pin 2 (TIOA0) is on TC0 Ch0
static uint32_t chID_2 = ID_TC0;  //tone pin 2 (TIOA0) is on TC0 Ch0
static Pio *p_2 = PIOB;
static uint32_t bit_2 = PIO_PB25B_TIOA0;

static Tc *chTC_3 = TC2;          //tone pin 3 (TIOA7) is on TC2 Ch1
static uint32_t chNo_3 = 1;       //tone pin 3 (TIOA7) is on TC2 Ch1
static uint32_t chID_3 = ID_TC7;  //tone pin 3 (TIOA7) is on TC2 Ch1
static Pio *p_3 = PIOC;
static uint32_t bit_3 = PIO_PC28B_TIOA7;

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

struct player
{

};


void configureToneTimer(Tc *chTC, uint32_t chNo, uint32_t chID, Pio *p, uint32_t bit) {
  // Configure TONE_PIN_2 pin as timer output
  pmc_enable_periph_clk( ID_PIOC ) ;    //tone pin 3 (TIOA7) is on PIOB
  int result = PIO_Configure( p,     //tone pin 3 (TIOA7) is on PIOB
			      PIO_PERIPH_B,               //tone pin 3 (TIOA7) is on PIOB
			      bit,            //tone pin 3 (TIOA7) is on PIOB, port PC28
			      PIO_DEFAULT);
  Serial.println(result);

  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(chID);          
  TC_Configure(chTC, chNo,              
	       TC_CMR_TCCLKS_TIMER_CLOCK4 |
	       TC_CMR_WAVE |         // Waveform mode
	       TC_CMR_WAVSEL_UP_RC | // Counter running up and reset when equals to RC
	       TC_CMR_ACPA_SET |     // RA compare sets TIOA
	       TC_CMR_ACPC_CLEAR );  // RC compare clears TIOA
  chTC->TC_CHANNEL[chNo].TC_IER=TC_IER_CPCS;  // RC compare interrupt       
  chTC->TC_CHANNEL[chNo].TC_IDR=~TC_IER_CPCS;                               
}

void setFrequencytone(Tc *chTC, uint32_t chNo, uint32_t frequency, int volume)
{
  if(frequency < 0 || frequency > 100001) {
    TC_Stop(chTC, chNo);                
    return;
  }
  const uint32_t rc = VARIANT_MCK / 128 / frequency; 
  //const uint32_t ra = rc >> 1; // 50% duty cycle 
  //const uint32_t ra = rc >> 2; // 20% duty cycle
  const uint32_t ra = rc >> volume; 
  TC_Stop(chTC, chNo);                  
  TC_SetRA(chTC, chNo, ra);             
  TC_SetRC(chTC, chNo, rc);             
  TC_Start(chTC, chNo);                 
}

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


void playTone() {

}

void sendStatistics() {

}

void setup() {
  Serial.begin(9600);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  configureToneTimer(chTC_2, chNo_2, chID_2, p_2, bit_2);
  configureToneTimer(chTC_3, chNo_3, chID_3, p_3, bit_3);

  setFrequencytone(chTC_2, chNo_2, 1000, 2); 
  delay(1500);
  setFrequencytone(chTC_3, chNo_3, 1000, 2);
  delay(1500);
  setFrequencytone(chTC_2, chNo_2, -1, 2);
  delay(1500);
  setFrequencytone(chTC_3, chNo_3, -1, 2);
  
  

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