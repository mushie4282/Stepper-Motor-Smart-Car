#include <stdint.h> // C99 data types
#include "TM4C123GH6PM.h"
// "tm4c123gh6pm.h"

// Function Prototypes (from startup.s)
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void); // Enable interrupts
void WaitForInterrupt(void); // Go to low power mode while waiting for the next interrupt

// function declarations
void GPIOPortInit(void); 
void GPIOPortF_Handler(void);
void SysTick_Handler(void);
void Delay_ms(int n);

// Port F0 and Port F4 (0x40025000 + 0x0004 + 0x0040)
#define BUTTONS (*((volatile uint32_t *) 0x40025044))
// Port B0 through Port B3 (0x40005000 + 0x0004 + 0x0008 + 0x0010 + 0x0020) = 0x4000503C
#define STEPPERS (*((volatile uint32_t *) 0x4000503C))

// global variables
uint32_t currentState;
uint32_t stepsRemaining;
uint32_t Input;

struct State {
uint32_t Out;
uint32_t Delay; // controlled by systick interrupt (1 = 0.5s delay)
const uint32_t Next[8]; // 4 for each wheel
};
typedef const struct State STyp;
// full wave stepping
#define S0 0
#define S1 1
#define S3 2
#define S4 3
//STyp FSM[4] = {
//{0x03, 1 ,{S0, S0, S0, S0, S0, S4, S1, S0}}, // S0
//{0x06, 1 ,{S1, S1, S1, S1, S1, S0, S3, S1}}, // S1
//{0x0C, 1 ,{S3, S3, S3, S3, S3, S1, S4, S3}}, // S3
//{0x09, 1 ,{S4, S4, S4, S4, S4, S3, S0, S4}} // S4
//};

int main(void) 
{
// Initialize Inputs, Outputs, SysTick, and Interrupts
	Input = 0;
	currentState = S0;
	GPIOPortInit(); 
	
	while(1)
		{
			//WaitForInterrupt();
			// below code should continuously rotate your stepper motor (clockwise)
			STEPPERS = 0x03; 
			Delay_ms(4); 
			STEPPERS = 0x06; 
			Delay_ms(4);
			STEPPERS = 0x0C; 
			Delay_ms(4); 
			STEPPERS = 0x09; 
			Delay_ms(4);
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
	GPIO_PORTB_DIR_R |= 0x0F; // set 1 for OUTPUT
	GPIO_PORTB_AFSEL_R &= ~0xFF; // clear to select regular I/O
	GPIO_PORTB_PUR_R &= ~0xFF; // no pull up resistor
	GPIO_PORTB_PDR_R &= ~0xFF; // no pull down resistor
	GPIO_PORTB_ODR_R &= ~0xFF; // no open drain
	GPIO_PORTB_DEN_R |= 0x0F; // enable digital pins
}

/* THIS IS ASSUMING WE USE ON BOARD BTNS FOR THIS PROJECT */
// Initialize GPIO Port F interrupt
void GPIOPortF_InterInit(void)
{
	// 5 conditions must be TRUE simultaneously for the edge-trigger interrupt to be requested
	GPIO_PORTF_IS_R |= 0x10; 						// level-sensitive
	GPIO_PORTF_IBE_R &= ~0x10;					// not both edges
	GPIO_PORTF_IEV_R &= ~0x10;					// low level event 
	GPIO_PORTF_ICR_R = 0x10; 						// clear flag
	GPIO_PORTF_IM_R |= 0x10; 						// arm interrupt on PF4 is set

	NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF) | 0x00200000; // set priority 1 (0010)
	NVIC_EN0_R = 0x40000000; 						// enable interrupt on 30 in NVIC
	EnableInterrupts();
}
// Handle GPIO Port F interrupts. When Port F interrupt triggers, 
// do what's necessary and update Input bits 0 and 1
void GPIOPortF_Handler(void) {
	GPIO_PORTF_ICR_R = 0X11; // clear interrupt
	if (BUTTONS & 0x10) // PF0 pressed
		{
			stepsRemaining = 16; // 180 degree turn
			Input &= ~0x01;
			Input |= 0x02;
		} 
	else if (BUTTONS & 0x01) // PF4 pressed
		{
			stepsRemaining = 8; // 90 degree turn
			Input &= ~0x02;
			Input |= 0x01;
		} 
	else 
		{
			stepsRemaining = 0;
			Input &= ~0x03;
		} 
} 


// SysTick Interrupt w/ Priority 3 to implement state delays
void SysTick_Init(unsigned long period)
{
	NVIC_ST_CTRL_R = 0; // disable SysTick during setup
	NVIC_ST_RELOAD_R = period * (8000000 - 1); // reload value
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
	// Update Input bit 3
	if (stepsRemaining > 0) 
		{
			Input |= 0x04;
		} 
	else 
		{
			Input &= ~0x04;
		} 
//		currentState = FSM[currentState].Next[Input];
//		STEPPERS = FSM[currentState].Out; // set stpper motor output
		
		// Reload SysTick timer with new value
		SysTick_Init(8000000); // dummy value in place 
}


/* Generates a delay in number of miliseocnds wit system clock of 16MHz */
void Delay_ms(int n)
{
    int a, b;
    for(a = 0 ; a < n; a++)
        for(b = 0; b < 3180; b++)
            {} /* execute NOP for one milisecond */
}
