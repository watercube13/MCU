
/* Sit & Heat firmware */
/*
* \file main.h
*
* \brief  Controller firmware used in electrical heated seats.
*/

/************************************************************************//**
* User definitions
****************************************************************************/

#define _AUTOSTART              // Heaters are switched on when someone sits down

#define EMPTYSEAT_TIME          18750    // Time till power-off mode (n x key_scan_period) (n x 6.4ms = 120s)
#define EMPTYSEAT_SWITCH        // Enable the seat switch time out
//#define DIRECTSTART             // Unit starts directly

#define STARTUPLEVEL            1u // - 1u // The level choosen after startup

#define HEAT_TOP_LEVEL0         0
#define HEAT_TOP_LEVEL1         0.50//0.33
#define HEAT_TOP_LEVEL2         0.75//0.66
#define HEAT_TOP_LEVEL3         1.0//0.70


#define LED_LEVEL0              0,0,0         
#define LED_LEVEL1              30,0,0        
#define LED_LEVEL2              60,60,0      
#define LED_LEVEL3              100,100,100     

#define LED_ALARM               156u    // Total LED blink time (n x PWM_LED_PERIOD)    
#define LED_ALARM_BLINK         78u     //led blink time (n x PWM_LED_PERIOD)        


/************************************************************************//**
* System definitions
****************************************************************************/

#define MAXLEVELS               3u      // Amount of power levels available

#define PWM_LED_PERIOD          63u     // Total led PWM time (n x 100us) (6.3ms)
#define PWM_HEAT_PERIOD         456u    // Total heater PWM time (n x 100us)

#define KEY_THRESHOLD           10u    // Time a key has to be pressed / released (n x PWM_LED_PERIOD)
#define KEY_REPEAT              200u   // Time after a key is repeated


#define VOLT_LOW_5              30u
#define VOLT_5                  50u
#define VOLT_HIGH_5             60u
#define VOLT_LOW_12             90u
#define VOLT_12                 120u
#define VOLT_HIGH_12            140u
#define VOLT_LOW_24             165u
#define VOLT_24                 240u
#define VOLT_HIGH_24            260u
#define VOLT_ERR                -1

#define CURRENT_MAX_5           4000
#define CURRENT_MAX_12          7000
#define CURRENT_MAX_24          2500

#define HEATER_5V_1_MAX         70u     // 7.0 ohm, normal 5V 1x heater 5.0 ohm (> open)
#define HEATER_5V_2_MAX         40u     // 4.0 ohm, normal 5V 2x heater 2.5 ohm
#define HEATER_5V_3_MAX         25u //20u     // 2.0 ohm, normal 5V 3x heater 1.8 ohm

#define HEATER_12V_1_MAX        110u    // 11.0 ohm, normal 12V 1x heater 7.2 & 9.6 ohm (> open)
#define HEATER_12V_2_MAX        60u     // 6.0 ohm, normal 12V 2x heater 3.6 & 4.8 ohm
#define HEATER_12V_3_MAX        40u //34u     // 3.4 ohm, normal 12V 3x heater 2.4 & 3.2 ohm

#define HEATER_24V_1_MAX        200u    // 20.0 ohm, normal 24V 1x heater 16.8 ohm (> open)
#define HEATER_24V_2_MAX        100u    // 10.0 ohm, normal 24V 2x heater 8.3 ohm
#define HEATER_24V_3_MAX        80u     // 8.0 ohm, normal 24V 3x heater 5.5 ohm


/************************************************************************//**
* prototypes
****************************************************************************/

/* initialise host app, pins, watchdog, etc */
static void init_system( void );

/* configure timer ISR to fire regularly */
static void init_timer1_isr( void );

//static void qt_set_parameters( void );
//uint16_t delta_acquire [QT_NUM_CHANNELS];
void scanTouchB( void );

#pragma vector=TIM1_COMPA_vect
__interrupt void timer1A_isr( void );

// Moving leds during startup
void lightShow(void);


/************************************************************************//**
* type definitions
****************************************************************************/

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;
typedef short int16_t;
typedef unsigned int uint32_t;

/* LED levels */
typedef struct {
  uint16_t LED1;
  uint16_t LED2;
  uint16_t LED3; // Also Amber led
} LEDs_t;

/* Heater PWM levels */
typedef struct {
  uint16_t A;
  uint16_t B;
} heatPWM_t;

//hallo
