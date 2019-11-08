 /*
 * RTC_MCP7940N_Example_Basic.c
 *
 * Created: 25.4.2018 
 *  Author: Vesa Salminen
		Toiminta: Testataan RTC-piiriä MCP7940N
		          Koodissa on myös viitteitä DS1307 piiriin
 */ 

#define F_CPU 8000000UL
#define F_SCL 100000L // I2C clock speed 100 kHz
#include <avr/io.h>
#include <avr/interrupt.h>
volatile uint8_t trigTimer0_OVF = 0;
volatile uint8_t mTimer0Trig = 0;
volatile uint8_t mINT0_Trig=0;	
#define ClearBit(x,y) x &= ~_BV(y) // equivalent to cbi(x,y)
#define SetBit(x,y) x |= _BV(y) // equivalent to sbi(x,y)
#define TW_START 0xA4 // send start condition (TWINT,TWSTA,TWEN)
#define TW_READY (TWCR & 0x80) // ready when TWINT returns to logic 1.
#define TW_STATUS (TWSR & 0xF8) // returns value of status register
#define TW_SEND 0x84 // send data (TWINT,TWEN)
#define TW_STOP 0x94 // send stop condition (TWINT,TWSTO,TWEN)
#define I2C_Stop() TWCR = TW_STOP // inline macro for stop condition
#define TW_NACK 0x84 // read data with NACK (last byte)
#define READ 1

// I2C bus address of DS1307 RTC for write
//#define DS1307 0xD0
// I2C bus address of MCP7940N RTC for write
#define MCP7940N_WRITE 0xDE

#define SECONDS_REGISTER 0x00
#define MINUTES_REGISTER 0x01
#define HOURS_REGISTER 0x02
#define DAYOFWK_REGISTER 0x03
#define DAYS_REGISTER 0x04
#define MONTHS_REGISTER 0x05
#define YEARS_REGISTER 0x06
#define CONTROL_REGISTER 0x07

#define MCP7940N_ST_BIT_ENABLE       0x80
#define MCP7940N_ST_BIT_DISABLE      0x7F
#define MCP7940N_SQWEN_BIT_ENABLE    0x40
#define MCP7940N_SQWEN_BIT_DISABLE   0xBF
#define MCP7940N_EXTOSC_BIT_ENABLE   0x08
#define MCP7940N_EXTOSC_BIT_DISABLE  0xF7

#define NUMBERS_TABLE_SIZE		10
#define DIGIT_PCS	             5
#define TIME_TABLE_SIZE        5
#define HOUR_10 0
#define HOUR_01 1
#define MIN_10	2
#define MIN_01	3
#define SEC			4

#define DIGIT_1_ON  	PORTC&=~(1<<PC0)
#define DIGIT_1_OFF   PORTC|=(1<<PC0)
#define DIGIT_2_ON  	PORTC&=~(1<<PC1)
#define DIGIT_2_OFF		PORTC|=(1<<PC1)
#define DIGIT_3_ON  	PORTC&=~(1<<PC2)
#define DIGIT_3_OFF		PORTC|=(1<<PC2)
#define DIGIT_4_ON		PORTC&=~(1<<PC3)
#define DIGIT_4_OFF		PORTC|=(1<<PC3)
#define DIGIT_5_ON		PORTD&=~(1<<PD4)
#define DIGIT_5_OFF		PORTD|=(1<<PD4)

void init_INT0(){
	 EICRA |= (1 << ISC01);    // INT0 The falling edge of INT0 generates an interrupt request
	 EIMSK |= (1 << INT0);     // Turns on INT0
}

void I2C_Init(){
// at 16 MHz, the SCL frequency will be 16/(16+2(TWBR)), assuming prescalar of 0.
// so for 100KHz SCL, TWBR = ((F_CPU/F_SCL)-16)/2 = ((16/0.1)-16)/2 = 144/2 = 72.
	TWSR = 0; // set prescalar to zero
	TWBR = ((F_CPU/F_SCL)-16)/2; // set SCL frequency in TWI bit register
}



uint8_t I2C_Start(){// generate a TW start condition
	TWCR = TW_START; // send start condition
	while (!TW_READY); // wait
	return (TW_STATUS==0x08); // return 1 if found; 0 otherwise
}


uint8_t I2C_SendAddr(uint8_t addr){	// send bus address of slave
	TWDR = addr; // load device's bus address
	TWCR = TW_SEND; // and send it
	while (!TW_READY); // wait
	return (TW_STATUS==0x18); // return 1 if found; 0 otherwise
}


uint8_t I2C_Write (uint8_t data){// sends a data byte to slave
	TWDR = data; // load data to be sent
	TWCR = TW_SEND; // and send it
	while (!TW_READY); // wait
	return (TW_STATUS!=0x28); // return 1 if found; 0 otherwise
}


uint8_t I2C_ReadNACK (){ // reads a data byte from slave
	TWCR = TW_NACK; // nack = not reading more data
	while (!TW_READY); // wait
	return TWDR;
}

//void I2C_WriteRegister(byte deviceRegister, byte data)
void I2C_WriteRegister(uint8_t deviceRegister, uint8_t data){
	I2C_Start();
	I2C_SendAddr(MCP7940N_WRITE); // send bus address
	I2C_Write(deviceRegister); // first byte = device register address
	I2C_Write(data); // second byte = data for device register
	I2C_Stop();
}

uint8_t  I2C_ReadRegister(uint8_t  busAddr, uint8_t deviceRegister){	
	uint8_t data = 0;
	I2C_Start();
	I2C_SendAddr(MCP7940N_WRITE); // send device bus address
	I2C_Write(deviceRegister); // set register pointer
	I2C_Start();
	I2C_SendAddr(MCP7940N_WRITE+READ); // restart as a read operation
	data = I2C_ReadNACK(); // read the register data
	I2C_Stop(); // stop
	return data;
}

void MCP7940N_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds){
// returns hours, minutes, and seconds in BCD format
	*hours = I2C_ReadRegister(MCP7940N_WRITE,HOURS_REGISTER);
	*minutes = I2C_ReadRegister(MCP7940N_WRITE,MINUTES_REGISTER);
	*seconds = I2C_ReadRegister(MCP7940N_WRITE,SECONDS_REGISTER);
	
	if (*hours & 0x40) // 12hr mode:
	*hours &= 0x1F; // use bottom 5 bits (pm bit = temp & 0x20)
	else *hours &= 0x3F; // 24hr mode: use bottom 6 bits
}

uint8_t I2C_Detect (uint8_t addr){
	// look for device at specified address; return 1=found, 0=not found
	TWCR = TW_START; // send start condition
	while (!TW_READY); // wait
	TWDR = addr; // load device's bus address
	TWCR = TW_SEND; // and send it
	while (!TW_READY); // wait
	return (TW_STATUS==0x18); // return 1 if found; 0 otherwise
	
}

uint8_t I2C_FindDevice(uint8_t start){
	uint8_t addr=0;
	// returns with address of first device found; 0=not found
	for (addr=start;addr<0xFF;addr++) // search all 256 addresses
	{
		if (I2C_Detect(addr)) // I2C detected?
		return addr; // leave as soon as one is found
	}
	return 0; // none detected, so return 0.
}

void ShowDevices(){
	// Scan I2C addresses and display addresses of all devices found
	uint8_t addr = 1;
	while (addr>0){
		addr = I2C_FindDevice(addr);
		if (addr>0) PORTB=~addr++;
	}
}

void init_Timer0_OVF(){
	TCCR0B |= (1 << CS01) | (1 << CS00);// set prescaler to 64
	//TCCR0B |= (1 << CS02); // set prescaler to 256
	//TCCR0B |= (1 << CS02) | (1 << CS00);// set prescaler to 1024
	TCNT0 = 0; // initialize counter
	// enable overflow interrupt
	TIMSK0 |= (1 << TOIE0);
}

ISR(TIMER0_OVF_vect){
	mTimer0Trig = 1;
}

ISR(INT0_vect){
	mINT0_Trig=1;
}


void digitHandle(const uint8_t dgtIdx, const uint8_t number){
	DIGIT_1_OFF;
	DIGIT_2_OFF;
	DIGIT_3_OFF;
	DIGIT_4_OFF;
	DIGIT_5_OFF;
	
	PORTB = number;
	
	switch(dgtIdx){
		case 0:
			DIGIT_1_ON;
		break;
		
		case 1:
			DIGIT_2_ON;
		break;

		case 2:
			DIGIT_3_ON;
		break;
		
		case 3:
			DIGIT_4_ON;
		break;
		
		case 4:
			DIGIT_5_ON;
		break;
		
		default:
			DIGIT_1_OFF;
			DIGIT_2_OFF;
			DIGIT_3_OFF;
			DIGIT_4_OFF;
			DIGIT_5_OFF;
		break;
	}
}

void MCP7940N_DisableSTOscillator(){
	uint8_t MCP7940Nregister=0;
	MCP7940Nregister=I2C_ReadRegister(MCP7940N_WRITE, SECONDS_REGISTER);
	MCP7940Nregister&=MCP7940N_ST_BIT_DISABLE;	
	I2C_WriteRegister(SECONDS_REGISTER, MCP7940Nregister);
}

MCP7940N_DisableSQWEscillator(){
	uint8_t MCP7940Nregister=0;
	MCP7940Nregister=I2C_ReadRegister(MCP7940N_WRITE, CONTROL_REGISTER);
	MCP7940Nregister&= MCP7940N_SQWEN_BIT_DISABLE;
	I2C_WriteRegister(CONTROL_REGISTER, MCP7940Nregister);
}

void MCP7940N_EnableSTOscillator(){
	uint8_t MCP7940Nregister=0;
	MCP7940Nregister=I2C_ReadRegister(MCP7940N_WRITE, SECONDS_REGISTER);
	MCP7940Nregister|=MCP7940N_ST_BIT_ENABLE;
	I2C_WriteRegister(SECONDS_REGISTER, MCP7940Nregister);
}

void MCP7940N_EnableSQWEN(){
	uint8_t MCP7940Nregister=0;
	MCP7940Nregister=I2C_ReadRegister(MCP7940N_WRITE, CONTROL_REGISTER);
	MCP7940Nregister|=MCP7940N_SQWEN_BIT_ENABLE;
	I2C_WriteRegister(CONTROL_REGISTER, MCP7940Nregister);
}


int main(void){
	uint8_t tableIndex  = 0;
	uint8_t digitIndex  = 0;
	uint8_t numCompl    = 0;
	uint16_t test =0;
	//Tunnit_10, Tunnit_01, Minuutit_10, Minuutit_01, Sekunnit
	uint8_t timeTable [TIME_TABLE_SIZE] = {0, 0, 0, 0, 0};
	uint8_t hours=0, minutes=0, seconds=0;
	const uint8_t numbers[NUMBERS_TABLE_SIZE]={0x40,0x79,0x24,0x30,0x19,\
                                     0x12,0x02,0x78,0x00,0x18};

	DDRB = 0xff; PORTB = 0xff;
	DDRD = 0x70; PORTD = 0x70; //MCP7940N MFP to PD2: must be input
	DDRC = 0x4f; // I2C bus: SCL (PC5) and SDA (PC4) must be input 
	PORTC = 0x0f;	
	init_INT0();
	init_Timer0_OVF();
	I2C_Init();
	sei();	
  
	//ShowDevices();
	//_delay_ms(2000);
	//PORTB=0xff;
	
	/* DS1307: In seconds register bit 7 is "CH" bit. When this bit is set to 1, the oscillator is disabled. When cleared to 0, the oscillator is enabled*/
	//seconds = 0x00; 
	
	/* MCP7940N: In seconds register bit 7 is "ST" bit. 
	Setting this bit to a ‘1’ starts the oscillator and clearing this bit to a ‘0’ stops the on-board oscillator.*/
	
	//TIMEKEEPING REGISTERS are in BCD mode. See datasheet.
	seconds = 0x54;//Just some value for testing
	minutes = 0x24; //Just some value for testing
	hours = 0x15; //Just some value for testing
	
	MCP7940N_DisableSTOscillator();
	I2C_WriteRegister(SECONDS_REGISTER, seconds);
	I2C_WriteRegister(MINUTES_REGISTER, minutes);
  I2C_WriteRegister(HOURS_REGISTER, hours);	
	MCP7940N_EnableSTOscillator();
	MCP7940N_EnableSQWEN();

	while (1){
		
		if(mTimer0Trig == 1){
			if (digitIndex < DIGIT_PCS-1){
				digitHandle(digitIndex, numbers[timeTable[tableIndex]]);
			}
			else{
				numCompl =~(timeTable[tableIndex]);
				digitHandle(digitIndex, numCompl);
			}

			digitIndex++;
			if (DIGIT_PCS <= digitIndex ){
				digitIndex=0;
			}

			tableIndex++;
			
			if(TIME_TABLE_SIZE <= tableIndex){
				tableIndex=0;
			}
			
			mTimer0Trig=0;
		}
		
		if (mINT0_Trig){
			MCP7940N_GetTime(&hours,&minutes,&seconds);
			//Note ST-bit in seconds register...
			timeTable[SEC]=seconds;
			mINT0_Trig=0;
		}
	}
}