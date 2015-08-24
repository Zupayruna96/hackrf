/*
 * Copyright 2012 Michael Ossmann
 * Copyright (C) 2012 Jared Boone
 *
 * This file is part of HackRF.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/sgpio.h>

#include <hackrf_core.h>
#include <rf_path.h>
#include <sgpio.h>
#include <tuning.h>

void tx_test() {
	// LSB goes out first, samples are 0x<Q1><I1><Q0><I0>
	volatile uint32_t buffer[] = {
		0xda808080,
		0xda80ff80,
		0x26808080,
		0x26800180,
	};
	uint32_t i = 0;

    rf_path_set_direction(RF_PATH_DIRECTION_TX);
	sgpio_cpld_stream_enable();
	
	while(true) {
		while(SGPIO_STATUS_1 == 0);
		SGPIO_REG_SS(SGPIO_SLICE_A) = buffer[(i++) & 3];
		SGPIO_CLR_STATUS_1 = 1;
	}
}

void rx_test() {
    volatile uint32_t buffer[4096];
    uint32_t i = 0;
	uint32_t magsq;
	int8_t sigi, sigq;

    rf_path_set_direction(RF_PATH_DIRECTION_RX);
    sgpio_cpld_stream_enable();

	gpio_set(PORT_LED1_3, (PIN_LED2)); /* LED2 on */
    while(true) {
        while(SGPIO_STATUS_1 == 0);
		gpio_set(PORT_LED1_3, (PIN_LED1)); /* LED1 on */
        SGPIO_CLR_STATUS_1 = 1;
        buffer[i & 4095] = SGPIO_REG_SS(SGPIO_SLICE_A);

		/* find the magnitude squared */
		sigi = buffer[i & 4095] & 0xff;
		sigq = (buffer[i & 4095] >> 8) & 0xff;
		magsq = sigi * sigi + sigq * sigq;
		
		/* illuminate LED3 only when magsq exceeds threshold */
		if (magsq > 0x1000)
			gpio_set(PORT_LED1_3, (PIN_LED3)); /* LED3 on */
		else
			gpio_clear(PORT_LED1_3, (PIN_LED3)); /* LED3 off */
		i++;
    }
}

int main(void) {

	const uint64_t freq = 2441000000U;

	sgpio_set_slice_mode(false);
	pin_setup();
	enable_1v8_power();
#ifdef HACKRF_ONE
	enable_rf_power();
#endif
	cpu_clock_init();
    ssp1_init();
    rf_path_init();

	set_freq(freq);

	rx_test();
	gpio_set(PORT_LED1_3, (PIN_LED2)); /* LED2 on */

	while (1) {

	}

	return 0;
}
