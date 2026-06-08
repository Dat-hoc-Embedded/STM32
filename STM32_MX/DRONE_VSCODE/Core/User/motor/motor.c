#include "motor.h"

static void gpio_pwm_init(void){
    /* 0. Cấp Clock cho GPIOB */
    MOTOR_GPIO_CLK_ENB(); 

    /* 1. Set Mode Alternate function cho PB6 -> PB9 */
    MOTOR_GPIO_PORT -> MODER &= ~((3 << (MOTOR1_PIN * 2)) | 
                                  (3 << (MOTOR2_PIN * 2)) | 
                                  (3 << (MOTOR3_PIN * 2)) |
                                  (3 << (MOTOR4_PIN * 2)) );

    MOTOR_GPIO_PORT -> MODER |= ((2 << (MOTOR1_PIN * 2)) | (2 << (MOTOR2_PIN * 2)) |
                                 (2 << (MOTOR3_PIN * 2)) | (2 << (MOTOR4_PIN * 2)) );
    
    /* 2. SET AF2 Cho PB6 -> PB9 */
    MOTOR_GPIO_PORT -> AFR[0] |= ((MOTOR1_AF << (MOTOR1_PIN * 4)) | (MOTOR2_AF << (MOTOR2_PIN * 4)));
    MOTOR_GPIO_PORT -> AFR[1] |= ((MOTOR3_AF << ((MOTOR3_PIN - 8) * 4)) | (MOTOR3_AF << ((MOTOR4_PIN - 8)  * 4)));
    
}
void motor_pwm_init(void)
{
    // Enable timer clock 
    MOTOR_TIMER_CLK_EN();
    gpio_pwm_init();

    /* 1. Tính toán PSC & ARR: 1 tick = 1us (1 MHz) , Period = 20ms (50 Hz)
        Mục tiêu 1us để đạt range cho ESC là 1000 us -> 2000 us             */
    uint32_t F_sys = HAL_RCC_GetSysClockFreq(); // Phụ thuộc vào SystemClock_Config () là 100 MHz
    MOTOR_TIMER -> PSC = (F_sys / (1e6)) - 1; // 100 - 1 = 99 
    MOTOR_TIMER -> ARR = (1e6 / PWM_FREQUENCY_HZ)- 1; // 20000 - 1 = 19999

    /* 2. Config CCMR (Compare/Capture Mode) 
        CCMR1: OC1M[4:6] = 110, OC1PE[3] = 1, OC2M[12:15] = 110, OC2PE[11] = 1 
        CCMR2: OC3M[4:6], OC3PE[3] , OC4M[12:15], OC4PE[11] 
    */
    MOTOR_TIMER -> CCMR1 |= ((6 << TIM_CCMR1_OC1M_Pos) | (1 << TIM_CCMR1_OC1PE_Pos) |
                            (6 << TIM_CCMR1_OC2M_Pos) | (1 << TIM_CCMR1_OC2PE_Pos) ); 

    MOTOR_TIMER -> CCMR2 |= ((6 << TIM_CCMR2_OC3M_Pos) | (1 << TIM_CCMR2_OC3PE_Pos) |
                            (6 << TIM_CCMR2_OC4M_Pos) | (1 << TIM_CCMR2_OC4PE_Pos) ); 

    MOTOR_TIMER -> CCER |= ((1 << TIM_CCER_CC1E_Pos) | (1 << TIM_CCER_CC2E_Pos) |
                           (1 << TIM_CCER_CC3E_Pos) | (1 << TIM_CCER_CC4E_Pos) );
    MOTOR_TIMER -> CR1 |= TIM_CR1_ARPE ;
    MOTOR_TIMER -> EGR |= TIM_EGR_BG ; 

    MOTOR1_SPEED = PWM_MIN_US;
    MOTOR2_SPEED = PWM_MIN_US;
    MOTOR3_SPEED = PWM_MIN_US;
    MOTOR4_SPEED = PWM_MIN_US;

    MOTOR_TIMER -> CR1 |= TIM_CR1_CEN;
}
void motor_pwm_set(uint8_t motor_id, uint16_t pulse_us)
{
    if (pulse_us > PWM_MAX_US) pulse_us = PWM_MAX_US;
    if (pulse_us < PWM_MIN_US) pulse_us = PWM_MIN_US;

    switch (motor_id)
    {
        case 1: MOTOR1_SPEED = pulse_us; break;
        case 2: MOTOR2_SPEED = pulse_us; break;
        case 3: MOTOR3_SPEED = pulse_us; break;
        case 4: MOTOR4_SPEED = pulse_us; break;
        default: break;
    }
}