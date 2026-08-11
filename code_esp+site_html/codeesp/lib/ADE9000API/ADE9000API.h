/*
  ADE9000API.h - Library for ADE9000/ADE9078 - Energy and PQ monitoring AFE
  Author:nchandra
  Date: 3-16-2017
*/
#ifndef ADE9000API_h
#define ADE9000API_h

/****************************************************************************************************************
 Includes
***************************************************************************************************************/

#include "Arduino.h"
#include "ADE9000RegMap.h"

// Enable old API by default; define OLDAPI=0 in build flags to hide old functions
#ifndef OLDAPI
#define OLDAPI 0
#endif

/****************************************************************************************************************
 Definitions
****************************************************************************************************************/
/*Configuration registers*/
#define ADE9000_PGA_GAIN 0x0000    	    /*PGA@0x0000. Gain of all channels=1*/
#define ADE9000_CONFIG0 0x00000000		/*Integrator disabled*/
#define ADE9000_CONFIG1	0x0002			/*CF3/ZX pin outputs Zero crossing */
#define ADE9000_CONFIG2	0x0C00			/*Default High pass corner frequency of 1.25Hz*/
#define ADE9000_CONFIG3	0x0000			/*Enable Peak and overcurrent detection*/
#define ADE9000_ACCMODE 0x0000			/*60Hz operation, 3P4W Wye configuration, signed accumulation*/
										/*Clear bit 8 i.e. ACCMODE=0x00xx for 50Hz operation*/
										/*ACCMODE=0x0x9x for 3Wire delta when phase B is used as reference*/	
#define ADE9000_TEMP_CFG 0x000C			/*Temperature sensor enabled*/
#define ADE9000_ZX_LP_SEL 0x001E		/*Line period and zero crossing obtained from combined signals VA,VB and VC*/	

#define ADE9000_MASK0 0x00000000		/*Enable PWRRDY interrupt*/			
#define ADE9000_MASK1 0x00000000		//0x3F201C0			/*Enable all ZXTOVx & SWELLx & DIPx & OI */

#define ADE9000_EVENT_MASK 0x00000000	/*Events disabled */
#define ADE9000_VLEVEL	0x0022EA28		/*Assuming Vnom=1/2 of full scale. */

										/*Refer Technical reference manual for detailed calculations.*/
#define ADE9000_DICOEFF 0x00000000 		/* Set DICOEFF= 0xFFFFE000 when integrator is enabled*/

/*Constant Definitions***/
#define ADE90xx_FDSP 8000   			/*ADE9000 FDSP: 8000sps, ADE9078 FDSP: 4000sps*/
#define ADE9000_RUN_ON 0x0001			/*DSP ON*/
/*Energy Accumulation Settings*/
#define ADE9000_EP_CFG 0x0021			/*Read energy register with reset*/
										/*Enable energy accumulation, accumulate samples at 8ksps*/
										/*latch energy accumulation after EGYRDY*/
										/*If accumulation is changed to half line cycle mode, change EGY_TIME*/
#define ADE9000_EGY_TIME 0x1F3F 		/*Accumulate 8000 samples (1 s @ 8 kHz FDSP)*/

#define ADE9000_PWR_TIME 0x1F3F 		/*Accumulate 8000 samples(0x1F3F)*/

/*Waveform buffer Settings*/
#define ADE9000_WFB_CFG 0x1000			/*Neutral current samples enabled, Resampled data enabled*/
										/*Burst all channels*/
// Size of resampled waveform buffer to read per channel.
// - Hardware maximum: 512 resampled points per channel (4 line cycles @ 128 pts/cycle)
// - Project setting: 384 points (3 line cycles) to support oscilloscope captures while
//   keeping memory usage reasonable. FFT processing continues to use only the first
//   128 samples from this buffer.
#define WFB_ELEMENT_ARRAY_SIZE 384  	/*size of buffer to read. 512 Max. Each element IA,VA...IN has max 512 points*/ 
										// [Size of waveform buffer/number of sample sets = 2048/4 = 512]
										/*(Refer ADE9000 technical reference manual for more details)*/

/*Full scale Codes referred from Datasheet.Respective digital codes are produced when ADC inputs are at full scale. Donot Change. */
#define ADE9000_RMS_FULL_SCALE_CODES  52702092
#define ADE9000_WATT_FULL_SCALE_CODES 20694066
#define ADE9000_RESAMPLED_FULL_SCALE_CODES  18196
#define ADE9000_PCF_FULL_SCALE_CODES  74532013

/*Size of array reading calibration constants from EEPROM*/
#define CALIBRATION_CONSTANTS_ARRAY_SIZE 13
#define ADE9000_EEPROM_ADDRESS 0x54			//1010xxxy xxx---> 100(A2,A1,A0 defined by hardware). y (1: Read, 0:Write)
#define ADE9000_EEPROM_PAGE_SIZE 16
#define ADE9000_EEPROM_WRITE_CYCLE_MS 10
#define EEPROM_WRITTEN 0x01

/*Address of registers stored in EEPROM.Calibration data is 4 bytes*/
#define ADDR_CHECKSUM_EEPROM 0x0800		 // Simple checksum to verify data transmission errors. Add all the registers upto CPGAIN. The lower 32 bits should match data starting @ADDR_CHECKSUM_EEPROM
#define ADDR_EEPROM_WRITTEN_BYTE 0x0804  //1--> EEPROM Written, 0--> Not written. One byte only

#define ADDR_AIGAIN_EEPROM 0x0004
#define ADDR_BIGAIN_EEPROM 0x000C
#define ADDR_CIGAIN_EEPROM 0x0010
#define ADDR_NIGAIN_EEPROM 0x0014
#define ADDR_AVGAIN_EEPROM 0x0018
#define ADDR_BVGAIN_EEPROM 0x001C
#define ADDR_CVGAIN_EEPROM 0x0020
#define ADDR_APHCAL0_EEPROM 0x0024
#define ADDR_BPHCAL0_EEPROM 0x0028
#define ADDR_CPHCAL0_EEPROM 0x002C
#define ADDR_APGAIN_EEPROM 0x0030
#define ADDR_BPGAIN_EEPROM 0x0034
#define ADDR_CPGAIN_EEPROM 0x0038
// General persistent register save/load helpers
#define ADDR_MASK0_EEPROM 0x1000
#define ADDR_MASK1_EEPROM 0x1010


/****************************************************************************************************************
 EEPROM Global Variables
****************************************************************************************************************/

extern uint32_t calibrationDatafromEEPROM[CALIBRATION_CONSTANTS_ARRAY_SIZE];
extern uint32_t ADE9000_CalibrationRegAddress[CALIBRATION_CONSTANTS_ARRAY_SIZE];
extern uint32_t ADE9000_Eeprom_CalibrationRegAddress[CALIBRATION_CONSTANTS_ARRAY_SIZE];

/****************************************************************************************************************
 Structures and Global Variables
****************************************************************************************************************/
#if OLDAPI

 struct ResampledWfbData
 {
  int16_t VA_Resampled[WFB_ELEMENT_ARRAY_SIZE];
  int16_t IA_Resampled[WFB_ELEMENT_ARRAY_SIZE];
  int16_t VB_Resampled[WFB_ELEMENT_ARRAY_SIZE];
  int16_t IB_Resampled[WFB_ELEMENT_ARRAY_SIZE];
  int16_t VC_Resampled[WFB_ELEMENT_ARRAY_SIZE];
  int16_t IC_Resampled[WFB_ELEMENT_ARRAY_SIZE];
  int16_t IN_Resampled[WFB_ELEMENT_ARRAY_SIZE];
 };

  struct ActivePowerRegs
 {
	int32_t ActivePowerReg_A;
	int32_t ActivePowerReg_B;
	int32_t ActivePowerReg_C;
 };
 
   struct ReactivePowerRegs
 {
	int32_t ReactivePowerReg_A;
	int32_t ReactivePowerReg_B;
	int32_t ReactivePowerReg_C;
 };
 
  struct ApparentPowerRegs
 {
	int32_t ApparentPowerReg_A;
	int32_t ApparentPowerReg_B;
	int32_t ApparentPowerReg_C;
 };

  struct VoltageRMSRegs
 {
	int32_t VoltageRMSReg_A;
	int32_t VoltageRMSReg_B;
	int32_t VoltageRMSReg_C;
 };

  struct CurrentRMSRegs
 {
	int32_t CurrentRMSReg_A;
	int32_t CurrentRMSReg_B;
	int32_t CurrentRMSReg_C;
	int32_t CurrentRMSReg_N;
 };

  struct FundActivePowerRegs
 {
	int32_t FundActivePowerReg_A;
	int32_t FundActivePowerReg_B;
	int32_t FundActivePowerReg_C;
 }; 
 
   struct FundReactivePowerRegs
 {
	int32_t FundReactivePowerReg_A;
	int32_t FundReactivePowerReg_B;
	int32_t FundReactivePowerReg_C;
 }; 
 
   struct FundApparentPowerRegs
 {
	int32_t FundApparentPowerReg_A;
	int32_t FundApparentPowerReg_B;
	int32_t FundApparentPowerReg_C;
 }; 
 
	struct FundVoltageRMSRegs
 {
	int32_t FundVoltageRMSReg_A;
	int32_t FundVoltageRMSReg_B;
	int32_t FundVoltageRMSReg_C;
 }; 
 
	struct FundCurrentRMSRegs
 {
	int32_t FundCurrentRMSReg_A;
	int32_t FundCurrentRMSReg_B;
	int32_t FundCurrentRMSReg_C;
	//Fundamental neutral RMS is not calculated 
 }; 
 
	 struct HalfVoltageRMSRegs
 {
	int32_t HalfVoltageRMSReg_A;
	int32_t HalfVoltageRMSReg_B;
	int32_t HalfVoltageRMSReg_C;
 }; 
 
	 struct HalfCurrentRMSRegs
 {
	int32_t HalfCurrentRMSReg_A;
	int32_t HalfCurrentRMSReg_B;
	int32_t HalfCurrentRMSReg_C;
	int32_t HalfCurrentRMSReg_N;	
 }; 
 
	 struct Ten12VoltageRMSRegs
 {
	int32_t Ten12VoltageRMSReg_A;
	int32_t Ten12VoltageRMSReg_B;
	int32_t Ten12VoltageRMSReg_C;	
 }; 
 
	 struct Ten12CurrentRMSRegs
 {
	int32_t Ten12CurrentRMSReg_A;
	int32_t Ten12CurrentRMSReg_B;
	int32_t Ten12CurrentRMSReg_C;
	int32_t Ten12CurrentRMSReg_N;	
 }; 
 
	 struct VoltageTHDRegs
 {
	int32_t VoltageTHDReg_A;
	int32_t VoltageTHDReg_B;
	int32_t VoltageTHDReg_C;
	float VoltageTHDValue_A;
	float VoltageTHDValue_B;
	float VoltageTHDValue_C;	
 }; 
 
	 struct CurrentTHDRegs
 {
	int32_t CurrentTHDReg_A;
	int32_t CurrentTHDReg_B;
	int32_t CurrentTHDReg_C;
	float CurrentTHDValue_A;
	float CurrentTHDValue_B;
	float CurrentTHDValue_C;	
 }; 
  
	 struct PowerFactorRegs
 {
	int32_t PowerFactorReg_A;
	int32_t PowerFactorReg_B;
	int32_t PowerFactorReg_C;	
	float PowerFactorValue_A;
	float PowerFactorValue_B;
	float PowerFactorValue_C;
 };  
 
	 struct PeriodRegs
 {
	int32_t PeriodReg_A;
	int32_t PeriodReg_B;
	int32_t PeriodReg_C;
	float FrequencyValue_A;
	float FrequencyValue_B;
	float FrequencyValue_C;	
 };   

	  struct AngleRegs
 {
	int16_t AngleReg_VA_VB;
	int16_t AngleReg_VB_VC;
	int16_t AngleReg_VA_VC;
	int16_t AngleReg_VA_IA;
	int16_t AngleReg_VB_IB;
	int16_t AngleReg_VC_IC;
	int16_t AngleReg_IA_IB;
	int16_t AngleReg_IB_IC;
	int16_t AngleReg_IA_IC;
	float AngleValue_VA_VB;
	float AngleValue_VB_VC;
	float AngleValue_VA_VC;
	float AngleValue_VA_IA;
	float AngleValue_VB_IB;
	float AngleValue_VC_IC;
	float AngleValue_IA_IB;
	float AngleValue_IB_IC;
	float AngleValue_IA_IC;	
 };   
 
	 struct TemperatureRegnValue
 {
	int16_t Temperature_Reg;
	float Temperature;	
 };   

struct ActiveEnergyRegs {
	int32_t ActiveEnergyReg_A;
	int32_t ActiveEnergyReg_B;
	int32_t ActiveEnergyReg_C;
};

struct ReactiveEnergyRegs {
	int32_t ReactiveEnergyReg_A;
	int32_t ReactiveEnergyReg_B;
	int32_t ReactiveEnergyReg_C;
};

struct ApparentEnergyRegs {
	int32_t ApparentEnergyReg_A;
	int32_t ApparentEnergyReg_B;
	int32_t ApparentEnergyReg_C;
};

#endif

#if !OLDAPI
/* ------------------------------------------------------------------------------------------------------------
 Indexed-table alternatives (non-breaking): easier programmatic access via indices
 Keep original structs above for compatibility. These provide array-based versions and enums for indices.
 ------------------------------------------------------------------------------------------------------------ */

/* Phase/Neutral index helpers */
enum PhaseIndex : uint8_t {
	PHASE_A = 0,
	PHASE_B = 1,
	PHASE_C = 2,
	PHASE_N = 3,
	PHASE_COUNT = 3,       /* A, B, C only */
	PHASE_N_COUNT = 4      /* A, B, C, N */
};

/* Channel index for resampled waveform buffers */
enum ResampledChannelIndex : uint8_t {
	CH_IA = 0,
	CH_VA = 1,
	CH_IB = 2,
	CH_VB = 3,
	CH_IC = 4,
	CH_VC = 5,
	CH_IN = 6,
	CH_COUNT = 7
};

/* Angle index for AngleRegs */
enum AngleIndex : uint8_t {
	ANG_VA_VB = 0,
	ANG_VB_VC = 1,
	ANG_VA_VC = 2,
	ANG_VA_IA = 3,
	ANG_VB_IB = 4,
	ANG_VC_IC = 5,
	ANG_IA_IB = 6,
	ANG_IB_IC = 7,
	ANG_IA_IC = 8,
	ANG_COUNT = 9
};

/* Array-based mirrors of the register groups */
struct ActivePowerRegsTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] */
};

struct ReactivePowerRegsTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] */
};

struct ApparentPowerRegsTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] */
};

struct VoltageRMSTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] */
};

struct CurrentRMSTbl {
	int32_t phase[PHASE_N_COUNT]; /* [A,B,C,N] */
};

struct FundActivePowerRegsTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] */
};

struct FundReactivePowerRegsTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] */
};

struct FundApparentPowerRegsTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] */
};

struct FundVoltageRMSTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] */
};

struct FundCurrentRMSTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] - neutral fundamental not provided */
};

struct HalfVoltageRMSTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] */
};

struct HalfCurrentRMSTbl {
	int32_t phase[PHASE_N_COUNT]; /* [A,B,C,N] */
};

struct Ten12VoltageRMSTbl {
	int32_t phase[PHASE_COUNT]; /* [A,B,C] */
};

struct Ten12CurrentRMSTbl {
	int32_t phase[PHASE_N_COUNT]; /* [A,B,C,N] */
};

struct ActiveEnergyRegsTbl {
    int32_t phase[PHASE_COUNT];   /* [A,B,C] */
};

struct ReactiveEnergyRegsTbl {
    int32_t phase[PHASE_COUNT];   /* [A,B,C] */
};

struct ApparentEnergyRegsTbl {
    int32_t phase[PHASE_COUNT];   /* [A,B,C] */
};

struct VoltageTHDTbl {
	int32_t reg[PHASE_COUNT];  /* [A,B,C] */
	float   value[PHASE_COUNT];/* [A,B,C] */
};

struct CurrentTHDTbl {
	int32_t reg[PHASE_COUNT];  /* [A,B,C] */
	float   value[PHASE_COUNT];/* [A,B,C] */
};

struct PowerFactorTbl {
	int32_t reg[PHASE_COUNT];  /* [A,B,C] */
	float   value[PHASE_COUNT];/* [A,B,C] */
};

struct PeriodTbl {
	int32_t reg[PHASE_COUNT];   /* [A,B,C] */
	float   freq[PHASE_COUNT];  /* [A,B,C] */
};

struct AngleTbl {
	int16_t reg[ANG_COUNT];
	float   value[ANG_COUNT];
};

struct TemperatureTbl {
	int16_t reg;    /* same as Temperature_Reg */
	float   value;  /* same as Temperature */
};

/* Indexed version of waveform buffer data */
struct ResampledWfbTable {
	int16_t ch[CH_COUNT][WFB_ELEMENT_ARRAY_SIZE]; /* [channel][sample] */
};
#endif

class ADE9000Class
{
	public:
		ADE9000Class();
		void SetupADE9000(void); 
		
		/*SPI Functions*/
		void SPI_Init(uint32_t SPI_speed , uint8_t chipSelect_Pin,
		               uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin);		
		void SPI_Write_16(uint16_t Address , uint16_t Data );
		void SPI_Write_32(uint16_t Address , uint32_t Data );		
		uint16_t SPI_Read_16(uint16_t Address);
		uint32_t SPI_Read_32(uint16_t Address);
#if OLDAPI				
		void SPI_Burst_Read_Resampled_Wfb(uint16_t Address, uint16_t Read_Element_Length, ResampledWfbData *ResampledData);    
		/*ADE9000 Calculated Parameter Read Functions*/
		void ReadActivePowerRegs(ActivePowerRegs *Data);
		void ReadReactivePowerRegs(ReactivePowerRegs *Data);
		void ReadApparentPowerRegs(ApparentPowerRegs *Data);
		void ReadVoltageRMSRegs(VoltageRMSRegs *Data);
		void ReadCurrentRMSRegs(CurrentRMSRegs *Data);
		void ReadFundActivePowerRegs(FundActivePowerRegs *Data);
		void ReadFundReactivePowerRegs(FundReactivePowerRegs *Data);
		void ReadFundApparentPowerRegs(FundApparentPowerRegs *Data);
		void ReadFundVoltageRMSRegs(FundVoltageRMSRegs *Data);
		void ReadFundCurrentRMSRegs(FundCurrentRMSRegs *Data);
		void ReadHalfVoltageRMSRegs(HalfVoltageRMSRegs *Data);
		void ReadHalfCurrentRMSRegs(HalfCurrentRMSRegs *Data);
		void ReadTen12VoltageRMSRegs(Ten12VoltageRMSRegs *Data);
		void ReadTen12CurrentRMSRegs(Ten12CurrentRMSRegs *Data);
		void ReadVoltageTHDRegsnValues(VoltageTHDRegs *Data);
		void ReadCurrentTHDRegsnValues(CurrentTHDRegs *Data);
		void ReadPowerFactorRegsnValues(PowerFactorRegs *Data);
		void ReadPeriodRegsnValues(PeriodRegs *Data);
		void ReadAngleRegsnValues(AngleRegs *Data);		
		void ReadTempRegnValue(TemperatureRegnValue *Data);
		void ReadActiveEnergyRegs(ActiveEnergyRegs *Data);
		void ReadReactiveEnergyRegs(ReactiveEnergyRegs *Data);
		void ReadApparentEnergyRegs(ApparentEnergyRegs *Data);
#endif

#if !OLDAPI
		void SPI_Burst_Read_Resampled_Wfb(uint16_t Address, uint16_t Read_Element_Length, ResampledWfbTable *tbl);
		void ReadActivePowerRegs(ActivePowerRegsTbl *tbl);	
		void ReadReactivePowerRegs(ReactivePowerRegsTbl *tbl);
		void ReadApparentPowerRegs(ApparentPowerRegsTbl *tbl);
		// Indexed-table overload: fills tbl->phase[PHASE_A|PHASE_B|PHASE_C]
		void ReadVoltageRMSRegs(VoltageRMSTbl *tbl);
		// Indexed-table overload: fills tbl->phase[PHASE_A|PHASE_B|PHASE_C|PHASE_N]
		void ReadCurrentRMSRegs(CurrentRMSTbl *tbl);
		void ReadFundActivePowerRegs(FundActivePowerRegsTbl *tbl);
		void ReadFundReactivePowerRegs(FundReactivePowerRegsTbl *tbl);
		void ReadFundApparentPowerRegs(FundApparentPowerRegsTbl *tbl);
		void ReadFundVoltageRMSRegs(FundVoltageRMSTbl *tbl);
		void ReadFundCurrentRMSRegs(FundCurrentRMSTbl *tbl);
		void ReadHalfVoltageRMSRegs(HalfVoltageRMSTbl *tbl);
		void ReadHalfCurrentRMSRegs(HalfCurrentRMSTbl *tbl);
		void ReadTen12VoltageRMSRegs(Ten12VoltageRMSTbl *tbl);
		void ReadTen12CurrentRMSRegs(Ten12CurrentRMSTbl *tbl);
		void ReadVoltageTHDRegsnValues(VoltageTHDTbl *tbl);
		void ReadCurrentTHDRegsnValues(CurrentTHDTbl *tbl);
		void ReadPowerFactorRegsnValues(PowerFactorTbl *tbl);
		void ReadPeriodRegsnValues(PeriodTbl *tbl);
		void ReadAngleRegsnValues(AngleTbl *tbl);
		void ReadTempRegnValue(TemperatureTbl *tbl);
		void ReadActiveEnergyRegs(ActiveEnergyRegsTbl *tbl);
		void ReadReactiveEnergyRegs(ReactiveEnergyRegsTbl *tbl);
		void ReadApparentEnergyRegs(ApparentEnergyRegsTbl *tbl);
		// Averaged power from accumulator registers (1-second time-averaged)
		void ReadAvgActivePowerRegs(ActivePowerRegsTbl *tbl);
		void ReadAvgReactivePowerRegs(ReactivePowerRegsTbl *tbl);
		void ReadAvgApparentPowerRegs(ApparentPowerRegsTbl *tbl);
		// Fundamental averaged power from accumulator registers
		void ReadAvgFundActivePowerRegs(FundActivePowerRegsTbl *tbl);
		void ReadAvgFundReactivePowerRegs(FundReactivePowerRegsTbl *tbl);
		void ReadAvgFundApparentPowerRegs(FundApparentPowerRegsTbl *tbl);
#endif		
		/*EEPROM Functions*/
		void writeByteToEeprom(uint16_t dataAddress, uint8_t data);	
		uint8_t ReadByteFromEeprom(uint16_t dataAddress);
		void writeBytesToEeprom(uint16_t address, const uint8_t* data, uint16_t length);
		void writeWordToEeprom(uint16_t address, uint32_t data);
		uint32_t readWordFromEeprom(uint16_t address);
		
		void savePersistentREG(const char* regName, uint32_t value);
		uint32_t loadPersistentREG(const char* regName);
	private:
		uint8_t  _chipSelect_Pin;
};


	

#endif

