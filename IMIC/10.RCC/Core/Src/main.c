#include "main.h"

#define RCC_BASE_ADDR  0x40023800
/*------------------- LEDs */
// LED on board is: PD12, PD13, PD14, PD15 đang ở trạng thái nối GND
#define GPIOD_BASE_ADDR 0x40020C00 // BASE ADDRESS GPIOD

// Function MODER INIT for LEDs
void LEDS_INIT(){

	uint32_t* GPIOD_MODER = GPIOD_BASE_ADDR + 0x00;
	// Reset bit MODER of PD 12,13,14,15
	*GPIOD_MODER &= ~(0xFF << 24); // 0xFF = 0x1111 1111

	// SET [01] cho MODE: OUTPUT
	*GPIOD_MODER |= (0b01 << 24); // LED GREEN bits [24:25]
	*GPIOD_MODER |= (0b01 << 26); // LED ORANGE ...
	*GPIOD_MODER |= (0b01 << 28); // LED RED ...
	*GPIOD_MODER |= (0b01 << 30); // LED BLUE ...
}
// Function for Control Leds
typedef enum{
	LED_GREEN,
	LED_ORANGE,
	LED_RED,
	LED_BLUE
}led_t;

typedef enum{
	OFF_LED,
	ON_LED
}led_state_t;

void LEDS_CONTROL(led_t led, led_state_t state){
	uint32_t *GPIOD_ODR = GPIOD_BASE_ADDR + 0x14;
	if (state == ON_LED)
		*GPIOD_ODR |= (0b1 << (12 + led));
	else
		*GPIOD_ODR &= ~(0b1 << (12 + led));
}
/*------------------- RCC */
void RCC_INIT(){
	/* -------- Choose Source Clock for PLL: HSE → PLLCFGR:PLLSRC[22] = 1. */
	uint32_t *RCC_PLLCFGR = (uint32_t *)(RCC_BASE_ADDR + 0x04);
	*RCC_PLLCFGR |= (0b1 << 22);
		// PLLM & PLLN & PLLP
	*RCC_PLLCFGR &= ~(0b11 << 16);
	*RCC_PLLCFGR |= ((8 << 0) | (200 << 6) | (0b00 << 16));

	/* -------- Set wait states (cycles) for FLASH */
	uint32_t *FLASH_ACR = (uint32_t *)(0x40023C00 + 0x00);
	*FLASH_ACR &= ~(0xF << 0);
	*FLASH_ACR |= (3 << 0);

	/* -------- On HSE & PLL Clock */
	uint32_t *RCC_CR = (uint32_t *)(RCC_BASE_ADDR + 0x00);
	*RCC_CR |= (0b1 << 16);
		// Wait until  HSERDY[17] = 1
	while (((*RCC_CR >> 17) & 1) == 0);
		// ON & wait PLL clock
	*RCC_CR |= (0b1 << 24);
	while (((*RCC_CR >> 25) & 1) == 0);

	/* -------- Set value of prescale */
		// Set AHB prescaler
	uint32_t *RCC_CFGR =(uint32_t *)(RCC_BASE_ADDR + 0x08);
	*RCC_CFGR &= ~(0xF << 4); // → as /1
	 	// Set APB1 prescaler (/2)
	*RCC_CFGR &= ~(0b111 << 10);
	*RCC_CFGR |= (0b100 << 10);
		// Set APB2 prescaler (/1)
	*RCC_CFGR &= ~(0b111 << 13); // → as /1


	/* -------- ON Switch bit to PLL → CFGR : SW [0:1] = 10*/
	*RCC_CFGR &= ~(0b11 << 0);
	*RCC_CFGR |= (0b10 << 0);
	while (((*RCC_CFGR >> 2) & 0b11) != 0b10); // wait till switch done

}
void RCC_ENABLE(){
	uint32_t *RCC_AHB1ENR = (uint32_t *)(RCC_BASE_ADDR + 0x30);
	*RCC_AHB1ENR |= (0b1 << 3);

}
int main(){
	HAL_Init();
	RCC_INIT();
	RCC_ENABLE();
	LEDS_INIT();
	while(1){
		LEDS_CONTROL(LED_BLUE, ON_LED);
		HAL_Delay(1000);
		LEDS_CONTROL(LED_BLUE, OFF_LED);
		HAL_Delay(1000);
	}
}
