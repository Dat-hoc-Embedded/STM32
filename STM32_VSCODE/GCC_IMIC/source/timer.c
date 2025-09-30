#include <stdint.h>

#define RCC_BASE_ADDR 0x40023800
#define TIM1_BASE_ADDR 0x40010000
void TIM_INIT(){
	// Khoi tao CLOCK cho TIMER 1
	uint32_t* RCC_APB2ENR = (uint32_t *)(RCC_BASE_ADDR + 0x44);
    *RCC_APB2ENR |= 1 << 0;


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