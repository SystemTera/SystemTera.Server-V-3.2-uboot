/*
 * board.h
 *
 * Variscite AM335X SOMs information header
 *
 * Copyright (C) 2013, Variscite, Ltd. - http://www.variscite.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR /PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * We have four pin mux functions that must exist.  We must be able to enable
 * uart0, for initial output and i2c1 to set PMIC and RMII.  We then have a
 * main pinmux function that can be overridden to enable all other pinmux that
 * is required on the board.
 */
void enable_uart0_pin_mux(void);
void enable_i2c1_pin_mux(void);
void enable_rmii1_pin_mux(void);
void enable_board_pin_mux();
#endif
