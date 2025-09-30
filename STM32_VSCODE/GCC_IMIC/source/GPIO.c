#include <stdint.h> // Để không báo lỗi kiểu dữ liệu uint32_t 
#include "GPIO.h"

#define GPIOD_BASE_ADDR 0x40020C00 // BASE ADDRESS GPIOD
#define RCC_BASE_ADDR 0x40023800

void LEDS_INIT(){
	// Khoi tao CLOCK cho GPIOD
	uint32_t* RCC_AHB1ENR = (uint32_t *)(RCC_BASE_ADDR + 0x30);
    *RCC_AHB1ENR |= 1 << 3;

	uint32_t* GPIOD_MODER = (uint32_t *)(GPIOD_BASE_ADDR + 0x00);
	// Reset bit MODER of PD 12,13,14,15
	*GPIOD_MODER &= ~(0xFF << 24); // 0xFF = 0x1111 1111

	// SET [01] cho MODE: OUTPUT
	*GPIOD_MODER |= (0b01 << 24); // LED GREEN bits [24:25]
	*GPIOD_MODER |= (0b01 << 26); // LED ORANGE ...
	*GPIOD_MODER |= (0b01 << 28); // LED RED ...
	*GPIOD_MODER |= (0b01 << 30); // LED BLUE ...
}

void LEDS_CONTROL(led_t led, led_state_t state){
	uint32_t *GPIOD_ODR = (uint32_t *)(GPIOD_BASE_ADDR + 0x14);
	if (state == ON_LED)
		*GPIOD_ODR |= (0b1 << (12 + led));
	else
		*GPIOD_ODR &= ~(0b1 << (12 + led));
}
