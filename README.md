# Overview
 A digital clock with alarm functionality. Designed for Atmel ATmega328P microchip, software written in C.
 
 User manuals: [ENG](/resources/Instruction_manuals/User_manual.pdf) [FI](/resources/Instruction_manuals/Käyttöohje.pdf)
 
 The solution file should be opened in Atmel Studio 7.

## Port configuration
This portion contains values that the ports of the microcontroller (Atmega328P) should be initialized to.

* Port B: 0x00 (all pins are inputs)
* Port C: 0x0F 
  * PC7: 0/1 Unused pin
  * PC6/RST: ?
  * PC4-5: in
  * PC0-3: out
* Port D: 0xE0 
  * PD7: 0/1	Unused pin
  * PD6: 1   Wakeup light
  * PD5: 1   Piezo element
  * PD4: 0   DIGITS on/off
  * PD3: 0 	 Button
  * PD2: 0 	 RTC
  * PD1: 0		 Button
  * PD0: 0		 Button

However if they need customization, you may use the following information to set the i/o:

>7654 3210 // ports represented in a byte
>
>xxxx xxxx // values the corresponding ports above will be set to (0 for input, 1 for output- example: 0000 1111 = 7654 are inputs, 3210 are outputs)
>
>8421 8421 // binary positional notation for each nibble (4-bit group) of the byte above, respectively - we will use this in the next phase


Next up, we need to convert from binary to decimal

If we had a value 0101 1101, it would be calculated like this:
>8\*0 + 4\*1 + 2\*0 + 1\*1 and 8\*1 + 4\*1 + 2\*0 + 1\*1

>\= 5 and 13, for each nibble of the byte, respectively



Now we just need to have the resulting decimal values converted into hexadecimal.

Read this like a dictionary, e.g. 10 means A, 8 means 8, 15 means F:

>   
>0 : 0 
>
>1 : 1
>
>2 : 2
>
>3 : 3
>
>4 : 4
>
>5 : 5
>
>6 : 6
>
>7 : 7
>
>8 : 8
>
>9 : 9
>
>10 : A
>
>11 : B
>
>12 : C
>
>13 : D
>
>14 : E
>
>15 : F

So if we take our example value (0101 1101 aka. 5 and 13):
>5 in hex  is: 5
>
>13 in hex is: D

So our value in hexadecimal is 5D, designated in C-language with 0x-prefix (for hexadecimal) as so: 0x5D

# Circuit
![image](https://user-images.githubusercontent.com/57489963/126193202-59e45f0e-cae4-4921-84c5-8d0e8f2d806b.png)


# Credits
Circuit design by Vesa Salminen
