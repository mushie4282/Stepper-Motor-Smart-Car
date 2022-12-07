<h1> CECS 346 SPRING PROJECT 3 </h1>
Final Project - Smart Car  
Materials  
- TM4C LaunchPad
- Car with 2 stepper motor driven wheels
- Power supply circuit
- IR avoidance sensor  
This project requires us to combine most of the concepts learned throughout the semester, including: <br>
1. interfacing an IR avoidance sensor
2. interfacing stepper motors using wave or full driving
3. using a Moorse Finite State Machine
4. using interrupts (GPIO & SysTick)

<h2> Summary: </h2>
The car will have two options, race or stage. Stagging will make the car reverse to get ready to race. While, race will make the car move forward for a maximum of 2 minutes if there is no objects in front of the car. If the IR sensor detects anything 2-3 inches away from the car, the car will stop moving. 
