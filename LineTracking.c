/*====================================================================================
 Program for for the line tracking sensor;
 This program reads AN0, AN4 and AN5.  I want that the trimpot reading to set the duration of the 
 interrupt delay, which in turn sets the DC motors' speed. AN4 and AN5
 represent the line tracking sensor results, and they are stored in LineLeft and
 LineRight variables.  If LineLeft > LineRight then step right stepper motor or if 
 LineLeft < LineRight then step left stepper motor.  The stepper motor will increment
 one step from every low to high transition of the Step Signal.  Therefore, it will
 take 2 Timer3 Interrupts to step the stepper motor one increment/step.
 Right Stepper Motor 	- Step Signal should be connected to RA2
                        - Direction Signal should be connected to RA3
 Left Stepper Motor 	- Step Signal should be connected to RB4
                    	- Direction Signal should be connected to RA4
 Direction is setup for stepping right stepper motor CW and left stepper motor CCW.
======================================================================================
 For information on PIC24H Compiler Peripheral Libraries refer to link below:
 file:///C:/Program%20Files%20(x86)/Microchip/xc16/v1.26/docs/periph_libs/16-bit%20Peripheral%20Libraries.htm
======================================================================================*/
#include "p24HJ128GP502.h"	//Include device library file
#include "adc.h"            //Include ADC library file
#include "timer.h"          //Include Timer Library file
#include "outcompare.h"		//Include Output Compare (PWM) library file // REV OTONIEL
#define  FCY 3685000


// Configuration Bits
_FOSCSEL(FNOSC_FRC & IESO_OFF);
_FOSC	(POSCMD_NONE & OSCIOFNC_ON & IOL1WAY_OFF & FCKSM_CSDCMD);
_FWDT	(FWDTEN_OFF & WINDIS_OFF & WDTPRE_PR32 & WDTPOST_PS1);
_FPOR	(FPWRT_PWR1 & ALTI2C_ON);
_FICD	(ICS_PGD1 & JTAGEN_OFF);

// Function prototypes
void InitIO(void);		
void InitPWM(void);		
void CondPWM(void);	
void InitTimer(void);			
void ADC(void);						
void Compare(void);					

// Global variables
//unsigned int StepRate = 0;      //For Timer3's Interrupt delay value. REV OTONIEL.
unsigned int LineLeft = 0;      //For ADC value from the left line tracking sensor
unsigned int LineRight = 0;     //For ADC value from the right line tracking sensor	
unsigned int StepStatusR = 0;	//Whether right stepper motor needs to step during next Timer3 Interrupt
unsigned int StepStatusL = 0;	//Whether left stepper motor needs to step during next Timer3 Interrupt'
unsigned int PWM1 = 65535;	//Initialize PWM1, used to store value of PWM Signal to motor 1 			
unsigned int PWM2 = 65535;	//Initialize PWM2, used to store value of PWM Signal to motor 2
                            //Note the value of 65535 represents 100% Duty Cycle = motor off 	

int main (void)
{
	InitIO();		//Call InitIO
	InitTimer();	//Call InitTimer
    InitPWM();      //Call InitPWM function

	while (1)		//Infinite Loop
	{
		ADC();		//Call ADC 
        CondPWM     // Calls the function to adjust the PWM signals. REV OTONIEL
		Compare();	//Call Compare, this is used to decide which stepper motors to step for the next
                    //Timer3 Interrupt
	}
}
/*********************************************************************************************************/
void InitIO(void)				
{
	//TRISAbits.TRISA0 = 1;   //Set RA0 as input (Variable Resistor connected to AN0). REV. OTONIEL
	TRISBbits.TRISB2 = 1;	//Set RB2 as input (Left Side Phototransistor of Line Tracking Sensor)
	TRISBbits.TRISB3 = 1;	//Set RB3 as input (Right Side Phototransistor of Line Tracking Sensor)
	
	TRISAbits.TRISA2 = 0;	//Set RA2 as output (Step Pin - Right Stepper Motor)
	TRISAbits.TRISA3 = 0;	//Set RA3 as output (Direction Pin - Right Stepper Motor)
	TRISBbits.TRISB4 = 0;	//Set RB4 as output (Step Pin - Left Stepper Motor)
	TRISAbits.TRISA4 = 0;	//Set RA4 as output (Direction Pin - Left Stepper Motor)
	 
	//LATAbits.LATA3 = 0;     //Set direction for right stepper motor to be CW (RA3). REV OTONIEL
	//LATAbits.LATA4 = 1;     //Set direction for left stepper motor to be CCW (RA4). REV OTONIEL
}
/*********************************************************************************************************/
//For more information on PIC24H Timers Peripheral Module Library refer to link below:
//file:///C:/Program%20Files%20(x86)/Microchip/xc16/v1.26/docs/periph_libs/dsPIC30F_dsPIC33F_PIC24H_dsPIC33E_PIC24E_Timers_Help.htm
void InitTimer(void)
{                                                   //Prescaler = 1:8 and Period = 0xFFFF
	OpenTimer3(T3_ON & T3_IDLE_STOP & T3_GATE_OFF & T3_SOURCE_INT & T3_PS_1_8 , 0xFFFF);
								
	ConfigIntTimer3(T3_INT_PRIOR_2 & T3_INT_ON);    //Turn Timer3 interrupt on
}
/*********************************************************************************************************/
//For more information on PIC24H ADC Peripheral Module Library refer to link below:
//file:///C:/Program%20Files%20%28x86%29/Microchip/xc16/v1.26/docs/periph_libs/dsPIC33F_PIC24H_dsPIC33E_PIC24E_ADC_Library_Help.htm
//********************************************************************************************************/
void ADC(void)
{                                  
  /*  //12-bit sampling 
                                    //Use dedicated ADC RC oscillator
                                    //Automatically start new conversion after previous
                                    //Use Avdd and Avss as reference levels
	OpenADC1( ADC_MODULE_OFF & ADC_AD12B_12BIT & ADC_FORMAT_INTG & ADC_CLK_AUTO & ADC_AUTO_SAMPLING_ON,
			  ADC_VREF_AVDD_AVSS & ADC_SCAN_OFF & ADC_ALT_INPUT_OFF,
			  ADC_SAMPLE_TIME_31 & ADC_CONV_CLK_INTERNAL_RC,
			  ADC_DMA_BUF_LOC_1,
			  ENABLE_AN0_ANA,
			  0,
			  0,
			  0 );
                                    //Select AN0
	SetChanADC1(0, ADC_CH0_NEG_SAMPLEA_VREFN & ADC_CH0_POS_SAMPLEA_AN0);
	AD1CON1bits.ADON = 1;           //Turn on ADC hardware module
	while (AD1CON1bits.DONE == 0);  //Wait until conversion is done
	StepRate = ReadADC1(0)*16;      //StepRate = converted results x16 to condition for full resolution
	AD1CON1bits.ADON = 0;           //Turn off ADC hardware module */ // REV. OTOLNIEL 
					
	OpenADC1( ADC_MODULE_OFF & ADC_AD12B_12BIT & ADC_FORMAT_INTG & ADC_CLK_AUTO & ADC_AUTO_SAMPLING_ON,
			  ADC_VREF_AVDD_AVSS & ADC_SCAN_OFF & ADC_ALT_INPUT_OFF,
			  ADC_SAMPLE_TIME_31 & ADC_CONV_CLK_INTERNAL_RC,
			  ADC_DMA_BUF_LOC_1,
			  ENABLE_AN4_ANA,
			  0,
			  0,
			  0 );
                                    //Select AN4
	SetChanADC1(0, ADC_CH0_NEG_SAMPLEA_VREFN & ADC_CH0_POS_SAMPLEA_AN4);
	AD1CON1bits.ADON = 1;       	//Turn on ADC hardware module
	while (AD1CON1bits.DONE == 0);  //Wait until conversion is done
	PWM1 = ReadADC1(0);     	//Read conversion results and store in LineLeft
	AD1CON1bits.ADON = 0;           //Turn off ADC hardware module
							
	OpenADC1( ADC_MODULE_OFF & ADC_AD12B_12BIT & ADC_FORMAT_INTG & ADC_CLK_AUTO & ADC_AUTO_SAMPLING_ON,
			  ADC_VREF_AVDD_AVSS & ADC_SCAN_OFF & ADC_ALT_INPUT_OFF,
			  ADC_SAMPLE_TIME_31 & ADC_CONV_CLK_INTERNAL_RC,
			  ADC_DMA_BUF_LOC_1,
			  ENABLE_AN5_ANA,
			  0,
			  0,
			  0 );
                                    //Select AN5
	SetChanADC1(0, ADC_CH0_NEG_SAMPLEA_VREFN & ADC_CH0_POS_SAMPLEA_AN5);
	AD1CON1bits.ADON = 1;           //Turn on ADC hardware module
	while (AD1CON1bits.DONE == 0);	//Wait until conversion is done
	PWM2 = ReadADC1(0);        //Read conversion results and store in LineRight
	AD1CON1bits.ADON = 0;           //Turn off ADC hardware module
}
/*********************************************************************************************************/
void Compare(void)
{

	if (LineLeft > 2000)	//Checks which stepper motor to step
	{									
        //Velocity of the left 75 and the right 25
		//StepStatusR = 1;        //Set variable to indicate to step right stepper motor. REV OTONIEL
	}									
	else if (LineRight < 2000)				
	{
        //Velocity of the left 25 and the right 75
		//StepStatusL = 1;        //Set variable to indicate to step left stepper motor. REV OTONIEL
	}
    
    else
        
    {
        // Set both velocities to be the same.  
        //Set variable to indicate to step both DC motors. REV OTONIEL
    }
}
/*********************************************************************************************************/
//For more information on PIC24H Interrupts, see the MPLAB® XC16 C Compiler User's Guide - Chapter 14. Interrupts
//
//For more information on PIC24H Timers Peripheral Module Library refer to link below:
//file:///C:/Program%20Files%20(x86)/Microchip/xc16/v1.26/docs/periph_libs/dsPIC30F_dsPIC33F_PIC24H_dsPIC33E_PIC24E_Timers_Help.htm
/*********************************************************************************************************/
void __attribute__((interrupt, auto_psv)) _T3Interrupt(void)
{
	DisableIntT3;               //Disable Timer3 Interrupt
	if(StepStatusR == 1)				
	{							
		LATAbits.LATA2 = 1 ^ PORTAbits.RA2; //Toggle signal to step right stepper motor
		StepStatusR = 0;                    //"^" indicates Exclusive OR function (XOR)
	}
	if (StepStatusL == 1)		
	{
		LATBbits.LATB4 = 1 ^ PORTBbits.RB4; //Toggle signal to step left stepper motor
		StepStatusL = 0;                    //"^" indicates Exclusive OR function (XOR)
	}
	WriteTimer3(StepRate);      //Setup Timer3 with the value of StepRate for the next T3 interrupt
	IFS0bits.T3IF = 0;          //Reset Timer3 interrupt flag
	EnableIntT3;                //Enable Timer3 interrupt
}
/*********************************************************************************************************/

//REV OTONIEL 
void InitPWM(void)
{
	DisableIntT2;		//Disable Timer2 Interrupt
	DisableIntOC1;		//Disable OC1 Interrupt
	DisableIntOC2;		//Disable OC2 Interrupt
                        //Timer2 is the clock source for OC1 and OC2
                        //Configure PWM mode for OC1 and OC2
	OpenOC1(OC_IDLE_CON & OC_TIMER2_SRC & OC_PWM_FAULT_PIN_DISABLE, 1, 1);
	OpenOC2(OC_IDLE_CON & OC_TIMER2_SRC & OC_PWM_FAULT_PIN_DISABLE, 1, 1);							
                        //Prescaler = 1:1 and Period = 0xFFFF
	OpenTimer2(T2_ON & T2_IDLE_CON & T2_GATE_OFF & T2_PS_1_1 & T2_32BIT_MODE_OFF & T2_SOURCE_INT, 0xFFFF);
}

//REV OTONIEL
void CondPWM(void)
{										
	PWM1 = ~(PWM1 *16);	//Invert, multiply by 16 to scale a 12 bit value to 16 bit memory location
	PWM2 = ~(PWM2 *16);	//Invert, multiply by 16 to scale a 12 bit value to 16 bit memory location
}

//REV OTONIEL    
void Drive(void)
{	
	SetDCOC1PWM(PWM1);	//Set duty cycle PWM1
	SetDCOC2PWM(PWM2);	//Set duty cycle PWM2
}


