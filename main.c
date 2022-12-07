#include <stdint.h> // C99 data types
#include "TM4C123GH6PM.h"
// "tm4c123gh6pm.h"

// Function Prototypes (from startup.s)
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void); // Enable interrupts
void WaitForInterrupt(void); // Go to low power mode while waiting for the next interrupt

// function declarations
void GPIOPortInit(void); 
void GPIOPortF_InterInit(void);
void GPIOPortF_Handler(void);
void SysTick_Handler(void);
void SysTick_Init(unsigned long period); // similiar to project 2 (in groups of 2ms)
void Delay_ms(int n);

// Port F0 and Port F4 (0x40025000 + 0x0004 + 0x0040)
#define BUTTONS (*((volatile uint32_t *) 0x40025044))
// Port B0 through Port B7
#define STEPPERS (*((volatile uint32_t *) 0x400053FC))

// global variables
uint32_t currentState;
uint32_t stepsRemaining;
uint32_t IR_Input;
// own testing purposes
uint8_t num; 
uint8_t arr[4] = {0x03, 0x06, 0x0C, 0x09};

struct State {
uint32_t Out;
uint32_t Delay; // controlled by systick interrupt (1 = 0.5s delay)
const uint32_t Next[8];
};
typedef const struct State STyp;
// full wave stepping
#define stepZero 0
#define stepOne 1
#define	stepTwo 2
#define stepThree 3
STyp FSM[4] = {
	// 					stop			 stop				stop 				stop			stop			 reverse		forward		 stop					 // directions
	{0x93, 1 , {stepZero,  stepZero,  stepZero,  stepZero,  stepZero,  stepThree, stepOne,   stepZero}},   // stepZero
	{0xC6, 1 , {stepOne,   stepOne,   stepOne,   stepOne,   stepOne,   stepZero,  stepTwo,   stepOne}},    // stepOne 
	{0x6C, 1 , {stepTwo,   stepTwo,   stepTwo,   stepTwo,   stepTwo,   stepOne,   stepThree, stepTwo}},    // stepTwo
	{0x39, 1 , {stepThree, stepThree, stepThree, stepThree, stepThree, stepTwo,   stepZero,  stepThree}}   // stepThree
};

int main(void) 
{
// Initialize Inputs, Outputs, SysTick, and Interrupts
	GPIOPortInit(); 
//	DisableInterrupts();
//	GPIOPortF_InterInit(); 

//	IR_Input = 0x04; -> set the move index of the IR_input
//	currentState = stepZero; // set to initial state
//	SysTick_Init(FSM[currentState].Delay); 	
//	stepsRemaining = 0; 
	
	// 90 degree turn
	for(num = 0; num < 125; num = num + 1)
		{
			for(IR_Input = 0; IR_Input < 4; IR_Input = IR_Input + 1)
				{
						STEPPERS = arr[IR_Input];
						Delay_ms(2);
				}
		}
				
	while(1)
		{
//			WaitForInterrupt();
		} 
} 

void GPIOPortInit(void)
{
	volatile unsigned long delay; 
	SYSCTL_RCGC2_R |= 0x00000022; // enable F and B clk
	delay = SYSCTL_RCGC2_R; // no operation delay
	
	// Initialize Port F
	GPIO_PORTF_LOCK_R = 0x4C4F434B;
	GPIO_PORTF_CR_R = 0x1F; 
	GPIO_PORTF_AMSEL_R = 0x00; 
	GPIO_PORTF_PCTL_R = 0x00000000; 
	GPIO_PORTF_DIR_R = 0x0E; // PF4, PF0 -> input    PF3, PF2, PF1 -> output
	GPIO_PORTF_AFSEL_R = 0x00; 
	GPIO_PORTF_PUR_R = 0x11; // enable pullup resistor (negative logic)
	GPIO_PORTF_DEN_R = 0x1F; 
	
	// Initialize Port B
	GPIO_PORTB_AMSEL_R &= ~0xFF; 
	GPIO_PORTB_PCTL_R &= ~0xFF; // select as GPIO port
	GPIO_PORTB_DIR_R |= 0xFF; // set 1 for OUTPUT
	GPIO_PORTB_AFSEL_R &= ~0xFF; // clear to select regular I/O
	GPIO_PORTB_PUR_R &= ~0xFF; // no pull up resistor
	GPIO_PORTB_PDR_R &= ~0xFF; // no pull down resistor
	GPIO_PORTB_ODR_R &= ~0xFF; // no open drain
	GPIO_PORTB_DEN_R |= 0xFF; // enable digital pins
}

/* THIS IS ASSUMING WE USE ON BOARD BTNS FOR THIS PROJECT */
// Initialize GPIO Port F interrupt
void GPIOPortF_InterInit(void)
{
	// 5 conditions must be TRUE simultaneously for the edge-trigger interrupt to be requested
	GPIO_PORTF_IS_R |= 0x11; 						// level-sensitive
	GPIO_PORTF_IBE_R &= ~0x11;					// not both edges
	GPIO_PORTF_IEV_R &= ~0x11;					// low level event 
	GPIO_PORTF_ICR_R = 0x11; 						// clear flag
	GPIO_PORTF_IM_R |= 0x11; 						// arm interrupt on PF4 is set

	NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF) | 0x00200000; // set priority 1 (0010)
	NVIC_EN0_R = 0x40000000; 						// enable interrupt on 30 in NVIC
	EnableInterrupts();
}
// Handle GPIO Port F interrupts. When Port F interrupt triggers, 
// do what's necessary and update IR_Input bits 0 and 1
void GPIOPortF_Handler(void) {
	GPIO_PORTF_ICR_R = 0X11; // clear interrupt
	
	if (BUTTONS & 0x10) // PF0 pressed -> FORWARD
		{
			stepsRemaining = 16; // 180 degree turn
			IR_Input &= ~0x01; // 1111 1110
			IR_Input |= 0x02; // 0000 0010 -> set bit 1
		} 
	else if (BUTTONS & 0x01) // PF4 pressed -> REVERSE
		{
			stepsRemaining = 8; // 90 degree turn
			IR_Input &= ~0x02; // 1111 1101
			IR_Input |= 0x01; // 0000 0001 -> set bit 0
		} 
	else // keep move index 1
		{
			stepsRemaining = 0; // no movement
			IR_Input &= ~0x03; // 1111 1100
		} 
}


// SysTick Interrupt w/ Priority 3 to implement state delays
// 62.5 ns per decrement -> 2ms / 62.5 ns -> 32,000 
// flag will be set when counter goes from 31,999 to 0
// after 2ms, the systick timer set the count flag bit of STCTRL register which 
// will send the interrupt signal to nested interrupt vector controller. 
void SysTick_Init(unsigned long period)
{
	NVIC_ST_CTRL_R = 0; // disable SysTick during setup
	NVIC_ST_RELOAD_R = period * (31999 - 1); // reload value
	NVIC_ST_CURRENT_R = 0; // any write to current clears it
	// priority 3
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0X00FFFFFF) | 0x60000000; // 0110 
	// enable SysTick with core clock and interrupts
	NVIC_ST_CTRL_R = 0x07; 
	EnableInterrupts();
}

// Handle SysTick generated interrupts. When timer interrupt triggers, 
//	do what's necessary, update state and outputs
void SysTick_Handler() 
{
	if (stepsRemaining > 0) 
		{
			stepsRemaining--; 
		} 
	// Update IR_Input bit 3
	if (stepsRemaining > 0) 
		{
			IR_Input |= 0x04;// 0000 0100 -> continue moving
		} 
	else 
		{
			IR_Input &= ~0x04;// 1111 1011 -> stop moving
		} 
		currentState = FSM[currentState].Next[IR_Input]; // next input
		STEPPERS = FSM[currentState].Out; // set stpper motor output
		
		// Reload SysTick timer with new value
		SysTick_Init(2); // 4 ms delay
}


/* Generates a delay in number of miliseocnds with system clock of 16MHz */
void Delay_ms(int n)
{
    int a, b;
    for(a = 0 ; a < n; a++)
        for(b = 0; b < 3180; b++)
            {} /* execute NOP for one milisecond */
}
