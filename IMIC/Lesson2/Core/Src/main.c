#include "main.h"
#define FLASH_BASE_ADDR 0x40023C00

void flash_erase_sector(int sec_num){
	uint32_t *FLASH_SR = (uint32_t *)(FLASH_BASE_ADDR + 0x0C);
	uint32_t *FLASH_CR = (uint32_t *)(FLASH_BASE_ADDR + 0x10);
	uint32_t *FLASH_KEYR = (uint32_t *)(FLASH_BASE_ADDR + 0x04);

	while(((*FLASH_CR) >> 31) == 1){
		*FLASH_KEYR = 0x45670123;
		*FLASH_KEYR = 0xCDEF89AB;
	}
	// Check bit BSY -> busy
	while(((*FLASH_SR >> 16) & 1) == 1);

	// Function erase
	*FLASH_CR |= (1 << 1);

	// Erase the number of sector
	*FLASH_CR &= ~(0b1111 << 3);
	*FLASH_CR |= (sec_num << 3);

	// Start erase
	*FLASH_CR |= ( 1 << 16 );

	while (((*FLASH_SR >> 16) & 1) == 1);

}
void flash_program(uint16_t *addr, uint16_t val){
	uint32_t *FLASH_SR = (uint32_t *)(FLASH_BASE_ADDR + 0x0C);
	uint32_t *FLASH_CR = (uint32_t *)(FLASH_BASE_ADDR + 0x10);
	uint32_t *FLASH_KEYR = (uint32_t *)(FLASH_BASE_ADDR + 0x04);

	while(((*FLASH_CR) >> 31) == 1){
		*FLASH_KEYR = 0x45670123;
		*FLASH_KEYR = 0xCDEF89AB;
	}

	while(((*FLASH_SR >> 16) & 1) == 1);
	*FLASH_CR |= 1 << 0;
	*addr = val;

	while(((*FLASH_SR >> 16) & 1) == 1);
}
int main(){
	flash_erase_sector(1);
	flash_program(0x08004000, 'xe');
	while(1){

	};
	return 0;
}
