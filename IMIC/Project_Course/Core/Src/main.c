/**
 ********************************************************************
 * @file 	main.c
 * @author 	Embeddat (embeddat.dev@gmail.com)
 * @brief 
 * @version 0.1
 * @date 	<2025-10-06>
 ******************************************************************** 
 * @copyright Copyright (c) 2025
 * @attention 
 * 
 * This file is main part of the Final Project for Embedded Systems Course. 
 * Hardware: STM32F411, MAX SYSCLK = 100 MHz , Setup CLOCK = 16 MHz (HSI)
 * 
 */

#include <main.h>

/**
 * @brief 	Initialize register related to GPIO for LED 
 * @retval 	None
 * @note 	LED on board STM32F411VET6: PD12(🟢), PD13(🟠), PD14(🔴), PD15(🔵) 
 * 		 	are connect to GND
 * @pre		GPIOD Clock must be enabled
 */
#define GPIOD_BASE_ADDR 0x40020C00 
void LEDS_INIT(){
	/// 1. Initialize CLOCK for GPIOD
	__HAL_RCC_GPIOD_CLK_ENABLE();
	/// 2. Configure GPIOD_MODER register 
	uint32_t* GPIOD_MODER = GPIOD_BASE_ADDR + 0x00;
		// Reset MODER register for PD12, 13, 14, 15
	*GPIOD_MODER &= ~(0xFF << 24); // 0xFF = 0x1111 1111

		// SET [01] cho MODE: OUTPUT
	*GPIOD_MODER |= (0b01 << 24); // LED GREEN bits [24:25]
	*GPIOD_MODER |= (0b01 << 26); // LED ORANGE ...
	*GPIOD_MODER |= (0b01 << 28); // LED RED ...
	*GPIOD_MODER |= (0b01 << 30); // LED BLUE ...
}

/**
 * @brief Digital Transformer name of LED (Gr,Or,Re,Bl) = [0:3] 
 * @see LEDS_CONTROL()
 */
typedef enum{
	LED_GREEN, 	/**< = 0 */
	LED_ORANGE, /**< = 1 */
	LED_RED,	/**< = 2 */
	LED_BLUE	/**< = 3 */
}led_t;

/**
 * @brief Digital Transformer State for LED (On = 1 & Off = 0)  
 * @see LEDS_CONTROL()
 */
typedef enum{
	OFF_LED,	/**< = 0 */
	ON_LED		/**< = 1 */
}led_state_t;

/**
 * @brief Control Led ON, OFF
 * @param led : Digital name of LED (0 -> 3)  
 * @param state : Digital state of LED (0 & 1)
 * @see led_t, led_state_t 
 */
void LEDS_CONTROL(led_t led, led_state_t state){
	uint32_t *GPIOD_ODR = GPIOD_BASE_ADDR + 0x14;
	if (state == ON_LED)
		*GPIOD_ODR |= (0b1 << (12 + led));
	else
		*GPIOD_ODR &= ~(0b1 << (12 + led));
}

/**
 * @brief 	Test blink full Led with T = 200ms
 * @see		LEDS_CONTROL(), HAL_Delay() 
 */
void BLINK_FULL_LEDS(){
	LEDS_CONTROL(LED_GREEN, ON_LED);
	LEDS_CONTROL(LED_ORANGE, ON_LED);
	LEDS_CONTROL(LED_RED, ON_LED);
	LEDS_CONTROL(LED_BLUE, ON_LED);
	HAL_Delay(200);

	LEDS_CONTROL(LED_GREEN, OFF_LED);
	LEDS_CONTROL(LED_ORANGE, OFF_LED);
	LEDS_CONTROL(LED_RED, OFF_LED);
	LEDS_CONTROL(LED_BLUE, OFF_LED);
	HAL_Delay(200);
}

/**
 * @brief 	Main function controls call to other APIs
 * 
 * @retval 	int 
 * @note 	HAL_INIT should be initilized first. 
 * 
 */
int main()
{
	HAL_Init();
	LEDS_INIT();
	while(1){
		BLINK_FULL_LEDS();
	}
	return 0;
}
