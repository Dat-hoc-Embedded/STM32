#include "main.h"

#define TIM2_BASE_ADDR  0x40000000
#define GPIOA_BASE_ADDR 0x40020000
void GPIOA_INIT()
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	uint32_t *GPIOA_MODER = (uint32_t *)(GPIOA_BASE_ADDR + 0x00);
	*GPIOA_MODER &= ~(0b11 << 0);
	*GPIOA_MODER |= (0b10 << 0); // PA0 -> Alternate fucntion mode

	uint32_t *GPIOA_AFRL = (uint32_t *)(GPIOA_BASE_ADDR + 0x20);
	*GPIOA_AFRL &= ~(0xF << 0);
	*GPIOA_AFRL |= (0b1 << 0);

}
void TIM2_IC_INIT()
{
	__HAL_RCC_TIM2_CLK_ENABLE();
	uint32_t *ARR = (uint32_t *)(TIM2_BASE_ADDR + 0x2C);
	*ARR = 0xFFFFFFFF;   // full range 32-bit

	uint16_t *PSC = (uint16_t *)(TIM2_BASE_ADDR + 0x28);
	*PSC = 15999;

	// 1 tick = 1ms -> 1 chu kỳ = 65535 * 1 ms ~ 65.5 s
	uint16_t *CCMR1 = (uint16_t *)(TIM2_BASE_ADDR + 0x18);
	*CCMR1 &= ~(0xFF << 0);
	*CCMR1 |= (0x01 << 0);
	//*CCMR1 |= (0x0011 << 4);

	uint16_t *CCER = (uint16_t *)(TIM2_BASE_ADDR + 0x20);
	*CCER &= ~(0b111 << 0);
	*CCER |= (0b1 << 0); // Capture enable và bắt cạnh lên (00)


	uint16_t *DIER = (uint16_t *)(TIM2_BASE_ADDR + 0x0C);
	*DIER |= (0b1 << 1); // Enable interrupt cho Input Capture channel 1

	uint32_t *ISER0 = (uint32_t *)(0xE000E100);
	*ISER0 |= ( 1 << 28); // NVIC phát TIM2

	uint32_t *CNT = (uint32_t *)(TIM2_BASE_ADDR + 0x24);
	*CNT = 0 ;

	uint16_t *EGR = (uint16_t *)(TIM2_BASE_ADDR + 0x14);
	*EGR |= (0b1 << 0);

	uint16_t *CR1 = (uint16_t *)(TIM2_BASE_ADDR + 0x00);
	*CR1 |= (0b1 << 0) ; // Enable counter

}

volatile uint32_t t1=0, t2=0, pulse = 0;
volatile uint8_t  capture_state=0; // 0 : thả, 1 : nhấn
volatile uint32_t previous = 0;
void TIM2_IRQHandler()
{
	uint16_t *SR = (uint16_t *)(TIM2_BASE_ADDR + 0x10);
	uint32_t *CCR1 = (uint32_t *)(TIM2_BASE_ADDR + 0x34);
	uint16_t *CCER = (uint16_t *)(TIM2_BASE_ADDR + 0x20);

	if (((*SR >> 1) & 0b1) == 1) // CC1IF = 1
	{
		uint32_t now = *CCR1;
		if (now - previous < 10)
			return;
		if(capture_state == 0)
		{
			t1 = (*CCR1 >> 0);
			*CCER &= ~(0b11 << 1);
			*CCER |= (1 << 1); // capture cạnh xuống
			capture_state = 1; // đang nhấn
		}
		else{
			t2 = (*CCR1 >> 0);
			if(t2 >= t1){
				pulse = (t2 - t1);
			}else{
				pulse = (0xFFFFFFFF - t1 + t2 + 1);
			}
			*CCER &= ~(0b11 << 1); // capture cạnh lên
			capture_state = 0; // đang thả

		}
		previous = now;
		*SR &= ~(0b1 << 1); // clear CC1IF để tắt ngắt.
	}
}
volatile uint32_t cnt = 0;
int main(){
	HAL_Init();
	TIM2_IC_INIT();
	GPIOA_INIT();

	while(1){
		cnt = *(uint32_t *)(TIM2_BASE_ADDR + 0x24);
	}


}
