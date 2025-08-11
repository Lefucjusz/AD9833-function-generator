#include "delay.h"
#include <ch32v00x.h>

/* Register bit definitions from RM */
#define SYSTICK_CTRL_STE_BIT (1 << 0)
#define SYSTICK_CTRL_STIE_BIT (1 << 1)
#define SYSTICK_CTRL_STCLK_BIT (1 << 2)
#define SYSTICK_CTRL_STRE_BIT (1 << 3)
#define SYSTICK_CTRL_SWIE_BIT (1 << 31)
#define SYSTICK_SR_CNTIF_BIT (1 << 0)

#define SYSTICK_IRQ_FREQ_HZ 1000

static volatile uint32_t ticks;

void delay_init(void)
{
    /* Set SysTick to generate interrupt every 1ms */
    SysTick->CNT = 0; // Clear counter
    SysTick->CMP = (SystemCoreClock / SYSTICK_IRQ_FREQ_HZ) - 1;
    SysTick->CTLR = SYSTICK_CTRL_STE_BIT | SYSTICK_CTRL_STIE_BIT | SYSTICK_CTRL_STCLK_BIT | SYSTICK_CTRL_STRE_BIT; // Start Systick, enable interrupt, clock = HCLK, enable auto-reload

    /* Enable SysTick IRQ in NVIC */
    NVIC_EnableIRQ(SysTicK_IRQn);
}

void delay_ms(uint32_t ms)
{
    const uint32_t start_tick = delay_get_ticks();
    while ((delay_get_ticks() - start_tick) <= ms) {
        continue;
    }
}

uint32_t delay_get_ticks(void)
{
    return ticks;
}

void SysTick_Handler(void)
{
    ++ticks;
    SysTick->SR = 0; // Clear comparison flag
}
