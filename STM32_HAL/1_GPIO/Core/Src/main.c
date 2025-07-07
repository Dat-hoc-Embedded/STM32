#include "main.h"

#define GPIOD_BASE_ADDR  0x40020C00

void GPIO_INIT(){
	// 1. Bật xung
	__HAL_RCC_GPIOD_CLK_ENABLE();

	// 2. Define base address GPIOD
	// 3. Truy cập thanh ghi GPIOD_MODER
	uint32_t *GPIOD_MODER = GPIOD_BASE_ADDR + 0x00;
	// 4. Reset bit -> Write 01 : OUTPUT
	*GPIOD_MODER &= (0xFF << 24);
	*GPIOD_MODER |= (0b01010101 << 24);

	// Chọn chế độ Ouput: Push pull
	uint32_t *GPIOD_OTYPER = GPIOD_BASE_ADDR + 0x04;
	*GPIOD_OTYPER &= ~(0b1111 << 12);
}
typedef enum{
	LED_GREEN,
	LED_ORANGE,
	LED_RED,
	LED_BLUE
} led_t;
typedef enum{
	OFF_LED,
	ON_LED
} led_state;
void LED_CONTROL(led_t led, led_state state){
	uint32_t *GPIOD_ODR = GPIOD_BASE_ADDR + 0x14;
//	*GPIOD_ODR |= (0b1 << 14);
//
//	HAL_Delay(1000);
//	*GPIOD_ODR &= ~(0b1 << 14);
//	HAL_Delay(1000);
	if (state == ON_LED)
		*GPIOD_ODR |= (0b1 << led + 12);
	else
		*GPIOD_ODR &= ~(0b1 << led + 12);

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
	HAL_Delay(1000);


}
int main(){
	HAL_Init();
	GPIO_INIT();
	while (1){
		for (int i = 0 ; i < 2; i ++){
			//HAL_GPIO_TogglePin(GPIOx, GPIO_Pin);
			LED_CONTROL(i, ON_LED);
			LED_CONTROL(i+2, ON_LED);
			HAL_Delay(1000);
			LED_CONTROL(i, OFF_LED);
			LED_CONTROL(i+2, OFF_LED);
			HAL_Delay(1000);
		}
	}

}
