/*
 * MCP7940_Func.c
 *
 * Created: 25.4.2018 11.55.43
 * Author : Vesa
 */ 

 // I2C bus address of MCP7940N RTC for write
#define MCP7940N_ADDRESS_WR 0xDE

#define SECONDS_REGISTER   	0x00
#define MINUTES_REGISTER 	0x01
#define HOURS_REGISTER		0x02
#define DAYOFWK_REGISTER 	0x03
#define DAYS_REGISTER 		0x04
#define MONTHS_REGISTER 	0x05
#define YEARS_REGISTER 		0x06
#define CONTROL_REGISTER 	0x07

#define MCP7940N_ST_BIT_ENABLE       0x80
#define MCP7940N_ST_BIT_DISABLE      0x7F
#define MCP7940N_SQWEN_BIT_ENABLE    0x40
#define MCP7940N_SQWEN_BIT_DISABLE   0xBF
#define MCP7940N_EXTOSC_BIT_ENABLE   0x08
#define MCP7940N_EXTOSC_BIT_DISABLE  0xF7
#define MCP7940N_VBATEN_BIT_ENABLE   0x08
#define MCP7940N_VBATEN_BIT_DISABLE  0xf7


void MCP7940N_Enable_ST_Oscillator(){
	uint8_t MCP7940Nregister=0;
	MCP7940Nregister=I2C_ReadRegister(MCP7940N_ADDRESS_WR, SECONDS_REGISTER);
	MCP7940Nregister|=MCP7940N_ST_BIT_ENABLE;
	I2C_WriteRegister(SECONDS_REGISTER, MCP7940Nregister);
}

void MCP7940N_Disable_ST_Oscillator(){
	uint8_t MCP7940Nregister=0;
	MCP7940Nregister=I2C_ReadRegister(MCP7940N_ADDRESS_WR, SECONDS_REGISTER);
	MCP7940Nregister&=MCP7940N_ST_BIT_DISABLE;	
	I2C_WriteRegister(SECONDS_REGISTER, MCP7940Nregister);
}

void MCP7940N_Enable_SQWEN(){
	uint8_t MCP7940Nregister=0;
	MCP7940Nregister=I2C_ReadRegister(MCP7940N_ADDRESS_WR, CONTROL_REGISTER);
	MCP7940Nregister|=MCP7940N_SQWEN_BIT_ENABLE;
	I2C_WriteRegister(CONTROL_REGISTER, MCP7940Nregister);
}

void MCP7940N_Disable_SQWEN(){
	uint8_t MCP7940Nregister=0;
	MCP7940Nregister=I2C_ReadRegister(MCP7940N_ADDRESS_WR, CONTROL_REGISTER);
	
	MCP7940Nregister&= MCP7940N_SQWEN_BIT_DISABLE;
	I2C_WriteRegister(CONTROL_REGISTER, MCP7940Nregister);
}

void MCP7940N_Enable_VBAT(){
	uint8_t MCP7940Nregister=0;
	MCP7940Nregister=I2C_ReadRegister(MCP7940N_ADDRESS_WR, DAYOFWK_REGISTER);
	MCP7940Nregister|=MCP7940N_VBATEN_BIT_ENABLE;
	I2C_WriteRegister(DAYOFWK_REGISTER, MCP7940Nregister);
}

void MCP7940N_Disble_VBAT(){
	uint8_t MCP7940Nregister=0;
	MCP7940Nregister=I2C_ReadRegister(MCP7940N_ADDRESS_WR, DAYOFWK_REGISTER);
	MCP7940Nregister&=MCP7940N_VBATEN_BIT_DISABLE;
	I2C_WriteRegister(DAYOFWK_REGISTER, MCP7940Nregister);
}
 
// Separates higer bits from uint8_t. Set higer bits to 
// lower bits. Etc: 1101 0110 ==> 0000 1101
uint8_t separateHigherBits(uint8_t bValue){
	uint8_t hBits=0;
	hBits=bValue;
	hBits>>=4;
	return hBits;
}

// Separates lower bits from uint8_t.
uint8_t separateLowerBits(uint8_t bValue){
	uint8_t lBits=0;
	lBits=bValue&0x0f;
	return lBits;
}

// Convert MCP7940N RTCSEC style register to uint8_t value.
// In RTCSEC register bit 7 is "Start Oscillator bit". Higher 6-4
// bits are Binary-Coded Decimal Value of Second’s Tens Digit.
// Lower 3-0 bits are Binary-Coded Decimal Value of Second’s Ones Digit
uint8_t convertBCDtoUint8_t(uint8_t byteValue){
	uint8_t lBits=0, hBits=0;
	hBits = byteValue & 0x70; //Clear bit 7 (and lower bits)
	hBits >>=4;
	lBits = byteValue & 0x0f; //Clear higher bits
	byteValue=0;
	for (uint8_t i=0;i<hBits*10;i++){
		byteValue++;
	}
	for (uint8_t i=0;i<lBits;i++){
		byteValue++;
	}
	return byteValue;
}

// Convert uint8_t value type to BCD-value.
//Note: If byteValue is bigger than 99 function returns FFH
//Etc.  1001 1001 is a real BCD-value but 1001 1010 is not a real BCD-value.
uint8_t convert_Uint8_t__To_BCD(uint8_t byteValue){
	uint8_t BCDValue=0;
	if(byteValue<=99){
		BCDValue=( (byteValue/10*16) + (byteValue%10) );
		}else{
		BCDValue=0xff;
	}
	return BCDValue;
}