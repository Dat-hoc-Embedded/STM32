#include "uart.h"
#include <main.h>
#include "string.h"
#include "stdio.h"
#include "stdarg.h"

void UART1_INIT(){
	__HAL_RCC_GPIOB_CLK_ENABLE();
	uint32_t *GPIOB_MODER = (uint32_t *)(GPIOB_BASE_ADDR + 0x00);
	uint32_t *GPIOB_AFRL = (uint32_t *)(GPIOB_BASE_ADDR + 0x20);
	*GPIOB_MODER &= ~(0b1111 << 12);
	*GPIOB_MODER |= (0b1010 << 12);

	*GPIOB_AFRL &= ~(0xFF << 24);
	*GPIOB_AFRL |= (0x77 << 24);

	// SET UP UART1
	__HAL_RCC_USART1_CLK_ENABLE();
	uint32_t *UART1_CR1 = (uint32_t *)(UART1_BASE_ADDR + 0x0C);
	uint32_t *UART1_BRR = (uint32_t *)(UART1_BASE_ADDR + 0x08);

	*UART1_CR1 |= (0b1 << 12); // Word length: 9 bits ( + 1 Parity)
	*UART1_CR1 |= (0b1 << 10); // Parity: Enable
	*UART1_CR1 &= ~(0b1 << 9); // Parity type: even
	*UART1_CR1 |= (0b11 << 2); // Transmit & Receiver enable
	*UART1_CR1 |= (0b1 << 13); // Enable UART

	*UART1_BRR = (104 << 4) | (3 << 0) ;// Set Baudrate 9600 bps

	/* ----------------- UART Interrupt */
	*UART1_CR1 |= (0b1 << 5); // Set bit RXNEIE
	// Position = 37 -> Bit 5 NVIC_ISER1(0xE000E104)

	uint32_t *NVIC_ISER1 = (uint32_t *)(0xE000E104);
	*NVIC_ISER1 |= (0b1 << 5);
}
void UART_SEND(char data){
	uint32_t *UART1_DR = (uint32_t *)(UART1_BASE_ADDR + 0x04);
	*UART1_DR = data;

	uint32_t *UART1_SR = (uint32_t *)(UART1_BASE_ADDR + 0x00);
	while(((*UART1_SR >> 6) & 1) == 0);
}

void my_printf(char *str,...){
	va_list list;
	va_start(list, str);
	char print_buf [128] = {0};
	vsprintf(print_buf, str, list);
	int len = strlen(print_buf);
	for (int i = 0; i < len; i++){
		UART_SEND(print_buf[i]);
	}
	va_end(list);
}