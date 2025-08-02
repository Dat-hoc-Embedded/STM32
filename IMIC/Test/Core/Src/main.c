#include "main.h"

#define FLASH_BASE_ADDR 0X40023C00

void flash_eraser(int sector_number){
	uint32_t *FLASH_SR = (uint32_t *)(FLASH_BASE_ADDR + 0x0C);
	uint32_t *FLASH_CR = (uint32_t *)(FLASH_BASE_ADDR + 0x10);
	uint32_t *FLASH_KEYR = (uint32_t *)(FLASH_BASE_ADDR + 0x04);

	while(((*FLASH_SR >> 16) & 1) == 1);

	*FLASH_KEYR = 0x45670123;
	*FLASH_KEYR = 0xCDEF89AB;

	*FLASH_CR |= 1 << 1;

	*FLASH_CR &= ~(0xF << 3);
	*FLASH_CR |= sector_number << 3;

	*FLASH_CR  |= 1 << 16 ;
	while(((*FLASH_SR >> 16) & 1) == 1);
}
void flash_program(uint8_t *address, uint8_t val){
	uint32_t *FLASH_SR = (uint32_t *)(FLASH_BASE_ADDR + 0x0C);
	uint32_t *FLASH_CR = (uint32_t *)(FLASH_BASE_ADDR + 0x10);
	uint32_t *FLASH_KEYR = (uint32_t *)(FLASH_BASE_ADDR + 0x04);
	while(((*FLASH_SR >> 16) & 1) == 1);

	*FLASH_KEYR = 0x45670123;
	*FLASH_KEYR = 0xCDEF89AB;

	*FLASH_CR |= 1 << 0;

	*address = val;
	while(((*FLASH_SR >> 16) & 1) == 1);
}
int main()
{
	while(1)
	{

	}
}
