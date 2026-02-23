/**
 * @file main.c
 * @brief IR transmitter main and EXTI handler (STM8S).
 *
 * On button press (EXTI Port C), transmits a single IR frame
 * using NEC-style timings and 38 kHz carrier.
 */
#include "infrared.h"

/**
 * @brief EXTI interrupt handler for GPIO Port C (button trigger).
 *
 * Triggered on falling edge of button input.
 * Sends one IR frame containing the predefined payload byte.
 *
 * @retval None
 */
@far @interrupt void EXTI_PORTC_IRQHandler(void){
	Signal_Send_Frame(&msg);
}

/**
 * @brief Main entry point for IR transmitter.
 *
 * Initializes clock, PWM carrier (TIM2), delay timer (TIM4),
 * button EXTI trigger, and enables global interrupts.
 *
 * Transmission is interrupt-driven (button press).
 *
 * @retval int Always returns 0 (not reached).
 */
int main(void){
	CLK_Transmitter_Init();
	TIM2_Transmitter_Init();
	TIM4_Transmitter_Init();
	Transmitter_Trigger_Init();
	Enable_Global_Interrupts();
	while (1){
	}
	return 0;
}
