#include "main.h"

/* --------------------- SPI_INIT*/
#define GPIOA_BASE_ADDR 0x40020000
#define GPIOE_BASE_ADDR 0x40021000
#define SPI1_BASE_ADDR 0x40013000

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

	// PE3: OUTPUT
	__HAL_RCC_GPIOE_CLK_ENABLE();
	uint32_t *GPIOE_MODER = GPIOE_BASE_ADDR + 0x00;
	*GPIOE_MODER &= ~(0b11 << 6);
	*GPIOE_MODER |= (0b01 << 6);

	// Config SPI control_register
	__HAL_RCC_SPI1_CLK_ENABLE();
	uint16_t *SPI1_CR1 = SPI1_BASE_ADDR + 0x00;
	*SPI1_CR1 |= (0b1 << 2); // Master mode
	*SPI1_CR1 |= (0b011 << 3); // Set Clock f/16
	*SPI1_CR1 |= (0b11 << 8); // Software slave management enable
	*SPI1_CR1 |= (1 << 6); // Enable SPI

	*SPI1_CR1 |= (1 << 1);  // CPOL = 1
	*SPI1_CR1 |= (1 << 0);  // CPHA = 1

}
volatile uint16_t tmp = 0 ;
void SPI1_TransmitReceive(uint16_t data){

	uint16_t *SPI1_DR = SPI1_BASE_ADDR + 0x0C;
	uint16_t *SPI1_SR = SPI1_BASE_ADDR + 0x08;

	// Transmit data
	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = data ;

	// Receive data
	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit
	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait
	tmp = *SPI1_DR;

	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = 0x00;

	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit
	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait

	tmp = *SPI1_DR;
}

void SPI1_TransmitReceive_register(uint16_t reg){

	uint16_t *SPI1_DR = SPI1_BASE_ADDR + 0x0C;
	uint16_t *SPI1_SR = SPI1_BASE_ADDR + 0x08;

	// Transmit data
	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = reg | (1 << 7);
	// Đợi quá trình truyền data hoàn thành
	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit

	// Kiểm tra thanh ghi RXNE chờ đến khi có dữ liệu rác
	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait
	tmp = *SPI1_DR; // đọc dữ liệu rác


	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = 0x00;
	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit


	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait
	tmp = *SPI1_DR;

}

void SPI1_TransmitReceive_write_register(uint16_t reg, uint8_t data){

	uint16_t *SPI1_DR = SPI1_BASE_ADDR + 0x0C;
	uint16_t *SPI1_SR = SPI1_BASE_ADDR + 0x08;

	// Transmit data
	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = reg | (1 << 7);
	// Đợi quá trình truyền data hoàn thành
	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit

	// Kiểm tra thanh ghi RXNE chờ đến khi có dữ liệu rác
	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait
	tmp = *SPI1_DR; // đọc dữ liệu rác


	while (((*SPI1_SR >> 1) & 1) == 0); // bit 1:TXE = 0 -> wait
	*SPI1_DR = data;
	while (((*SPI1_SR >> 7) & 1) == 1); // Busy bit


	while (((*SPI1_SR >> 0) & 1) == 0); // bit 0: RXNE = 0 -> wait
	tmp = *SPI1_DR;

}
void CS_LOW(){
	uint32_t *GPIOE_ODR = GPIOE_BASE_ADDR + 0x14;
	*GPIOE_ODR &= ~(1 << 3);
}
void CS_HIGH(){
	uint32_t *GPIOE_ODR = GPIOE_BASE_ADDR + 0x14;
	*GPIOE_ODR |= 1 << 3;
}
int main(){
	HAL_Init();
	SPI_INIT();

	while(1){
		CS_LOW();
		SPI1_TransmitReceive(0x8f);
		CS_HIGH();
		HAL_Delay(1000);
	}
	return 0;
}
