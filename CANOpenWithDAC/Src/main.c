/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f0xx_hal.h"
#include "main_hal.h"
#include "CANopen.h"
#include "stdlib.h"



/* Private variables ---------------------------------------------------------*/
CanTxMsgTypeDef CAN_TX_Msg;
CanRxMsgTypeDef CAN_RX_Msg;
__IO uint16_t u16Timer = 0;
__IO uint8_t u8TmrCallbackEnabled = 0;
uint16_t u16Voltage = 0;
uint8_t u8I = 0;

typedef	union
{
	uint32_t u32[2];
	uint16_t u16[4];
	uint8_t u8[8];
	int32_t i32[2];
	int16_t i16[4];
	int8_t i8[8];
} CAN_Variant_t;

CAN_Variant_t CanMSG;

#define TMR_TASK_INTERVAL   (1000)          /* Interval of tmrTask thread in microseconds */
#define INCREMENT_1MS(var)  (var++)         /* Increment 1ms variable in tmrTask */

/* Global variables and objects */
    volatile uint16_t   CO_timer1ms = 0U;   /* variable increments each millisecond */ 
		volatile bool FAILSTATE = false; // added foas global FAILSTATE variable
		volatile float Encoder_Set = 300; // Encoder = position 0
		volatile float CAN_DATA[10];//CANDATA array for data validation
		volatile int resetbit = 5;
/* Private function prototypes -----------------------------------------------*/
uint8_t u8Data[32];


/* timer thread executes in constant intervals ********************************/
static void tmrTask_thread(void)
{
  //for(;;) 
	{
    /* sleep for interval */

    INCREMENT_1MS(CO_timer1ms);

    if(CO->CANmodule[0]->CANnormal) 
		{
			bool_t syncWas;

			/* Process Sync and read inputs */
			syncWas = CO_process_SYNC_RPDO(CO, TMR_TASK_INTERVAL);

			/* Further I/O or nonblocking application code may go here. */

			/* Write outputs */
			CO_process_TPDO(CO, syncWas, TMR_TASK_INTERVAL);

			/* verify timer overflow */
			if(0) 
			{
					CO_errorReport(CO->em, CO_EM_ISR_TIMER_OVERFLOW, CO_EMC_SOFTWARE_INTERNAL, 0U);
			}
    }
  }
}


/* CAN interrupt function *****************************************************/
void /* interrupt */ CO_CAN1InterruptHandler(void)
{
  CO_CANinterrupt_Rx(CO->CANmodule[0]);


  /* clear interrupt flag */
} 


/**
  * @brief  Transmission  complete callback in non blocking mode 
  * @param  hcan: pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
	// Implement CAN RX
	CO_CAN1InterruptHandler();
	
	__HAL_CAN_ENABLE_IT(hcan, CAN_IT_FMP0);
}

/**
  * @brief  SYSTICK callback.
  * @retval None
  */
void HAL_SYSTICK_Callback(void)
{
	// Decrement u16Timer every 1 ms down to 0
  if ( u16Timer > 0 )
		u16Timer--;
	
	if ( u8TmrCallbackEnabled )
	  tmrTask_thread();
}

void reset_variables(void)
	{
		//((void (code *) (void)) 0x0000) ();
		//exit(1);
		MainInit();
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		resetbit = resetbit - 1;
		SetAnalogOutput(0);
		Encoder_Set = 300;
		CAN_DATA[0] = 0;CAN_DATA[1] = 0;CAN_DATA[2] = 0;CAN_DATA[3] = 0;CAN_DATA[4] = 0;
		CAN_DATA[5] = 0;CAN_DATA[6] = 0;CAN_DATA[7] = 0;CAN_DATA[8] = 0;CAN_DATA[9] = 0;
		HAL_GPIO_WritePin(Out1_HS_GPIO_Port, Out1_HS_Pin, GPIO_PIN_RESET); //JUMPER Set pin 2 (OUT1) HIGH / TRUE
		HAL_GPIO_WritePin(Out2_HS_GPIO_Port, Out2_HS_Pin, GPIO_PIN_RESET); //MICROSWITCH 1 + 2 Set pin 3 (OUT2) HIGH / TRUE
		HAL_GPIO_WritePin(Out3_HS_GPIO_Port, Out3_HS_Pin, GPIO_PIN_RESET);
		
		if (resetbit == 0)
			{
				FAILSTATE =0;
				resetbit = 5;
			}
		
	}
		

bool SW1(){//s1 analog to bool conversion with threshhold 20mV
		float IN2 = ReadAnalogInput(ADC_IN2);
	  bool Sw1 = 0;
		if(IN2 < 20)
			{
				Sw1=0;
			} 
		else 
			{
				Sw1=1;
			}
		
		return Sw1;
}

bool SW2(){//s2 analog to bool conversion with threshhold 20mV
		float IN3 = ReadAnalogInput(ADC_IN3);
	  bool Sw2 = 0;
		if(IN3 < 20)
			{
				Sw2=0;
			} 
		else 
			{
				Sw2=1;
			}
		
		return Sw2;
}
int Initialize_outputs(){
	//STG-826 
	// 6 INPUTS(3X DIGITAL,3 X ANALOG 0-34VDC), 
	// 4 OUTPUTS (3X DIGITAL, 1X ANALOG)
	//========================================================================
	
	
	//initialize Analog output 5V 
	//for encoder with DAC functionality (OUT4) analogset to 5V
	//DAC1_CHANNEL_1_WritePin(GPIOC,GPIO_PIN_4,50); // trigger set analog OUT4 to 5V

	SetAnalogOutput(5100);
	//Initialize Jumper output set HIGH (24V)
	HAL_GPIO_WritePin(Out1_HS_GPIO_Port, Out1_HS_Pin, GPIO_PIN_SET); //JUMPER Set pin 2 (OUT1) HIGH / TRUE
	
	//Initialize Microswitch Status powered outputs
	HAL_GPIO_WritePin(Out2_HS_GPIO_Port, Out2_HS_Pin, GPIO_PIN_SET); //MICROSWITCH 1 + 2 Set pin 3 (OUT2) HIGH / TRUE
	HAL_GPIO_WritePin(Out3_HS_GPIO_Port, Out3_HS_Pin, GPIO_PIN_SET); //MICROSWITCH 3 + 4 Set pin 4 (OUT3) HIGH / TRUE
	

	return(true);
}


float EN1_filter()//uint16_t n)
{
		/** OLD code
	
	Enc_Val_raw = ReadAnalogInput(ADC_IN1);
	
	//n=1;
	//uint16_t data[n];
	float SUM = 0;
	float Enc_val = 0;
	float Enc_Val_raw;
	float Average;
	int i = 1;
	bool state=1;
//	while (i<=n)
//		{
//		  uint8_t Enc_old; 
//			Enc_Val_raw = ReadAnalogInput(ADC_IN1);//HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0);// READ ENCODER VALUE float value NOT digital value
//			//Normalization
//			//____________________
//			
//			
//			//data[n] = Enc_val;
////			if(state) {Enc_old = Enc_Val_raw; state =0; }
////			else {state=1;}
////			//Enc_Val_raw = 90*Enc_Val_raw+(1-90)*Enc_old;//exponential filter
//			SUM = SUM + Enc_Val_raw;   							//Sum for Average 
//			i = i+1;														//value counter
//			
//		}
	Average = (long)SUM/(long)(n);
	//SUM
	Average = Average - 300;//Subtraction
	Average = Average / 970;      			//Division
	Average = Average * 1023;           //GAIN
	//HAL_Delay(100);
		*/
	
	
	float Enc_Val_raw = ReadAnalogInput(ADC_IN1);
	
	float Enc_Val =(((Enc_Val_raw-285)/918)*1023);
	
	if(__fabs(Encoder_Set-(int)Enc_Val_raw) > 8)
		{
			//float Enc_Val_raw = ReadAnalogInput(ADC_IN1);
			Enc_Val =(((Enc_Val_raw-285)/918)*1023);
			Encoder_Set = Enc_Val_raw;
		}
	else
		{
			Enc_Val =(((Encoder_Set-285)/918)*1023);
			//float Enc_Val_raw1 = ReadAnalogInput(ADC_IN1);
			//float Enc_Val_1 = (((Enc_Val_raw1-285)/918)*1023);
		
		}	
	if ( Enc_Val < 70)//EMG
		{
			Enc_Val = 0;
		}
	if ( Enc_Val > 1020)//TMAX
		{
			Enc_Val = 1023;
		}
	if ( 635 > Enc_Val && Enc_Val > 455)//IDLE
		{
			Enc_Val = 546;
		}
	if ( 152 > Enc_Val && Enc_Val > 114)//BMAX
		{
			Enc_Val = 114;
		}
	if ( 455 > Enc_Val && Enc_Val > 440)//BMIN
		{
			Enc_Val = 455;
		}		
	if ( 640 > Enc_Val && Enc_Val > 635)//TMIN
		{
			Enc_Val = 636;
		}
		
	return((long)Enc_Val);//Enc_Val_raw);//
}

 float Validaton(float Enc_valid){
	 
	 Enc_valid = EN1_filter();
	 //float ArrEnc[10]= {0,0,0,0,0,0,0,0,0,0};
	 
	 if(Enc_valid > -1 && Enc_valid < 1024) // Datavalidility check {Enc_DataVal}
	 {
		 CAN_DATA[0] = true;
	 }
	 else
	 {
		 CAN_DATA[0] = false;
	 }
	 
	 if(Enc_valid >= 636 && Enc_valid <= 1023) // TRACTION Pos active {TrBr_T}
	 {
		 CAN_DATA[1] = true;
		 bool SW_1 = SW1();
		 if(SW_1 && Enc_valid >= 670 && 1023 >= Enc_valid )//Check status of S1 {MICRO1_TrBr_Ko}
			{
				CAN_DATA[5] = 1;
			}
		 else
			{
				CAN_DATA[5]=0;
			} 
	 }
	 else
	 {
		 CAN_DATA[1] = false;
		 bool SW_1 = SW1();
		
		 if(SW_1 && Enc_valid >= 0 && 602 >= Enc_valid)
			{
				CAN_DATA[5] = 0;
			}
		 
	 }

	 if(Enc_valid > 455 && Enc_valid < 635) // IDLE Pos active {TrBr_Zero}
			{
				CAN_DATA[2] = true;
				bool SW_2 = SW2();
		 if(SW_2)//Check status of S2 {MICRO2_TrBr_Ko}
				{
						CAN_DATA[6] = 0;
				}
 
			}
	 else
		 {
			 
			 CAN_DATA[2] = false;
			 bool SW_2 = SW2();
			 bool F1=0;
			 bool F2=0;
			 if(SW_2 && Enc_valid > 0 && Enc_valid < 421)
					{
						CAN_DATA[6]=1;
						F1=1;
					}
			 if (SW_2 && Enc_valid > 671 && Enc_valid < 1023)
					{
						CAN_DATA[6]=1;
						F2=1;
					}
			 if(!F1 && !F2)
				 {
					 CAN_DATA[6] = 0;
				 }
		 }
	 
/**	 if(Enc_valid >= 100 && Enc_valid < 500) // BRAKE Pos active {TrBr_B} 
//	 {
//		 CAN_DATA[3] = true;
//		 if(HAL_GPIO_ReadPin(DIN6_Port,DIN6_Pin)){CAN_DATA[8] = 0;}else{CAN_DATA[8]=1;} //Check status of S4 {MICRO4_TrBr_Ko}
//	 }
//	 else
//	 {
//		 CAN_DATA[3] = false;
//		 if(HAL_GPIO_ReadPin(DIN6_Port,DIN6_Pin)){CAN_DATA[8] = 1;}else{CAN_DATA[8]=0;}
//	 }*/
	 /**	 if(Enc_valid > 109 && Enc_valid < 500) // BRAKE Pos active {TrBr_B} inverted
//	 {
//		 CAN_DATA[3] = false;
//		 if(HAL_GPIO_ReadPin(DIN6_Port,DIN6_Pin)){CAN_DATA[8] = 1;}else{CAN_DATA[8]=0;} //Check status of S4 {MICRO4_TrBr_Ko}
//	 }
//	 else
//	 {
//		 CAN_DATA[3] = true;
//		 if(HAL_GPIO_ReadPin(DIN6_Port,DIN6_Pin)){CAN_DATA[8] = 0;}else{CAN_DATA[8]=1;}
//	 }*/
	 
	 if(Enc_valid >= 0 && Enc_valid <= 100) // EMERGENCY Pos active {TrBr_EMG} S3 {MICRO3_TrBr_Ko}
	 {
		 CAN_DATA[4] = true;
		 if(HAL_GPIO_ReadPin(DIN4_Port,DIN4_Pin) && Enc_valid >= 0 && Enc_valid <= 80)
				{
					CAN_DATA[7] = 0;
				}
			
	 }
	 else
	 {
		  CAN_DATA[4] = false;
			if(HAL_GPIO_ReadPin(DIN4_Port,DIN4_Pin) && Enc_valid >= 148 && Enc_valid <= 1023)
				{
					CAN_DATA[7] = 1;
				}
		  
	 }
	/** if(Enc_valid > 0 && Enc_valid <= 10) // EMERGENCY Pos active {TrBr_EMG} inverted
	 {
		 CAN_DATA[4] = false;
		 if(HAL_GPIO_ReadPin(DIN5_Port,DIN5_Pin)){CAN_DATA[7] = 0;}else{CAN_DATA[7]=1;} //Check status of S3 {MICRO3_TrBr_Ko}
	 }
	 else
	 {
		 CAN_DATA[4] = true;
		 if(HAL_GPIO_ReadPin(DIN5_Port,DIN5_Pin)){CAN_DATA[7] = 1;}else{CAN_DATA[7]=0;}
	 }*/
	 if((!CAN_DATA[0]) || CAN_DATA[5] || CAN_DATA[6] || CAN_DATA[7] || CAN_DATA[8] || FAILSTATE)
		{
			CAN_DATA[9] = 0;
			FAILSTATE = true;
		} 
		else
		{
			CAN_DATA[9]=1;
		}
		
	 //return(CAN_DATA);
 }


 
 uint8_t Dataset(bool b1,bool b2,bool b3,bool b4,bool b5,bool b6,bool b7,bool b8)
{
	int i;

	static uint8_t dataBits[8]= {0};
	dataBits[0] = b1;
	dataBits[1] = b2;
	dataBits[2] = b3;
	dataBits[3] = b4;
	dataBits[4] = b5;
	dataBits[5] = b6;
	dataBits[6] = b7;
	dataBits[7] = b8;
	
//	//int value=0;
	uint8_t u8 = 0;
	int power = 1;
	
	for (i=0;i<8;i++)
	{
		u8 += dataBits[7-i]*power;
		power *=2;
	}
	
	
	for (i=0;i<2;i++)
	{
		u8 += dataBits[1-i]*power;
		power *=2;
	}

		
		
//	
//	u8 |= ((uint8_t)b1 << 0);
//	u8 |= ((uint8_t)b2 << 1);
//	u8 |= ((uint8_t)b3 << 2);
//	u8 |= ((uint8_t)b4 << 3);
//	u8 |= ((uint8_t)b5 << 4);
//	u8 |= ((uint8_t)b6 << 5);
//	u8 |= ((uint8_t)b7 << 6);
//	u8 |= ((uint8_t)b8 << 7);
	
  return u8;
}
 


int main(void)
{
	// Do not change the system clock above 16 MHz! Higher speed can lead to the destruction of the module!
	
	CO_NMT_reset_cmd_t reset = CO_RESET_NOT; 
  // System init	
  MainInit();
	bool STAT2 = Initialize_outputs();
	// LED On
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	SetAnalogOutput(5100);
	// Start DAC output
	HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
	
	// =======================================================================
	// Set up baudrate
	hcan.Instance->BTR &= 0xFFFFFC00;
	hcan.Instance->BTR |= CAN_250K;
	// =======================================================================
	float Enc_Val_raw = ReadAnalogInput(ADC_IN1);
	/* increase variable each startup. Variable should be stored in EEPROM. */
	OD_powerOnCounter++;
 	bool STAT1 = Initialize_outputs();
  /* Infinite loop */
  while (1)
  {
		// =======================================================================
		// CANOpen sample code
		// Untested port of https://github.com/CANopenNode/CANopenNode
		// =======================================================================
		
		while(reset != CO_RESET_APP)
		{
			/* CANopen communication reset - initialize CANopen objects *******************/
			CO_ReturnError_t err;
			uint16_t timer1msPrevious;
			//SetAnalogOutput(5100);
			/* disable CAN and CAN interrupts */
      CanDisable();
			uint8_t NodeID1 = 56; //Default set Node ID if Jumper open/FALSE
			bool NodeID_condition = 0;
			if (HAL_GPIO_ReadPin(DIN5_Port,DIN5_Pin))//ReadAnalogInput(ADC_IN2))  //Condition for noe ID is HIGH / TRUE
			{
				uint8_t NodeID1 = 58; //CPU1-CAB2
				NodeID_condition = 1;
			}
			else
			{
				uint8_t NodeID1 = 56; //CPU1-CAB1
				NodeID_condition = 0;
			}
			/* initialize CANopen */
			err = CO_init(0/* CAN module address */, NodeID1/* NodeID */, CAN_250K /* bit rate */);
			if(err != CO_ERROR_NO)
			{
					while(1)
					{
						//int NodeID1 = 56; //Default set Node ID if Jumper open/FALSE
						bool NodeID = 0;
						if (HAL_GPIO_ReadPin(DIN5_Port,DIN5_Pin))//ReadAnalogInput(ADC_IN2))  //Condition is TRUE if pin 13 of register c is HIGH / TRUE
						{
							int NodeID1 = 58; //CPU1-CAB2
							NodeID_condition = 1;
						}
						else
						{
							int NodeID1 = 56; //CPU1-CAB1
							NodeID_condition = 0;
						}
						
						
						STAT1 = Initialize_outputs();
						//========================================================================
						//Read analog inputs and call arithmitic function for n values of encoder
						//1. Moving average function + convert Float to LONG int 
						//2. Switch status readout
						//========================================================================
						 
						
						//long Enc_Val_filtered = EN1_filter(100);//Readout sensor value 0.21-4.08V translate to 0-1023 and filter noise for n variables
						
						
						// LED flicker for error
						if ( u16Timer == 0 )
						{
							u16Timer = 100;
							HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);


							// =======================================================================
							// Analog Output sample code
							// Increment Output voltage every one ms by 50 mV
							// If Output voltage overstepped 5100 mV it starts with 0
							// =======================================================================
							
//							u16Voltage = u16Voltage + 50;
//							if ( u16Voltage >= 5100 )
//								u16Voltage = 0;
						

						}
					}
					/* CO_errorReport(CO->em, CO_EM_MEMORY_ALLOCATION_ERROR, CO_EMC_SOFTWARE_INTERNAL, err); */
			}


			/* Configure Timer interrupt function for execution every 1 millisecond */

			
			/* Configure CAN transmit and receive interrupt */
      CanEnable();


			/* start CAN */
			CO_CANsetNormalMode(CO->CANmodule[0]);

			reset = CO_RESET_NOT;
			timer1msPrevious = CO_timer1ms;
			
			// Enable timer
			u8TmrCallbackEnabled = 1;

			while(reset == CO_RESET_NOT)
			{
				/* loop for normal program execution ******************************************/
				uint16_t timer1msCopy, timer1msDiff;

				timer1msCopy = CO_timer1ms;
				timer1msDiff = timer1msCopy - timer1msPrevious;
				timer1msPrevious = timer1msCopy;

				
				/* CANopen process */
				reset = CO_process(CO, timer1msDiff, NULL);

				/* Nonblocking application code may go here. */
				// LED handling:
				STAT1 = Initialize_outputs();
				
				// Send it by CAN
				hcan.pTxMsg->IDE = CAN_ID_STD;
				//========================================================================
				//Read analog inputs and call arithmitic function for n values of encoder
				//1. Moving average function + convert Float to LONG int 
				//2. Switch status readout
				//========================================================================
				if (HAL_GPIO_ReadPin(DIN5_Port,DIN5_Pin))//)
					{
						hcan.pTxMsg->StdId = 0x00003A;   //Reciever adres: 0x003A (DMA-15)
					}
				else
					{
						hcan.pTxMsg->StdId = 0x000038;   //Reciever adres: 0x0038 (DMA-15)
					} 
				
				hcan.pTxMsg->DLC = 4 ;					
				
				//Debugging code-----------------------------
			//_____________________________________________
				
				float JumperState = HAL_GPIO_ReadPin(DIN5_Port,DIN5_Pin);//ReadAnalogInput(ADC_IN2);
				float EncoderState = ReadAnalogInput(ADC_IN1);
				float EncoderState21 =	EN1_filter();
				float EncoderState3 = (((EncoderState-300)/970)*1023);
				float EncoderState4 = (((EncoderState-285)/918)*1023);
				bool S1 = SW1();
			  bool S2 = SW2();//ReadAnalogInput(ADC_IN3);//s2
				bool S3 = HAL_GPIO_ReadPin(DIN4_Port,DIN4_Pin);//s3
				bool Jumper = HAL_GPIO_ReadPin(DIN5_Port,DIN5_Pin);
				//bool S4 = HAL_GPIO_ReadPin(DIN6_Port,DIN6_Pin);
				
			//---------------------------------------------
			//---------------------------------------------
				float Enc_Val_filtered = EN1_filter();//Readout sensor value 0.21-4.08V translate to 0-1023 and filter noise for n variables
				long Enc_Val_filtered1 = (long)Enc_Val_filtered;
				//int * CAN_DATA[10] = {Validaton(Enc_Val_filtered)};//Function to validate the microswitches and encoder validility and convert them to an array
				
				Validaton(Enc_Val_filtered);//Function to validate the microswitches and encoder validility and convert them to an array
//CAN_DATA[10] = 
				CanMSG.u32[0] = 0;
				CanMSG.u32[1] = 0;
				
				CanMSG.u8[0] = *((uint8_t*)&(Enc_Val_filtered1)+1); //high byte (0x12)Enc_Val_filtered;
				CanMSG.u8[1] = *((uint8_t*)&(Enc_Val_filtered1)+0); //low byte  (0x34)Enc_Val_filtered;
				bool Enc_Data_Val 	= (bool)CAN_DATA[0];//129   10000001
				bool TrBr_T 				= (bool)CAN_DATA[1];
				bool TrBr_Zero			= (bool)CAN_DATA[2];//163   10100011
				bool TrBr_B					= (bool)CAN_DATA[3];//197   11000101
				bool TrBr_EMG				= (bool)CAN_DATA[4];// 1    00000001
				bool MICRO1_TrBr_Ko	= (bool)CAN_DATA[5];
				bool MICRO2_TrBr_Ko	= (bool)CAN_DATA[6];
				bool MICRO3_TrBr_Ko	= (bool)CAN_DATA[7];//129   10000001
				bool MICRO4_TrBr_Ko	= (bool)CAN_DATA[8];
				bool TrBr_dataValid = (bool)CAN_DATA[9];	
				
				uint8_t dataset1 = Dataset(CAN_DATA[0],CAN_DATA[1],CAN_DATA[2],CAN_DATA[3],CAN_DATA[4],CAN_DATA[5],CAN_DATA[6],CAN_DATA[7]);
				uint8_t dataset2 = Dataset(0,0,0,0,0,0,CAN_DATA[8],CAN_DATA[9]);
				CanMSG.u8[2] = dataset1;
				CanMSG.u8[3] = dataset2;
				// Transfer data
				hcan.pTxMsg->Data[0] = CanMSG.u8[0];
				hcan.pTxMsg->Data[1] = CanMSG.u8[1];
				hcan.pTxMsg->Data[2] = CanMSG.u8[2];
				hcan.pTxMsg->Data[3] = CanMSG.u8[3];
				
				if (FAILSTATE == true)
					{
						reset_variables();
						main();
					}
				
//				for ( u8I=0; u8I<8; u8I++ )
//					hcan.pTxMsg->Data[u8I] = CanMSG.u8[u8I];
				// Send it by CAN
				
				
				HAL_CAN_Transmit(&hcan, 25);
				
				{
					//	HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
					//	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);	
					//	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);	
				
					if ( u16Timer == 0 )
					{
						u16Timer = 1000;
						HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
					}
				}
								/* CANopen process */
				reset = CO_process(CO, timer1msDiff, NULL);
				// Watchdog refresh
				#if ( PRODUCTION_VERSION == 1 )
					HAL_IWDG_Refresh(&hiwdg);
					#warning Production version, Debugging not possible! <<<<<<<<<<<<<<<<<<<<<
				#else
					#warning Debug version without watch dog! <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
				#endif

				/* Process EEPROM */
			}
		}			

    /* stop threads */

    /* delete objects from memory */
    CO_delete(0/* CAN module address */);
 		
		
  }
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
