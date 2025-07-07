#include "main.h"
	/* -------------------------- LED  */
#define GPIOD_BASE_ADDR 0x40020C00
void LedsInit()
{
	__HAL_RCC_GPIOD_CLK_ENABLE();
	uint32_t* GPIOD_MODER = GPIOD_BASE_ADDR + 0x00;
	// set PD12, PD13, PD14, PD15 as OUTPUT
	*GPIOD_MODER &= ~(0xFF << 24); // clear 8 bit 24 -> 31 to 0.

	*GPIOD_MODER |= (0b01 << 24);
	*GPIOD_MODER |= (0b01 << 26);
	*GPIOD_MODER |= (0b01 << 28);
	*GPIOD_MODER |= (0b01 << 30);
}
// led = 0 green, 1 = orange, 2 = red, 3 = blue
// state: 0 = OFF, 1 = ON
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
void LedCtrl(led_t led, led_state_t state)
{
	uint32_t* GPIOD_ODR = GPIOD_BASE_ADDR + 0x14;
	if(state == ON_LED){
		*GPIOD_ODR |= 1 << (led + 12);
	}
	else{
		*GPIOD_ODR &= ~(1 << (led + 12));
	}
}
/* -------------------------- Button */
#define GPIOA_BASE_ADDR 0x40020000
void ButtonInit()
{
	__HAL_RCC_GPIOA_CLK_ENABLE(); // bật clock cho thanh ghi

	// set PA0 as INPUT mode , khong can PULL_UP, PULL_DOW vi trong mach da co dien tro R35
		// Set 2 bit bat dau tu bit 0 cua thanh ghi MODER bang 0b00 <che do input>
	uint32_t* GPIOA_MODER = GPIOA_BASE_ADDR + 0x00;
	*GPIOA_MODER &= ~(0b11 << 0); // clear bit
}
char ButtonGetState()
{
	// read bit 0 of IDR
	uint32_t *GPIOA_IDR = GPIOA_BASE_ADDR + 0x10;
	if (((*GPIOA_IDR >> 0 ) & 0b1) == 1)
		return 1;
	else
		return 0;
}
/* ---------------------- Interrupt */
#define Rising_trigger 0x08
#define Falling_trigger 0x0C
#define Interrupt_mask_register 0x00
#define EXTI_BASE_ADDR 0x40013C00
#define Pending_Register 0x14 // clear interrupt
/* Khi có nút nhấn thì sẽ nhảy ngay tới hàm này xong đó vào hàm main */

void EXTI0_IRQHandler(){
	// Only when Detect rising/falling edge will conduct this function.
	// If you hold the button will no conduct this function.
	if (ButtonGetState() == 1){
		LedCtrl(LED_BLUE, ON_LED);
	}else{
		LedCtrl(LED_BLUE, OFF_LED);
	}

	// Set bit thanh ghi Pending register
	uint32_t *EXTI_PR = EXTI_BASE_ADDR + Pending_Register;
	*EXTI_PR |= (0b1 << 0);
}
void Custome_IRQHandler(){
	// Only when Detect rising/falling edge will conduct this function.
	// If you hold the button will no conduct this function.
	if (ButtonGetState() == 1){
		LedCtrl(LED_GREEN, ON_LED);
	}else{
		LedCtrl(LED_GREEN, OFF_LED);
	}

	// Set bit thanh ghi Pending register
	uint32_t *EXTI_PR = EXTI_BASE_ADDR + Pending_Register;
	*EXTI_PR |= (0b1 << 0);
}
void EXTI0_Init(){
	// Set thanh ghi Rising & Falling
	uint32_t *EXTI_RTSR = EXTI_BASE_ADDR + Rising_trigger;
	uint32_t *EXTI_FTSR = EXTI_BASE_ADDR + Falling_trigger;
	*EXTI_RTSR |= (0b1 << 0);
	*EXTI_FTSR |= (0b1 << 0);

	// Set Interrupt mask register -> not masked pin to transfer interrupt signal
	uint32_t *EXTI_IMR = EXTI_BASE_ADDR + Interrupt_mask_register;
	*EXTI_IMR |= (0b1 << 0);

	// Set bit 6 of NVIC_ISER0 -> NVIC accept signal from EXTI
	uint32_t *NVIC_ISER0 = 0xE000E100;
	*NVIC_ISER0 |= (0b1 << 6);

	/* ------------------ Change Interrupt function*/
	/* Copy vector table to RAM */
	uint8_t *vttb = 0x00000000; // uint8_t boi vi copy 8 bytes một
	uint8_t *ram = 0x20000000;
	for (int i = 0; i < 0x198 ; i++){
		*(ram + i) = * (vttb + i);
	}
	/* Thông báo NVIC vector table được chuyển lên Ram */
	uint32_t *VTOR = 0xE000ED08;
	*VTOR = 0x20000000;

	uint32_t *fn = 0x20000058;
	*fn = Custome_IRQHandler;

}
int main(){
	HAL_Init();
	LedsInit();
	ButtonInit();
	EXTI0_Init();

	while(1)
	{
		LedCtrl(LED_RED, ON_LED);
		HAL_Delay(1000);
		LedCtrl(LED_RED, OFF_LED);
		HAL_Delay(1000);
	};
	return 0;
}
