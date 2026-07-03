#include <main.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"  // GPIO HAL
#include "uart/uart.h"
#include "motor/motor.h"
#include "RC/receiver.h"
#include <stdio.h>

/* ====== CLOCK DEBUG STRUCTURE ====== */
typedef struct {
	uint32_t HCLK;          // AHB Clock (System Clock)
	uint32_t PCLK1;         // APB1 Clock (Timers, UART, I2C, SPI)
	uint32_t PCLK2;         // APB2 Clock (ADC, GPIO, advanced timers)
	uint32_t SystemCoreClock; // SystemCoreClock variable
	uint32_t SysTickClock;  // SysTick frequency
} ClockConfig_t;

/* Global clock variables for debug */
ClockConfig_t current_clocks = {0};
char clock_debug_msg[256] = {0};

void SystemClock_Config()
{
	//  ------------------- SET UP 100MHZ 
    // 1. Turn on HSE: 8 MHz
	RCC -> PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSE; 

	// 2. PLLM = (/)8 ; PLLP = (/)2 ; PLLN = (*)200; (→ 100 MHz)
	RCC -> PLLCFGR &= ~(RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk | RCC_PLLCFGR_PLLP_Msk);
	RCC -> PLLCFGR |= ((8 << RCC_PLLCFGR_PLLM_Pos) | (200 << RCC_PLLCFGR_PLLN_Pos) | (0 << RCC_PLLCFGR_PLLP_Pos));

	// 3. Set wait states (cycles) for Flash (100 MHz @ Vdd >=2.7V → 3WS)
	FLASH -> ACR &= ~FLASH_ACR_LATENCY_Msk;
	FLASH -> ACR |= FLASH_ACR_LATENCY_3WS; 

	// 4. ON HSE & PLL Clock 
	RCC -> CR |= RCC_CR_HSEON;
	while (!(RCC -> CR & RCC_CR_HSERDY)); // WAIT HSERDY = 1 
	RCC -> CR |= RCC_CR_PLLON;
	while (!(RCC -> CR & RCC_CR_PLLRDY)); 

	// 5. SET PRESCALERS FOR AHB, APB1, APB2
	RCC -> CFGR &= ~RCC_CFGR_HPRE; // AHB PRESCALER = /1

	RCC -> CFGR &= ~(RCC_CFGR_PPRE1_Msk); 
	RCC -> CFGR |= RCC_CFGR_PPRE1_DIV2; // APB1 PRESCALER = /2 = 50 MHz

	RCC -> CFGR &= ~RCC_CFGR_PPRE2_Msk; // APB2 PRESCALER = /1
	RCC -> CFGR |= RCC_CFGR_PPRE2_DIV1; // APB2 PRESCALER = /1

	// 6. SWITCH SYSCLK TO PLL 
	RCC -> CFGR &= ~RCC_CFGR_SW; 
	RCC -> CFGR |= RCC_CFGR_SW_PLL;
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

void Clock_Init()
{
	HAL_Init();
	SystemClock_Config(); // SET RCC -> CLOCK = 100 MHz 
	SystemCoreClockUpdate();  // CẬP NHẬT HCLK → SystemCoreClock
	HAL_InitTick(TICK_INT_PRIORITY); // cập nhật lại SysTick theo HCLK mới
	
	//uint32_t PCLK1 = HAL_RCC_GetPCLK1Freq();
}

/* ====== CLOCK DEBUG FUNCTIONS ====== */

/**
 * @brief Kiểm tra Clock Source hiện tại (HSI, HSE, hay PLL)
 * @details Đọc bit SWS trong RCC->CFGR để xác định SYSCLK source
 * @
 * return Clock source string
 */
const char* Clock_GetSource(void)
{
	uint32_t sws = (RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos;
	
	switch (sws) {
		case 0: return "HSI (16 MHz) ⚠️  DEFAULT";  // 00 = HSI
		case 1: return "HSE (8 MHz)";                // 01 = HSE
		case 2: return "PLL (100 MHz) ✅";           // 10 = PLL
		case 3: return "PLLI2S";                     // 11 = PLLI2S
		default: return "Unknown";
	}
}

/**
 * @brief In chi tiết register RCC để thấy cấu hình hiện tại
 */
void Clock_PrintRawRegisters(void)
{
	printf("\n╔════════════════════════════════════════════════════════════╗\n");
	printf("║              RAW RCC REGISTER VALUES                       ║\n");
	printf("╚════════════════════════════════════════════════════════════╝\n\n");
	
	printf("RCC->CR (Control Register):        0x%08lX\n", RCC->CR);
	printf("  └─ HSION (HSI Enable):           %d\n", (RCC->CR & RCC_CR_HSION) ? 1 : 0);
	printf("  └─ HSERDY (HSE Ready):           %d\n", (RCC->CR & RCC_CR_HSERDY) ? 1 : 0);
	printf("  └─ HSEON (HSE Enable):           %d\n", (RCC->CR & RCC_CR_HSEON) ? 1 : 0);
	printf("  └─ PLLON (PLL Enable):           %d\n", (RCC->CR & RCC_CR_PLLON) ? 1 : 0);
	printf("  └─ PLLRDY (PLL Ready):           %d\n", (RCC->CR & RCC_CR_PLLRDY) ? 1 : 0);
	printf("\n");
	
	printf("RCC->CFGR (Config Register):       0x%08lX\n", RCC->CFGR);
	printf("  └─ SWS (Clock Source):           0x%ld (%s)\n", 
		(RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos, Clock_GetSource());
	printf("  └─ HPRE (AHB Prescaler):         %ld\n", (RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos);
	printf("  └─ PPRE1 (APB1 Prescaler):       %ld\n", (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos);
	printf("  └─ PPRE2 (APB2 Prescaler):       %ld\n", (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos);
	printf("\n");
	
	printf("RCC->PLLCFGR (PLL Config):         0x%08lX\n", RCC->PLLCFGR);
	printf("  └─ PLLSRC (PLL Source):          %ld (%s)\n", 
		(RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> RCC_PLLCFGR_PLLSRC_Pos,
		((RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> RCC_PLLCFGR_PLLSRC_Pos) ? "HSE" : "HSI");
	printf("  └─ PLLM (Division):              %ld\n", (RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos);
	printf("  └─ PLLN (Multiplication):        %ld\n", (RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos);
	printf("  └─ PLLP (Division):              %ld (÷%ld)\n", 
		(RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> RCC_PLLCFGR_PLLP_Pos,
		2 * (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> RCC_PLLCFGR_PLLP_Pos) + 1));
	printf("\n");
	
	printf("FLASH->ACR (Flash Control):        0x%08lX\n", FLASH->ACR);
	printf("  └─ LATENCY (Wait States):        %ld WS\n", FLASH->ACR & FLASH_ACR_LATENCY_Msk);
	printf("\n════════════════════════════════════════════════════════════\n\n");
}

/**
 * @brief Cập nhật các clock frequencies vào structure để debug
 * @details Đọc tất cả clock frequences từ HAL/RCC registers
 */
void Clock_UpdateDebugValues(void)
{
	current_clocks.HCLK = HAL_RCC_GetHCLKFreq();        // AHB = 100 MHz
	current_clocks.PCLK1 = HAL_RCC_GetPCLK1Freq();      // APB1 = 50 MHz (÷2)
	current_clocks.PCLK2 = HAL_RCC_GetPCLK2Freq();      // APB2 = 100 MHz (÷1)
	current_clocks.SystemCoreClock = SystemCoreClock;   // System clock variable
	current_clocks.SysTickClock = HAL_RCC_GetSysClockFreq(); // System clock
}

/**
 * @brief In thông tin clock qua UART để debug
 */
void Clock_Debug_Print(void)
{
	Clock_UpdateDebugValues();
	
	printf("\n");
	printf("========== CLOCK CONFIGURATION DEBUG ==========\n");
	printf("HCLK (AHB):         %lu Hz (%lu MHz)\n", current_clocks.HCLK, current_clocks.HCLK / 1000000);
	printf("PCLK1 (APB1):       %lu Hz (%lu MHz)  [÷2 Prescaler]\n", current_clocks.PCLK1, current_clocks.PCLK1 / 1000000);
	printf("PCLK2 (APB2):       %lu Hz (%lu MHz)  [÷1 Prescaler]\n", current_clocks.PCLK2, current_clocks.PCLK2 / 1000000);
	printf("SystemCoreClock:    %lu Hz (%lu MHz)\n", current_clocks.SystemCoreClock, current_clocks.SystemCoreClock / 1000000);
	printf("SysTick Clock:      %lu Hz (%lu MHz)\n", current_clocks.SysTickClock, current_clocks.SysTickClock / 1000000);
	printf("Current Clock Source: %s\n", Clock_GetSource());
	printf("============================================\n\n");
}

/**
 * @brief In chi tiết setup clock - các bước đang diễn ra
 */
void Clock_Setup_Explanation(void)
{
	printf("\n");
	printf("╔════════════════════════════════════════════════════════════╗\n");
	printf("║       CLOCK CONFIGURATION - DETAILED EXPLANATION           ║\n");
	printf("╚════════════════════════════════════════════════════════════╝\n\n");
	
	printf("📌 Step 1: Enable HSE (High-Speed External Oscillator)\n");
	printf("   - HSE Frequency: 8 MHz (External Crystal)\n");
	printf("   - RCC->CR |= RCC_CR_HSEON\n");
	printf("   - Status: HSERDY = %d (Ready)\n\n", (RCC->CR & RCC_CR_HSERDY) ? 1 : 0);
	
	printf("📌 Step 2: Configure PLL (Phase-Locked Loop)\n");
	printf("   - Input: HSE = 8 MHz\n");
	printf("   - PLLM (÷8):  8 MHz / 8 = 1 MHz\n");
	printf("   - PLLN (×200): 1 MHz × 200 = 200 MHz (VCO)\n");
	printf("   - PLLP (÷2):  200 MHz / 2 = 100 MHz ← OUTPUT\n");
	printf("   - Register: PLLCFGR = 0x%08lX\n\n", RCC->PLLCFGR);
	
	printf("📌 Step 3: Set Flash Latency (Wait States)\n");
	printf("   - Clock: 100 MHz @ Vdd >= 2.7V → Need 3 Wait States\n");
	printf("   - FLASH->ACR = 0x%08lX (LATENCY = %ld WS)\n\n", FLASH->ACR, FLASH->ACR & FLASH_ACR_LATENCY_Msk);
	
	printf("📌 Step 4: Enable PLL Clock\n");
	printf("   - RCC->CR |= RCC_CR_PLLON\n");
	printf("   - Status: PLLRDY = %d (Ready)\n\n", (RCC->CR & RCC_CR_PLLRDY) ? 1 : 0);
	
	printf("📌 Step 5: Set Prescalers\n");
	printf("   - AHB Prescaler:  /1  → HCLK = 100 MHz\n");
	printf("   - APB1 Prescaler: /2  → PCLK1 = 50 MHz  (Timers, UART1)\n");
	printf("   - APB2 Prescaler: /1  → PCLK2 = 100 MHz (ADC, GPIO, Timer4)\n");
	printf("   - CFGR = 0x%08lX\n\n", RCC->CFGR);
	
	printf("📌 Step 6: Switch System Clock to PLL\n");
	printf("   - RCC->CFGR |= RCC_CFGR_SW_PLL\n");
	printf("   - Clock Source: PLL (SWS = %ld)\n\n", (RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos);
	
	printf("📌 RESULT: System now running at 100 MHz\n");
	printf("════════════════════════════════════════════════════════════\n\n");
}

/**
 * @brief Kiểm tra: Nếu K cấu hình clock thì sẽ là HSI 16MHz
 * @details Hàm này in ra thông tin gì sẽ xảy ra nếu chỉ gọi HAL_Init() mà không gọi SystemClock_Config()
 */
void Clock_ShowDefaultState(void)
{
	printf("\n");
	printf("╔════════════════════════════════════════════════════════════╗\n");
	printf("║          DEFAULT CLOCK STATE (NẾU K CẤU HÌNH)             ║\n");
	printf("╚════════════════════════════════════════════════════════════╝\n\n");
	
	printf("Nếu CHỈ gọi HAL_Init() mà KHÔNG gọi SystemClock_Config():\n\n");
	
	printf("├─ Clock Source: HSI (High-Speed Internal Oscillator)\n");
	printf("├─ HSI Frequency: 16 MHz (±1 accuracy)\n");
	printf("├─ PLL Status: DISABLED ❌\n");
	printf("├─ System Clock: 16 MHz (direct from HSI)\n");
	printf("└─ Prescalers: /1 (no prescaling)\n\n");
	
	printf("📊 Default Clock Frequencies (WITHOUT Clock_Init):\n");
	printf("┌─────────────────────────────────────────────────────────┐\n");
	printf("│ HCLK (AHB):      16 MHz                                  │\n");
	printf("│ PCLK1 (APB1):    16 MHz                                  │\n");
	printf("│ PCLK2 (APB2):    16 MHz                                  │\n");
	printf("│ SystemCoreClock: 16 MHz                                  │\n");
	printf("└─────────────────────────────────────────────────────────┘\n\n");
	
	printf("⚠️  PROBLEMS Without Clock_Init():\n");
	printf("┌─────────────────────────────────────────────────────────┐\n");
	printf("│ 1. Motor PWM Frequency:                                  │\n");
	printf("│    PSC = (16 MHz / 1 MHz) - 1 = 15                      │\n");
	printf("│    Freq = 16 MHz / 16 / 20000 ≈ 50 Hz ???              │\n");
	printf("│    ❌ WRONG! Prescaler calculation bị sai               │\n");
	printf("│    Actual: ~800 Hz (motor sẽ quay cực nhanh!)           │\n");
	printf("│                                                          │\n");
	printf("│ 2. UART Baud Rate:                                       │\n");
	printf("│    Designed for 50 MHz APB1, but gets 16 MHz           │\n");
	printf("│    ❌ Baud rate không chính xác, garbage data           │\n");
	printf("│                                                          │\n");
	printf("│ 3. Timer Resolution:                                     │\n");
	printf("│    62.5 ns (thay vì 10 ns)                              │\n");
	printf("│    ❌ 6.25x kém chính xác                               │\n");
	printf("│                                                          │\n");
	printf("│ 4. HAL_Delay(1000):                                      │\n");
	printf("│    ❌ Mất ~6.25 giây (thay vì 1 giây)                  │\n");
	printf("│                                                          │\n");
	printf("│ 5. SysTick Interrupt:                                    │\n");
	printf("│    ❌ Chạy chậm 6.25x, interrupt handling bị trễ        │\n");
	printf("└─────────────────────────────────────────────────────────┘\n");
	
	printf("\n🔍 How to Check Current Clock Source in Code:\n");
	printf("┌─────────────────────────────────────────────────────────┐\n");
	printf("│ uint32_t sws = (RCC->CFGR & RCC_CFGR_SWS) >> 2;        │\n");
	printf("│ if (sws == 0) → HSI 16 MHz ⚠️                          │\n");
	printf("│ if (sws == 1) → HSE 8 MHz                               │\n");
	printf("│ if (sws == 2) → PLL 100 MHz ✅                          │\n");
	printf("│                                                          │\n");
	printf("│ Current Value: sws = %ld (%s)                         │\n", 
		(RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos, Clock_GetSource());
	printf("└─────────────────────────────────────────────────────────┘\n");
	printf("════════════════════════════════════════════════════════════\n\n");
}

/**
 * @brief So sánh: Default Clock vs Configured Clock
 */
void Clock_Compare_Default(void)
{
	printf("\n");
	printf("╔════════════════════════════════════════════════════════════╗\n");
	printf("║      DEFAULT CLOCK vs CONFIGURED CLOCK                     ║\n");
	printf("╚════════════════════════════════════════════════════════════╝\n\n");
	
	printf("┌─────────────────────────────────────────────────────────────┐\n");
	printf("│ WITHOUT SystemClock_Config() + Clock_Init()                │\n");
	printf("├─────────────────────────────────────────────────────────────┤\n");
	printf("│ STM32F411 Boot Mode (Internal HSI only):                   │\n");
	printf("│  • HSI Clock: 16 MHz (High-Speed Internal Oscillator)      │\n");
	printf("│  • PLL: DISABLED                                            │\n");
	printf("│  • System Clock: 16 MHz (using HSI directly)               │\n");
	printf("│  • HCLK:   16 MHz                                           │\n");
	printf("│  • PCLK1:  16 MHz (÷1 prescaler)                           │\n");
	printf("│  • PCLK2:  16 MHz (÷1 prescaler)                           │\n");
	printf("│                                                             │\n");
	printf("│ ⚠️  PROBLEMS:                                              │\n");
	printf("│  • Timer PWM frequency: 16 MHz / 1MHz = 16µs resolution    │\n");
	printf("│  • UART baud rates: Not accurate (timing based on 16MHz)   │\n");
	printf("│  • Motor PWM: WRONG frequency (not 50Hz anymore!)          │\n");
	printf("│  • SysTick: Running slower (1.6x slower)                  │\n");
	printf("└─────────────────────────────────────────────────────────────┘\n\n");
	
	printf("┌─────────────────────────────────────────────────────────────┐\n");
	printf("│ WITH SystemClock_Config() + Clock_Init() ✅               │\n");
	printf("├─────────────────────────────────────────────────────────────┤\n");
	printf("│ STM32F411 Optimized Mode (PLL from HSE):                   │\n");
	printf("│  • HSE Clock:  8 MHz (External Crystal)                    │\n");
	printf("│  • PLL Output: 100 MHz                                      │\n");
	printf("│  • System Clock: 100 MHz (using PLL)                       │\n");
	printf("│  • HCLK:  100 MHz                                           │\n");
	printf("│  • PCLK1: 50 MHz (÷2 prescaler)                            │\n");
	printf("│  • PCLK2: 100 MHz (÷1 prescaler)                           │\n");
	printf("│                                                             │\n");
	printf("│ ✅ BENEFITS:                                               │\n");
	printf("│  • Timer PWM frequency: 100 MHz / 1MHz = 1µs resolution    │\n");
	printf("│  • UART baud rates: Accurate (115200 baud works perfectly) │\n");
	printf("│  • Motor PWM: 50 Hz frequency ✅ (ESC compatible)          │\n");
	printf("│  • SysTick: Running at full speed (100x faster timing)     │\n");
	printf("│  • Performance: 6.25x faster than HSI mode                 │\n");
	printf("└─────────────────────────────────────────────────────────────┘\n\n");
}
volatile uint32_t systick = 0;

/* ====== LED CONTROL FUNCTIONS ====== */

/**
 * @brief Khởi tạo GPIO cho LED
 * @details Cấu hình GPIO pin như output cho LED
 * @note Cần định nghĩa LED_GPIO_PORT, LED_PIN trong board_config.h
 *       Hoặc thay đổi theo pin thực tế của bạn
 */
void LED_Init(void)
{
	/* GPIOB Clock Enable */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	/* Cấu hình Pin PB5 (hoặc thay đổi theo pin LED của bạn) */
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_5;           // ← CHANGE THIS to your LED pin
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // Output Push-Pull
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);     // ← CHANGE THIS to your LED GPIO PORT
	
	/* Set LED OFF initially */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);  // HIGH = OFF
}

/**
 * @brief Toggle LED (bật/tắt)
 */
void LED_Toggle(void)
{
	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);  // ← CHANGE according to your LED pin
}

/**
 * @brief Bật LED
 */
void LED_On(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);  // LOW = ON
}

/**
 * @brief Tắt LED
 */
void LED_Off(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);    // HIGH = OFF
}
int main(){
	Clock_Init();
	// UART1_INIT();
	// LED_Init();  // Initialize LED

	/* ====== DEBUG: Print Clock Configuration ====== */
	// Clock_Debug_Print();              // In tất cả clock frequencies hiện tại
	// Clock_PrintRawRegisters();        // In raw RCC register values
	// Clock_ShowDefaultState();         // In thông tin nếu không cấu hình
	// Clock_Setup_Explanation();        // In chi tiết setup đang diễn ra
	// Clock_Compare_Default();          // So sánh default vs configured clock

	/* -------------- Initialize Motor ----------*/ 
	motor_pwm_init();
	for (volatile int i = 0; i < 2e6 ; i ++);

	/* -------------- Initialize RX ------------*/
	// rx_init();

	while (1){

		motor_pwm_set(1, 1200);
			};

}
