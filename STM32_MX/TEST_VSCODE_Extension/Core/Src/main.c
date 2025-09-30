#include <main.h>
#include "stm32f4xx.h"
#include "uart/uart.h"
#include "motor/motor.h"

void SystemClock_Config()
{
	//  ------------------- SET UP 100MHZ 
    // 1. Turn on HSE: 8 MHz
	RCC -> PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSE ; 

	// 2. PLLM = (/)8 ; PLLP = (/)2 ; PLLN = (*)200; (→ 100 MHz)
	RCC -> PLLCFGR &= ~(RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk | RCC_PLLCFGR_PLLP_Msk);
	RCC -> PLLCFGR |= ((8 << RCC_PLLCFGR_PLLM_Pos) | (200 << RCC_PLLCFGR_PLLN_Pos) | (0 << RCC_PLLCFGR_PLLP_Pos));

	// 3. Set wait states (cycles) for Flash (100 MHz @ Vdd >=2.7V → 3WS)
	FLASH -> ACR &= ~FLASH_ACR_LATENCY_Msk;
	FLASH -> ACR |= FLASH_ACR_LATENCY_3WS; 

	// 4. ON HSE & PLL Clock 
	RCC -> CR |= RCC_CR_HSEON;
	while (!(RCC -> CR & RCC_CR_HSERDY)); // WAIT HSERDY = 1 
	RCC -> CR |= RCC_CR_PLLON;
	while (!(RCC -> CR & RCC_CR_PLLRDY)); 

	// 5. SET PRESCALERS FOR AHB, APB1, APB2
	RCC -> CFGR &= ~RCC_CFGR_HPRE; // AHB PRESCALER = /1

	RCC -> CFGR &= ~(RCC_CFGR_PPRE1_Msk); 
	RCC -> CFGR |= RCC_CFGR_PPRE1_DIV2; // APB1 PRESCALER = /2

	RCC -> CFGR &= ~RCC_CFGR_PPRE2_Msk; // APB2 PRESCALER = /1
	RCC -> CFGR |= RCC_CFGR_PPRE2_DIV1; // APB2 PRESCALER = /1

	// 6. SWITCH SYSCLK TO PLL 
	RCC -> CFGR &= ~RCC_CFGR_SW; 
	RCC -> CFGR |= RCC_CFGR_SW_PLL;
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

void Clock_Init()
{
	HAL_Init();
	SystemClock_Config(); // SET RCC -> CLOCK = 100 MHz 
	SystemCoreClockUpdate();  // CẬP NHẬT HCLK → SystemCoreClock
	HAL_InitTick(TICK_INT_PRIORITY); // cập nhật lại SysTick theo HCLK mới
}

int main(){
	Clock_Init();

	//UART1_INIT();
	motor_pwm_init();
	for (volatile int i = 0; i < 2e6 ; i ++);

	while (1){
		// for(volatile int i = PWM_MIN_US ; i <= 2000 ; i ++) motor_pwm_set(1, i);
		// HAL_Delay(1000);
		// for(volatile int i = PWM_MAX_US ; i >= 1000 ; i --) motor_pwm_set(1, i);
		// HAL_Delay(1000);

		motor_pwm_set(4, 1000);
	};
}
