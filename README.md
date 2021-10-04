# Overview
 A regular alarm clock with the usual functionality you'd expect. 

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

>7654 3210 // ports
>
>0101 1101 // values the corresponding ports above will be set to (0 for input, 1 for output)
>
>8421 8421 // binary positional notation for the first 4 digits

The above would be calculated:
>8\*0, 4\*1, 2\*0, 1\*1 and 8\*1, 4\*1, 2\*0, 1\*1

>\= 5 and 13



Now we just need to have the resulting value in hexadecimal:

>All 16 hex digits, below them the corresponding values in decimal:    
>0 1 2 3 4 5 6 7 8 9 A  B  C  D  E  F  
>0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
>
>5 in hex  is: 5
>13 in hex is: D

So our value in hexadecimal is 5D, designated in C-language with 0x-prefix (for hexadecimal) as so: 0x5D

# Circuit
![image](https://user-images.githubusercontent.com/57489963/126193202-59e45f0e-cae4-4921-84c5-8d0e8f2d806b.png)


# Credits
Circuit design by Vesa Salminen
