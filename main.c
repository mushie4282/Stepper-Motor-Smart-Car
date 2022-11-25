#include <stdint.h> // C99 data types
#include "TM4C123GH6PM.h"
// "tm4c123gh6pm.h"

// Function Prototypes (from startup.s)
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void); // Enable interrupts
void WaitForInterrupt(void); // Go to low power mode while waiting for the next interrupt

// function declarations
void GPIOPortF_Handler(void);
void SysTick_Handler(void);

//#define BUTTONS (*((volatile uint32_t *) ...))
//#define STEPPERS (*((volatile uint32_t *) ...))

// global variables
uint32_t currentState;
uint32_t stepsRemaining;
uint32_t Input;

struct State {
uint32_t Out;
uint32_t Delay;
const uint32_t Next[8]; // 4 for each wheel
};
typedef const struct State STyp;
// full wave stepping
#define S0 0x03
#define S1 0x06
#define S3 0x0C
#define S4 0x09
//STyp FSM[4] = {
//{...,...,{S0, S0, S0, S0, S0, S4, S1, S0}}, // S0
//{...,...,{S1, S1, S1, S1, S1, S0, S3, S1}}, // S1
//{...,...,{S3, S3, S3, S3, S3, S1, S4, S3}}, // S3
//{...,...,{S4, S4, S4, S4, S4, S3, S0, S4}} // S4
//};

int main(void) {
// Initialize Inputs, Outputs, SysTick, and Interrupts
 Input = 0;
currentState = S0;
 while(1){
WaitForInterrupt();
 } 
} 

// Handle GPIO Port F interrupts. When Port F interrupt triggers, 
// do what's necessary and update Input bits 0 and 1
void GPIOPortF_Handler(void) {
	GPIO_PORTF_ICR_R = 0X11; // clear interrupt
	if (PF0 pressed) 
		{
			stepsRemaining = 16; // 180 degree turn
			Input &= ~0x01;
			Input |= 0x02;
		} 
	else if (PF4 pressed) 
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

// Handle SysTick generated interrupts. When timer interrupt triggers, 
//do what's necessary, update state and outputs
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

}
