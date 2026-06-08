#ifndef UART_H 
#define UART_H


/* ------------------------ UART */
#define GPIOB_BASE_ADDR 0x40020400
#define UART1_BASE_ADDR 0x40011000

void UART1_INIT(void);
void UART_SEND(char data);
void my_printf(char *,...);

#endif