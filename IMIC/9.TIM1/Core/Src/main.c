#include "main.h"

/*------------------- LEDs */
// LED on board is: PD12, PD13, PD14, PD15 đang ở trạng thái nối GND
#define GPIOD_BASE_ADDR 0x40020C00 // BASE ADDRESS GPIOD

// Function MODER INIT for LEDs
void LEDS_INIT(){
	// Khoi tao CLOCK cho GPIOD
	__HAL_RCC_GPIOD_CLK_ENABLE();

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

#define TIM1_BASE_ADDR 0x40010000
void TIM_INIT(){
	__HAL_RCC_TIM1_CLK_ENABLE();
	uint16_t*ARR = (uint16_t *)(TIM1_BASE_ADDR + 0x2C);
	*ARR = 1000;
	// set PSC = 16000 - 1
	uint16_t *PSC = (uint16_t *)(TIM1_BASE_ADDR + 0x28);
	*PSC = 16000 - 1;

	// Enable Counter
	uint16_t *CR1 = (uint16_t *)(TIM1_BASE_ADDR + 0x00);
	*CR1 |= 1 << 0;

	// Set Interrupt
	uint16_t *DIER = (uint16_t *)(TIM1_BASE_ADDR + 0x0C);
	*DIER |= ( 1 << 0);
	uint32_t *ISER0 = (uint32_t *)(0xE000E100);
	*ISER0 |= ( 1 << 25);
}
int tim_cnt ;
void TIM1_UP_TIM10_IRQHandler()
{
	tim_cnt ++;
	uint16_t *SR = (uint16_t *)(TIM1_BASE_ADDR + 0x10);
	*SR &= ~(1 << 0);
}
void my_Delay(int msec){
//	uint16_t *SR = (uint16_t *)(TIM1_BASE_ADDR + 0x10);
//	for (int i = 0 ; i < sec; i ++){
//		while (((*SR >> 0) & 1) == 0);
//		*SR &= ~(1 << 0);
//	}
	tim_cnt = 0;
	while (tim_cnt < msec/1000);
}
int main(){
	HAL_Init();
	LEDS_INIT();
	TIM_INIT();
	while (1){
		LEDS_CONTROL(LED_RED, ON_LED);
		my_Delay(1000);
		LEDS_CONTROL(LED_RED, OFF_LED);
		my_Delay(1000);

	};
	return 0;
}
