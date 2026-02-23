/**
 * @file infrared.h
 * @brief IR transmitter + receiver interface (STM8S103F3P).
 *
 * Provides configuration macros, shared globals, and APIs for:
 * - IR Transmitter (38 kHz carrier gating + frame send)
 * - IR Receiver (edge capture + frame process + UART reporting)
 *
 * @author MehulSaini
 * @date 2026-03-23
 */
 
#ifndef INFRARED_H
#define INFRARED_H

#include "stm8s.h"
#include "stm8s_clk.h"
#include "stm8s_tim2.h"
#include "stm8s_tim4.h"
#include "stm8s_gpio.h"
#include "stm8s_exti.h"
#include "stm8s_uart1.h"

/* ===================== GPIO CONFIG ===================== */

/** @brief Transmitter trigger button GPIO port. */
#define BUTTON_PORT 		GPIOC

/** @brief Transmitter trigger button GPIO pin. */
#define BUTTON_PIN			GPIO_PIN_3

/** @brief Status LED GPIO port. */
#define LED_PORT				GPIOD

/** @brief Status LED GPIO pin. */
#define LED_PIN					GPIO_PIN_3

/** @brief IR receiver input GPIO port. */
#define IR_PORT					GPIOA

/** @brief IR receiver input GPIO pin. */
#define IR_PIN					GPIO_PIN_2

/** @brief UART1 TX GPIO port. */
#define UART_TX_PORT		GPIOD

/** @brief UART1 TX GPIO pin. */
#define UART_TX_PIN			GPIO_PIN_5

/* ===================== IR TIMINGS (microseconds) ===================== */

/** @brief Header mark duration (carrier ON). */
#define HDR_MARK_US    (9000u)

/** @brief Header space duration (carrier OFF). */
#define HDR_SPACE_US   (4500u)

/** @brief Bit mark duration (carrier ON). */
#define BIT_MARK_US    (560u)

/** @brief Logic '0' space duration (carrier OFF). */
#define ZERO_SPACE_US     (560u)

/** @brief Logic '1' space duration (carrier OFF). */
#define ONE_SPACE_US     (1690u)

/** @brief End-of-frame mark duration (carrier ON). */
#define END_MARK_US    (560u)

/* ===================== EXPORTED DATA ===================== */

/** @brief Example payload defined in (infrared.c). */
extern const uint8_t msg;

/** @brief Captured pulse-width samples (receiver side). */
extern uint16_t received[20];

/** @brief Current receiver sample index. */
extern uint8_t index;

/** @brief Receiver frame-complete flag. */
extern uint8_t received_flag;

/* ===================== TRANSMITTER API ===================== */

/**
 * @brief Initializes clocks required for IR transmission.
 * @retval None
 */
void CLK_Transmitter_Init(void);

/**
 * @brief Initializes TIM2 for ~38 kHz PWM carrier generation (TX).
 * @retval None
 */
void TIM2_Transmitter_Init(void);

/**
 * @brief Initializes TIM4 for microsecond delays (TX).
 * @retval None
 */
void TIM4_Transmitter_Init(void);

/**
 * @brief Initializes trigger input (button + EXTI) for transmission.
 * @retval None
 */
void Transmitter_Trigger_Init(void);

/**
 * @brief Enables global interrupts.
 * @retval None
 */
void Enable_Global_Interrupts(void);

/**
 * @brief Busy-wait delay in microseconds.
 * @param us Delay duration in microseconds.
 * @retval None
 */
void Delay_us(uint16_t us);

/**
 * @brief Sends one full IR frame (header + payload + end mark).
 * @param data Pointer to payload.
 * @retval None
 */
void Signal_Send_Frame(const uint8_t * data);

/* ===================== RECEIVER API ===================== */

/**
 * @brief Initializes clocks required for IR reception.
 * @retval None
 */
void CLK_Receiver_Init(void);

/**
 * @brief Initializes TIM2 for timing/capture used by IR receiver.
 * @retval None
 */
void TIM2_Receiver_Init(void);

/**
 * @brief Initializes IR input trigger (EXTI) for edge capture.
 * @retval None
 */
void Receiver_Trigger_Init(void);

/**
 * @brief Initializes UART1 TX for receiver debug output.
 * @retval None
 */
void UART1TX_Receiver_Init(void);

/**
 * @brief Captures one IR frame into a pulse-width buffer.
 *
 * @param receive Pointer to pulse-width array to fill (microseconds/ticks).
 * @param idx Pointer to sample index variable.
 * @param rec_flg Pointer to frame-complete flag variable.
 *
 * @retval None
 */
void Signal_Receive_Frame(uint16_t * receive, uint8_t * idx, uint8_t * rec_flg);

/**
 * @brief Decodes captured frame and transmits decoded info via UART1.
 *
 * @param receive Pointer to pulse-width capture buffer.
 * @param idx Pointer to valid sample count/index.
 * @param rec_flg Pointer to frame-complete flag.
 *
 * @retval None
 */
void Decode_Signal_And_Transmit_Via_UART(uint16_t * receive,  uint8_t * idx, uint8_t * rec_flg);

/**
 * @brief Processes a captured IR frame and returns the decoded byte.
 *
 * Validates header timing and decodes subsequent mark/space timings
 * into a single 8-bit value.
 *
 * @param receive Pointer to captured pulse-width buffer.
 *
 * @retval 0x00 Frame invalid or decoding failed.
 * @retval other Decoded 8-bit frame value.
 */
uint8_t Signal_Process_Frame(uint16_t * receive);

#endif /* INFRARED_H */