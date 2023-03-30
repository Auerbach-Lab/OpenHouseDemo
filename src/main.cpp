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
T0  TC0  0   TCLK0  TIOA0  TIOB0  TC0_IRQn  TC0_Handler  ID_TC0 
T1  TC0  1   TCLK1  TIOA1  TIOB1  TC1_IRQn  TC1_Handler  ID_TC1
T2  TC0  2   TCLK2  TIOA2  TIOB2  TC2_IRQn  TC2_Handler  ID_TC2
T3  TC1  0   TCLK3  TIOA3  TIOB3  TC3_IRQn  TC3_Handler  ID_TC3 
T4  TC1  1   TCLK4  TIOA4  TIOB4  TC4_IRQn  TC4_Handler  ID_TC4
T5  TC1  2   TCLK5  TIOA5  TIOB5  TC5_IRQn  TC5_Handler  ID_TC5
T6  TC2  0   TCLK6  TIOA6  TIOB6  TC6_IRQn  TC6_Handler  ID_TC6
T7  TC2  1   TCLK7  TIOA7  TIOB7  TC7_IRQn  TC7_Handler  ID_TC7 
T8  TC2  2   TCLK8  TIOA8  TIOB8  TC8_IRQn  TC8_Handler  ID_TC8 
  */

#define DEBOUNCE_DELAY 15
#define TONE_STOP -1
#define TONE_FREQ 1000
#define TONE_DUR 300
#define DONE_FREQ 2000
#define DONE_DUR 1000
#define LOUD 1
#define MID 5
#define QUIET 9

struct tone_pin {
  Tc *chTC;
  uint32_t chNo;
  uint32_t chID;
  Pio *p;
  uint32_t bit;
};

struct player
{
  struct tone_pin pin;
  uint32_t button_pin;
  uint32_t led_red_pin;
  uint32_t led_green_pin;
  unsigned long lastDebounceTime {0};
  unsigned long scheduledMillis {0};
  unsigned long doneMillis {0};
  int redState {HIGH};
  int greenState {HIGH};
  int buttonState {HIGH};
  int lastState {HIGH};
  bool playing {false}; //currently making noise for this player
  int trial {0};
  int32_t reactionMillis[10];
  char id[8];
};

static struct player players[5];
unsigned long currentMillis = 0;
uint8_t volumes[10] = {LOUD, QUIET, MID, LOUD, LOUD, QUIET, MID, QUIET, LOUD, MID};

void sendStatistics(player &p) {
    float loud = (p.reactionMillis[3] + p.reactionMillis[4] + p.reactionMillis[8]) / 3;
    float mid = (p.reactionMillis[2] + p.reactionMillis[6] + p.reactionMillis[9]) / 3;
    float quiet = (p.reactionMillis[1] + p.reactionMillis[5] + p.reactionMillis[7]) / 3;
    Serial.print("player=");
    Serial.print(p.id);
    Serial.print(" loud=");
    Serial.print(loud);
    Serial.print(" mid=");
    Serial.print(mid);
    Serial.print(" quiet=");
    Serial.println(quiet);
}


void configureToneTimer(tone_pin &pin) {
  Tc *chTC = pin.chTC;
  uint32_t chNo = pin.chNo;
  uint32_t chID = pin.chID;
  Pio *p = pin.p;
  uint32_t bit = pin.bit;

  // Configure TONE_PIN_2 pin as timer output
  pmc_enable_periph_clk( ID_PIOC ) ;  //tone pin 3 (TIOA7) is on PIOB
  int result = PIO_Configure( p,      //tone pin 3 (TIOA7) is on PIOB
			      PIO_PERIPH_B,             //tone pin 3 (TIOA7) is on PIOB
			      bit,                      //tone pin 3 (TIOA7) is on PIOB, port PC28
			      PIO_DEFAULT);
  //Serial.println(result);

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

void setFrequencytone(tone_pin &pin, uint32_t frequency, int volume) {
  Tc *chTC = pin.chTC;
  uint32_t chNo = pin.chNo;

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


void onRelease(player &p) {
  int32_t reactionMillis = millis() - p.scheduledMillis;
  if ((reactionMillis >= 0) && (reactionMillis < 1000)) { //correctly released button after tone played
    p.greenState = HIGH;
    p.reactionMillis[p.trial] = reactionMillis;
    p.trial++;
  } else {  //released button too early or too late
    p.scheduledMillis = 0;
    p.redState = HIGH;    
  } 
  
  if (p.trial >= sizeof p.reactionMillis / sizeof p.reactionMillis[0]) {  // reaction times array is full, i.e. we've done all our trials
    p.greenState = HIGH;
    p.redState = HIGH;
    sendStatistics(p);
    p.trial = 0;
    p.doneMillis = millis() + 250;
  }
}

void onPress(player &p) {
  p.scheduledMillis = millis() + random(1000,2000);
  // Serial.print("player=");
  // Serial.print(p.id);
  // Serial.print(" trial=");
  // Serial.print(p.trial);
  // Serial.print(" scheduled=");
  // Serial.println(p.scheduledMillis);
  p.redState = LOW;
  p.greenState = LOW;
}

/*
    TC   Ch  ExClk  I/O A  I/O B  NVIC_IRQ  IRQ_handler  PMC_id
T0  TC0  0   TCLK0  TIOA0  TIOB0  TC0_IRQn  TC0_Handler  ID_TC0 << tone pin 2 is TIOA0
T1  TC0  1   TCLK1  TIOA1  TIOB1  TC1_IRQn  TC1_Handler  ID_TC1 
T2  TC0  2   TCLK2  TIOA2  TIOB2  TC2_IRQn  TC2_Handler  ID_TC2
T3  TC1  0   TCLK3  TIOA3  TIOB3  TC3_IRQn  TC3_Handler  ID_TC3 
T4  TC1  1   TCLK4  TIOA4  TIOB4  TC4_IRQn  TC4_Handler  ID_TC4
T5  TC1  2   TCLK5  TIOA5  TIOB5  TC5_IRQn  TC5_Handler  ID_TC5
T6  TC2  0   TCLK6  TIOA6  TIOB6  TC6_IRQn  TC6_Handler  ID_TC6 << tone pin 5 is TIOa6
T7  TC2  1   TCLK7  TIOA7  TIOB7  TC7_IRQn  TC7_Handler  ID_TC7 << tone pin 3 is TIOa7
T8  TC2  2   TCLK8  TIOA8  TIOB8  TC8_IRQn  TC8_Handler  ID_TC8 << tone pin 11 is TIOa8
*/

void setup() {
  Serial.begin(9600);
  players[0].pin = {TC0, 0, ID_TC0, PIOB, PIO_PB25B_TIOA0}; //tone pin 2
  players[0].led_red_pin = 48;   //uint32_t led_red_pin;
  players[0].led_green_pin = 50;   //uint32_t led_green_pin;
  players[0].button_pin = 52;  //uint32_t button_pin;
  strcpy(players[0].id, "Purple");
  
  players[1].pin ={TC2, 1, ID_TC7, PIOC, PIO_PC28B_TIOA7}; //tone pin 3
  players[1].led_red_pin = 49;
  players[1].led_green_pin = 51;
  players[1].button_pin = 53;
  strcpy(players[1].id, "White");

  players[2].pin ={TC2, 0, ID_TC6, PIOC, PIO_PC25B_TIOA6}; //tone pin 5
  players[2].led_red_pin = 42;
  players[2].led_green_pin = 44;
  players[2].button_pin = 46;
  strcpy(players[2].id, "Green");

  players[3].pin ={TC2, 2, ID_TC8, PIOD, PIO_PD7B_TIOA8}; //tone pin 11
  players[3].led_red_pin = 43;
  players[3].led_green_pin = 45;
  players[3].button_pin = 47;
  strcpy(players[3].id, "Blue");

  
  for (int i=0; i < sizeof players / sizeof players[0]; i++) {
    configureToneTimer(players[i].pin);  
    pinMode(players[i].led_red_pin, OUTPUT);
    pinMode(players[i].led_green_pin, OUTPUT);
    pinMode(players[i].button_pin, INPUT_PULLUP);
  }

  // setFrequencytone(players[0].pin, 1000, 2); 
  // delay(1000);
  // setFrequencytone(players[1].pin, 1000, 2);
  // delay(1000);
  // setFrequencytone(players[0].pin, -1, 2); 42
  // delay(1000);
  // setFrequencytone(players[1].pin, -1, 2);
}

void loop() {
  for (int i=0; i < sizeof players / sizeof players[i]; i++) {
    int reading = digitalRead(players[i].button_pin);
    currentMillis = millis();
    if(reading != players[i].lastState) {
      players[i].lastDebounceTime = currentMillis;
    }
    if ((currentMillis - players[i].lastDebounceTime) > DEBOUNCE_DELAY) {
      if (reading != players[i].buttonState) {
        players[i].buttonState = reading;
        if (players[i].buttonState == HIGH) {
          onRelease(players[i]);
        } else {
          onPress(players[i]);
        }
      }
    }
    if(!players[i].playing && players[i].scheduledMillis && (currentMillis > players[i].scheduledMillis)) {
      players[i].playing = true;
      setFrequencytone(players[i].pin, TONE_FREQ, volumes[players[i].trial]);
      //Serial.println("tone start");
    }
    if(players[i].playing && players[i].scheduledMillis && (currentMillis > players[i].scheduledMillis + TONE_DUR)) {
      setFrequencytone(players[i].pin, TONE_STOP, 0);
      //Serial.println("tone stop");
      players[i].scheduledMillis = 0;
      players[i].playing = false;
    }
    if(!players[i].playing && players[i].doneMillis && (currentMillis > players[i].doneMillis)) {
      players[i].playing = true;
      setFrequencytone(players[i].pin, DONE_FREQ, MID);
      //Serial.println("tone start");
    }
    if(players[i].playing && players[i].doneMillis && (currentMillis > players[i].doneMillis + DONE_DUR)) {
      setFrequencytone(players[i].pin, TONE_STOP, 0);
      //Serial.println("tone stop");
      players[i].doneMillis = 0;
      players[i].playing = false;
    }

    players[i].lastState = reading;
    digitalWrite(players[i].led_red_pin, players[i].redState);
    digitalWrite(players[i].led_green_pin, players[i].greenState);
  }
}