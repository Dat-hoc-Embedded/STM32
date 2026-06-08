#include "receiver.h"
#include "board_config.h"
#include "stm32f4xx.h" 

static volatile uint16_t s_commit[RX_PPM_CH_NUM] = {0};
static volatile uint16_t s_frame[RX_PPM_CH_NUM] = {0};
static volatile uint8_t s_idx = 0;
static volatile uint32_t s_last_edge = 0;
static volatile uint32_t s_last_update_ms = 0; 

static inline uint32_t sys_now_ms(void) {return HAL_GetTick();} // return uTick (ms) 

static void Gpio_ppm_af(void)
{
    // Moder: alternate
    RX_PPM_GPIO_EN();
    RX_PPM_GPIO_PORT -> MODER &= ~(3U << (RX_PPM_PIN * 2U));
    RX_PPM_GPIO_PORT -> MODER |=  (2U << (RX_PPM_PIN * 2U));
    // No  pull up - no pull down
    RX_PPM_GPIO_PORT -> PUPDR &= ~(GPIO_PUPDR_PUPD0_Msk);
    // Speed High
    RX_PPM_GPIO_PORT -> OSPEEDR |= (GPIO_SPEED_HIGH << (RX_PPM_PIN * 2U)); 

    // Auto AF select
    if (RX_PPM_PIN < 8){
        RX_PPM_GPIO_PORT -> AFR[0] &= (0xF << (RX_PPM_PIN * 4));
        RX_PPM_GPIO_PORT -> AFR[0] |= ((uint32_t)RX_PPM_AF << (RX_PPM_PIN * 4));
    }else{
        uint32_t pos = (RX_PPM_PIN - 8) * 4;
        RX_PPM_GPIO_PORT -> AFR[1] &= (0xF << pos);
        RX_PPM_GPIO_PORT -> AFR[1] |= ((uint32_t)RX_PPM_AF << pos);
    }
}

void rx_init(void)
{
    // Initialize GPIO: PAO (AF1) + Timer2 (APB1) Clock
    Gpio_ppm_af();
    RX_PPM_TIMER_EN(); 

    // Off timer to config 
    RX_PPM_TIMER -> CR1 &= ~TIM_CR1_CEN;
    
    // Prescaler & ARR (TIM2 là 32 bit): 
        // PSC = fsys/ftick - 1 
    RX_PPM_TIMER -> PSC = (RX_TIMER_CLK_HZ) / (1e6) - 1;   
    RX_PPM_TIMER -> ARR = TIM_ARR_ARR_Msk; // FULL 

    // Input Capture CH1 trên TI1<Channel 1> (CC1S = 01) , filter channel IC1F = 0b11
    RX_PPM_TIMER -> CCMR1 &= (TIM_CCMR1_CC1S_Msk | TIM_CCMR1_IC1F_Msk) ; 
    RX_PPM_TIMER -> CCMR1 |= (1 << TIM_CCMR1_CC1S_Pos);

    // Select Polarity 
    #if RX_PPM_CAPTURE_RISING 
        RX_PPM_TIMER -> CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP); //rising
    #else
        RX_PPM_TIMER -> CCER |= (TIM_CCER_CC1P) ;
        RX_PPM_TIMER -> CCER &= ~(TIM_CCER_CC1NP);
    #endif
    
    // Enable Input Capture & Interrupt 
    RX_PPM_TIMER -> CCER |= TIM_CCER_CC1E; 
    RX_PPM_TIMER -> DIER |= TIM_DIER_CC1IE;

    // NVIC
    NVIC_SetPriority(TIM2_IRQn, 5);
    NVIC_EnableIRQ(TIM2_IRQn);

    // ON TIMER
    RX_PPM_TIMER -> CR1 |= TIM_CR1_CEN; 
    
    // Reset state

}
uint16_t rx_get_ch(uint8_t ch)
{
    if(ch < 1 || ch > RX_PPM_CH_NUM) return 0;
    return s_commit[ch - 1];
}
void rx_get_all_us(uint16_t out[])
{
    for (int i = 0 ; i < RX_PPM_CH_NUM ; i++){
        out[i] = s_commit[i] ; 
    }
}
bool rx_is_failsafe(void){
    return ((sys_now_ms() - s_last_update_ms) > RX_FAILSAFE_TIMEOUT_MS);
}

void TIM2_IRQHANDLER(){
    if(RX_PPM_TIMER -> SR & TIM_SR_CC1IF)
    {
        RX_PPM_TIMER -> SR &= ~(TIM_SR_CC1IF); 
        uint32_t now = RX_PPM_TIMER -> CCR1; 
        uint32_t dt = now - s_last_edge;
        s_last_edge = now; 

        if (dt  >= RX_PPM_SYNC_US)
        {
            s_idx = 0;
        }else {
            if (s_idx < RX_PPM_CH_NUM){
                uint16_t width = (uint16_t)dt;
                if(width >= RX_MIN_US && width <= RX_MAX_US){
                    s_frame[s_idx++] = width;
                    if (s_idx == RX_PPM_CH_NUM){
                        for (int i = 0 ; i < RX_PPM_CH_NUM; i ++){
                            s_commit[i] = s_frame[i];
                            s_last_update_ms = sys_now_ms();
                        }
                    } 
                }else{
                    // Value no normal ->  not get frame 
                    s_idx = 0; 
                }
            }
        }

    }
}