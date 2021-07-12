/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body Changed in accordance with 50Mz Project
	* Company						 : Santon Switchgear / santon Holland BV
	* Author/Editor			 : Zander van der Steege
	* Project code based on Example of STMicroelectronics
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
#include <stdlib.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
CanTxMsgTypeDef CAN_TX_Msg;
CanRxMsgTypeDef CAN_RX_Msg;
__IO uint16_t u16Timer = 0;
__IO uint8_t u8TmrCallbackEnabled = 0;
int NodeID = 56;

#define TMR_TASK_INTERVAL   (100)          /* Interval of tmrTask thread in microseconds */
#define INCREMENT_1MS(var)  (var++)         /* Increment 1ms variable in tmrTask */

/* Global variables and objects */
    volatile uint16_t   CO_timer1ms = 0U;   /* variable increments each millisecond */ 

/* Private function prototypes -----------------------------------------------*/
uint8_t u8Data[16];


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


int Initialize_outputs(){
	//STG-826 
	// 6 INPUTS(3X DIGITAL,3 X ANALOG 0-34VDC), 
	// 4 OUTPUTS (3X DIGITAL, 1X ANALOG)
	//========================================================================
	
	
	//initialize Analog output 5V 
	//for encoder with DAC functionality (OUT4) analogset to 5V
	//DAC1_CHANNEL_1_WritePin(GPIOC,GPIO_PIN_4,50); // trigger set analog OUT4 to 5V
	
	//Initialize Jumper output set HIGH (24V)
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, 1); //JUMPER Set pin 2 (OUT1) HIGH / TRUE
	
	//Initialize Microswitch Status powered outputs
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, 1); //MICROSWITCH 1 + 2 Set pin 3 (OUT2) HIGH / TRUE
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, 1); //MICROSWITCH 3 + 4 Set pin 4 (OUT3) HIGH / TRUE
	
	
	return(true);
}


float EN1_filter(n)
{
	
	//uint16_t data[n];
	float SUM = 0;
	float Enc_val = 0;
	int N=n;
	long Average;
	while (n<=25)
		{
		float Enc_Val_raw = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0);// READ ENCODER VALUE float value NOT digital value
		//Normalization
		//____________________
		
		Enc_val = Enc_Val_raw - 2.917;//Subtraction
		Enc_val = Enc_val / 0.94;      			//Division
		Enc_val = Enc_val * 1023;           //GAIN
	  //data[n] = Enc_val;
		SUM = SUM + Enc_val;   							//Sum for Average 
		n = n+1;														//value counter
		}
	
	return(Average = (long)SUM/(long)N);
}

 int * Validaton(Enc_valid){
	 static int Data[10]= {0};
	 
	 if(Enc_valid > -5 && Enc_valid < 1028) // Datavalidility check {Enc_DataVal}
	 {
		 Data[0] = true;
	 }
	 else
	 {
		 Data[0] = false;
	 }
	 
	 if(Enc_valid > 631 && Enc_valid < 1028) // TRACTION Pos active {TrBr_T}
	 {
		 Data[1] = true;
		 if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_3)){Data[5] = 0;}else{Data[5]=1;} //Check status of S1 {MICRO1_TrBr_Ko}
	 }
	 else
	 {
		 Data[1] = false;
		 if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_3)){Data[5] = 1;}else{Data[5]=0;}
	 }
	 
	 if(Enc_valid > 541 && Enc_valid < 551) // IDLE Pos active {TrBr_Zero}
	 {
		 Data[2] = true;
		 if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4)){Data[6] = 0;}else{Data[6]=1;} //Check status of S2 {MICRO2_TrBr_Ko}
	 }
	 else
	 {
		 Data[2] = false;
		 if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4)){Data[6] = 1;}else{Data[6]=0;}
	 }
	 
	 if(Enc_valid > 109 && Enc_valid < 500) // BRAKE Pos active {TrBr_B}
	 {
		 Data[3] = true;
		 if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6)){Data[8] = 0;}else{Data[8]=1;} //Check status of S4 {MICRO4_TrBr_Ko}
	 }
	 else
	 {
		 Data[3] = false;
		 if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6)){Data[8] = 1;}else{Data[8]=0;}
	 }
	 
	 	 if(Enc_valid > -5 && Enc_valid < 5) // EMERGENCY Pos active {TrBr_EMG} 
	 {
		 Data[4] = true;
		 if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5)){Data[7] = 1;}else{Data[7]=0;} //Check status of S3 {MICRO3_TrBr_Ko}
	 }
	 else
	 {
		 Data[4] = false;
		 if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5)){Data[7] = 0;}else{Data[7]=1;}
	 }
	 if( Data[5] || Data[6] || Data[7] || Data[8]){Data[9] = 0;} else{Data[9]=1;}
	 return(Data);
 }



int main(void)
{
	// Do not change the system clock above 16 MHz! Higher speed can lead to the destruction of the module!
	
	CO_NMT_reset_cmd_t reset = CO_RESET_NOT; 
  // System init	
  MainInit();
	// LED On
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	
	// =======================================================================
	// Set up baudrate
	hcan.Instance->BTR &= 0xFFFFFC00;
	hcan.Instance->BTR |= CAN_250K;
	// =======================================================================
	
	/* increase variable each startup. Variable should be stored in EEPROM. */
	OD_powerOnCounter++;
 	
  /* Infinite loop */
  while (1)
  {
		// =======================================================================
		// CANOpen sample code
		// Untested port of https://github.com/CANopenNode/CANopenNode
		// =======================================================================
		bool STAT1 = Initialize_outputs();
		//========================================================================
			//CAN-Open NodeID depending on if Jumper of OUT1 is TRUE/FALSE
			//TRUE : closed/TRUE (Node ID = 58) 
			//FALSE: open/FALSE  (Node ID = 56)
			//========================================================================
			int NodeID1 = 56; //Default set Node ID if Jumper open/FALSE
			bool NodeID = 0;
			if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1))  //Condition is TRUE if pin 13 of register c is HIGH / TRUE
			{
				int NodeID1 = 58; //CPU1-CAB2
				NodeID = 1;
			}
			else
			{
				int NodeID1 = 56; //CPU1-CAB1
				NodeID = 0;
			}
			
			
			STAT1 = Initialize_outputs();
			//========================================================================
			//Read analog inputs and call arithmitic function for n values of encoder
			//1. Moving average function + convert Float to LONG int 
			//2. Switch status readout
			//========================================================================
			 
			
			long Enc_Val_filtered = EN1_filter(25);//Readout sensor value 0.21-4.08V translate to 0-1023 and filter noise for n variables
			
			int * CAN_DATA[10] = {Validaton(Enc_Val_filtered)};//Function to validate the microswitches and encoder validility and convert them to an array

		while(reset != CO_RESET_APP)
		{
			
			
			
			
			/* CANopen communication reset - initialize CANopen objects *******************/
			CO_ReturnError_t err;
			uint16_t timer1msPrevious;

			/* disable CAN and CAN interrupts */
      CanDisable();

			/* initialize CANopen */
			err = CO_init(0/* CAN module address */, NodeID1 /* NodeID */, CAN_250K /* bit rate */);
			if(err != CO_ERROR_NO)
			{
					while(1)
					{
						// LED flicker for error
						if ( u16Timer == 0 )
						{
							u16Timer = 100;
							HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
						}
					}
					/* CO_errorReport(CO->em, CO_EM_MEMORY_ALLOCATION_ERROR, CO_EMC_SOFTWARE_INTERNAL, err); */
			}


			/* Configure Timer interrupt function for execution every 1 millisecond */
			
			//50MZ CANOPEN SETUP Not functioning
			//==========================================================================
			hcan.pTxMsg->IDE = CAN_ID_STD;//Standard-ID
			if (NodeID){hcan.pTxMsg->StdId = 0x003A;   //Reciever adres: 0x003A (DMA-15)
				}
			else{hcan.pTxMsg->StdId = 0x0038;   //Reciever adres: 0x0038 (DMA-15)
				}
			int * Data[16] = {0};
			
			hcan.pTxMsg->DLC = 18;         //Message length: 18 bytes
//			//==========================================================================
//			//50MZ CANOPEN MSG Extracted from CAN_DATA list                              NEED TO FIGURE OUT OUT TO INSERT CAN_DATA[] INTO Data[]
			hcan.pTxMsg-> Data [0] = 0x09; // Enc_Val 				[long]
			hcan.pTxMsg-> Data [1] = 0x01; // Enc_Data_Val		[bool]
			hcan.pTxMsg-> Data [2] = 0x01; // TrBr_T 					[bool]
			hcan.pTxMsg-> Data [3] = 0x01; // TrBr_Zero				[bool]
			hcan.pTxMsg-> Data [4] = 0x01; // TrBr_B     			[bool]
			hcan.pTxMsg-> Data [5] = 0x01; // TrBr_EMG				[bool]
			hcan.pTxMsg-> Data [6] = 0x01; // MICRO1_TrBr_Ko	[bool]
			hcan.pTxMsg-> Data [7] = 0x01; // MICRO2_TrBr_Ko	[bool]
//			hcan.pTxMsg-> Data [8] = 0x01; // MICRO3_TrBr_Ko	[bool]
//			hcan.pTxMsg-> Data [9] = 0x01; // MICRO4_TrBr_Ko	[bool]
//			hcan.pTxMsg-> Data [10] = 0x01; // TrBr_dataValid	[bool]
//			HAL_CAN_Transmit (& hcan, 10);  // 10ms time delay for safe transmission
//			}
			long Enc_Val = Enc_Val_filtered;
			bool Enc_Data_Val 	= CAN_DATA[0];
			bool TrBr_T 				= CAN_DATA[1];
			bool TrBr_Zero			= CAN_DATA[2];
			bool TrBr_B					= CAN_DATA[3];
			bool TrBr_EMG				= CAN_DATA[4];
			bool MICRO1_TrBr_Ko	= CAN_DATA[5];
			bool MICRO2_TrBr_Ko	= CAN_DATA[6];
			bool MICRO3_TrBr_Ko	= CAN_DATA[7];
			bool MICRO4_TrBr_Ko	= CAN_DATA[8];
					
				
			hcan.pTxMsg->Data[0] = Enc_Val; //Freigabevariable �bergeben 
			hcan.pTxMsg->Data[1] = Enc_Data_Val;    //Stromvariable �bergeben
			hcan.pTxMsg->Data[2] = TrBr_T; //Schrittzahl Low-Byte �bergeben
			hcan.pTxMsg->Data[3] = TrBr_Zero; //Schrittzahl High-Byte �bergeben
			hcan.pTxMsg->Data[4] = TrBr_B; //MinimalFrequenz Low-Byte �bergeben
			hcan.pTxMsg->Data[5] = TrBr_EMG; //MinimalFrequenz High-Byte �bergeben
			hcan.pTxMsg->Data[6] = MICRO1_TrBr_Ko; //MaximalFrequenz Low-Byte �bergeben
			hcan.pTxMsg->Data[7] = MICRO2_TrBr_Ko; //MaximalFrequenz Low-Byte �bergeben
			hcan.pTxMsg->Data[8] = MICRO3_TrBr_Ko;
			HAL_CAN_Transmit(&hcan, 10);  //10ms Zeitverz�gerung f�r sicheres Senden	
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
    CO_delete(NodeID1/* CAN module address */);
 		
		
  }
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/