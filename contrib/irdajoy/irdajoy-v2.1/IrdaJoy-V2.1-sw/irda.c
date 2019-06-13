/*
 *  Copyright (C) 2008-2009 Florent Bedoiseau (electronique.fb@free.fr)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// File : irda.c
// Microchip C18 compiler : MPASMWIN.exe v5.30.01, mplink.exe v4.30.01, mcc18.exe v3.30
// 18F452 10MHz Crystal (T=400ns)
// USART     : 9600 8N1
// IrDA (SIR): 9600 8N1 : 0 : 0 pendant 84 us + 1 pendant 16us, 1 : 0 pendant 100us

// Assignation des pins :
// -----------------------------------------------
// Pic : Rôle							: Pin Sub9
// -----------------------------------------------
// RA0 : Potentiomètre JOY_0			: 9
// RA1 : Potentiomètre JOY_1			: 5
// RB0 : Joystick numérique : J_BUTTON	: 6
// RB1 : NC								: Néant
// RB2 : CFG0							: Néant
// RB3 : CFG1							: Néant
// RB7 : Joystick numérique : J_UP		: 1
// RB6 : Joystick numérique : J_DOWN	: 2
// RB5 : Joystick numérique : J_LEFT	: 3
// RB4 : Joystick numérique : J_RIGHT	: 4
// RD7 : LED IR							: Néant
// RC5 : LED Power						: Néant
// VCC/VDD								: 7
// GND/VSS								: 8
// -----------------------------------------------

// Frame : 5 bytes : 	'#',
// 						[X,UP,DN,LT,RT,BT0,AN0-b9,AN0-b8],
//						[AN0-b7,AN0-b6,AN0-b5,AN0-b4,AN0-b3,AN0-b1,AN0-b1,AN0-b0],
//						[X,X,X,X,X,BT1,AN1-b9,AN0-b8],
//						[AN1-b7,AN1-b6,AN1-b5,AN1-b4,AN1-b3,AN1-b1,AN1-b1,AN1-b0]
#include <p18f452.h>
#include <delays.h>
#include <usart.h>
#include <adc.h>
#include <portb.h>
#include <stdlib.h>

// CONFIG
#pragma config OSC = HS, OSCS = OFF
#pragma config PWRT = OFF
#pragma config BOR = OFF
#pragma config WDT = OFF
#pragma config CCP2MUX = OFF
#pragma config LVP = OFF

// --------------------
// Delay macros 
// Delay1TCY() ou Nop()
// Delay10TCYx()
// Delay100TCYx()
// Delay1KTCYx ()
// Delay10KTCYx ()
// --------------------
#define wait_84us()                             \
  { Delay10TCYx(21); } // 210x400ns=84us

#define wait_16us()                             \
  { Delay10TCYx(4); } // 40x400ns=16us

#define wait_500ms()                                \
  { Delay10KTCYx (125); } // 1250000x400ns=500ms

#define wait_20ms()                             \
  { Delay10KTCYx (5); } // 5=>20ms

#define wait_12ms()                             \
  { Delay10KTCYx (3); } // 5=>20ms

// PORTS
#define LED_DIR  TRISDbits.TRISD7
#define LED_PORT PORTDbits.RD7

#define LED_POWER_DIR  TRISCbits.TRISC5
#define LED_POWER_PORT PORTCbits.RC5

#define BUTTON_0 PORTBbits.RB0
#define BUTTON_1 PORTBbits.RB1

#define SWT_CFG_0 PORTBbits.RB2 // 0: 20ms between frames, 1:20ms between chunks
#define SWT_CFG_1 PORTBbits.RB3 // 0: No USART, 1: sending frame both IRDA & USART

// STATES
#define PREPARE_CONV 0
#define START_CONV   1
#define WAIT_CONV    2
#define PREPARE_MSG  3
#define SEND_MSG     4

#define FRAME_SIZE   5


// Protos
void usart_send_byte (char byte);
void irda_send_byte (char byte);
void InterruptHandlerHigh (void);
void InterruptHandlerLow (void);

// 1 byte sent : 1040us ~
void irda_send_byte (char byte) {
  char cnt;
  // Sending Start bit (pulse)
  wait_84us();
  LED_PORT=1;
  wait_16us();
  LED_PORT=0;
  // Sending 8 bits

  cnt=8;
  while (cnt) {
    wait_84us();
    if (!(byte & 0x01)) LED_PORT=1; // pulse
    wait_16us();
    LED_PORT=0;
    byte >>=1;
    --cnt;
  }

  // Sending Stop bit (no pulse)
  wait_84us();
  wait_16us();
}

void usart_send_byte (char byte) {
  while (BusyUSART());
  putcUSART (byte);
}

void main () {
  char i;
  char atari;

  int canvalue[2]; // 2 AN value max
  char frame [5]; // current frame
     
  int state; // state machine
  int channel; // CAN channel

  char ct_pwm, ct_value, ct_dir; // PWM
  
  INTCONbits.GIEH = 0; // Disable all high int
  INTCONbits.GIEL = 0; // Disable all low int

  // IR led
  LED_DIR = 0; // Output
  LED_PORT = 0; // Off
   
  // PWM power led
  LED_POWER_DIR = 0; // Output
  LED_POWER_PORT = 0; //Off
  ct_pwm=0; // Compteur de pas pour definir un cycle PWM
  ct_value=0; // Compteur permettant de definir un changement de valeur
  ct_dir=0; // Sens croissant (0), decroissant (1)

  // Joystick/buttons ports (PORTB)
  OpenPORTB (PORTB_CHANGE_INT_OFF & PORTB_PULLUPS_ON);

  // Joystick/buttons ports
  TRISB = 0xFF; // Input for all

  // spbrg = F / (16 * (x + 1))
  // @4MHz   19200 : USART_BRGH_HIGH, 12
  // @8MHz   19200 : USART_BRGH_HIGH, 25
  // @7.2MHz 19200 : USART_BRGH_HIGH, 22
  // @10MHz  19200 : USART_BRGH_HIGH, 32
  // @10MHz  9600  : USART_BRGH_HIGH, 65
  OpenUSART (USART_TX_INT_OFF &
             USART_RX_INT_OFF &
             USART_ASYNCH_MODE &
             USART_EIGHT_BIT &
             USART_CONT_RX &
             USART_BRGH_HIGH, 65); // 9600 @ 10Mhz

  OpenADC (ADC_FOSC_32 // Max Freq : 20Mhz
		   & ADC_RIGHT_JUST
		   & ADC_3ANA_0REF, // AN0, AN1, Vref+=Vdd, Vref-=Vss
		   ADC_CH0 & ADC_CH1
		   & ADC_INT_OFF); // No INT

  state = PREPARE_CONV;
  channel = 0;

  while (1) {
    switch (state) {

    case PREPARE_CONV: // Preparing conversion
      switch (channel) {
      case 0: 
        SetChanADC (ADC_CH0); 
        state = START_CONV;
        break;
      case 1: 
        SetChanADC (ADC_CH1);
        state = START_CONV;
        break;
      default:
        state = PREPARE_MSG;
        channel = 0;
        break;
      }
      break;

    case START_CONV: // Start conversion
      ConvertADC ();// lancement conversions
      state = WAIT_CONV;
      break;

    case WAIT_CONV: // Waiting for end of conversion
      if (!BusyADC()) {
        canvalue[channel]=ReadADC ();
        channel++;
        state = PREPARE_CONV;
      }
      break;

    case PREPARE_MSG: // Preparing msg
      frame [0] = '#'; // Simple start tag
      frame [1] = (char) ((canvalue[0] & 0xFF00) >> 8); // AN0 : JOY_0 MSB
      frame [2] = (char) (canvalue[0] & 0x00FF);		// JOY_0 LSB
      frame [3] = (char) ((canvalue[1] & 0xFF00) >> 8); // AN1 : JOY_1 MSB
      frame [4] = (char) (canvalue[1] & 0x00FF);		// JOY_1 LSB
	
      // Adding button states. 0 : Not pressed, 1 : Pressed
      if (!BUTTON_0) { frame [1] |= 0x04; }
      if (!BUTTON_1) { frame [3] |= 0x04; }

      // Adding atari joystick port
      atari = ((PORTB ^ 0xFF) & 0xF0) >> 1; // UP, DOWN, LEFT, RIGHT
      frame [1] |= atari;
						
      state = SEND_MSG;
      break;

    case SEND_MSG:
      for (i=0; i < FRAME_SIZE; i++) { // Sending frame both in IrDA and USART
        if (SWT_CFG_0) usart_send_byte (frame[i]); // DEBUG
        else irda_send_byte (frame[i]);
      }
                          
      // Wait 20ms between 2 frames
      wait_20ms();
      //wait_12ms();

      channel = 0;
      state = PREPARE_CONV;
      break;


              
    default:
      state = PREPARE_CONV;
      break;
    }
  }
}


#pragma code InterruptVectorHigh = 0x08
void InterruptVectorHigh (void) {
  _asm
    goto InterruptHandlerHigh //jump to interrupt routine
    _endasm
    }

#pragma code
#pragma interrupt InterruptHandlerHigh

void InterruptHandlerHigh () {
}

#pragma code InterruptVectorLow = 0x18
void InterruptVectorLow (void) {
  _asm
    goto InterruptHandlerLow //jump to interrupt routine
    _endasm
    }

#pragma code
#pragma interrupt InterruptHandlerLow

void InterruptHandlerLow () {
}
