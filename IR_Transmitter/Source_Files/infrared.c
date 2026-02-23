/**
 * @file infrared.c
 * @brief IR transmitter and receiver implementation (STM8S).
 *
 * Transmitter:
 * - TIM2 generates ~38 kHz PWM carrier and is gated to create marks/spaces.
 * - TIM4 provides microsecond delays.
 *
 * Receiver:
 * - TIM2 runs as a ~1 MHz free-running counter for edge timing.
 * - EXTI captures rising/falling edges and stores durations into a buffer.
 * - Decoded frame is transmitted via UART1 TX.
 *
 * @author MehulSaini
 * @date 2026-03-23
 */
 
#include "infrared.h"

/* ===================== SHARED GLOBALS ===================== */

/**
 * @brief Example transmit payload byte.
 *
 * Used by transmitter to send one byte frame.
 */
const uint8_t msg = 0xAA;	// example value, change accordingly

/** @brief Receiver buffer index (edge sample index). */
uint8_t index = 0;

/** @brief Receiver edge timing buffer (microseconds/ticks depending on TIM2 config). */
uint16_t received[20];

/** @brief Receiver frame complete flag (1 when buffer is full). */
uint8_t received_flag = 0;

/* ===================== TRANSMITTER ===================== */

/**
 * @brief Initializes clock system for transmitter.
 *
 * Enables HSI and peripheral clocks for TIM2 (carrier PWM) and TIM4 (delay).
 *
 * @retval None
 */
void CLK_Transmitter_Init(void){
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	CLK_HSICmd(ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, ENABLE);
}

/**
 * @brief Initializes TIM2 to generate ~38 kHz PWM carrier on CH1.
 *
 * PWM output is later gated ON/OFF to generate NEC marks/spaces.
 *
 * @note TIM2 CH1 is on PD4 on STM8S (connect IR LED with proper driver/resistor).
 * @retval None
 */
void TIM2_Transmitter_Init(void){
	TIM2_TimeBaseInit(TIM2_PRESCALER_1, 420);	/* ~38KHz */
	TIM2_OC1Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, 210, TIM2_OCPOLARITY_HIGH);	/* 50% duty cycle */
	TIM2_ARRPreloadConfig(ENABLE);
	TIM2_OC1PreloadConfig(ENABLE);
	TIM2_Cmd(ENABLE);
}

/**
 * @brief Enables or disables the PWM carrier output (gating).
 *
 * @param en ENABLE to output carrier, DISABLE to stop carrier.
 * @retval None
 */
void TIM2_PWM_Switch(FunctionalState en){
	TIM2_CCxCmd(TIM2_CHANNEL_1, en);	//Used for PWM gating
}

/**
 * @brief Initializes TIM4 for microsecond delay timing.
 *
 * @retval None
 */
void TIM4_Transmitter_Init(void){
	TIM4_TimeBaseInit(TIM4_PRESCALER_16, 0xFF);	//using to create Delay_us()
	TIM4_Cmd(ENABLE);
}

/**
 * @brief Initializes transmitter trigger input (button + EXTI).
 *
 * Configures the button pin with pull-up and interrupt on falling edge.
 *
 * @retval None
 */
void Transmitter_Trigger_Init(void){
	GPIO_Init(BUTTON_PORT, BUTTON_PIN, GPIO_MODE_IN_PU_IT);
	EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_FALL_ONLY);
}

/**
 * @brief Enables global interrupts.
 *
 * @retval None
 */
void Enable_Global_Interrupts(void){
	_asm("rim");
}


/**
 * @brief Busy-wait delay in microseconds using TIM4 counter.
 *
 * @param us Delay duration in microseconds.
 * @retval None
 */
void Delay_us(uint16_t us){
	while(us > 255){
		TIM4_SetCounter(0);
		while(TIM4_GetCounter() < 255);
		us -= 255;
	}
	TIM4_SetCounter(0);
	while(TIM4_GetCounter() < us);
}

/**
 * @brief Sends one "mark + space" by gating the IR carrier.
 *
 * @param on_us  Mark duration (carrier ON) in microseconds.
 * @param off_us Space duration (carrier OFF) in microseconds.
 * @retval None
 */
void Signal_Mark_Space(uint16_t on_us, uint16_t off_us){
	TIM2_PWM_Switch(ENABLE);
	Delay_us(on_us);
	TIM2_PWM_Switch(DISABLE);
	Delay_us(off_us);
}

/**
 * @brief Sends one bit using configured mark/space timings.
 *
 * @param bit 0 sends ZERO timing, non-zero sends ONE timing.
 * @retval None
 */
void Signal_Send_Bit(uint8_t bit){
	if (bit){
		Signal_Mark_Space(BIT_MARK_US, ONE_SPACE_US);
	}
	else{
		Signal_Mark_Space(BIT_MARK_US, ZERO_SPACE_US);
	}
}

/**
 * @brief Sends one byte LSB-first.
 *
 * @param b Byte to send.
 * @retval None
 */

void Signal_Send_Byte(uint8_t b){
	int i = 0;
	for (i = 0; i < 8; i++){
		Signal_Send_Bit((uint8_t)(b & 0x01u));
		b >>= 1;
	}
}

/**
 * @brief Sends a complete frame (header + 1-byte payload + end mark).
 *
 * @param data Pointer to payload byte.
 * @retval None
 */
void Signal_Send_Frame(const uint8_t * data){
	Signal_Mark_Space(HDR_MARK_US, HDR_SPACE_US);
	Signal_Send_Byte(*data);
	Signal_Mark_Space(END_MARK_US, 0);
	Delay_us(20000);
}

/* ===================== RECEIVER ===================== */

/**
 * @brief Initializes clock system for receiver timing.
 *
 * Enables HSI and peripheral clock for TIM2.
 *
 * @retval None
 */
void CLK_Receiver_Init(void){
	CLK_HSICmd(ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
}

/**
 * @brief Initializes TIM2 for receiver timestamping (~1 MHz).
 *
 * TIM2 is used as a counter; edge timestamps are read using TIM2_GetCounter().
 *
 * @retval None
 */
void TIM2_Receiver_Init(void){
	TIM2_TimeBaseInit(TIM2_PRESCALER_16, 0xFFFF);	/* ~1MHz */
	TIM2_Cmd(ENABLE);
}

/**
 * @brief Initializes receiver GPIO/EXTI and a status LED.
 *
 * IR input pin is configured as input with interrupt on rising/falling edges.
 * LED pin is configured as output and set LOW initially.
 *
 * @retval None
 */
void Receiver_Trigger_Init(void){
	GPIO_Init(LED_PORT, LED_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
	GPIO_Init(IR_PORT, IR_PIN, GPIO_MODE_IN_PU_IT);
	GPIO_WriteLow(LED_PORT, LED_PIN);
	EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOA, EXTI_SENSITIVITY_RISE_FALL);
}

/**
 * @brief Initializes UART1 TX for decoded frame output.
 *
 * @retval None
 */
void UART1TX_Receiver_Init(void){
	UART1_DeInit();
	GPIO_Init(UART_TX_PORT, UART_TX_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);
	UART1_Init(9600, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO, UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TX_ENABLE);
	UART1_Cmd(ENABLE);
}

/**
 * @brief Captures edge timing values into the receive buffer.
 *
 * Intended to be called on every IR edge (EXTI ISR). Stores TIM2 counter values
 * into @p receive and sets @p rec_flg when the buffer is full.
 *
 * @param receive Pointer to receive timing buffer.
 * @param idx Pointer to current write index.
 * @param rec_flg Pointer to frame complete flag (set to 1 when full).
 *
 * @retval None
 */
void Signal_Receive_Frame(uint16_t * receive, uint8_t * idx, uint8_t * rec_flg){
	if(*idx == 0){
		receive[*idx] = 0;
	}
	else if(*idx > 0 && *idx < 19){
		receive[*idx] = TIM2_GetCounter();
	}
	else if(*idx == 19){
		receive[*idx] = TIM2_GetCounter();
		*rec_flg = 1;
	}
	*idx = *idx + 1;
	TIM2_SetCounter(0);
}

/**
 * @brief Processes a captured frame and returns decoded 1-byte value.
 *
 * Performs simple timing validation for header and bit spaces and decodes
 * bits into a single byte.
 *
 * @param receive Pointer to captured timing buffer.
 *
 * @retval 0x00 Decoding failed / invalid frame.
 * @retval other Decoded byte value.
 */
uint8_t Signal_Process_Frame(uint16_t * receive){
	uint8_t frame = 0x00;	//returns 0x00 if data is invalid
	uint8_t bit = 0;
	int i = 0;
	if((receive[1]/1000)!=9){
		return 0x00;
	}
	if((receive[2]/1000)!=4){
		return 0x00;;
	}
	for(i = 3; i < 18; i=i+2){
		if((receive[i]<1000) && (receive[i+1]<1000)){
			frame &= ~(1<<bit);
			bit++;
		}
		else if((receive[i]<1000) && (receive[i+1]>1000)){
			frame |= (1<<bit);
			bit++;
		}
		else{
			return 0x00;
		}
	}
	return frame;
}

/**
 * @brief Decodes captured frame and transmits result via UART1.
 *
 * When @p rec_flg is set, turns ON LED, decodes frame, sends one byte over UART,
 * then clears flag and resets index.
 *
 * @param receive Pointer to captured timing buffer.
 * @param idx Pointer to index variable (reset to 0 after decode).
 * @param rec_flg Pointer to frame complete flag (cleared after decode).
 *
 * @retval None
 */
void Decode_Signal_And_Transmit_Via_UART(uint16_t * receive,  uint8_t * idx, uint8_t * rec_flg){
	if(*rec_flg == 1){
		GPIO_WriteHigh(LED_PORT, LED_PIN);
		UART1_SendData8(Signal_Process_Frame(receive));
		*rec_flg = 0;
		*idx = 0;
		GPIO_WriteLow(LED_PORT, LED_PIN);
	}
}