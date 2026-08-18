/*
  ADE9000API.cpp - Library for ADE9000/ADE9078 - Energy and PQ monitoring AFE
  Date: 3-16-2017
*/
#include <Arduino.h>
#include <SPI.h>
#include "ADE9000API.h"
#include <Wire.h>

/*EEPROM data structure*/
uint32_t ADE9000_CalibrationRegAddress[CALIBRATION_CONSTANTS_ARRAY_SIZE]={ADDR_AIGAIN, ADDR_BIGAIN, ADDR_CIGAIN, ADDR_NIGAIN,ADDR_AVGAIN, ADDR_BVGAIN, ADDR_CVGAIN,ADDR_APHCAL0, ADDR_BPHCAL0, ADDR_CPHCAL0, ADDR_APGAIN, ADDR_BPGAIN, ADDR_CPGAIN};
uint32_t ADE9000_Eeprom_CalibrationRegAddress[CALIBRATION_CONSTANTS_ARRAY_SIZE]={ADDR_AIGAIN_EEPROM, ADDR_BIGAIN_EEPROM, ADDR_CIGAIN_EEPROM, ADDR_NIGAIN_EEPROM,ADDR_AVGAIN_EEPROM, ADDR_BVGAIN_EEPROM, ADDR_CVGAIN_EEPROM,ADDR_APHCAL0_EEPROM, ADDR_BPHCAL0_EEPROM, ADDR_CPHCAL0_EEPROM, ADDR_APGAIN_EEPROM, ADDR_BPGAIN_EEPROM, ADDR_CPGAIN_EEPROM};

ADE9000Class::ADE9000Class()
{
	
}

/* 
Description: Initializes the ADE9000. The initial settings for registers are defined in ADE9000API.h header file
Input: Register settings in header files
Output: 
*/

void ADE9000Class::SetupADE9000(void)
{
	 SPI_Write_16(ADDR_PGA_GAIN,ADE9000_PGA_GAIN);     
	 SPI_Write_32(ADDR_CONFIG0,ADE9000_CONFIG0); 
	 SPI_Write_16(ADDR_CONFIG1,ADE9000_CONFIG1);
	 SPI_Write_16(ADDR_CONFIG2,ADE9000_CONFIG2);
	 SPI_Write_16(ADDR_CONFIG3,ADE9000_CONFIG3);
	 SPI_Write_16(ADDR_ACCMODE,ADE9000_ACCMODE);
	 SPI_Write_16(ADDR_TEMP_CFG,ADE9000_TEMP_CFG);
	 SPI_Write_16(ADDR_ZX_LP_SEL,ADE9000_ZX_LP_SEL);
	 SPI_Write_32(ADDR_MASK0,ADE9000_MASK0);
	// Helper: load persistent MASK1 if available
    SPI_Write_32(ADDR_MASK0, loadPersistentREG("MASK0"));
	SPI_Write_32(ADDR_MASK1, loadPersistentREG("MASK1"));

	SPI_Write_32(ADDR_EVENT_MASK,ADE9000_EVENT_MASK);
	SPI_Write_16(ADDR_WFB_CFG,ADE9000_WFB_CFG);
	SPI_Write_32(ADDR_VLEVEL,ADE9000_VLEVEL);
	SPI_Write_32(ADDR_DICOEFF,ADE9000_DICOEFF);
	SPI_Write_16(ADDR_EGY_TIME,ADE9000_EGY_TIME);
	// Configure PWR_TIME for 1.0 second accumulation windows
	// PWR_TIME = 7999 → N = 8000 samples at FDSP = 8kHz → T = 1.0 second
	SPI_Write_16(ADDR_PWR_TIME, 7999);
	SPI_Write_16(ADDR_EP_CFG,ADE9000_EP_CFG); //Energy accumulation ON
	SPI_Write_16(ADDR_RUN,ADE9000_RUN_ON); //DSP ON
}

// General persistent register save/load helpers
#define ADE9000_MASK1_EEPROM_ADDR 0x1000
#define ADE9000_MASK0_EEPROM_ADDR 0x1010

uint16_t getRegEepromAddr(const char* regName) {
	if (strcmp(regName, "MASK0") == 0) return ADDR_MASK0_EEPROM;
	if (strcmp(regName, "MASK1") == 0) return ADDR_MASK1_EEPROM;
	// Add more registers here as needed
	return 0xFFFF; // Invalid
}

uint32_t getRegDefault(const char* regName) {
	if (strcmp(regName, "MASK0") == 0) return ADE9000_MASK0;
	if (strcmp(regName, "MASK1") == 0) return ADE9000_MASK1;
	// Add more registers here as needed
	return 0;
}

uint32_t ADE9000Class::loadPersistentREG(const char* regName) {
	uint16_t addr = getRegEepromAddr(regName);
	if (addr == 0xFFFF) return getRegDefault(regName);
	uint32_t val = readWordFromEeprom(addr);
	if (val == 0xFFFFFFFF || val == 0x00000000) {
		return getRegDefault(regName);
	}
	return val;
}

void ADE9000Class::savePersistentREG(const char* regName, uint32_t value) {
	uint16_t addr = getRegEepromAddr(regName);
	if (addr == 0xFFFF) return;
	writeWordToEeprom(addr, value);
}

/* 
Description: Initializes the arduino SPI port using SPI.h library
Input: SPI speed, chip select pin
Output:-
*/
void ADE9000Class::SPI_Init(uint32_t SPI_speed , uint8_t chipSelect_Pin,
                              uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin)
{
	// Initiate SPI port using explicit pins.
	// (SPI.begin() without pins relies on Arduino defaults, which may differ
	// between ESP32-WROOM-32 and ESP32-S3.)
	SPI.begin(sckPin, misoPin, mosiPin, chipSelect_Pin);
	SPI.beginTransaction(SPISettings(SPI_speed,MSBFIRST,SPI_MODE0));		//Setup SPI parameters
	pinMode(chipSelect_Pin, OUTPUT);		//Set Chip select pin as output	
	digitalWrite(chipSelect_Pin, HIGH);		//Set Chip select pin high 

	_chipSelect_Pin = chipSelect_Pin;
}

/* 
Description: Writes 16bit data to a 16 bit register. 
Input: Register address, data
Output:-
*/

void ADE9000Class:: SPI_Write_16(uint16_t Address , uint16_t Data )
{
	uint16_t temp_address;
	
	digitalWrite(_chipSelect_Pin, LOW);
	temp_address = ((Address << 4) & 0xFFF0);	//shift address  to align with cmd packet
	SPI.transfer16(temp_address);
	SPI.transfer16(Data);
	
	digitalWrite(_chipSelect_Pin, HIGH); 	
}

/* 
Description: Writes 32bit data to a 32 bit register. 
Input: Register address, data
Output:-
*/

void ADE9000Class:: SPI_Write_32(uint16_t Address , uint32_t Data )
{
	uint16_t temp_address;
	uint16_t temp_highpacket;
	uint16_t temp_lowpacket;

	temp_highpacket= (Data & 0xFFFF0000)>>16;
	temp_lowpacket= (Data & 0x0000FFFF);
	
	digitalWrite(_chipSelect_Pin, LOW);
	
	temp_address = ((Address << 4) & 0xFFF0);	//shift address  to align with cmd packet
	SPI.transfer16(temp_address);
	SPI.transfer16(temp_highpacket);
	SPI.transfer16(temp_lowpacket);
	
	digitalWrite(_chipSelect_Pin, HIGH); 	
	
}

/* 
Description: Reads 16bit data from register. 
Input: Register address
Output: 16 bit data
*/

uint16_t ADE9000Class:: SPI_Read_16(uint16_t Address)
{
	uint16_t temp_address;
	uint16_t returnData;
	
	digitalWrite(_chipSelect_Pin, LOW);
	
	temp_address = (((Address << 4) & 0xFFF0)+8);
	SPI.transfer16(temp_address);
	returnData = SPI.transfer16(0);
	
	digitalWrite(_chipSelect_Pin, HIGH);
	return returnData;
}

/* 
Description: Reads 32bit data from register. 
Input: Register address
Output: 32 bit data
*/

uint32_t ADE9000Class:: SPI_Read_32(uint16_t Address)
{
	uint16_t temp_address;
	uint16_t temp_highpacket;
	uint16_t temp_lowpacket;
	uint32_t returnData;
	
	digitalWrite(_chipSelect_Pin, LOW);
	
	temp_address = (((Address << 4) & 0xFFF0)+8);
	SPI.transfer16(temp_address);
	temp_highpacket = SPI.transfer16(0);
	temp_lowpacket = SPI.transfer16(0);	
	
	digitalWrite(_chipSelect_Pin, HIGH);
	
	returnData = temp_highpacket << 16;
	returnData = returnData + temp_lowpacket;
	
	return returnData;

}
/* 
Description: Burst reads the content of waveform buffer. This function only works with resampled data. Configure waveform buffer to have Resampled data, and burst enabled (BURST_CHAN=0000 in WFB_CFG Register).
Input: The starting address. Use the starting address of a data set. e.g 0x800, 0x804 etc to avoid data going into incorrect arrays. 
	   Read_Element_Length is the number of data sets to read. If the starting address is 0x800, the maximum sets to read are 512.
Output: Resampled data returned in structure
*/

/*					old API 					*/
#if OLDAPI
void ADE9000Class:: SPI_Burst_Read_Resampled_Wfb(uint16_t Address, uint16_t Read_Element_Length, ResampledWfbData *ResampledData)
{
	uint16_t temp;
	uint16_t i;
 

	digitalWrite(_chipSelect_Pin, LOW);
  
	SPI.transfer16(((Address << 4) & 0xFFF0)+8);  //Send the starting address
 
  //burst read the data upto Read_Length 
	for(i=0;i<Read_Element_Length;i++) 
		{
		  ResampledData->IA_Resampled[i] =  SPI.transfer16(0);
		  ResampledData->VA_Resampled[i] =  SPI.transfer16(0);
		  ResampledData->IB_Resampled[i] =  SPI.transfer16(0);
		  ResampledData->VB_Resampled[i] =  SPI.transfer16(0);
		  ResampledData->IC_Resampled[i] =  SPI.transfer16(0);
		  ResampledData->VC_Resampled[i] =  SPI.transfer16(0);
		  ResampledData->IN_Resampled[i] =  SPI.transfer16(0);
		}
	digitalWrite(_chipSelect_Pin, HIGH);
}

/* 
Description: Reads the Active power registers AWATT,BWATT and CWATT
Input: Structure name
Output: Active power codes stored in respective structure
*/
void ADE9000Class:: ReadActivePowerRegs(ActivePowerRegs *Data)
{
	Data->ActivePowerReg_A = int32_t (SPI_Read_32(ADDR_AWATT));
	Data->ActivePowerReg_B = int32_t (SPI_Read_32(ADDR_BWATT));
	Data->ActivePowerReg_C = int32_t (SPI_Read_32(ADDR_CWATT));
}

void ADE9000Class:: ReadReactivePowerRegs(ReactivePowerRegs *Data)
{
	Data->ReactivePowerReg_A = int32_t (SPI_Read_32(ADDR_AVAR));
	Data->ReactivePowerReg_B = int32_t (SPI_Read_32(ADDR_BVAR));
	Data->ReactivePowerReg_C = int32_t (SPI_Read_32(ADDR_CVAR));	
}

void ADE9000Class:: ReadApparentPowerRegs(ApparentPowerRegs *Data)
{
	Data->ApparentPowerReg_A = int32_t (SPI_Read_32(ADDR_AVA));
	Data->ApparentPowerReg_B = int32_t (SPI_Read_32(ADDR_BVA));
	Data->ApparentPowerReg_C = int32_t (SPI_Read_32(ADDR_CVA));	
}

void ADE9000Class:: ReadVoltageRMSRegs(VoltageRMSRegs *Data)
{
	Data->VoltageRMSReg_A = int32_t (SPI_Read_32(ADDR_AVRMS));
	Data->VoltageRMSReg_B = int32_t (SPI_Read_32(ADDR_BVRMS));
	Data->VoltageRMSReg_C = int32_t (SPI_Read_32(ADDR_CVRMS));	
}

void ADE9000Class:: ReadCurrentRMSRegs(CurrentRMSRegs *Data)
{
	Data->CurrentRMSReg_A = int32_t (SPI_Read_32(ADDR_AIRMS));
	Data->CurrentRMSReg_B = int32_t (SPI_Read_32(ADDR_BIRMS));
	Data->CurrentRMSReg_C = int32_t (SPI_Read_32(ADDR_CIRMS));
	Data->CurrentRMSReg_N = int32_t (SPI_Read_32(ADDR_NIRMS));
	
}

void ADE9000Class::ReadActiveEnergyRegs(ActiveEnergyRegs *Data)
{
	Data->ActiveEnergyReg_A = int32_t (SPI_Read_32(ADDR_AWATTHR_HI));
	Data->ActiveEnergyReg_B = int32_t (SPI_Read_32(ADDR_BWATTHR_HI));
	Data->ActiveEnergyReg_C = int32_t (SPI_Read_32(ADDR_CWATTHR_HI));
}

void ADE9000Class::ReadReactiveEnergyRegs(ReactiveEnergyRegs *Data)
{
	Data->ReactiveEnergyReg_A = int32_t (SPI_Read_32(ADDR_AVARHR_HI));
	Data->ReactiveEnergyReg_B = int32_t (SPI_Read_32(ADDR_BVARHR_HI));
	Data->ReactiveEnergyReg_C = int32_t (SPI_Read_32(ADDR_CVARHR_HI));
}

void ADE9000Class::ReadApparentEnergyRegs(ApparentEnergyRegs *Data)
{
	Data->ApparentEnergyReg_A = int32_t (SPI_Read_32(ADDR_AVAHR_HI));
	Data->ApparentEnergyReg_B = int32_t (SPI_Read_32(ADDR_BVAHR_HI));
	Data->ApparentEnergyReg_C = int32_t (SPI_Read_32(ADDR_CVAHR_HI));
}

void ADE9000Class:: ReadFundActivePowerRegs(FundActivePowerRegs *Data)
{
	Data->FundActivePowerReg_A = int32_t (SPI_Read_32(ADDR_AFWATT));
	Data->FundActivePowerReg_B = int32_t (SPI_Read_32(ADDR_BFWATT));
	Data->FundActivePowerReg_C = int32_t (SPI_Read_32(ADDR_CFWATT));	
}

void ADE9000Class:: ReadFundReactivePowerRegs(FundReactivePowerRegs *Data)
{
	Data->FundReactivePowerReg_A = int32_t (SPI_Read_32(ADDR_AFVAR));
	Data->FundReactivePowerReg_B = int32_t (SPI_Read_32(ADDR_BFVAR));
	Data->FundReactivePowerReg_C = int32_t (SPI_Read_32(ADDR_CFVAR));	
}

void ADE9000Class:: ReadFundApparentPowerRegs(FundApparentPowerRegs *Data)
{
	Data->FundApparentPowerReg_A = int32_t (SPI_Read_32(ADDR_AFVA));
	Data->FundApparentPowerReg_B = int32_t (SPI_Read_32(ADDR_BFVA));
	Data->FundApparentPowerReg_C = int32_t (SPI_Read_32(ADDR_CFVA));	
}

void ADE9000Class:: ReadFundVoltageRMSRegs(FundVoltageRMSRegs *Data)
{
	Data->FundVoltageRMSReg_A = int32_t (SPI_Read_32(ADDR_AVFRMS));
	Data->FundVoltageRMSReg_B = int32_t (SPI_Read_32(ADDR_BVFRMS));
	Data->FundVoltageRMSReg_C = int32_t (SPI_Read_32(ADDR_CVFRMS));	
}

void ADE9000Class:: ReadFundCurrentRMSRegs(FundCurrentRMSRegs *Data)
{
	Data->FundCurrentRMSReg_A = int32_t (SPI_Read_32(ADDR_AIFRMS));
	Data->FundCurrentRMSReg_B = int32_t (SPI_Read_32(ADDR_BIFRMS));
	Data->FundCurrentRMSReg_C = int32_t (SPI_Read_32(ADDR_CIFRMS));	
}

void ADE9000Class:: ReadHalfVoltageRMSRegs(HalfVoltageRMSRegs *Data)
{
	Data->HalfVoltageRMSReg_A = int32_t (SPI_Read_32(ADDR_AVRMSONE));
	Data->HalfVoltageRMSReg_B = int32_t (SPI_Read_32(ADDR_BVRMSONE));
	Data->HalfVoltageRMSReg_C = int32_t (SPI_Read_32(ADDR_CVRMSONE));	
}

void ADE9000Class:: ReadHalfCurrentRMSRegs(HalfCurrentRMSRegs *Data)
{
	Data->HalfCurrentRMSReg_A = int32_t (SPI_Read_32(ADDR_AIRMSONE));
	Data->HalfCurrentRMSReg_B = int32_t (SPI_Read_32(ADDR_BIRMSONE));
	Data->HalfCurrentRMSReg_C = int32_t (SPI_Read_32(ADDR_CIRMSONE));
	Data->HalfCurrentRMSReg_N = int32_t (SPI_Read_32(ADDR_NIRMSONE));
}

void ADE9000Class:: ReadTen12VoltageRMSRegs(Ten12VoltageRMSRegs *Data)
{
	Data->Ten12VoltageRMSReg_A = int32_t (SPI_Read_32(ADDR_AVRMS1012));
	Data->Ten12VoltageRMSReg_B = int32_t (SPI_Read_32(ADDR_BVRMS1012));
	Data->Ten12VoltageRMSReg_C = int32_t (SPI_Read_32(ADDR_CVRMS1012));	
}

void ADE9000Class:: ReadTen12CurrentRMSRegs(Ten12CurrentRMSRegs *Data)
{
	Data->Ten12CurrentRMSReg_A = int32_t (SPI_Read_32(ADDR_AIRMS1012));
	Data->Ten12CurrentRMSReg_B = int32_t (SPI_Read_32(ADDR_BIRMS1012));
	Data->Ten12CurrentRMSReg_C = int32_t (SPI_Read_32(ADDR_CIRMS1012));
	Data->Ten12CurrentRMSReg_N = int32_t (SPI_Read_32(ADDR_NIRMS1012));	
	
}

void ADE9000Class:: ReadVoltageTHDRegsnValues(VoltageTHDRegs *Data)
{
	uint32_t tempReg;
	float tempValue;
	
	tempReg=int32_t (SPI_Read_32(ADDR_AVTHD)); //Read THD register
	Data->VoltageTHDReg_A = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %
	Data->VoltageTHDValue_A=tempValue;	
	tempReg=int32_t (SPI_Read_32(ADDR_BVTHD)); //Read THD register
	Data->VoltageTHDReg_B = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %
	Data->VoltageTHDValue_B=tempValue;		
	tempReg=int32_t (SPI_Read_32(ADDR_CVTHD)); //Read THD register
	Data->VoltageTHDReg_C = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %
	Data->VoltageTHDValue_C=tempValue;			
}

void ADE9000Class:: ReadCurrentTHDRegsnValues(CurrentTHDRegs *Data)
{
	uint32_t tempReg;
	float tempValue;	
	
	tempReg=int32_t (SPI_Read_32(ADDR_AITHD)); //Read THD register
	Data->CurrentTHDReg_A = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %	
	Data->CurrentTHDValue_A=tempValue;		
	tempReg=int32_t (SPI_Read_32(ADDR_BITHD)); //Read THD register
	Data->CurrentTHDReg_B = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %	
	Data->CurrentTHDValue_B=tempValue;
	tempReg=int32_t (SPI_Read_32(ADDR_CITHD)); //Read THD register
	Data->CurrentTHDReg_C = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %		
	Data->CurrentTHDValue_C=tempValue;
}

void ADE9000Class:: ReadPowerFactorRegsnValues(PowerFactorRegs *Data)
{
	uint32_t tempReg;
	float tempValue;	
	
	tempReg=int32_t (SPI_Read_32(ADDR_APF)); //Read PF register
	Data->PowerFactorReg_A = tempReg;
	tempValue=(float)tempReg/(float)134217728; //Calculate PF	
	Data->PowerFactorValue_A=tempValue;			
	tempReg=int32_t (SPI_Read_32(ADDR_BPF)); //Read PF register
	Data->PowerFactorReg_B = tempReg;
	tempValue=(float)tempReg/(float)134217728; //Calculate PF	
	Data->PowerFactorValue_B=tempValue;	
	tempReg=int32_t (SPI_Read_32(ADDR_CPF)); //Read PF register
	Data->PowerFactorReg_C = tempReg;
	tempValue=(float)tempReg/(float)134217728; //Calculate PF	
	Data->PowerFactorValue_C=tempValue;
}

void ADE9000Class:: ReadPeriodRegsnValues(PeriodRegs *Data)
{
	uint32_t tempReg;
	float tempValue;	
	tempReg=int32_t (SPI_Read_32(ADDR_APERIOD)); //Read PERIOD register
	Data->PeriodReg_A = tempReg;
	tempValue=(float)(8000*65536)/(float)(tempReg+1); //Calculate Frequency	
	Data->FrequencyValue_A = tempValue;
	tempReg=int32_t (SPI_Read_32(ADDR_BPERIOD)); //Read PERIOD register
	Data->PeriodReg_B = tempReg;
	tempValue=(float)(8000*65536)/(float)(tempReg+1); //Calculate Frequency	
	Data->FrequencyValue_B = tempValue;
	tempReg=int32_t (SPI_Read_32(ADDR_CPERIOD)); //Read PERIOD register
	Data->PeriodReg_C = tempReg;
	tempValue=(float)(8000*65536)/(float)(tempReg+1); //Calculate Frequency	
	Data->FrequencyValue_C = tempValue;
}

void ADE9000Class:: ReadAngleRegsnValues(AngleRegs *Data)
{

	uint32_t tempReg;	
	uint16_t temp;
	float mulConstant;
	float tempValue;
	
	temp=SPI_Read_16(ADDR_ACCMODE); //Read frequency setting register
	if((temp&0x0100)>=0)
		{
			mulConstant=0.02109375;  //multiplier constant for 60Hz system
		}
	else
		{
			mulConstant=0.017578125; //multiplier constant for 50Hz system		
		}
	
	tempReg=int16_t (SPI_Read_32(ADDR_ANGL_VA_VB)); //Read ANGLE register
	Data->AngleReg_VA_VB=tempReg;
	tempValue=tempReg*mulConstant;	//Calculate Angle in degrees					
	Data->AngleValue_VA_VB=tempValue;
	tempReg=int16_t (SPI_Read_32(ADDR_ANGL_VB_VC));
	Data->AngleReg_VB_VC=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_VB_VC=tempValue;	
	tempReg=int16_t (SPI_Read_32(ADDR_ANGL_VA_VC));
	Data->AngleReg_VA_VC=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_VA_VC=tempValue;	
	tempReg=int16_t (SPI_Read_32(ADDR_ANGL_VA_IA));
	Data->AngleReg_VA_IA=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_VA_IA=tempValue;	
	tempReg=int16_t (SPI_Read_32(ADDR_ANGL_VB_IB));
	Data->AngleReg_VB_IB=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_VB_IB=tempValue;	
	tempReg=int16_t (SPI_Read_32(ADDR_ANGL_VC_IC));
	Data->AngleReg_VC_IC=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_VC_IC=tempValue;		
	tempReg=int16_t (SPI_Read_32(ADDR_ANGL_IA_IB));
	Data->AngleReg_IA_IB=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_IA_IB=tempValue;	
	tempReg=int16_t (SPI_Read_32(ADDR_ANGL_IB_IC));
	Data->AngleReg_IB_IC=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_IB_IC=tempValue;	
	tempReg=int16_t (SPI_Read_32(ADDR_ANGL_IA_IC));
	Data->AngleReg_IA_IC=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_IA_IC=tempValue;						
}

/* 
Description: Starts a new acquisition cycle. Waits for constant time and returns register value and temperature in Degree Celsius
Input:	Structure name
Output: Register reading and temperature value in Degree Celsius
*/

void ADE9000Class:: ReadTempRegnValue(TemperatureRegnValue *Data)
{
	uint32_t trim;
	uint16_t gain;
	uint16_t offset;
	uint16_t tempReg; 
	float tempValue;
	
	SPI_Write_16(ADDR_TEMP_CFG,ADE9000_TEMP_CFG);//Start temperature acquisition cycle with settings in defined in ADE9000_TEMP_CFG
	delay(2); //delay of 2ms. Increase delay if TEMP_TIME is changed

	trim = SPI_Read_32(ADDR_TEMP_TRIM);
	gain= (trim & 0xFFFF);  //Extract 16 LSB
	offset= ((trim>>16)&0xFFFF); //Extract 16 MSB
	tempReg= SPI_Read_16(ADDR_TEMP_RSLT);	//Read Temperature result register
	tempValue= (float)(offset>>5)-((float)tempReg*(float)gain/(float)65536); 
	
	Data->Temperature_Reg=tempReg;
	Data->Temperature=tempValue;
}
#endif
/*			new API (indexed tables)			*/
#if !OLDAPI
// Waveform buffer table overload
void ADE9000Class:: SPI_Burst_Read_Resampled_Wfb(uint16_t Address, uint16_t Read_Element_Length, ResampledWfbTable *tbl)
{
	digitalWrite(_chipSelect_Pin, LOW);
	SPI.transfer16(((Address << 4) & 0xFFF0)+8);
	for (uint16_t i = 0; i < Read_Element_Length; i++) {
		tbl->ch[CH_IA][i] =  SPI.transfer16(0);
		tbl->ch[CH_VA][i] =  SPI.transfer16(0);
		tbl->ch[CH_IB][i] =  SPI.transfer16(0);
		tbl->ch[CH_VB][i] =  SPI.transfer16(0);
		tbl->ch[CH_IC][i] =  SPI.transfer16(0);
		tbl->ch[CH_VC][i] =  SPI.transfer16(0);
		tbl->ch[CH_IN][i] =  SPI.transfer16(0);
	}
	digitalWrite(_chipSelect_Pin, HIGH);
}

// Table overloads for other measurements
void ADE9000Class:: ReadActivePowerRegs(ActivePowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AWATT));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BWATT));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CWATT));
}

void ADE9000Class:: ReadReactivePowerRegs(ReactivePowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AVAR));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BVAR));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CVAR));
}

void ADE9000Class:: ReadApparentPowerRegs(ApparentPowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AVA));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BVA));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CVA));
}

void ADE9000Class:: ReadVoltageRMSRegs(VoltageRMSTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AVRMS));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BVRMS));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CVRMS));
}

void ADE9000Class:: ReadCurrentRMSRegs(CurrentRMSTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AIRMS));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BIRMS));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CIRMS));
	tbl->phase[PHASE_N] = int32_t (SPI_Read_32(ADDR_NIRMS));
}

void ADE9000Class::ReadActiveEnergyRegs(ActiveEnergyRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AWATTHR_HI));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BWATTHR_HI));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CWATTHR_HI));
}

void ADE9000Class::ReadReactiveEnergyRegs(ReactiveEnergyRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AVARHR_HI));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BVARHR_HI));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CVARHR_HI));
}

void ADE9000Class::ReadApparentEnergyRegs(ApparentEnergyRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t(SPI_Read_32(ADDR_AVAHR_HI));
	tbl->phase[PHASE_B] = int32_t(SPI_Read_32(ADDR_BVAHR_HI));
	tbl->phase[PHASE_C] = int32_t(SPI_Read_32(ADDR_CVAHR_HI));
}

// Averaged power from accumulator registers (hardware time-averaged over PWR_TIME+1 samples)
void ADE9000Class::ReadAvgActivePowerRegs(ActivePowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t(SPI_Read_32(ADDR_AWATT_ACC));
	tbl->phase[PHASE_B] = int32_t(SPI_Read_32(ADDR_BWATT_ACC));
	tbl->phase[PHASE_C] = int32_t(SPI_Read_32(ADDR_CWATT_ACC));
}

void ADE9000Class::ReadAvgReactivePowerRegs(ReactivePowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t(SPI_Read_32(ADDR_AVAR_ACC));
	tbl->phase[PHASE_B] = int32_t(SPI_Read_32(ADDR_BVAR_ACC));
	tbl->phase[PHASE_C] = int32_t(SPI_Read_32(ADDR_CVAR_ACC));
}

void ADE9000Class::ReadAvgApparentPowerRegs(ApparentPowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t(SPI_Read_32(ADDR_AVA_ACC));
	tbl->phase[PHASE_B] = int32_t(SPI_Read_32(ADDR_BVA_ACC));
	tbl->phase[PHASE_C] = int32_t(SPI_Read_32(ADDR_CVA_ACC));
}

// Fundamental averaged power from accumulator registers (hardware time-averaged)
void ADE9000Class::ReadAvgFundActivePowerRegs(FundActivePowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t(SPI_Read_32(ADDR_AFWATT_ACC));
	tbl->phase[PHASE_B] = int32_t(SPI_Read_32(ADDR_BFWATT_ACC));
	tbl->phase[PHASE_C] = int32_t(SPI_Read_32(ADDR_CFWATT_ACC));
}

void ADE9000Class::ReadAvgFundReactivePowerRegs(FundReactivePowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t(SPI_Read_32(ADDR_AFVAR_ACC));
	tbl->phase[PHASE_B] = int32_t(SPI_Read_32(ADDR_BFVAR_ACC));
	tbl->phase[PHASE_C] = int32_t(SPI_Read_32(ADDR_CFVAR_ACC));
}

void ADE9000Class::ReadAvgFundApparentPowerRegs(FundApparentPowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t(SPI_Read_32(ADDR_AFVA_ACC));
	tbl->phase[PHASE_B] = int32_t(SPI_Read_32(ADDR_BFVA_ACC));
	tbl->phase[PHASE_C] = int32_t(SPI_Read_32(ADDR_CFVA_ACC));
}

void ADE9000Class:: ReadFundActivePowerRegs(FundActivePowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AFWATT));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BFWATT));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CFWATT));
}

void ADE9000Class:: ReadFundReactivePowerRegs(FundReactivePowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AFVAR));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BFVAR));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CFVAR));
}

void ADE9000Class:: ReadFundApparentPowerRegs(FundApparentPowerRegsTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AFVA));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BFVA));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CFVA));
}

void ADE9000Class:: ReadFundVoltageRMSRegs(FundVoltageRMSTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AVFRMS));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BVFRMS));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CVFRMS));
}

void ADE9000Class:: ReadFundCurrentRMSRegs(FundCurrentRMSTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AIFRMS));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BIFRMS));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CIFRMS));
}

void ADE9000Class:: ReadHalfVoltageRMSRegs(HalfVoltageRMSTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AVRMSONE));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BVRMSONE));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CVRMSONE));
}

void ADE9000Class:: ReadHalfCurrentRMSRegs(HalfCurrentRMSTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AIRMSONE));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BIRMSONE));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CIRMSONE));
	tbl->phase[PHASE_N] = int32_t (SPI_Read_32(ADDR_NIRMSONE));
}

void ADE9000Class:: ReadTen12VoltageRMSRegs(Ten12VoltageRMSTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AVRMS1012));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BVRMS1012));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CVRMS1012));
}

void ADE9000Class:: ReadTen12CurrentRMSRegs(Ten12CurrentRMSTbl *tbl)
{
	tbl->phase[PHASE_A] = int32_t (SPI_Read_32(ADDR_AIRMS1012));
	tbl->phase[PHASE_B] = int32_t (SPI_Read_32(ADDR_BIRMS1012));
	tbl->phase[PHASE_C] = int32_t (SPI_Read_32(ADDR_CIRMS1012));
	tbl->phase[PHASE_N] = int32_t (SPI_Read_32(ADDR_NIRMS1012));
}

void ADE9000Class:: ReadVoltageTHDRegsnValues(VoltageTHDTbl *tbl)
{
	uint32_t r;
	r = SPI_Read_32(ADDR_AVTHD); tbl->reg[PHASE_A] = r; tbl->value[PHASE_A] = (float)r*100.0f/134217728.0f;
	r = SPI_Read_32(ADDR_BVTHD); tbl->reg[PHASE_B] = r; tbl->value[PHASE_B] = (float)r*100.0f/134217728.0f;
	r = SPI_Read_32(ADDR_CVTHD); tbl->reg[PHASE_C] = r; tbl->value[PHASE_C] = (float)r*100.0f/134217728.0f;
}

void ADE9000Class:: ReadCurrentTHDRegsnValues(CurrentTHDTbl *tbl)
{
	uint32_t r;
	r = SPI_Read_32(ADDR_AITHD); tbl->reg[PHASE_A] = r; tbl->value[PHASE_A] = (float)r*100.0f/134217728.0f;
	r = SPI_Read_32(ADDR_BITHD); tbl->reg[PHASE_B] = r; tbl->value[PHASE_B] = (float)r*100.0f/134217728.0f;
	r = SPI_Read_32(ADDR_CITHD); tbl->reg[PHASE_C] = r; tbl->value[PHASE_C] = (float)r*100.0f/134217728.0f;
}

void ADE9000Class:: ReadPowerFactorRegsnValues(PowerFactorTbl *tbl)
{
	uint32_t r;
	r = SPI_Read_32(ADDR_APF); tbl->reg[PHASE_A] = r; tbl->value[PHASE_A] = (float)r/134217728.0f;
	r = SPI_Read_32(ADDR_BPF); tbl->reg[PHASE_B] = r; tbl->value[PHASE_B] = (float)r/134217728.0f;
	r = SPI_Read_32(ADDR_CPF); tbl->reg[PHASE_C] = r; tbl->value[PHASE_C] = (float)r/134217728.0f;
}

void ADE9000Class:: ReadPeriodRegsnValues(PeriodTbl *tbl)
{
	uint32_t r;
	r = SPI_Read_32(ADDR_APERIOD); tbl->reg[PHASE_A] = r; tbl->freq[PHASE_A] = (float)(8000.0f*65536.0f)/(float)(r+1);
	r = SPI_Read_32(ADDR_BPERIOD); tbl->reg[PHASE_B] = r; tbl->freq[PHASE_B] = (float)(8000.0f*65536.0f)/(float)(r+1);
	r = SPI_Read_32(ADDR_CPERIOD); tbl->reg[PHASE_C] = r; tbl->freq[PHASE_C] = (float)(8000.0f*65536.0f)/(float)(r+1);
}

void ADE9000Class:: ReadAngleRegsnValues(AngleTbl *tbl)
{
	uint16_t temp = SPI_Read_16(ADDR_ACCMODE);
	float mulConstant;
	if((temp&0x0100)>=0)
	{
		mulConstant=0.02109375f;  // 60Hz
	}
	else
	{
		mulConstant=0.017578125f; // 50Hz
	}
	int16_t r;
	r = int16_t (SPI_Read_32(ADDR_ANGL_VA_VB)); tbl->reg[ANG_VA_VB]=r; tbl->value[ANG_VA_VB]=r*mulConstant;
	r = int16_t (SPI_Read_32(ADDR_ANGL_VB_VC)); tbl->reg[ANG_VB_VC]=r; tbl->value[ANG_VB_VC]=r*mulConstant;
	r = int16_t (SPI_Read_32(ADDR_ANGL_VA_VC)); tbl->reg[ANG_VA_VC]=r; tbl->value[ANG_VA_VC]=r*mulConstant;
	r = int16_t (SPI_Read_32(ADDR_ANGL_VA_IA)); tbl->reg[ANG_VA_IA]=r; tbl->value[ANG_VA_IA]=r*mulConstant;
	r = int16_t (SPI_Read_32(ADDR_ANGL_VB_IB)); tbl->reg[ANG_VB_IB]=r; tbl->value[ANG_VB_IB]=r*mulConstant;
	r = int16_t (SPI_Read_32(ADDR_ANGL_VC_IC)); tbl->reg[ANG_VC_IC]=r; tbl->value[ANG_VC_IC]=r*mulConstant;
	r = int16_t (SPI_Read_32(ADDR_ANGL_IA_IB)); tbl->reg[ANG_IA_IB]=r; tbl->value[ANG_IA_IB]=r*mulConstant;
	r = int16_t (SPI_Read_32(ADDR_ANGL_IB_IC)); tbl->reg[ANG_IB_IC]=r; tbl->value[ANG_IB_IC]=r*mulConstant;
	r = int16_t (SPI_Read_32(ADDR_ANGL_IA_IC)); tbl->reg[ANG_IA_IC]=r; tbl->value[ANG_IA_IC]=r*mulConstant;
}

void ADE9000Class:: ReadTempRegnValue(TemperatureTbl *tbl)
{
	uint32_t trim;
	uint16_t gain;
	uint16_t offset;
	uint16_t tempReg;
	SPI_Write_16(ADDR_TEMP_CFG,ADE9000_TEMP_CFG);
	delay(2);
	trim = SPI_Read_32(ADDR_TEMP_TRIM);
	gain= (trim & 0xFFFF);
	offset= ((trim>>16)&0xFFFF);
	tempReg= SPI_Read_16(ADDR_TEMP_RSLT);
	tbl->reg = tempReg;
	tbl->value = (float)(offset>>5)-((float)tempReg*(float)gain/(float)65536);
}

#endif

/* 
Description: Writes one byte of data to EEPROM
Input: Data and EEPROM address
Output:-
*/

void ADE9000Class:: writeByteToEeprom(uint16_t dataAddress, uint8_t data)
{
	uint8_t temp;
	temp= (dataAddress>>8);
	Wire.beginTransmission(ADE9000_EEPROM_ADDRESS); // device address is specified in datasheet
	Wire.write(byte(temp));            // MSB Address
	temp= (dataAddress & (0xFF));
	Wire.write(byte(temp));           //LSB Address
	Wire.write(byte(data));             // 
	Wire.endTransmission();     // stop transmitting	
}

void ADE9000Class::writeBytesToEeprom(uint16_t address, const uint8_t* data, uint16_t length)
{
	if (data == nullptr || length == 0) {
		return;
	}

	uint16_t offset = 0;
	while (offset < length) {
		const uint16_t chunkAddress = address + offset;
		const uint16_t pageOffset = chunkAddress % ADE9000_EEPROM_PAGE_SIZE;
		const uint16_t spaceInPage = ADE9000_EEPROM_PAGE_SIZE - pageOffset;
		const uint16_t chunkLength = min<uint16_t>(length - offset, spaceInPage);

		Wire.beginTransmission(ADE9000_EEPROM_ADDRESS);
		Wire.write(byte(chunkAddress >> 8));
		Wire.write(byte(chunkAddress & 0xFF));
		for (uint16_t index = 0; index < chunkLength; ++index) {
			Wire.write(data[offset + index]);
		}
		Wire.endTransmission();
		delay(ADE9000_EEPROM_WRITE_CYCLE_MS);
		offset += chunkLength;
	}
}

/* 
Description: REads one byte of data from EEPROM
Input: EEPROM address
Output:- 8 bit data
*/	

uint8_t ADE9000Class:: ReadByteFromEeprom(uint16_t dataAddress)
{
	uint8_t returndata;
	uint8_t temp;
	temp= (dataAddress>>8);
	Wire.beginTransmission(byte(ADE9000_EEPROM_ADDRESS)); // device address is specified in datasheet
	Wire.write(byte(temp)); // MSB
	temp= (dataAddress & (0xFF));
	Wire.write(byte(temp)); // LSB
	Wire.endTransmission();
	Wire.requestFrom(byte(ADE9000_EEPROM_ADDRESS),1);
	if (Wire.available())  
		{
		  returndata = Wire.read();
		}
	
	return returndata;	
}

/* 
Description: Writes 4 bytes into EEPROM in continuous locations
Input: Data and EEPROM address
Output:-
*/

void ADE9000Class:: writeWordToEeprom(uint16_t address, uint32_t data)
{
	uint8_t bytes[sizeof(data)];
	uint32_t returnedValue;
	bytes[0] = uint8_t(data & 0xFF);
	bytes[1] = uint8_t((data >> 8) & 0xFF);
	bytes[2] = uint8_t((data >> 16) & 0xFF);
	bytes[3] = uint8_t((data >> 24) & 0xFF);
	writeBytesToEeprom(address, bytes, sizeof(bytes));
	returnedValue = readWordFromEeprom(address);
	if(returnedValue!=data)
	{
	  Serial.println("Write Not Successful"); //Check if data write is successful
	  Serial.println("Address: ");
	  Serial.print(address);
	  Serial.println("Data: ");
	  Serial.print(data,HEX);
	}	
}

/* 
Description: Reads 4 bytes stored in EEPROM
Input: Starting EEPROM address
Output:- 4 byte wide data
*/

uint32_t ADE9000Class:: readWordFromEeprom(uint16_t address)
{
	uint32_t returndata;
	returndata=0;
	uint8_t temp;
	temp = ReadByteFromEeprom(address);
	returndata = temp;    //LSB
	delay(10);
	temp = ReadByteFromEeprom(address+1);
	returndata |= (temp<<8);
	delay(10);
	temp = ReadByteFromEeprom(address+2);
	returndata |= (temp<<16);
	delay(10);
	temp = ReadByteFromEeprom(address+3);
	returndata |= (temp<<24);  //MSB  
	return returndata;	
}




