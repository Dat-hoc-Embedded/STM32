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

/* --------------------------------- DMA */
#define DMA2_BASE_ADDR 0x40026400
char rx_buf[5544];
int rx_index = 0;
void DMA_Init()
{
	__HAL_RCC_DMA2_CLK_ENABLE();

	// Set sender address
	uint32_t* DMA_S2PAR = (uint32_t*)(DMA2_BASE_ADDR + 0x18 + 0x18 * 2);
	*DMA_S2PAR =  0x40011004; // address of DR register of USART

	// Set receiver address
	uint32_t* DMA_S2M0AR = (uint32_t*)(DMA2_BASE_ADDR + 0x1C + 0x18 * 2);
	*DMA_S2M0AR = rx_buf;

	// Set number of data
	uint32_t* DMA_S2NDTR = (uint32_t*)(DMA2_BASE_ADDR + 0x14 + 0x18 * 2);
	*DMA_S2NDTR = sizeof(rx_buf);

	// Set channel
	uint32_t* DMA_S2CR = (uint32_t*)(DMA2_BASE_ADDR + 0x10 + 0x18 * 2);
	*DMA_S2CR |= (4 << 25); // Channel 4

	*DMA_S2CR |= (1 << 10);  // Enable MINC Mode
	*DMA_S2CR |= (1 << 8);  // Enable CIRC Mode
	*DMA_S2CR |= (1 << 0);  // Enable stream

	/* DMA send an interrupt signal to NVIC*/
	*DMA_S2CR |= 1 << 4;
	uint32_t* ISER1 = (uint32_t*)0xE000E104;
	*ISER1 |= 1 << (58 - 32);

}

char recv_new_fw_complete = 0; // Tạo biến này để set nó lên 1 khi nhận đủ data truyền vào
void DMA2_Stream2_IRQHandler()
{
	__asm("NOP");
	// Tạo cờ ngắt
	uint32_t*  DMA_LIFCR = (uint32_t*)(DMA2_BASE_ADDR + 0x08);
	*DMA_LIFCR |= 1 << 21;

	// Đổi giá trị của biến recv_new_fw_complete = 1 khi truyền đủ data từ file .bin
	recv_new_fw_complete = 1;
}

/* -------------------------- USART  */
#define UART1_BASE_ADDR 0x40011000
#define GPIOB_BASE_ADDR 0x40020400
void UART_init()
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	uint32_t* GPIOB_MODER = (uint32_t*)(GPIOB_BASE_ADDR + 0x00);
	*GPIOB_MODER &= ~(0b1111 << 12);
	*GPIOB_MODER |= (0b10 << 12) | (0b10 << 14);
	uint32_t* GPIOB_AFRL = (uint32_t*)(GPIOB_BASE_ADDR + 0x20);
	*GPIOB_AFRL &= ~(0xff << 24);
	*GPIOB_AFRL |= (0b0111 << 24) | (0b0111 << 28);

	__HAL_RCC_USART1_CLK_ENABLE();
	uint32_t *USART_CR1 = (uint32_t*)(UART1_BASE_ADDR + 0x0C);
	uint32_t *USART_BRR = (uint32_t*)(UART1_BASE_ADDR + 0x08);
	*USART_CR1 |= (1 << 12);
	*USART_CR1 |= (1 << 10);
	*USART_CR1 &= ~(1 << 9);
	*USART_BRR = 104 << 4 | 3;
	*USART_CR1 |= (0b11 << 2);
	*USART_CR1 |= (1 << 13);
#if 0
	// enable interrupt for uart 1
	*USART_CR1 |= 1 << 5;
	uint32_t *ISER1 =(uint32_t *)0xE000E104;
	*ISER1 |= (1 << 5); // Cho NVIC châp nhận tín hiệu ngắt
#else
	uint32_t *USART_CR3 = (uint32_t*)(UART1_BASE_ADDR + 0x14);
	*USART_CR3 |= (1<<6); // set bit 6
	DMA_Init();
#endif
}

void UART_send(char* data)
{
	uint32_t * UART_DR = (uint32_t*)(UART1_BASE_ADDR + 0x04);
	uint32_t *UART_SR = (uint32_t*)(UART1_BASE_ADDR + 0x00);

	*UART_DR = data;
	// wait until transmission is complete.
	while(((*UART_SR >> 6) & 1) == 0){ // check transmit complete?
	}
}
// Tao ham printf
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
void my_printf(char* str, ...)
{
	va_list list;
	va_start(list, str);
	char print_buf[128] = {0};
	vsprintf(print_buf, str, list);
	int len = strlen(print_buf);
	for (int i = 0 ; i < len; i ++){
		UART_send(print_buf[i]);
	}
	va_end(list);
}

void USART1_IRQHandler()
{
	uint32_t * UART_DR = (uint32_t*)(UART1_BASE_ADDR + 0x04);
	uint32_t *UART_SR = (uint32_t*)(UART1_BASE_ADDR + 0x00);
	while (((*UART_SR >> 5) & 1) == 0);
	rx_buf[rx_index] = *UART_DR;
	rx_index ++;
}

/* -------------------------------- FLASH */
#define FLASH_INTERFACE_BASE_ADDR 0x40023C00

__attribute__((section(".ham_tren_ram"))) void flash_erase_sector (int sec_num){
	uint32_t *FLASH_SR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x0C);
	uint32_t *FLASH_CR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x10);

	if((*FLASH_CR >> 31) == 1){
		uint32_t* FLASH_KEYR = (FLASH_INTERFACE_BASE_ADDR + 0x04);
		*FLASH_KEYR = 0x45670123;
		*FLASH_KEYR = 0xCDEF89AB;
	}
	if(sec_num > 7){
		return;
	}
	// Check BSY bit in SR register, nếu FLASH bận thì BSY = 1 mình sẽ chờ
	while (((*FLASH_SR >> 16) & 1) == 1);

	// Set SER bit & SNB bit in CR register
	*FLASH_CR |= 1 << 1;
	*FLASH_CR |= (sec_num << 3);

	// Set the STRT bit in the FLASH_CR register
	*FLASH_CR |= (1 << 16);

	// Wait for the BSY bit to be clear
	while(((*FLASH_SR >> 16) & 1) == 1);

}
__attribute__((section(".ham_tren_ram"))) void flash_program(uint8_t* addr, uint8_t val){
	uint32_t *FLASH_SR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x0C);
	uint32_t *FLASH_CR = (uint32_t*)(FLASH_INTERFACE_BASE_ADDR + 0x10);

	if((*FLASH_CR >> 31) == 1){
		// unlock by the unlock sequence
		uint32_t* FLASH_KEYR = FLASH_INTERFACE_BASE_ADDR + 0x04;
		*FLASH_KEYR = 0x45670123;
		*FLASH_KEYR = 0xCDEF89AB;
	}
	// Check BSY bit in the FLASH_SR register
	while (((*FLASH_SR >> 16) & 1) == 1);

	// SET the PG bit in the FLASH_CR register.
	*FLASH_CR |= 1 << 0;

	// Ghi dữ liệu vào địa chỉ memory.
	*addr = val;

	// Wait for the BSY bit to be cleared.
	while(((*FLASH_SR >> 16) & 1) == 1);
}

__attribute__((section(".ham_tren_ram"))) void update(){



	if (recv_new_fw_complete == 1)
	{
		__asm("CPSID i"); 	// disable all interrupt
		flash_erase_sector(0);
		for (int i = 0 ; i < sizeof(rx_buf); i++){
			flash_program(0x08000000 + i, rx_buf[i]);
		}
		uint32_t *AIRCR = (uint32_t*)(0xE000ED0C);
		*AIRCR = (0x5FA << 16) | (1 << 2);
	}
}
int main(){
	HAL_Init();
	LedsInit();
	ButtonInit();
	UART_init();

	my_printf(" DAY LA FIRMWARE VER 6.0\n");
	while(1)
	{
//		LedCtrl(LED_RED, ON_LED);
//		HAL_Delay(1000);
//		LedCtrl(LED_RED, OFF_LED);
//		HAL_Delay(1000);
		update();

	};
	return 0;
}
