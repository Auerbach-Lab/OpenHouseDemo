/* Tone generation for DUE
 see http://arduino.cc/forum/index.php/topic,136500.msg1029238.html#msg1029238 and 
 http://asf.atmel.com/docs/latest/sam3a/html/group__sam__drivers__pmc__group.html and
 http://asf.atmel.com/docs/latest/sam.drivers.usart.usart_synchronous_example.sam3u_ek/html/sam_pio_quickstart.html
 and http://ko7m.blogspot.com.au/2015/01/arduino-due-timers-part-1.html
 
 The Atmel Power Management Controller (pmc) optimizes power consumption by controlling
 all system and user peripheral clocks, including enabling/disabling clock inputs to 
 many peripherals and the Cortex-M Processor.
 VARIANT_MCK = 84000000 (Master ClocK freq) 

 Table showing relationship between the Timer/Counter, it's channels, the IRQ to use, what 
 IRQ function must be called and the power management ID for that peripheral. 

  TC Ch NVIC_IRQ IRQ_handler PMC_id
  ---------------------------------
  TC0 0 TC0_IRQn TC0_Handler ID_TC0
  TC0 1 TC1_IRQn TC1_Handler ID_TC1
  TC0 2 TC2_IRQn TC2_Handler ID_TC2
  TC1 0 TC3_IRQn TC3_Handler ID_TC3 << using this one below
  TC1 1 TC4_IRQn TC4_Handler ID_TC4
  TC1 2 TC5_IRQn TC5_Handler ID_TC5
  TC2 0 TC6_IRQn TC6_Handler ID_TC6
  TC2 1 TC7_IRQn TC7_Handler ID_TC7
  TC2 2 TC8_IRQn TC8_Handler ID_TC8 
  */

#define TONEpin    3                                 // piezo buzzer pin
#include <Streaming.h>

const int toneFreq = 4000;                           // tone starting value, Hz

volatile static int32_t toggles;                    // number of ups/downs in a tone burst 
 
void setupTC(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t freq){
  pmc_set_writeprotect(false);                       // disable write protection of timer registers
  pmc_enable_periph_clk((uint32_t) irq);             // enable clock / interrupt 
//pmc_enable_periph_clk((uint32_t) ID_TC3);          // alternate syntax, using PMC_id instead
  TC_Configure(tc, channel,            
               TC_CMR_TCCLKS_TIMER_CLOCK4 |          // TIMER_CLOCK4: MCK/128=656,250Hz. 16 bits so 656,250/65,536=~10Hz/bit
               TC_CMR_WAVE |                         // Waveform mode
               TC_CMR_WAVSEL_UP_RC |                // Counter running up and reset when = Register C value (rcVal)
               TC_CMR_ACPA_SET |     // RA compare sets TIOA
	             TC_CMR_ACPC_CLEAR );  // RC compare clears TIOA

  const uint32_t rcVal = VARIANT_MCK/256/freq;       // target value for counter, before it resets
//Serial << "rcVal: " << rcVal << " toggles: " << toggles << '\n'; 
  //TC_SetRA(tc, channel, rcVal >> 6);                    // could also use Register A for 50% duty cycle square wave
  TC_SetRC(tc, channel, rcVal);
  TC_Start(tc, channel);
  (*tc).TC_CHANNEL[channel].TC_IER =  TC_IER_CPCS;   // IER: CPCS bit enables RC compare interrupt when set 
  (*tc).TC_CHANNEL[channel].TC_IDR = ~TC_IER_CPCS;   // IDR: clear CPCS bit = don't disable RC compare interrupt
  NVIC_EnableIRQ(irq);                               // Enable TC3_IRQn in the Nested Vector Interrupt Controller)
  }

void TC3_Handler(void){                              // timer ISR  TC1 ch 0
  TC_GetStatus(TC1,0);
  if (toggles != 0){
    digitalWrite(TONEpin,!digitalRead(TONEpin));     // invert the pin state (i.e toggle)
    if (toggles > 0) toggles--;
    } 
  //else noTone();                                   // seems superfluous ?
  }
/*
void noTone(){
  TC_Stop(tc,channel);                               // stop timer
  digitalWrite(TONEpin,LOW);                         // no signal on pin
  }*/

void tone(int32_t duration){                        // duration in ms
  const uint32_t rcVal = VARIANT_MCK/256/toneFreq;  // target value for counter, before it resets (= 82 for 4kHz)
  toggles = 2*toneFreq*duration/1000;               // calculate no of waveform edges (rises/falls) for the tone burst
  setupTC(TC1,0,TC3_IRQn,toneFreq);                 // Start Timer/Counter 1, channel 0, interrupt, frequency
  }

void beep()  {tone(50);}
void beep2() {beep(); delay(150); beep();}
void beep3() {beep(); delay(150); beep(); delay(150); beep();}
void beep4() {beep(); delay(150); beep(); delay(150); beep(); delay(150); beep();}

void setup() {
  Serial.begin(115200);
  pinMode(TONEpin, OUTPUT);
  
  Serial << F("\n==== Simple Due tone example ====\n");
  beep();  delay(1000);                             // 1 beep
  beep2(); delay(1000);                             // 2
  beep3(); delay(1000);                             // 3
  beep4();                                          // 4
  Serial << F("done...\n");
  }

void loop() {}