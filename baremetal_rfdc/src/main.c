/*
 * RFSoC4x2 bare-metal bring-up: clock chips over PS SPI0, then the RF data converter.
 *   1. LMK04828 (245.76 MHz) and both LMX2594 (491.52 MHz) programmed with write_clk()
 *   2. DAC tile 228 / ADC tile 226 started, tile state and PLL lock printed
 *   3. fine mixers set to the carrier (DAC +f, ADC -f)
 * UART1 115200: c = reprogram clocks and restart the tiles, +/- = carrier +/-100 MHz.
 * Copyright (c) 2026, Yijie Yu. BSD-3-Clause.
 */

#include "xparameters.h"
#include "xil_printf.h"
#include "xuartps_hw.h"
#include "sleep.h"
#include "LMK_LMX.h"
#include "rfdc_util.h"

static int carrier_mhz = 600;

static void bring_up(void)
{
    xil_printf("Programming LMK04828 / LMX2594 over SPI0...\r\n");
    write_clk(0);       /* LMK04828 */
    write_clk(1);       /* LMX2594 */
    write_clk(2);       /* LMX2594 */
    sleep(1);

    if (rfdc_init())
        xil_printf("RF tiles not ready: LMK PLL1 may still be settling after power-on, press 'c'\r\n");
    rfdc_set_carrier(carrier_mhz);
}

int main(void)
{
    xil_printf("\r\n==== RFSoC4x2 clock + RFDC bring-up ====\r\n");
    bring_up();
    xil_printf("keys: c reprogram clocks | + / - carrier +/-100 MHz\r\n");

    for (;;) {
        if (!XUartPs_IsReceiveData(STDIN_BASEADDRESS))
            continue;
        switch (XUartPs_ReadReg(STDIN_BASEADDRESS, XUARTPS_FIFO_OFFSET)) {
        case 'c': bring_up(); break;
        case '+': carrier_mhz += 100; rfdc_set_carrier(carrier_mhz); break;
        case '-': carrier_mhz -= 100; rfdc_set_carrier(carrier_mhz); break;
        default: break;
        }
    }
    return 0;
}
