#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "stm32f4xx.h"

/* ------------- 1.  PWM OUTPUT COMPARE FOR ESC
    - Document: Datasheet tr44
    - Choices: Use TIM4, CH1 -> CH4 ~ PB6 -> PB9  */
#define MOTOR_TIMER TIM4
#define MOTOR_TIMER_CLK_EN() (RCC -> APB1ENR |= RCC_APB1ENR_TIM4EN)

#define MOTOR_GPIO_PORT GPIOB 
#define MOTOR_GPIO_CLK_ENB() (RCC-> AHB1ENR |= RCC_AHB1ENR_GPIOBEN)

    // Channel mapping 
#define MOTOR1_PIN 6 // PB6
#define MOTOR1_AF 2 // AF2 : TIM4_CH1
#define MOTOR1_SPEED (MOTOR_TIMER -> CCR1)

#define MOTOR2_PIN 7 // PB7
#define MOTOR2_AF 2 // AF2 : TIM4_CH1
#define MOTOR2_SPEED (MOTOR_TIMER -> CCR2)

#define MOTOR3_PIN 8 // PB8
#define MOTOR3_AF 2 // AF2 : TIM4_CH1
#define MOTOR3_SPEED (MOTOR_TIMER -> CCR3)

#define MOTOR4_PIN 9 // PB9
#define MOTOR4_AF 2 // AF2 : TIM4_CH1
#define MOTOR4_SPEED (MOTOR_TIMER -> CCR4)

    // -------- PWM PARAMETERS --------
#define PWM_FREQUENCY_HZ 50         // 50Hz cho ESC
#define PWM_MIN_US       1000
#define PWM_MAX_US       2000
#define PWM_INIT_US      1500

/* ------------- 2.  PWM INPUT CAPTURE FROM RECEIVER ------------ 
    
*/
    // Set Timer 2 -> PA0 
#define RX_TIMER_CLK_HZ  100e6  // 100 MHz
#define RX_PPM_TIMER TIM2
#define RX_PPM_TIMER_EN() (RCC->APB1ENR |= RCC_APB1ENR_TIM2EN)

#define RX_PPM_GPIO_PORT GPIOA
#define RX_PPM_GPIO_EN() (RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN)
#define RX_PPM_PIN 0U // PA0 
#define RX_PPM_AF 1   // AF1 cho TIM2_CH1

    // Channel & sync 
#define RX_PPM_CH_NUM 8             // * channel 
#define RX_PPM_SYNC_US 4000U        // >4ms is considered sync threshold -> frame reset 
#define RX_FAILSAFE_TIMEOUT_MS 50U  // Loss signal if >50ms system don't update

#define RX_PPM_CAPTURE_RISING 1 // Edge mode capture : 1 = rising, 0 = falling 
#define RX_MIN_US 900
#define RX_MAX_US 2100

#endif  /* BOARD_CONFIG_H */