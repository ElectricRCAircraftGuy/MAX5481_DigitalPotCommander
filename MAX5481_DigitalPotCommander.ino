/*
MAX5481_serial_commander4
new name: MAX5481_DigitalPotCommander
-Code to command this 10-bit digital potentiometer
--It uses SPI serial protocol:  http://arduino.cc/en/Reference/SPI
By Gabriel Staples
http://www.ElectricRCAircraftGuy.com 
-click "Contact me" at the top of my website to find my email address 
25 May 2014
*/

/*
MAX5481 1024-step Digital Potentiometer Circuit:
Pin  Name      Connect to what?
1    VDD       +5V (Important: connect a 0.1uF [or larger] ceramic capacitor from VDD to GND, as close to the device as possible) [I'm using a 1uF multi-layer ceramic cap]
2    GND       GND/0V
3    CS (SS)   Any Arduino pin, let's use D10, since this pin cannot be used as an input anyway, during SPI operation, or else the ATmega328 hardware will force the 
               Arduino from SPI Master into SPI Slave mode.  See: http://arduino.cc/en/Reference/SPI
4    SCK       Arduino pin D13
5    MOSI      Arduino pin D11
6    SPI/UD    +5V to select SPI mode
7    X
8    X
9    X
10   L         GND/0V; this is the LOW side of the voltage divider
11   W         Vout; this is the wiper on the potentiometer
12   H         +5V; this is the HIGH side of the voltage divider
13   X
14   VSS       GND/0V (make sure to tie this to GND)
*/

/*
===================================================================================================
  LICENSE & DISCLAIMER
  Copyright (C) 2014 Gabriel Staples.  All right reserved.
  
  ------------------------------------------------------------------------------------------------
  License: GNU Lesser General Public License Version 3 (LGPLv3) - https://www.gnu.org/licenses/lgpl.html
  ------------------------------------------------------------------------------------------------
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/
===================================================================================================
*/

/*
Note: for details on SPI, see: http://arduino.cc/en/Reference/SPI
and http://playground.arduino.cc/Code/Spi (including the topic, "Why doesn't my LED turn on after starting SPI?")
*/

#include <SPI.h>

//Global Variables
const byte ledPin = 13; // the number of the LED pin
const uint8_t slaveSelectPin = 10; //SPI Slave Select pin for MAX5481 digital pot; this can be *any* output pin you arbitrarily choose
float VDD_measured = 4.4; //measured supply voltage to the device

void setup()
{
  Serial.begin(115200);
  
  // Set SS to high so a connected slave chip will be "deselected" by default (see SPI.cpp for details: C:\Program Files\Arduino\hardware\arduino\avr\libraries\SPI\SPI.cpp)
  digitalWrite(slaveSelectPin,HIGH);
  //set the digital pot's Slave Select pin as an output
  pinMode(slaveSelectPin,OUTPUT);
  
  //initialize SPI
//  SPI.begin(); 
  ///////////in order to NOT mess up operation of LED 13, do not begin SPI until just before sending SPI data
  
//  SPI.setDataMode(SPI_MODE0); //I think this is the correct mode; I also think this is the default mode, so I'll just comment this out
//  SPI.setDataMode(SPI_MODE2); //test mode
//  SPI.setBitOrder(MSBFIRST); //this is correct; the LSBFIRST bit order definitely does NOT work--it scrambles the commands (I checked); Note: default mode is
                               //"MSBFIRST," so I'll just comment this line out
  
  Serial.println(F("Enter your desired command (0-1023) as an int, with end of line char on.\n"
                 "Type just a \"c\" to copy the EEPROM val to the wiper.\n"
                 "Type a \"t\" for True to have future commands also get stored into the pot's EEPROM.\n"
                 "Type a \"f\" for False to NOT have future commands also get stored into the pot's EEPROM"));
}

void loop()
{
  //local variables
  static boolean writeEEPROM = false;
  
  //Blink LED 13
  static unsigned long LED_blink_delay = 5000; //ms; time between toggles of LED
  blink_LED_13(LED_blink_delay);
  
  //read in serial commands, & update digital pot accordingly
  if (Serial.available()>0)
  {
    if (Serial.peek()=='c')
    {
      Serial.println("copying EEPROM value to wiper register");
      Serial.read(); //read in the 'c'
      Serial.read(); //read in the '\n'
      copy_EEPROM_to_wiper();
    }
    else if (Serial.peek()=='t')
    {
      Serial.println("writeEEPROM = true");
      Serial.read(); //read in the 't'
      Serial.read(); //read in the '\n'
      writeEEPROM = true;
    }
    else if (Serial.peek()=='f')
    {
      Serial.println("writeEEPROM = false");
      Serial.read(); //read in the 'f'
      Serial.read(); //read in the '\n'
      writeEEPROM = false;
    }
    else //the first char coming in is NOT a 'c', 't', or 'f', so it must be the wiper command
    {
      unsigned int command = Serial.parseInt();
      if (Serial.read()=='\n') //read in the last char & ensure it is an end-of-line char
      {
        command = constrain(command,0,1023);
        float V_out_calculated = command/1023.0*VDD_measured;
     
        //print data
        Serial.print("command = "); Serial.println(command);
        Serial.print("V_out_calculated = "); Serial.println(V_out_calculated);
        
        digitalPotWrite(command, writeEEPROM);
      }
    }
  } //end of if serial is available
} //end of loop()


void digitalPotWrite(unsigned int command, boolean writeEEPROM)
{
  //prepare SPI
  SPI.begin();
  
  //local constants
  const byte WRITE_WIPER = 0x00; //command to write to the wiper register only
  const byte WRITE_EEPROM = 0x20; //command to copy the wiper register into the non-volatile memory (EEPROM) of the digital pot
  //const byte COPY_EEPROM_TO_WIPER = 0x30; //command to copy the EEPROM value to the wiper register
  
  //command the new wiper setting (requires sending 3 bytes)
  digitalWrite(slaveSelectPin,LOW); //set the SS pin low to select the chip
  SPI.transfer(WRITE_WIPER); //Byte 1: the command byte
  SPI.transfer(highByte(command<<6)); //Byte 2: the upper 8 bits of the 10-bit command: (D9.D8.D7.D6.D5.D4.D3.D2)
  SPI.transfer(lowByte(command<<6)); //Byte 3: the lower 2 bits of the 10-bit command, with 6 zeros to the right of them: (D1.D0.x.x.x.x.x.x)
  digitalWrite(slaveSelectPin,HIGH); //set the SS pin high to "latch the data into the appropriate control register" (see datasheet pg. 14)
  
  //copy the wiper register into the non-volatile memory (EEPROM) of the digital pot, if commanded (requires sending only 1 byte)
  if (writeEEPROM)
  {
    Serial.println("writing to EEPROM");
//    delay(10); //wait a short time for the previous command to get properly set; this delay is not necessary, apparently, as determined through testing
    digitalWrite(slaveSelectPin,LOW); //set the SS pin low to select the chip
    SPI.transfer(WRITE_EEPROM); //Byte 1: the command byte
    digitalWrite(slaveSelectPin,HIGH); //set the SS pin high to "latch the data into the appropriate control register" (see datasheet pg. 14 & 16)
    delay(13); //wait 13ms (see datasheet pg. 16 under the paragraph titled "Copy Wiper Register to NV Register"--they require a 12ms wait time, so I'll wait 13ms to be sure)
    Serial.println("done writing to EEPROM");
  }
  
  //End SPI communications, so that LED 13 can be used as normal as an indicator
//  delay(1000); //TEST CODE, JUST TO SEE WHAT HAPPENS TO THE LED13 STATE DURING SPI TRANMISSIONS. (note: SPI.begin() simply turns off LED13, no matter what state it was in,
               //and SPI.end() simply restores LED13 back to its previously-commanded output value, no matter what state it was in.
  SPI.end();
}

//copy the value stored in the EEPROM to the wiper register, to command the wiper to go there
void copy_EEPROM_to_wiper()
{
  //prepare SPI
  SPI.begin();
  
  const byte COPY_EEPROM_TO_WIPER = 0x30; //command to copy the EEPROM value to the wiper register
  digitalWrite(slaveSelectPin,LOW); //set the SS pin low to select the chip
  SPI.transfer(COPY_EEPROM_TO_WIPER); //Byte 1: the command byte
  digitalWrite(slaveSelectPin,HIGH); //set the SS pin high to "latch the data into the appropriate control register" (see datasheet pg. 14 & 16)
  
  //End SPI communications, so that LED 13 can be used as normal as an indicator
  SPI.end();
}

//----------------------------------------------------------------------------------------------------------------------------------
//blink_LED_13()
//----------------------------------------------------------------------------------------------------------------------------------
//Blink LED 13
void blink_LED_13(long LED_blink_delay) //be sure to use a *signed* input parameter data type so that 0 (LED solid off) and -1 (LED solid on) are both possible
{
  //local variables
  static unsigned long t_start_LED = millis();
  static boolean LED_state = LOW;
  
  if (LED_blink_delay==0) //if in fail-safe mode
  {
    digitalWrite(ledPin,LOW); //keep the LED off
  }
  else if (LED_blink_delay==-1) //if we want LED steady on
  {
    digitalWrite(ledPin,HIGH); //keep the LED on
  }
  else if (millis() - t_start_LED >= LED_blink_delay) //if time to blink
  {
    t_start_LED = millis(); //ms; update
    LED_state = !LED_state; //toggle LED state
    digitalWrite(ledPin,LED_state);
  }
  //THE BELOW ELSE STATEMENT WAS DEEMED UNNECESSARY, AS I HAVE DISCOVERED that once you do SPI.end(), the previously-commanded output value on the CLK pin 
  //(pin 13) is automatically restored to the pin, apparently via the value retained in a hardware register or something. So, code below not necessary....
//  else
//  {
//    digitalWrite(ledPin,LED_state); //Write the last known LED_state to the pin, to ensure the LED is in the correct state.
//                                    //This else statement, for example, is necessary when using SPI communications in conjunction with LED 13, because when 
//                                    //SPI.begin() is called, LED 13 is automatically turned off so that the SPI clock (SCK) can run on Pin 13 instead.  
//                                    //Once you are done with SPI communications, you can then call SPI.end(), and then call blink_LED_13(), to restore the previous
//                                    //LED 13 state, via this else statement right here.
//  }
} //end of blink_LED_13
