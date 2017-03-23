/*
Copyright 2014 Dustin L. Westaby

----------------------------------------------------------------------
        Decrement Counter for ATtiny4313
----------------------------------------------------------------------

Author:     Dustin Westaby
Date Created:   11/15/09
Last Modified:  01/04/14
Purpose:  Counter program for two digit display to sync with LTAR

----------------------------------------------------------------------

Usage:
Unhosted Games:
- Turn tagger off and on, this will wake the display
- Press shield button to switch between 10 and 25 health gametypes.
- Press trigger to start game, 10 second countdown begins
- Display will show remaining health / shields during game.

Hosted Games:
- Setup hosted game with the health / shield numbers using instructions for number entry.
- Turn the tagger off and on while holding the shield button, display will show dashes '--' until you release the shield button to confirm advanced mode.
- Join hosted game by holding reload (dome light is blinking).
- Once you have joined (dome light stops blinking and is solid lit), press the shield button twice to tell the display the game is ready.
- The display will wait for the dome light to go dark, indicating the hosted game has started, a 10 second countdown begins. :)

Number Entry:
Note: The circuit has a memory, the last entered health / shield numbers will always be displayed when advanced mode is selected.
- Turn on tagger while holding the shield button, this wakes the display and enters advanced mode.  The display will show dashes '--' to indicate selection.
- Let go of shield button and the health number is displayed first.
- To change the number, pull the trigger.  You may also hold the trigger down for faster number entry.
- Press the shield button to save the health number.
- The shield number is now displayed second.
- You may also change this number.
- Press the shield button a second time to save the shield number.
- The game will begin after a 10 second countdown begins.

----------------------------------------------------------------------

Programming Stuff:
Compiled with cloud based Clang Compiler on codebender.cc

Revisions List:
11/15/09  Initial program, Display pins configured, Count loops added
11/16/09  Firing modes added, Low Power loops added
12/09/09  Added device select for different Board Revisions
07/25/13  Re-wrote display subroutines to be more efficient - http://pastebin.com/7wjmQj6H
08/12/13  Converted to Arduino Compiler - http://pastebin.com/nezr1ZxJ
12/27/13  Now for lasertag.  Wire FX to the HIT+ Signal from the LTAR main board. LED to shield.
12/28/13  Ported from Arduino Due to ATTINY4313
12/29/13  Expanded sleep modes to cover more timeouts. - http://pastebin.com/RtXGVq4y
12/31/13  Added advanced number selection for hosted gametypes. - http://pastebin.com/huP5bcie
01/04/14  Debugged advanced number selection, added fast number selection. - http://pastebin.com/S2HmMAt9

ATTiny4313 Pinouts [Arduino]

                +----v---+
    [17]   (RST)| 01  20 |(VCC)         [-]
    [0]     PD0 | 02  19 | PB7 (SCL)    [16]
    [1]     PD1 | 03  18 | PB6 (MISO)   [15]
    [2]     PA1 | 04  17 | PB5 (MOSI)   [14]
    [3]     PA0 | 05  16 | PB4          [13]
    [4]     PD2 | 06  15 | PB3          [12]
    [5]     PD3 | 07  14 | PB2          [11]
    [6]     PD4 | 08  13 | PB1          [10]
    [7]     PD5 | 09  12 | PB0          [9]
    [-]    (GND)| 10  11 | PD6          [8]
                +--------+
                 17 Total

7-Segment Display Layout

         1A            2A
       ------        ------
      |      |      |      |
    1F|      |1B  2F|      |2B
      |  1G  |      |  2G  |
       ------        ------
      |      |      |      |
    1E|      |1C  2E|      |2C
      |  1D  |      |  2D  |
       ------  .     ------  .

Pinout Mapping

Arduino  I/O  Signal Type      Name
-------------------------------------
   0      O   Display Output   2C
   1      O   Display Output   2G
   2      O   Display Output   2D
   3      O   Display Output   2E
   4      O   Display Output   1C
   5      O   Display Output   1D
   6      O   Display Output   1E
   7      I   Button Input     Fire Button
   8      O   Display Output   1F
   9      O   Display Output   1G
   10     O   Display Output   1A
   11     O   Display Output   1B
   12     I   Button Input     Hit Beacon Light
   13     I   Button Input     Shield Button
   14     O   Display Output   2F
   15     O   Display Output   2A
   16     O   Display Output   2B

----------------------------------------------------------------------
*/

//--------------------------------------
//              Includes               |
//--------------------------------------
#include <EEPROM.h>
#include <avr/sleep.h>

//--------------------------------------
//         Globals & Constants         |
//--------------------------------------

/* EEPROM stuff */
#define ADVANCED_HEALTH_ADDRESS 46
#define ADVANCED_SHIELD_ADDRESS 47

/* Health and Shield values */
int health;
int shields_remaining;

long shield_timing;

/* Pin Assignments */
int Hit_Beacon_Pin = 12;
int Shield_Pin     = 13;
int Fire_Pin       = 7;

/* Pin Assignment look up tables */
byte pinMap_digit1[7] =
{
	10,   // 1A
	11,   // 1B
	4,    // 1C
	5,    // 1D
	6,    // 1E
	8,    // 1F
	9,    // 1G
};

byte pinMap_digit2[7] =
{
	15,   // 2A
	16,   // 2B
	0,    // 2C
	2,    // 2D
	3,    // 2E
	14,   // 2F
	1     // 2G
};

//--------------------------------------
//              Functions              |
//--------------------------------------

void setup()
{
	int segment;
	long startup_timeout = 0;
	long timeout_max = 61000;

	/* Set Pin Directions */
	for (segment = 0; segment < 7; segment++)
	{
		pinMode( pinMap_digit1[segment], OUTPUT);
		pinMode( pinMap_digit2[segment], OUTPUT);
	}

	pinMode(Hit_Beacon_Pin, INPUT);
	pinMode(Shield_Pin, INPUT);
	pinMode(Fire_Pin, INPUT);

	/* ====================================
	          Game Type Collection
	   ==================================== */

	/* Powered on while holding the Shield button */
	if (button_is_pressed(Shield_Pin))
	{
		/* ====================================
		             Advanced Mode
		   ==================================== */

		/* Display Dashes */
		for (int i = 0; i < 7; i = i + 1)
		{
			digitalWrite( pinMap_digit1[i], (0b01000000 >> i) & 1);
			digitalWrite( pinMap_digit2[i], (0b01000000 >> i) & 1);
		}

		/* Wait for button release */
		while (button_is_pressed(Shield_Pin));

		timeout_max = 110000;

		//get last used numbers from eeprom
		health = EEPROM.read(ADVANCED_HEALTH_ADDRESS);
		shields_remaining = EEPROM.read(ADVANCED_SHIELD_ADDRESS);

		/* Reset numbers in EEPROM if unknown */
		if ( ( health > 99 ) || ( shields_remaining > 99 ) )
		{
			EEPROM.write ( ADVANCED_HEALTH_ADDRESS, 30 );
			EEPROM.write ( ADVANCED_SHIELD_ADDRESS, 10 );

			health = 30;
			shields_remaining = 10;
		}

		display_num( health );

		/* Get health and shield numbers from advanced mode */
		advanced_mode();

		display_num( health );

	}
	else
	{
		/* ====================================
		              Normal Mode
		   ==================================== */
		/* Pick up and go, Unhosted Gametypes */

		/* Default game info */
		health = 10;
		shields_remaining = 15;
		display_num( health );

		/* Wait for Fire Button Press (starts game) */
		while (!button_is_pressed(Fire_Pin))
		{
			/* Check for Shield Button Press */
			if(button_is_pressed(Shield_Pin))
			{
				/* Determine selected game type */
				if (health == 10)
				{
					health = 25;
					shields_remaining = 30;
				}
				else
				{
					health = 10;
					shields_remaining = 15;
				}

				/* Display new health count */
				display_num( health );

				/* Button was pressed, reset timeouts */
				startup_timeout = millis();
				timeout_max = 110000;

				/* Do not update the game type selection again until user releases button */
				while (button_is_pressed(Shield_Pin));
			}
			else if ( startup_timeout + timeout_max <= millis())
			{
				/* 60 second timeout has occured during startup */
				shutdown_now();
			}
		}
	}


	/* ====================================
	            Startup Animation
	   ==================================== */

	/* Assume a hosted game, wait for Beacon to go dark */
	while ( button_is_pressed(Hit_Beacon_Pin) );

	/* Pre-game Count Down */
	for( int i = 10; i > 0 ; i--)
	{
		display_num( i );
		delay(1000);
	}
	display_num( 0x00 );
	delay(1000);
	display_num( health );

}

void loop()
{
	/* ====================================
	             Program Run
	   ==================================== */

	/* Wait for Beacon to be lit */
	while (!button_is_pressed(Hit_Beacon_Pin))
	{
		/* Check for shield events */
		shield_handler();
	}

	/* Beacon has lit, update health */
	if(health > 0)
	{

		/* Decrement health and update display */
		display_num( --health );

		/* Wait for Beacon to go dark again */
		while ( button_is_pressed(Hit_Beacon_Pin) );

	}
	else
	{
		/* No health remaining, ignore buttons until reset */

		/* One last display update, ensure zeros */
		display_num( 0x00 );

		/* Keep display on for 60 seconds before shutdown */
		delay(60000);
		shutdown_now();
	}

}

//--------------------------------------
//            Advanced Mode            |
//--------------------------------------
void advanced_mode()
{
	long button_down_time = 0;

	/* While shield is not pressed, read fire presses to select health number */
	while (!button_is_pressed(Shield_Pin))
	{
		/* Check for fire Button Press */
		if(button_is_pressed(Fire_Pin))
		{
			/* Slow selection */

			inc_health();
			button_down_time = millis();

			/* Do not update the game type selection again until user releases button */
			while (button_is_pressed(Fire_Pin))
			{
				if ((button_down_time + 1000) < millis())
				{
					//enter long press, number (fast) selection
					inc_health();
					delay(100);
				}
			}


		}

	} /* end loop: shield button press */

	/* Wait for Shield button to be released */
	while ( button_is_pressed(Shield_Pin) );


	//save health selection for future
	EEPROM.write ( ADVANCED_HEALTH_ADDRESS, health );

	/* Display shield */
	display_num( shields_remaining );

	/* While Shield is not pressed, read shield presses to select shield number */
	while (!button_is_pressed(Shield_Pin))
	{
		/* Check for Fire Button Press */
		if(button_is_pressed(Fire_Pin))
		{

			inc_shield();
			button_down_time = millis();

			/* Do not update the game type selection again until user releases button */
			while (button_is_pressed(Fire_Pin))
			{
				if ((button_down_time + 1000) < millis())
				{
					//enter long press, number (fast) selection
					inc_shield();
					delay(100);
				}
			}
		}

	}

	/* Wait for FIRE button to be released */
	while ( button_is_pressed(Shield_Pin) );

	//save shield selection for future
	EEPROM.write ( ADVANCED_SHIELD_ADDRESS, shields_remaining );

	/* Shield has been pressed twice, proceed to start the game */
}

void inc_health()
{

	if (health < 99)
	{
		health += 1;
	}
	else
	{
		health = 1;
	}

	/* Display new health count */
	display_num( health );

}

void inc_shield()
{

	if (shields_remaining < 99)
	{
		shields_remaining += 1;
	}
	else
	{
		shields_remaining = 1;
	}

	/* Display new shield count */
	display_num( shields_remaining );

}

//--------------------------------------
//          Shutdown Subroutine        |
//--------------------------------------
void shutdown_now()
{

	/* Turn off display */
	for (int i = 0; i < 7; i = i + 1)
	{
		digitalWrite( pinMap_digit1[i], 0);
		digitalWrite( pinMap_digit2[i], 0);
	}

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here
	sleep_enable();              // enables the sleep bit in the mcucr register
	sleep_mode();                // here the device is actually put to sleep!!

	/* Hold Here */
	while(1);
}

//--------------------------------------
//          Shield Subroutines         |
//--------------------------------------
void shield_handler()
{
	int shield_min;

	/* Check for shield button press */
	if (button_is_pressed(Shield_Pin))
	{
		/* Shield button has been pressed (1st press = shield active) */

		/* Save current system time */
		shield_timing = millis();

		/* Display remaining shields */
		display_num(shields_remaining);

		/* Determine the min count allowed for shields to reach */
		if (shields_remaining - 10 > 0)
		{
			shield_min = shields_remaining - 10;
		}
		else
		{
			shield_min = 0;
		}

		/* Wait for shield button to be released */
		while (button_is_pressed(Shield_Pin) && (shields_remaining > shield_min))
		{
			update_shield();
		}

		/* Wait for shield button to be pressed again (2nd press = shield inactive) */
		while (!button_is_pressed(Shield_Pin) && (shields_remaining > shield_min))
		{
			update_shield();
		}

		/* Ensure an acurate shield count */
		shields_remaining = shields_remaining - (( millis() - shield_timing ) / 999 );

		/* Wait for shield button to be released */
		while (button_is_pressed(Shield_Pin) && (shields_remaining > shield_min));
		display_num(health);

		//there is a bug here, after deactivating, if the user continues to hold the shield, hits will not register
		// so... don't do that.


		/* Max time was not used.  Subtract 1 second. */
		if (shields_remaining > shield_min)
		{
			shields_remaining -= 1;
		}

	}
}

/* Some binary flash space savings */
void update_shield()
{
	/* Update shield once per second */
	if(shield_timing + 999 <= millis())
	{
		/* Subtract remaining shields */
		shields_remaining = shields_remaining - (( millis() - shield_timing ) / 999 );

		/* Save current system time */
		shield_timing = millis();

		/* Display remaining shields */  //999 instead of 1000 corrects for rounding
		display_num(shields_remaining);
	}
}

//--------------------------------------
//          Button Subroutines         |
//--------------------------------------

int button_is_pressed(int button )
{
	/* Check if button was pressed for at least 55ms */
	if (digitalRead(button))
	{
		delay(55);
		if (digitalRead(button))
		{
			return true;
		}
	}
	return false;
}

//--------------------------------------
//         Display Subroutines         |
//--------------------------------------
void display_num(int disp_output)
{
	/* Look Up Table */
	byte seg_array[16] =
	{
		//  Segments  Hex Number
		//   GFEDCBA
		0b00111111, // 0  126
		0b00000110, // 1  6
		0b01011011, // 2  91
		0b01001111, // 3  79
		0b01100110, // 4  102
		0b01101101, // 5  109
		0b01111101, // 6  125
		0b00000111, // 7  7
		0b01111111, // 8  127
		0b01100111, // 9  103
		0b01110111, // A  119
		0b01111100, // B  124
		0b00111001, // C  57
		0b01011110, // D  94
		0b01111001, // E  121
		0b01110001  // F  113
		//0b01000000  // -  (This entry is impossible for BCD)
	};

	/* Variables */
	int digit1;
	int digit2;

	/* ====================================
	              BCD Conversion
	   ==================================== */

	/* Decimal Numbers need to be in BCD form */
	disp_output = (disp_output / 10) * 16 + (disp_output % 10);

	digit1 = (disp_output >> 4 ) & 0x0F;;
	digit2 = disp_output & 0x0F;

	/* ====================================
	          Assemble Ouput Digits
	   ==================================== */

	for (int i = 0; i < 7; i = i + 1)
	{
		digitalWrite( pinMap_digit1[i], (seg_array[digit1] >> i) & 1);
		digitalWrite( pinMap_digit2[i], (seg_array[digit2] >> i) & 1);
	}

} /* End Display Num Function */
