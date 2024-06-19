
/* Sit & Heat firmware */
/*
* \file main.c
*
* \brief  Controller firmware used in electrical heated seats.
*
*/
/************************************************************************//**
* include files
****************************************************************************/

#include "ioavr.h"
#include <intrinsics.h>
#include <stdlib.h>
#include "main.h"



/************************************************************************//**
* macros
****************************************************************************/
#define LED_DDR()           DDRA |= ((1u << 4u)|(1u << 5u)); \
                            DDRC |= (1u << 0u);
                            
#define LED_ALL_ON()        PORTA |= ((1u << 4u)|(1u << 5u)); \
                            PORTC |= (1u << 0u);
                            
#define LED_ALL_OFF()       PORTA &= ~((1u << 4u)|(1u << 5u)); \
                            PORTC &= ~(1u << 0u);
                           
#define LED1_ON()          PORTA |=  (1u << 4u);  
#define LED1_OFF()         PORTA &= ~(1u << 4u);  
#define LED2_ON()          PORTA |=  (1u << 5u);  
#define LED2_OFF()         PORTA &= ~(1u << 5u);  
#define LED3_ON()          PORTC |=  (1u << 0u);  
#define LED3_OFF()         PORTC &= ~(1u << 0u);

#define HEAT_DDR()          DDRC |= (1u << 1u) ;

#define HEAT_T_ON()         PORTC |= (1u << 1u);

#define HEAT_T_OFF()        PORTC &= ~(1u << 1u);

#define HEAT_ALL_OFF()      PORTC &= ~(1u << 1u); 

                            
#define SW1                (PINA & (1u << 0u)) // + button
#define SW2                (PINA & (1u << 1u)) // - button
#define P_SW               (PINC & (1u << 1u)) // on button

                            
#define SEAT_SW            (PINA & (1u << 3u))
                            
#define U_SENSE           (PINB & (1u << 0u)) // Input voltage sense line
#define SENSE             (PINB & (1u << 1u)) // Bottom heater sense line                            
#define I_SENSE           (PINA & (1u << 2u)) // Bottom heater sense line  
                            
/************************************************************************//**
* global variables
****************************************************************************/
/* PWM registers */
static LEDs_t ledTarget = {0,0,0};
static int8_t ledBlink = 0;
static volatile heatPWM_t heater = { 0, PWM_HEAT_PERIOD };

static volatile uint8_t level = 0u;
static volatile uint8_t powerOn = 0u;

static volatile uint16_t battVolt = 40u, battLevel = 50u;
static volatile uint32_t battCurrent = 0;
static volatile uint8_t battLow = 0u;

static volatile uint8_t stLedTimeout = 0;
static volatile uint8_t ADMUXchannelselect = 0, heaterDisable = 0;

static enum {
  NORMAL,
  ALARM_VOLT,
  ALARM_AMP
}LED_MODE;

/************************************************************************//**
* static variables
****************************************************************************/

/* Heater PWM levels */
static const heatPWM_t heatLevel[MAXLEVELS] = {
//  { HEAT_TOP_LEVEL0 * PWM_HEAT_PERIOD, 0 },
  { HEAT_TOP_LEVEL1 * PWM_HEAT_PERIOD, 0 },
  { HEAT_TOP_LEVEL2 * PWM_HEAT_PERIOD, 0 },
  { HEAT_TOP_LEVEL3 * PWM_HEAT_PERIOD, 0 },
};

static const LEDs_t ledLevel[MAXLEVELS] = {
//  { LED_LEVEL0 },
  { LED_LEVEL1 },
  { LED_LEVEL2 },
  { LED_LEVEL3 },
};


/****************************************************************************
* PWM Blinking function
* Sets and Resets Heater signal
* Charges gate capacitor 
****************************************************************************/
static void Heat_cycle(void)
{
    HEAT_T_OFF();
    __delay_cycles(5);
    HEAT_T_ON();
    __delay_cycles(10);
    HEAT_T_OFF();
    __delay_cycles(8);
    HEAT_T_ON();
    __delay_cycles(10);
     HEAT_T_OFF();
    __delay_cycles(8);
    HEAT_T_ON();
    __delay_cycles(10);
     HEAT_T_OFF();
    __delay_cycles(10);
    HEAT_T_ON();
}


/************************************************************************//**
*Name : main\n
*------------------------------------------------------------------------------\n
*Purpose: main code entry point\n
*Input  : n/a\n
*Output : n/a\n
*Notes  :\n
****************************************************************************/



__task void main( void )
{
  
  LED_MODE = NORMAL;
  uint16_t ovfCurrent = 0, uvfVolt = 0, ovfVolt = 0;
  /* initialise host app, pins, watchdog, etc */ 
  init_system(); 
  
  /* Light show leds during startup */
  lightShow();
  
  /* configure timer ISR to fire regularly */
  init_timer1_isr();
  
  /* Direcxt start */
  #ifdef DIRECTSTART
      level = STARTUPLEVEL;
      ledTarget.LED1 = ledLevel[level].LED1 ;
      ledTarget.LED2 = ledLevel[level].LED2 ;
      ledTarget.LED3 = ledLevel[level].LED3 ;
      powerOn = 1;
  #endif
  /* enable interrupts */
  __enable_interrupt();
  /* loop forever */
  while(1)
  {
    if(LED_MODE == NORMAL || LED_MODE == ALARM_VOLT)
    {
      if (battLevel == VOLT_5 && battVolt <= VOLT_LOW_5)
      {
        if(uvfVolt<1000) uvfVolt += 20;
      }
      else if (battLevel == VOLT_12 && battVolt <= VOLT_LOW_12)
      {
        if(uvfVolt<1000) uvfVolt += 20;
      }
      else if (battLevel == VOLT_24 && battVolt <= VOLT_LOW_24)
      {
        if(uvfVolt<1000) uvfVolt += 20;
      }
      else
        if(uvfVolt > 5) uvfVolt -= 5;
      
      if (battLevel == VOLT_5 && battVolt >= VOLT_HIGH_5)
      {
        if(ovfVolt<1000) ovfVolt += 20;
      }     
      else if (battLevel == VOLT_12 && battVolt >= VOLT_HIGH_12)
      {
        if(ovfVolt<1000) ovfVolt += 20;
      }     
      else if (battLevel == VOLT_24 && battVolt >= VOLT_HIGH_24)
      {
        if(ovfVolt<1000) ovfVolt += 20;
      }
      else
        if(ovfVolt > 5) ovfVolt -= 5;
      
      if(ovfVolt > 100)
      {
        LED_MODE = ALARM_VOLT;
        ledTarget.LED2 = LED_ALARM_BLINK;
        ledTarget.LED1 = ledTarget.LED3 = 0;
      }
      else if(uvfVolt > 100)
      {
        LED_MODE = ALARM_VOLT;
        ledTarget.LED3 = LED_ALARM_BLINK;
        ledTarget.LED1 = ledTarget.LED2 = 0;
        if(battLevel == VOLT_5)
          heaterDisable = 1;
      }
      else if(LED_MODE == ALARM_VOLT)
      {
        LED_MODE = NORMAL;
        ledTarget.LED1 = ledLevel[level].LED1;
        ledTarget.LED2 = ledLevel[level].LED2;
        ledTarget.LED3 = ledLevel[level].LED3;
        if(heaterDisable == 1 && battLevel == VOLT_5)
          heaterDisable = 0;
      }
    }
    
    if(battLevel == VOLT_5 && battCurrent > CURRENT_MAX_5)
    {
      if(ovfCurrent < 1000)ovfCurrent += 20;
    }
    else if(battLevel == VOLT_12 && battCurrent > CURRENT_MAX_12)
    {
      if(ovfCurrent < 1000)ovfCurrent += 20;
    }
    else if(battLevel == VOLT_24 && battCurrent > CURRENT_MAX_24)
    {
      if(ovfCurrent < 1000)ovfCurrent += 20;
    }
    else 
    {
      if(ovfCurrent > 5)ovfCurrent -= 5;
    }
    
    if(ovfCurrent >= 70 && ovfCurrent < 750)
    {
      LED_MODE = ALARM_AMP;
      ledTarget.LED1 = ledTarget.LED2 = ledTarget.LED3 = LED_ALARM_BLINK;
    }
    else if(ovfCurrent >= 750 && heaterDisable == 0)
    {
      ovfCurrent = 2000;
      heaterDisable = 1;
    }
    else if(ovfCurrent < 10 && LED_MODE == ALARM_AMP)
    {
      LED_MODE = NORMAL;
      ledTarget.LED1 = ledLevel[level].LED1;
      ledTarget.LED2 = ledLevel[level].LED2;
      ledTarget.LED3 = ledLevel[level].LED3;
      heaterDisable = 0;
    }
    __delay_cycles(600000);
    
  }
  
  
}


/************************************************************************//**
*Name : init_timer1_isr\n
*------------------------------------------------------------------------------\n
*Purpose: configure timer1 to generate PWM IRQ's\n
*Input  : n/a\n
*Output : n/a\n
*Notes  :\n
****************************************************************************/

void init_timer1_isr( void )
{
  /* Set counter top value so every 100us an interrupt is generated */
  OCR1A = ( 100u );
  
  /* trigger port B 6.4ms later */
  //OCR1B = ( 50u );
  
  /* timer prescaler = system clock / 8  = 1MHz*/
  /* timer mode = CTC (count up to compare value, then reset) */
  // TCCR1A = (uint8_t) ((1 << CTC1) | (0 << CS12) | (1 << CS11) | (0 << CS10));
  // TCCR1A = (uint8_t) ((1 << TCCR1A_CTC1) | (1 << TCCR1A_CS11));
  TCCR1A = 0x0A;
  
  /* enable timer ISR */
  TIMSK_OCIE1A = 1u;
}


/************************************************************************//**
* Name : init_system\n
*------------------------------------------------------------------------------\n
*Purpose: initialise host app, pins etc\n
*Input  : n/a\n
*Output : n/a\n
*Notes  :\n
****************************************************************************/

void init_system( void )
{
  /* Set internal System clock to 8MHz. */
  CCP    = 0xD8u; // Unlock protected I/O register.
  CLKMSR = 0x00u;
  CCP    = 0xD8u; // Unlock protected I/O register.
  CLKPSR = 0x00u;
  
  /* Pull reset pin high */
  PORTC |= 1u << 3u;
  
  /* Activate led and heater output ports */
  LED_DDR();
  HEAT_DDR();
  
  /* Activate the AD converter */
  ADMUX = 0x48;
  //            (1 << ADMUX_REFS) |      // Sets ref. voltage to 1.1V
  //            (1 << ADMUX_MUX3)  |     // use ADC2 for input (PB0), MUX bit 3
  //            (0 << ADMUX_MUX2)  |     // use ADC2 for input (PB0), MUX bit 2
  //            (0 << ADMUX_MUX1)  |     // use ADC2 for input (PB0), MUX bit 1
  //            (0 << ADMUX_MUX0);       // use ADC2 for input (PB0), MUX bit 0
  
  ADCSRA = 0x87;
  //            (1 << ADCSRA_ADEN)  |     // Enable ADC 
  //            (1 << ADCSRA_ADPS2) |     // set prescaler to 128, bit 2 
  //            (1 << ADCSRA_ADPS1) |     // set prescaler to 128, bit 1 
  //            (1 << ADCSRA_ADPS0);      // set prescaler to 128, bit 0 
  
  ADCSRB = 0x00;
  
  PORTCR =  0x10;
  DIDR0 = 0x04;
  
  ADCSRA |=  0xC0; //(1 << ADCSRA_ADSC);       // Start conversion 
  
}


/************************************************************************//**
* Name : scanKeys\n
*----------------------------------------------------------------------------\n
*Purpose: Scan the buttons and seat switch\n
*Input  : n/a\n
*Output : n/a\n
*Notes  :\n
****************************************************************************/

void scanKeys( void )
{
  static uint16_t switchCntr = 0u;
  static uint16_t seatSwCntr = 0u;
  
  uint8_t buttons = 0;
  
  /* Disable timer ISR */
  //   TIMSK_OCIE1A = 0u;
  
  /* Check buttons */
  if (SW1) buttons += 0x01; // + button
  if (SW2) buttons += 0x02; // - button
  //if (P_SW) buttons += 0x04; // on button
  
  switch (buttons) {
  case 0x01: // + button
  case 0x02: // - button
  //case 0x04: // on button
    if (switchCntr == KEY_THRESHOLD) {
      
      switch (buttons) {
      case 0x01: // + button
        if (powerOn == 1) {
          if (level < MAXLEVELS - 1) level++;
        } else {
          powerOn = 1;
          level = 0;
        }
        if(LED_MODE == NORMAL){
        ledTarget.LED1 = ledLevel[level].LED1;
        ledTarget.LED2 = ledLevel[level].LED2;
        ledTarget.LED3 = ledLevel[level].LED3;
        }
        // Check if the seatswitch counter is active and reset in that case
        if (seatSwCntr > 0) seatSwCntr = EMPTYSEAT_TIME;
        break;
        
      case 0x02: // - button
        if (level == 0) powerOn = 0;
        if (level > 0) level--;
        if(LED_MODE == NORMAL){
        ledTarget.LED1 = ledLevel[level].LED1;
        ledTarget.LED2 = ledLevel[level].LED2;
        ledTarget.LED3 = ledLevel[level].LED3;
        }
        // Check if the seatswitch counter is active and reset in that case
        if (seatSwCntr > 0) seatSwCntr = EMPTYSEAT_TIME;
        break;
        
      default: // None or multiple keys pressed
        break;
      }
      switchCntr++;
      
    } else {
      switchCntr++;
      /* Repeat the last button */
      if ((switchCntr >= KEY_REPEAT)) {
        switchCntr = KEY_THRESHOLD;
      }
    }
    break;
    
  default: // None or multiple buttons pressed
    if (switchCntr < KEY_THRESHOLD / 2) {
      if (switchCntr > 0) switchCntr --;
    } else {
      switchCntr = KEY_THRESHOLD / 2;
    }
    break;
  }
  /* Check seat switch */
  if ((SEAT_SW)) {   // Chair is empty (high)
    if (seatSwCntr == 1u) { // Power off if counter almost runs out
#ifdef EMPTYSEAT_SWITCH
      powerOn = 0;
#endif
    } 
    // Decrease timer
    if (seatSwCntr > 0) {
      seatSwCntr--;
    } 
  } else { // Chair is in use (low)
#ifdef _AUTOSTART
    // Switch the heaters on right away when someone sits down
    if ((seatSwCntr == 0)&&(powerOn == 0)) {
      level = STARTUPLEVEL;
      ledTarget.LED1 = ledLevel[level].LED1;
      ledTarget.LED2 = ledLevel[level].LED2;
      ledTarget.LED3 = ledLevel[level].LED3;
      powerOn = 1;
    }
#endif
          
    seatSwCntr = EMPTYSEAT_TIME;
  }
}

/************************************************************************//**
* Name : lightShow\n
*----------------------------------------------------------------------------\n
*Purpose: Gives status flashes during startup\n
*1 flash  = Normal startup
*2 flahes = Top heater not connected
*3 flahes = Bottom heater not connected
*Inverted = Seat switch active
*Input  : n/a\n
*Output : n/a\n
*Notes  :\n
****************************************************************************/
void lightShow(void) {
  uint8_t flash[3] = {0, 0, 0};
  uint8_t cnt;
  uint16_t adcVolt = 0, adcCurr =0;
  uint16_t res;
  LED_ALL_OFF();
  __delay_cycles(1500000);

  ADCSRA |= 0xC0; 
  while((ADCSRA_ADSC)) ;
  adcVolt = ((ADC * 49) / 170);
  
  if (adcVolt < VOLT_LOW_5 )
  {
    return;
  }
  if ((adcVolt >= VOLT_LOW_5) && (adcVolt <= VOLT_HIGH_5))
  {     
    battLevel = VOLT_5;
    flash[0] = 0x1;
  }
  else if ((adcVolt >= VOLT_LOW_12) && (adcVolt <= VOLT_HIGH_12))
  {
    battLevel = VOLT_12;
    flash[0] = 0x2;
  }
  else if ((adcVolt >= VOLT_LOW_24) && (adcVolt <= VOLT_HIGH_24))
  {
    battLevel = VOLT_24;
    flash[0] = 0x4;
  }
  else
  {
    battLevel = VOLT_ERR;
    flash[0] = 0x7;
  }
  
  res = 0xffff;
  while (res--)
  {   
    if(!SEAT_SW)
    {
      flash[2] = 0x7;
      break;
    }
    else
    {
      flash[2] = 0x1;
    }
  }
  ADMUX = 0x42;
  res = 0xffff;
  while(res--)
  {
    ADCSRA |= 0xC0; 
    Heat_cycle();
  }
  while((ADCSRA_ADSC)) ;
  adcCurr = ADC; //100V/V over 1mOhm, remove x10 factor for res scaling
#ifdef TESTCURRENT
  while(1)
  {
    res = 0xffff;
    while(res--)
    {
      ADCSRA |= 0xC0; 
      Heat_cycle();
    }
    while((ADCSRA_ADSC)) ;
    adcCurr = ADC ; //100V/V over 1mOhm, remove x10 factor for res scaling
    res = (adcVolt*100) / adcCurr;
    if( res > 20 && res <= 34)
    {
      LED_ALL_OFF();
      LED1_ON();
    }
    else if (res > 34 && res <= 50)
    {
      LED_ALL_OFF();
      LED2_ON();
    }
    else if (res > 60 && res <= 110)
    {
      LED_ALL_OFF();
      LED3_ON();
    }
  }
#endif
  res = (adcVolt*100) / adcCurr;
  switch(battLevel)
  {
  case VOLT_5:
    if(res <= HEATER_5V_3_MAX)
    {
      flash[1] = 0x4;
    }
    else if(res <= HEATER_5V_2_MAX)
    {
      flash[1] = 0x2;
    }
    else if(res <= HEATER_5V_1_MAX)
    {
      flash[1] = 0x1;
    }
    else
    {
      flash[1] = 0x7;
    }
    break;
  case VOLT_12:
    if(res <= HEATER_12V_3_MAX)
    {
      flash[1] = 0x4;
    }
    else if(res <= HEATER_12V_2_MAX)
    {
      flash[1] = 0x2;
    }
    else if(res <= HEATER_12V_1_MAX)
    {
      flash[1] = 0x1;
    }
    else
    {
      flash[1] = 0x7;
    }
    break;
  case VOLT_24:
    if(res <= HEATER_24V_3_MAX)
    {
      flash[1] = 0x4;
    }
    else if(res <= HEATER_24V_2_MAX)
    {
      flash[1] = 0x2;
    }
    else if(res <= HEATER_24V_1_MAX)
    {
      flash[1] = 0x1;
    }
    else
    {
      flash[1] = 0x7;
    }
    break;
  }
  for (cnt=0; cnt < 3; cnt++) {
    if(flash[cnt] & 0x1)
    {
      LED1_ON(); 
    }
    if (flash[cnt] & 0x2) 
    {
      LED2_ON();
    }
    if (flash[cnt] & 0x4)
    {
      LED3_ON();
    }
    __delay_cycles(1000000);
    LED_ALL_OFF();
    __delay_cycles(2500000);
  }
    
  LED_ALL_OFF();
  
  }
/************************************************************************//**
* Interrupt handlers
****************************************************************************/

// Every 100us the PWM counter is increased and compared to the RGB values.
#pragma vector=TIM1_COMPA_vect
__interrupt void timer1A_isr( void )
{
  static LEDs_t ledCurrent = {0,0,0};
  static uint8_t pwmLedCtr = 0u;
  static uint16_t pwmHeatCntr = 0u;
  static uint8_t blinkTmr = 0;
  static uint8_t heatActivate = 0;
  

  //  powerOn = 1u;
  if (powerOn == 0) { // Power off
    LED_ALL_OFF();
    HEAT_ALL_OFF();
    if(pwmLedCtr == PWM_LED_PERIOD)
    {
      pwmLedCtr = 0;
      ledCurrent.LED1 = 0;
      ledCurrent.LED2 = 0;
      ledCurrent.LED3 = 0;
    }
  } else { // Power on
    
    // Reset led counter and start leds (every 6.4ms)
    if (pwmLedCtr == PWM_LED_PERIOD) {
        pwmLedCtr = 0u;
        if (ledCurrent.LED1 > 0) LED1_ON();
        if (ledCurrent.LED2 > 0) LED2_ON();
        if (ledCurrent.LED3 > 0) LED3_ON();
        
        if(LED_MODE == NORMAL){

           // Modify the led target color
          if (ledCurrent.LED1 > ledTarget.LED1) ledCurrent.LED1--;
          if (ledCurrent.LED1 < ledTarget.LED1) ledCurrent.LED1++;
        
          if (ledCurrent.LED2 > ledTarget.LED2) ledCurrent.LED2--;
          if (ledCurrent.LED2 < ledTarget.LED2) ledCurrent.LED2++;
        
          if (ledCurrent.LED3 > ledTarget.LED3) ledCurrent.LED3--;
          if (ledCurrent.LED3 < ledTarget.LED3) ledCurrent.LED3++;
        }
        else{
          ledCurrent.LED1 = ledTarget.LED1;
          ledCurrent.LED2 = ledTarget.LED2;
          ledCurrent.LED3 = ledTarget.LED3;
        }
        if(blinkTmr == LED_ALARM)
          blinkTmr = 0;
        blinkTmr++;

    }
    if(LED_MODE == NORMAL)
    {
      // Compare counter to led values 
      if (pwmLedCtr >= ledCurrent.LED1) LED1_OFF();
      if (pwmLedCtr >= ledCurrent.LED2) LED2_OFF();
      if (pwmLedCtr >= ledCurrent.LED3) LED3_OFF();

    }
    else if(LED_MODE != NORMAL)
    {
      if (blinkTmr >= ledCurrent.LED1) LED1_OFF();
      if (blinkTmr >= ledCurrent.LED2) LED2_OFF();
      if (blinkTmr >= ledCurrent.LED3) LED3_OFF();
      
    }
    
    // Reset heater counter, start heater-A and stop heater-B (every 60ms)
    if (pwmHeatCntr >= PWM_HEAT_PERIOD) {
      pwmHeatCntr = 0u;
      
      // Set heater PWM levels 
      heater.A = heatLevel[level].A;
      
      //Start heater PWM sequence 
      if (heater.A > 0u) heatActivate = 1; //if (heater.A > 0u) HEAT_T_OFF();
      
    } else{
      
      // Compare counter to heater values 
      if (pwmHeatCntr == heater.A) heatActivate = 0; // HEAT_T_OFF();
    }
    //Increase heater counter
    pwmHeatCntr++;
    if (heatActivate && !heaterDisable){
      Heat_cycle();
    }
    else
    {
      HEAT_T_OFF();
    }
  } // end power on

  // Increase led counter 
  pwmLedCtr++;
  
  
  // Check keys once per led period 
  if (pwmLedCtr == PWM_LED_PERIOD / 2) { 
    scanKeys();
  }
  

  // Check battery voltage once per led period 
  if (ADMUXchannelselect == 0 && pwmLedCtr == PWM_LED_PERIOD - 10
      && !ADCSRA_ADSC){
    battVolt = ADC * 28 / 98;
    ADMUX = 0x42;
    ADMUXchannelselect = 1;
  //            (1 << ADMUX_REFS) |      // Sets ref. voltage to 1.1V
  //            (1 << ADMUX_MUX3)  |     // use ADC2 for input (PA2), MUX bit 3
  //            (1 << ADMUX_MUX2)  |     // use ADC2 for input (PA2), MUX bit 2
  //            (0 << ADMUX_MUX1)  |     // use ADC2 for input (PA2), MUX bit 1
  //            (0 << ADMUX_MUX0);       // use ADC2 for input (PA2), MUX bit 0
    ADCSRA |= 0xC0; 
   }
  //measure once every heat cycle
   else if(ADMUXchannelselect == 1 && pwmHeatCntr < (heater.A - 5)
           && !ADCSRA_ADSC){
     battCurrent = (ADC * 10); 
     ADMUX = 0x48;
//            (1 << ADMUX_REFS) |      // Sets ref. voltage to 1.1V
//            (1 << ADMUX_MUX3)  |     // use ADC2 for input (PB0), MUX bit 3
//            (0 << ADMUX_MUX2)  |     // use ADC2 for input (PB0), MUX bit 2
//            (0 << ADMUX_MUX1)  |     // use ADC2 for input (PB0), MUX bit 1
//            (0 << ADMUX_MUX0);       // use ADC2 for input (PB0), MUX bit 0
    ADMUXchannelselect = 0;
    ADCSRA |= 0xC0; 
      
  }
  
  
 }
  



