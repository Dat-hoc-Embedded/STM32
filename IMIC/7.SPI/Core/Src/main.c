#include "main.h"

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
void UART_SEND(uint8_t data){
	uint32_t *UART1_DR = (uint32_t *)(UART1_BASE_ADDR + 0x04);
	*UART1_DR = data;

	uint32_t *UART1_SR = (uint32_t *)(UART1_BASE_ADDR + 0x00);
	while(((*UART1_SR >> 6) & 1) == 0);
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

/* --------------------- SPI_INIT*/
#define GPIOA_BASE_ADDR 0x40020000
#define GPIOE_BASE_ADDR 0x40021000
#define SPI1_BASE_ADDR 0x40013000

void CS_LOW(){
	uint32_t *GPIOE_ODR = GPIOE_BASE_ADDR + 0x14;
	*GPIOE_ODR &= ~(1 << 3);
}
void CS_HIGH(){
	uint32_t *GPIOE_ODR = GPIOE_BASE_ADDR + 0x14;
	*GPIOE_ODR |= 1 << 3;
}
void SPI_INIT(){
	// PA5, PA6, PA7: Alternate function mode
	__HAL_RCC_GPIOA_CLK_ENABLE();
	uint32_t *GPIOA_MODER = GPIOA_BASE_ADDR + 0x00;
	*GPIOA_MODER &= ~(0b111111 << 10);
	*GPIOA_MODER |= (0b101010 << 10);

	// PA5 : SPI1_SCK, PA6 : SPI1_MISO, PA7 : SPI1_MOSI -> vào AF5: 0101
	uint32_t *GPIOA_AFRL = GPIOA_BASE_ADDR + 0x20;
	*GPIOA_AFRL &= ~(0xFFF << 20);
	*GPIOA_AFRL |= 0x555 << 20;

	// PE3: CS -> General purpose output mode
	__HAL_RCC_GPIOE_CLK_ENABLE();
	uint32_t *GPIOE_MODER = GPIOE_BASE_ADDR + 0x00;
	*GPIOE_MODER &= ~(0b11 << 6);
	*GPIOE_MODER |= (0b01 << 6);

	CS_HIGH();
	// Config SPI control_register
	__HAL_RCC_SPI1_CLK_ENABLE();
	uint16_t *SPI1_CR1 = SPI1_BASE_ADDR + 0x00;
	*SPI1_CR1 |= (0b1 << 2); // Master mode
	*SPI1_CR1 |= (0b011 << 3); // Set Clock f/16 = 16MHz / 16 = 1MHz <= 10 MHz (oke)
	*SPI1_CR1 |= (0b11 << 8); // Software slave management enable
	*SPI1_CR1 |= (1 << 1);  // CPOL = 1
	*SPI1_CR1 |= (1 << 0);  // CPHA = 1
	*SPI1_CR1 |= (1 << 6); // Enable SPI


}
uint8_t SPI1_Read_Slave(uint8_t reg){
	uint8_t temp = 0;

	uint16_t *SPI1_DR = SPI1_BASE_ADDR + 0x0C;
	uint16_t *SPI1_SR = SPI1_BASE_ADDR + 0x08;

	// Transmit data
	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = reg | (1 << 7);
	// Đợi quá trình truyền data hoàn thành
	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit

	// Kiểm tra thanh ghi RXNE chờ đến khi có dữ liệu rác
	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait
	temp = *SPI1_DR; // đọc dữ liệu rác


	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = 0x00;

	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit
	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait
	temp = *SPI1_DR;

	return temp;
}
uint8_t SPI1_Write_Slave(uint8_t reg, uint8_t data){

	uint8_t data_back = 0;
	uint16_t *SPI1_DR = SPI1_BASE_ADDR + 0x0C;
	uint16_t *SPI1_SR = SPI1_BASE_ADDR + 0x08;


	// Transmit data
	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = reg | (0 << 7);
	// Đợi quá trình truyền data hoàn thành
	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit

	// Kiểm tra thanh ghi RXNE chờ đến khi có dữ liệu rác
	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait
	uint16_t rb = *SPI1_DR; // đọc dữ liệu rác


	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = data;
	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit


	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait
	data_back = *SPI1_DR;

	return data_back ;
}

volatile uint16_t tmp = 0 ;
int main(){
	HAL_Init();
	UART1_INIT();
	SPI_INIT();

	while(1){
		CS_LOW();
		tmp = SPI1_Read_Slave(0b0001111); // Adress of Who_Am_I: 000 1111
		CS_HIGH();
		HAL_Delay(1000);

		my_printf("Gia tri cua thanh Who_Am_I = %d \n",tmp);
	}
	return 0;
}
