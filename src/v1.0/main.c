/*
 * avr-alarm-clock v1.0.c
 *
 * Created: 20.9.2018 12.35.28
 * Author : Viljami Mäkelä
 * Notes  : Awfully written vocational school project, kept mostly intact for checking back in future
 *			to see how far I've come in terms of programming skill.
 *
 */
 
#define F_CPU 1000000UL
#define F_SCL 100000L       // I2C clock speed 100 kHz
#include <avr/io.h>
#include <avr/interrupt.h>
 
 /* GLOBAL VARIABLES */
uint8_t tenminutes = 0;
uint8_t tenhours = 0;
uint8_t hours = 0, minutes = 0, seconds = 0; 
uint8_t alar1 = 0, alar2 = 0, alar3 = 0, alar4 = 0; // stores set alarm

// new stuff to be implemented:
struct alarm // stores set alarm
{
	uint8_t DG1;
	uint8_t DG2;
	uint8_t DG3;
	uint8_t DG4;
};

//implemented new stuff:
struct runtime_registry // wrapped runtime variables
{
	// vars for changing mode & to temporarily store modified time
	uint8_t mode;		// current program mode
	
	uint8_t edit;		// time edit mode (0 = off, 1 = on)
	uint8_t a, b, c, d; // temporary time storage
	
	
	uint8_t acceptTime_once;
	uint8_t acceptAlarm_once;
	
	uint8_t alarm_on;        //if an alarm has been set, the value should be 1
}; 


// to count the number of overflows
volatile uint8_t overflows;
 
//experimental
#define RESET_enter_once {enter_once[0] = 0;enter_once[1] = 0;enter_once[2] = 0;enter_once[3] = 0;enter_once[4] = 0;enter_once[5] = 0;enter_once[6] = 0;enter_once[7] = 0;enter_once[8] = 0;}
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
 
//RTC REGISTERS
 // I2C bus address of MCP7940N RTC for write
#define MCP7940N_ADDRESS_WRITE  0xDE
 
#define SECONDS_REGISTER        0x00        //seconds
#define MINUTES_REGISTER        0x01        //minutes
#define HOURS_REGISTER          0x02        //hours
#define DAYOFWK_REGISTER        0x03        //day of the week
#define DAYS_REGISTER           0x04        //days
#define MONTHS_REGISTER         0x05        //months
#define YEARS_REGISTER			0x06        //years
#define CONTROL_REGISTER        0x07
 
#define MCP7940N_ST_BIT_ENABLE       0x80   //value for enabling RTC/oscillator
#define MCP7940N_ST_BIT_DISABLE      0x7F   //value for disabling RTC/oscillator
#define MCP7940N_SQWEN_BIT_ENABLE    0x40
#define MCP7940N_SQWEN_BIT_DISABLE   0xBF
#define MCP7940N_EXTOSC_BIT_ENABLE   0x08
#define MCP7940N_EXTOSC_BIT_DISABLE  0xF7
 
#define MCP7940N_VBATEN_BIT_ENABLE   0x08
#define MCP7940N_VBATEN_BIT_DISABLE  0xf7
 
 
//RTC FUNCTIONS
void I2C_Init() {
    // at 16 MHz, the SCL frequency will be 16/(16+2(TWBR)), assuming prescaler of 0.
    // so for 100KHz SCL, TWBR = ((F_CPU/F_SCL)-16)/2 = ((16/0.1)-16)/2 = 144/2 = 72.
    TWSR = 0; // set prescaler to zero
    TWBR = ((F_CPU / F_SCL) - 16) / 2; // set SCL frequency in TWI bit register
}
 
uint8_t I2C_Start() {// generate a TW start condition
    TWCR = TW_START; // send start condition
    while (!TW_READY); // wait
    return (TW_STATUS == 0x08); // return 1 if found; 0 otherwise
}
 
uint8_t I2C_SendAddr(uint8_t addr) {    // send bus address of slave
    TWDR = addr; // load device's bus address
    TWCR = TW_SEND; // and send it
    while (!TW_READY); // wait
    return (TW_STATUS == 0x18); // return 1 if found; 0 otherwise
}
 
uint8_t I2C_Write(uint8_t data) {// sends a data byte to slave
    TWDR = data; // load data to be sent
    TWCR = TW_SEND; // and send it
    while (!TW_READY); // wait
    return (TW_STATUS != 0x28); // return 1 if found; 0 otherwise
}
 
uint8_t I2C_ReadNACK() { // reads a data byte from slave
    TWCR = TW_NACK; // nack = not reading more data
    while (!TW_READY); // wait
    return TWDR;
}
 
//void I2C_WriteRegister(MODIFIABLE REGISTER, MODIFYING)
void I2C_WriteRegister(uint8_t deviceRegister, uint8_t data) {
    I2C_Start();
    I2C_SendAddr(MCP7940N_ADDRESS_WRITE); // send bus address
    I2C_Write(deviceRegister); // first byte = device register address
    I2C_Write(data); // second byte = data for device register
    I2C_Stop();
}
 
uint8_t  I2C_ReadRegister(uint8_t  busAddr, uint8_t deviceRegister) {
    uint8_t data = 0;
    I2C_Start();
    I2C_SendAddr(MCP7940N_ADDRESS_WRITE); // send device bus address
    I2C_Write(deviceRegister); // set register pointer
    I2C_Start();
    I2C_SendAddr(MCP7940N_ADDRESS_WRITE + READ); // restart as a read operation
    data = I2C_ReadNACK(); // read the register data
    I2C_Stop(); // stop
    return data;
}
 
//Enable RTC oscillator
void MCP7940N_Enable_ST_Oscillator() {
    uint8_t MCP7940Nregister = 0;
    MCP7940Nregister = I2C_ReadRegister(MCP7940N_ADDRESS_WRITE, SECONDS_REGISTER);
    MCP7940Nregister |= MCP7940N_ST_BIT_ENABLE;
    I2C_WriteRegister(SECONDS_REGISTER, MCP7940Nregister);
}
 
void MCP7940N_Disable_ST_Oscillator() {
    uint8_t MCP7940Nregister = 0;
    MCP7940Nregister = I2C_ReadRegister(MCP7940N_ADDRESS_WRITE, SECONDS_REGISTER);
    MCP7940Nregister &= MCP7940N_ST_BIT_DISABLE;
    I2C_WriteRegister(SECONDS_REGISTER, MCP7940Nregister);
}
 
//SQUARE WAVE OUTPUT ON-OFF
void MCP7940N_Enable_SQWEN() {
    uint8_t MCP7940Nregister = 0;
    MCP7940Nregister = I2C_ReadRegister(MCP7940N_ADDRESS_WRITE, CONTROL_REGISTER);
    MCP7940Nregister |= MCP7940N_SQWEN_BIT_ENABLE;
    I2C_WriteRegister(CONTROL_REGISTER, MCP7940Nregister);
}
 
void MCP7940N_Disable_SQWEN() {
    uint8_t MCP7940Nregister = 0;
    MCP7940Nregister = I2C_ReadRegister(MCP7940N_ADDRESS_WRITE, CONTROL_REGISTER);
 
    MCP7940Nregister &= MCP7940N_SQWEN_BIT_DISABLE;
    I2C_WriteRegister(CONTROL_REGISTER, MCP7940Nregister);
}

 // BATTERYUSAGE ON-OFF
 void MCP7940N_Enable_VBAT(){
	 uint8_t MCP7940Nregister=0;
	 MCP7940Nregister=I2C_ReadRegister(MCP7940N_ADDRESS_WRITE, DAYOFWK_REGISTER);
	 MCP7940Nregister|=MCP7940N_VBATEN_BIT_ENABLE;
	 I2C_WriteRegister(DAYOFWK_REGISTER, MCP7940Nregister);
 }

 void MCP7940N_Disable_VBAT(){
	 uint8_t MCP7940Nregister=0;
	 MCP7940Nregister=I2C_ReadRegister(MCP7940N_ADDRESS_WRITE, DAYOFWK_REGISTER);
	 MCP7940Nregister&=MCP7940N_VBATEN_BIT_DISABLE;
	 I2C_WriteRegister(DAYOFWK_REGISTER, MCP7940Nregister);
 }
 
// FUNCTIONS FOR GETTING TIME
uint8_t DEC2BCD(uint8_t Decimal) {  // DECIMAL TO BCD CONVERTER
    return(Decimal / 10) << 4 | (Decimal % 10);
}
 
void MCP7940N_GetTime(uint8_t* hours, uint8_t* minutes, uint8_t* seconds) {
    // returns hours, minutes, and seconds in BCD format
    *hours = I2C_ReadRegister(MCP7940N_ADDRESS_WRITE, HOURS_REGISTER);
    *minutes = I2C_ReadRegister(MCP7940N_ADDRESS_WRITE, MINUTES_REGISTER);
    *seconds = I2C_ReadRegister(MCP7940N_ADDRESS_WRITE, SECONDS_REGISTER);
 
    if (*hours & 0x40) // 12hr mode:
        * hours &= 0x1F; // use bottom 5 bits (pm bit = temp & 0x20)
    else *hours &= 0x3F; // 24hr mode: use bottom 6 bits
}
 
uint8_t BCD2DEC(unsigned char x) {
    return x - 6 * (x >> 4);
}
 
// DIGIT ON-OFF
#define DG1_ON  PORTC&=~(1 << PC0)
#define DG1_OFF PORTC|=(1 << PC0)
 
#define DG2_ON  PORTC&=~(1 << PC1)
#define DG2_OFF PORTC|=(1 << PC1)
 
#define DG3_ON  PORTC&=~(1 << PC2)
#define DG3_OFF PORTC|=(1 << PC2)
 
#define DG4_ON  PORTC&=~(1 << PC3)
#define DG4_OFF PORTC|=(1 << PC3)
 
// LETTERS
#define AA   0x88
#define CC   0xC6 
#define Cc   0xA7
#define Dd   0xA1
#define EE   0x86
#define FF   0x8E
#define GG   0x02
#define II   0xCF
#define LL   0xC7
#define NN   0xAB
#define Oo   0xA3
#define PP   0x8C
#define RR   0xAF
#define SS   0x92
#define TT   0x87
#define UU   0xE3
#define YY   0x11
#define BL   0xFF


//BUTTONS LEFT TO RIGHT (SW4-SW2)
uint8_t     SW4 = 0;
uint8_t     SW3 = 0;
uint8_t     SW2 = 0;
 
//DIGITS & PIEZO
void DG_ALLON()
{
    DG1_ON;
    DG2_ON;
    DG3_ON;
    DG4_ON;
}
 
void DG_ALLOFF()
{
    DG1_OFF;
    DG2_OFF;
    DG3_OFF;
    DG4_OFF;
}
 
#define PZ_ON   PORTD &=~(1 << PD5) 
#define PZ_OFF  PORTD |= (1 << PD5) 
 
#define WL_ON   PORTD &=~(1 << PD6)
#define WL_OFF  PORTD |= (1 << PD6)
 
// FUNCTIONS //

void word(uint8_t DG1, uint8_t DG2, uint8_t DG3, uint8_t DG4) // displays a word
{	const uint8_t DGs[] = { DG1, DG2, DG3, DG4 };
    DG_ALLOFF();
    static int place = 0;
    place = (place + 1) % 4;
	PORTB = DGs[place];
    switch (place)
    {
        case 0:
        {
            DG1_ON;
            break;
        }
        case 1:
        {
            DG2_ON;
            break;
        }
        case 2:
        {
            DG3_ON;
            break;
        }
        case 3:
        {
            DG4_ON;
            break;
        }
    }
}
 
// CURRENT TIME //
const uint8_t num[10] = { 0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x10 };
 
void display(uint8_t DG1, uint8_t DG2, uint8_t DG3, uint8_t DG4)
{
    const uint8_t DGs[] = { DG1, DG2, DG3, DG4 };
    DG_ALLOFF();
    static int place = 0;
    place = (place + 1) % 4;
    PORTB = num[DGs[place]];
    switch (place)
    {
        case 0:
        {
            DG1_ON;
            break;
        }
        case 1:
        {
            DG2_ON;
            break;
        }
        case 2:
        {
            DG3_ON;
            break;
        }
        case 3:
        {
            DG4_ON;
            break;
        }
    }
}
// 0 = testing
// 1 = set time             (after initial setup accessible via SW4 + SW3 + SW2)
// 2 = current time
// 3 = set alarm
// 4 = overflow to mode 2
 
//fixed version
void I2C_WRITEREGISTER(uint8_t deviceRegister, uint8_t data) {
    I2C_Start();
    I2C_SendAddr(MCP7940N_ADDRESS_WRITE); // send bus address
    I2C_Write(deviceRegister); // first byte = device register address
    I2C_Write(data); // second byte = data for device register
    I2C_Stop();
 
    uint8_t load = 0x00;
    I2C_WriteRegister(SECONDS_REGISTER, load);
    MCP7940N_Enable_ST_Oscillator(); MCP7940N_Enable_SQWEN();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 

void acceptTime(struct runtime_registry rreg);
void acceptAlarm(struct runtime_registry rreg);
 
void translateRTC()
{
    MCP7940N_GetTime(&hours, &minutes, &seconds); //AJAN KÄÄNTÖ
	tenhours	=	(hours >> 4) & 0x7;
    hours		=	hours & 0xf;
	tenminutes	=	(minutes >> 4) & 0x7;
    minutes		=	minutes & 0x0f;
	
    seconds = BCD2DEC(seconds);
}
 
int main()
{
    seconds = 0x00; minutes = 0x00; hours = 0x00; I2C_WriteRegister(SECONDS_REGISTER, seconds); I2C_WriteRegister(MINUTES_REGISTER, minutes); I2C_WriteRegister(HOURS_REGISTER, hours);
    MCP7940N_Enable_ST_Oscillator(); MCP7940N_Enable_SQWEN();MCP7940N_Enable_VBAT();
 
    // I/O ALUSTUKSET //
    DDRB = 0xFF;                    //LED PATTERN REGISTER
    DDRC = 0x0F; PORTC = 0x00;      //DIGITs (on-off)
    DDRD = 0x70; PORTD = 0x60;      //BUTTONS REGISTER
	
	struct runtime_registry rreg = {.mode=1, .edit=0, .a=0, .b=0, .c=0, .d=0, .acceptTime_once=0, .acceptAlarm_once=0, .alarm_on=0};
     
    rreg.mode = 1;               //start-up mode, default=1 (mode 0 reserved for experimenting)
    rreg.edit = 1;               // 0 - not editing, 1 - editing (time)
 
    int delay_actual	= 500;
    int delay_holder	= 0;
    int enter_actual	= 0;   
    int enter_holder	= 0;
	int enter_once[8]	= {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t first_setup = 0;
 
    uint8_t mode2_txtreset = 0;
    uint8_t mode3_txtreset = 0;
 
    // OHJELMAKIERTO  //
    while (1)
    {
        SW4 = PIND & 0x02; // You can read status from variable (0 = when pressed)
        SW3 = PIND & 0x01;
        SW2 = PIND & 0x08;
        DG_ALLOFF();
		if(SW4 == 0)			{delay_actual=1;}
		if(SW3 == 0 && SW2 == 0){delay_actual=10;}
		if(SW3 != 0 && SW2 != 0){delay_actual=400;}
		
		
		
        //MODE

        if (rreg.mode == 0) {
			PZ_ON;
			  if (delay_holder > delay_actual) { ++rreg.mode, delay_holder = 0; }
			  else { ++delay_holder;PZ_OFF; }
			
        } //reserved for testing
 
        if (SW4 == 0 && SW3 != 0 && SW2 != 0) {
            
            if (delay_holder > delay_actual) { ++rreg.mode, delay_holder = 0; }
            else { ++delay_holder; }
 
        }
 
        if (SW4 == 0 && SW3 == 0 && SW2 == 0 && first_setup >= 2 && rreg.mode != 3)
        {
            rreg.acceptTime_once = 0; rreg.mode = 1; mode2_txtreset = 0; enter_holder = 0; rreg.edit = 1; rreg.a = tenhours, rreg.b = hours, rreg.c = tenminutes, rreg.d = minutes;
        } //enter time adjustment
///////////////////////////////////////////////////////////////
        if (rreg.mode == 1)
        {	display(rreg.a, rreg.b, rreg.c, rreg.d); // display modified time value to be written	
			if (enter_once[0] == 0) {enter_holder=0;enter_actual=20000;++enter_once[0];} else {enter_once[0] = 1;}
        setup_start:
            if (first_setup == 0) {
                word(FF, RR, SS, TT);
 
                if (enter_holder > enter_actual) { ++first_setup; /*continue*/; }
                else { ++enter_holder; goto setup_start; };
            }
 
            else if (first_setup >= 2 && rreg.edit == 1) {
                word(EE, Dd, II, TT);
				if (enter_once[1] == 0) {enter_holder=0;enter_actual=20000;++enter_once[1];} else {enter_once[1] = 1;}
				
                if (enter_holder > enter_actual) { ++rreg.edit/*continue*/; }
                else { ++enter_holder; goto setup_start; }
 
            }
 
 
            if (rreg.edit == 2) { enter_holder = 900; }
 
            
        }
 
        if (rreg.mode == 2) //DISPLAY CURRENT TIME
        {
            if (rreg.edit >= 2) { --rreg.edit;}	//initial statements
			if (enter_once[2] == 0) {enter_holder=0;enter_actual=15000;++enter_once[2];} else {enter_once[2] = 1;}	//
			display(tenhours, hours, tenminutes, minutes); // show current time			
        setup_end:
            if (mode2_txtreset == 0 && rreg.edit == 1) {
                 word(GG, Oo, Oo, Dd); 
 
                if (enter_holder > enter_actual) { ++first_setup;acceptTime(rreg); /*continue*/; }
                else { ++enter_holder; goto setup_end; };
 
            } rreg.edit = 0;
 
            //AFTER SETUP (if first_setup is 2 or more, it was complete)
			if (enter_once[3] == 0) {enter_holder=0;enter_actual=20000;++enter_once[3];} else {enter_once[3] = 1;}
        entrytext_mode2:
            if (mode2_txtreset == 0) { word(CC, UU, RR, RR); }
 
            if (enter_holder > enter_actual) { ++mode2_txtreset; if (mode2_txtreset != 0) { mode2_txtreset = 1; }; /*continue*/; }
            else { ++enter_holder; goto entrytext_mode2; };
 
            translateRTC();
        }
 
        // ALARM MODE
        if (rreg.alarm_on == 2 && SW3 == 0) { rreg.alarm_on = 0; PZ_OFF;WL_OFF;rreg.acceptAlarm_once = 0;}
        if (rreg.alarm_on == 1)
        {
            if
                (
                    alar1 == tenhours &&
                    alar2 == hours &&
                    alar3 == tenminutes &&
                    alar4 == minutes
                )
            {
                PZ_ON; rreg.alarm_on = 2;WL_ON;
            }
 
        }
		
        if (rreg.mode == 3) //SET ALARMS (loans a function form mode 1)
        {display(rreg.a, rreg.b, rreg.c, rreg.d); // display value to be written
			    
			      if (SW2==0 && SW4==0 && SW3==0)
			      {	rreg.acceptAlarm_once = 0;
			          uint8_t temp = 0; rreg.alarm_on = 0;
						if (enter_once[4] == 0) {enter_holder=0;enter_actual=20000;++enter_once[4];} else {enter_once[4] = 1;}
			      alarm_declined:
			          if (temp == 0) { word(CC, NN, CC, LL); }
			          if (enter_holder > enter_actual) { ++temp; if (temp != 0) { temp = 1; } goto canceled/*continue*/; }
			          else { ++enter_holder; goto alarm_declined; }
			      }
	
						if (enter_once[5] == 0) {enter_holder=0;enter_actual=20000;++enter_once[5];} else {enter_once[5] = 1;}
			  entrytext_mode3:
			      if (mode3_txtreset == 0) { word(AA, LL, AA, RR); }
			      if (enter_holder > enter_actual) { ++mode3_txtreset; if (mode3_txtreset != 0) { mode3_txtreset = 1; }; /*continue*/; }
			      else { ++enter_holder; goto entrytext_mode3; };
					  //welcome back
			      if (SW2 == 0 && SW4 == 0) {
			          acceptAlarm(rreg); uint8_t temp = 0; rreg.alarm_on = 1;
						if (enter_once[6] == 0) {enter_holder=0;enter_actual=20000;++enter_once[6];} else {enter_once[6] = 1;}
			      alarm_accepted:
			          if (temp == 0) { word(GG, Oo, Oo, Dd); }
			          if (enter_holder > enter_actual) { ++temp; if (temp != 0) { temp = 1; };/*continue*/ /*experimental*/ }
			          else { ++enter_holder; goto alarm_accepted; };
 
			      canceled:
				  
			          mode2_txtreset = 0;
					  
					  //reinvent easier way later
					  RESET_enter_once;
						
						 /*enter_holder = 900;*/ rreg.mode = 2;
			      }
			      
			      
			  }
 
 
        //END STATEMENTS BEFORE MODECYCLE
        if (rreg.mode != 2) { mode2_txtreset = 0; }
        if (rreg.mode != 3) { mode3_txtreset = 0; }
        if (rreg.mode == 4) { RESET_enter_once; rreg.mode = 2; }                    //MODE OVERFLOW (return to first mode)
        if (first_setup == 2) { first_setup = 3; }
 
        // mode 1: set current time (FIRST MODE AFTER POWERCYCLE)
        if (rreg.mode == 1 || rreg.mode == 3)
        {
            if (SW2 == 0 && SW3 != 0) //TIME ADDITION
            {
                if (rreg.d <= 9)
                {
                    if (delay_holder > delay_actual) { ++rreg.d, delay_holder = 0; }
                    else { ++delay_holder; }
                }
                else { ++rreg.c, rreg.d = 0; }
 
                if (rreg.c > 5) { rreg.c = 0; ++rreg.b; }
                if (rreg.b > 9) { rreg.b = 0; ++rreg.a; }
                if (rreg.a == 2 && rreg.b == 3 && rreg.c == 5 && rreg.d == 9) { rreg.a = 0, rreg.b = 0, rreg.c = 0, rreg.d = 0; }
            }
 
            if (SW3 == 0 && SW2 != 0) //TIME DEDUCTION
            {
                if (rreg.d >= 0)
                {
                    if (delay_holder > delay_actual) { --rreg.d, delay_holder = 0; }
                    else { ++delay_holder; }
                }
 
                if (rreg.d > 250)
                {
                    rreg.d = 9;
                    rreg.c--;
                    if (rreg.c > 250)
                    {
                        rreg.c = 5;
                        rreg.b--;
                        if (rreg.b > 250)
                        {
                            rreg.b = 9;
                            rreg.a--;
                            if (rreg.a > 250)
                            {
                                rreg.a = 2;
                                rreg.b = 3;
                                rreg.c = 5;
                                rreg.d = 9;
                            }
                        }
                    }
                }
            }
        }
    }
}

void acceptTime(struct runtime_registry rreg)
{
	if (rreg.acceptTime_once == 0)
	{
		I2C_WRITEREGISTER(0x02, (rreg.a << 4) | (rreg.b % 10));
		I2C_WRITEREGISTER(0x01, (rreg.c << 4) | (rreg.d % 10));
	}
	
	rreg.acceptTime_once = 1;
}

void acceptAlarm(struct runtime_registry rreg)
{
	if (rreg.acceptAlarm_once == 0) {
		alar1 = rreg.a;
		alar2 = rreg.b;
		alar3 = rreg.c;
		alar4 = rreg.d;
		rreg.alarm_on = 1;
	}
	rreg.acceptAlarm_once = 1;
}
