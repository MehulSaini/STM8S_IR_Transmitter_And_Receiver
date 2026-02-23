/**
 * @file main.c
 * @brief IR receiver main and EXTI handler (STM8S).
 *
 * Captures IR edges using EXTI on IR input pin, stores timing values,
 * decodes a frame when complete, and transmits decoded byte via UART1.
 *
 * @author MehulSaini
 * @date 2026-02-23
 */
 
#include "infrared.h"

/**
 * @brief EXTI interrupt handler for GPIO Port A (IR input).
 *
 * Called on rising/falling edges of the IR demodulator output pin.
 * Captures timing samples into the receiver buffer.
 *
 * @retval None
 */
@far @interrupt void EXTI_PORTA_IRQHandler(void){
	Signal_Receive_Frame(received, &index, &received_flag);
}

/**
 * @brief Main entry point for IR receiver.
 *
 * Initializes receiver peripherals (clock, TIM2 timing, EXTI trigger, UART1 TX),
 * enables global interrupts, then continuously checks for a complete frame
 * to decode and transmit.
 *
 * @retval int Always returns 0 (not reached in normal operation).
 */
int main(void){
	CLK_Receiver_Init();
	TIM2_Receiver_Init();
	Receiver_Trigger_Init();
	UART1TX_Receiver_Init();
	Enable_Global_Interrupts();
	while (1){
		Decode_Signal_And_Transmit_Via_UART(received, &index, &received_flag);
	}
	return 0;
}
