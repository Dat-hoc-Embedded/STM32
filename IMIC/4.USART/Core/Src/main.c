# include "main.h"
/* ---------------- LED INIT*/
#define GPIOD_BASE_ADDR 0x40020C00
void LED_INIT(){
	__HAL_RCC_GPIOD_CLK_ENABLE();
	uint32_t *GPIOD_MODER = GPIOD_BASE_ADDR + 0x00;

	*GPIOD_MODER &= ~(0xFF << 24);
	*GPIOD_MODER |= (0b01010101 << 24);
}
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
void LED_CONTROL(led_t led, led_state_t state){
	uint32_t *GPIOD_ODR = GPIOD_BASE_ADDR + 0x14;
	if (state == ON_LED){
		*GPIOD_ODR |= (0b1 << (12 + led));
	}else{
		*GPIOD_ODR &= ~(0b1 << (12 + led));
	}
}
/* ------------------------ UART */
#define GPIOB_BASE_ADDR 0x40020400
#define UART1_BASE_ADDR 0x40011000

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
void UART_SEND_STRING(char *str){
	for (int i = 0 ; i < strlen(str); i++){
		UART_SEND(str[i]);
	}
}

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
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

char rx_buf[8];
int rx_index = 0;
	// Interupt function
void USART1_IRQHandler()
{
	uint32_t *UART_DR = (uint32_t*)(UART1_BASE_ADDR + 0x04);
	uint32_t *UART_SR = (uint32_t*)(UART1_BASE_ADDR + 0x00);
	while (((*UART_SR >> 5) & 1) == 0);
	rx_buf[rx_index] = *UART_DR;
	rx_index ++;
	/* ------------- "LED ON" -> ON , "LED OFF" -> OFF */
	if (!strcmpi(rx_buf,"LED ON")){
		LED_CONTROL(LED_GREEN, ON_LED);
		memset(rx_buf, 0, sizeof(rx_buf));
		rx_index = 0;
	}else if(!strcmpi(rx_buf,"LED OFF")) {
		LED_CONTROL(LED_GREEN, OFF_LED);
		memset(rx_buf, 0, sizeof(rx_buf));
		rx_index = 0;
	}
}
int main(){
	HAL_Init();
	LED_INIT();
	UART1_INIT();
	while(1){
		/* ---------- Send 1 byte ---------- */
//		UART_SEND('D');
//		UART_SEND('A');
//		UART_SEND('T');
//		UART_SEND('\n');

		/* ------------- Function my_printf() -------- */
//		int x = 10;
//		my_printf("Day la diem lap trinh cua: %d \n",x);
//		HAL_Delay(1000);

		/* -------------- Send multi data -------- */
//		char msg[] = "CORE use UART transmit to PC \n";
//		UART_SEND_STRING(msg);
//		HAL_Delay(1000);

		/* ------------- Test Interrupt: Send and Leds */
//		LED_CONTROL(LED_RED, ON_LED);
//		HAL_Delay(1000);
//		LED_CONTROL(LED_RED, OFF_LED);
//		HAL_Delay(1000);

	}
	return 0;
}
