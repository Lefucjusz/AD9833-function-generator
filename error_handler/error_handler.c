#include "error_handler.h"
#include <hd44780.h>
#include <delay.h>
#include <ch32v00x.h>

void error_handler(void)
{
    /* Stop servicing interrups */
    __disable_irq();

    /* Reset the chip */
    NVIC_SystemReset();

    /* We should never reach this point */
    __asm__ __volatile__("j .\n");
}

void error_handler_message(const char *msg)
{
    if (msg != NULL) {
        hd44780_gotoxy(1, 1);
        hd44780_write_string(msg);
    }

    /* Let the user see the message */
    delay_ms(2500);

    error_handler();
}
