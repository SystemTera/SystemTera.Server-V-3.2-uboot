/*
 * mux.c
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/hardware.h>
#include <asm/arch/mux.h>
#include <asm/io.h>
#include <i2c.h>
#include "board.h"

static struct module_pin_mux rev_pin_mux[] = {
	{OFFSET(lcd_data7), (MODE(7) | PULLUDEN | RXACTIVE)},
	{OFFSET(lcd_vsync), (MODE(7) | PULLUDEN | RXACTIVE)},
	{OFFSET(lcd_data5), (MODE(7) | PULLUDEN | RXACTIVE)},
	{-1},
};

static struct module_pin_mux uart0_pin_mux[] = {
	{OFFSET(uart0_rxd), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* UART0_RXD */
	{OFFSET(uart0_txd), (MODE(0) | PULLUDEN)},		        /* UART0_TXD */
	{-1},
};

static struct module_pin_mux uart3_pin_mux[] = {
	{OFFSET(spi0_cs1), (MODE(1) | PULLUDEN | RXACTIVE)},	/* UART3_RXD */
	{OFFSET(ecap0_in_pwm0_out), (MODE(1) | PULLUDEN)},	    /* UART3_TXD */
	{-1},
};

static struct module_pin_mux gpio0_2_pin_mux[] = {
	{OFFSET(spi0_sclk), MODE(7) | PULLUDEN},	   /* GPIO0_2, LCD Backlight */
	{-1},
};

static struct module_pin_mux nand_pin_mux[] = {
	{OFFSET(gpmc_ad0), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD0 */
	{OFFSET(gpmc_ad1), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD1 */
	{OFFSET(gpmc_ad2), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD2 */
	{OFFSET(gpmc_ad3), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD3 */
	{OFFSET(gpmc_ad4), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD4 */
	{OFFSET(gpmc_ad5), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD5 */
	{OFFSET(gpmc_ad6), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD6 */
	{OFFSET(gpmc_ad7), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD7 */
	{OFFSET(gpmc_wait0), (MODE(0) | RXACTIVE | PULLUP_EN)}, /* NAND WAIT */
	{OFFSET(gpmc_wpn), (MODE(7) | PULLUP_EN | RXACTIVE)},	/* NAND_WPN */
	{OFFSET(gpmc_csn0), (MODE(0) | PULLUDEN)},	/* NAND_CS0 */
	{OFFSET(gpmc_advn_ale), (MODE(0) | PULLUDEN)}, /* NAND_ADV_ALE */
	{OFFSET(gpmc_oen_ren), (MODE(0) | PULLUDEN)},	/* NAND_OE */
	{OFFSET(gpmc_wen), (MODE(0) | PULLUDEN)},	/* NAND_WEN */
	{OFFSET(gpmc_be0n_cle), (MODE(0) | PULLUDEN)},	/* NAND_BE_CLE */
	{-1},
};

static struct module_pin_mux i2c1_pin_mux[] = {
	{OFFSET(spi0_d1), (MODE(2) | RXACTIVE | PULLUDEN | SLEWCTRL)},	/* I2C_DATA */
	{OFFSET(spi0_cs0), (MODE(2) | RXACTIVE | PULLUDEN | SLEWCTRL)},	/* I2C_SCLK */
	{-1},
};

static struct module_pin_mux gpio2_19_pin_mux[] = {
   {OFFSET(mii1_rxd2), MODE(7) | PULLUDEN},	   /* GPIO2_19, PHY resrt */
   {-1},
};

static struct module_pin_mux clkout_pin_mux[] = {
   {OFFSET(xdma_event_intr0), MODE(3)},	   /* clkout1 */
   {-1},
};

static struct module_pin_mux bt_uart_select_pin_mux[] = {
   {OFFSET(xdma_event_intr1), MODE(7)},	   /* GPIO0_20 */
   {-1},
};

static struct module_pin_mux rmii1_pin_mux[] = {
   {OFFSET(mii1_crs), MODE(1) | RXACTIVE},     /* RMII1_CRS */
   {OFFSET(mii1_rxerr), MODE(1) | RXACTIVE},   /* RMII1_RXERR */
   {OFFSET(mii1_txen), MODE(1)},               /* RMII1_TXEN */
   {OFFSET(mii1_txd1), MODE(1)},               /* RMII1_TXD1 */
   {OFFSET(mii1_txd0), MODE(1)},               /* RMII1_TXD0 */
   {OFFSET(mii1_rxd1), MODE(1) | RXACTIVE},    /* RMII1_RXD1 */
   {OFFSET(mii1_rxd0), MODE(1) | RXACTIVE},    /* RMII1_RXD0 */
   {OFFSET(mdio_data), MODE(0) | RXACTIVE | PULLUP_EN}, /* MDIO_DATA */
   {OFFSET(mdio_clk), MODE(0) | PULLUP_EN},    /* MDIO_CLK */
   {OFFSET(rmii1_refclk), MODE(0) | RXACTIVE}, /* RMII1_REFCLK */
   {-1},
};

static struct module_pin_mux mmc0_pin_mux[] = {
   {OFFSET(mmc0_dat3), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT3 */
   {OFFSET(mmc0_dat2), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT2 */
   {OFFSET(mmc0_dat1), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT1 */
   {OFFSET(mmc0_dat0), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT0 */
   {OFFSET(mmc0_clk), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_CLK */
   {OFFSET(mmc0_cmd), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_CMD */
   {OFFSET(gpmc_be1n), (MODE(5) | RXACTIVE | PULLUP_EN)},	/* MMC0_CD */
   {-1},
};

/* SystemTera.Server.V specific pin mux for KNX bus interface */
static struct module_pin_mux knx_pin_mux[] = {
   {OFFSET(uart1_rxd), MODE(0) | RXACTIVE | PULLUP_EN},
   {OFFSET(uart1_txd), MODE(0) | PULLUDEN},
   {OFFSET(mii1_rxclk), MODE(7) | RXACTIVE | PULLUDEN},    /* NCN5120 nSave signal */
   {OFFSET(mii1_txd3), MODE(7)},                           /* KNX programming mode led */
   {OFFSET(mii1_txd2), MODE(7) | RXACTIVE | PULLUDEN},     /* KNX programming mode button */
   {-1},
};

/* SystemTera.Server.V specific pin mux for digital inputs */
static struct module_pin_mux digital_in_pin_mux[] = {
   {OFFSET(gpmc_a4), MODE(7) | RXACTIVE | PULLUDEN},   /* IN1 */
   {OFFSET(gpmc_a2), MODE(7) | RXACTIVE | PULLUDEN},   /* IN2 */
   {OFFSET(gpmc_a0), MODE(7) | RXACTIVE | PULLUDEN},   /* IN3 */
   {OFFSET(gpmc_a6), MODE(7) | RXACTIVE | PULLUDEN},   /* IN4 */
   {OFFSET(gpmc_a8), MODE(7) | RXACTIVE | PULLUDEN},   /* IN5 */
   {OFFSET(gpmc_a10), MODE(7) | RXACTIVE | PULLUDEN},  /* IN6 */
   {OFFSET(gpmc_a7), MODE(7) | RXACTIVE | PULLUDEN},   /* IN7 */
   {OFFSET(gpmc_a5), MODE(7) | RXACTIVE | PULLUDEN},   /* IN8 */
   {-1},
};

/* SystemTera.Server.V specific pin mux for relais outputs */
static struct module_pin_mux digital_out_pin_mux[] = {
   {OFFSET(gpmc_a9), MODE(7)},   /* OUT1 */
   {OFFSET(gpmc_a11), MODE(7)},  /* OUT2 */
   {-1},
};

/* SystemTera.Server.V specific pin mux for EIA-485 half-duplex uart */
static struct module_pin_mux eia485_pin_mux[] = {
   {OFFSET(emu1), MODE(7)},  /* TX enable */
   {-1},
};

/* SystemTera.Server.V specific pin mux for general purpose red/green leds */
static struct module_pin_mux gpled_pin_mux[] = {
   {OFFSET(lcd_ac_bias_en), MODE(7)},  /* LED 1 red */
   {OFFSET(gpmc_a3), MODE(7)},         /* LED 1 green */
   {OFFSET(gpmc_a1), MODE(7)},         /* LED 2 green */
   {OFFSET(lcd_hsync), MODE(7)},       /* LED 2 red */
   {OFFSET(lcd_pclk), MODE(7)},        /* LED 3 green */
   {OFFSET(gpmc_csn3), MODE(7)},       /* LED 3 red */
   {-1},
};

/* SystemTera.Server.V specific pin mux for miscellaneous functions */
static struct module_pin_mux misc_pin_mux[] = {
   {OFFSET(emu0), MODE(7)},  /* optional switch */
   {-1},
};

void enable_uart0_pin_mux(void)
{
	configure_module_pin_mux(uart0_pin_mux);
}

void enable_uart3_pin_mux(void)
{
	configure_module_pin_mux(uart3_pin_mux);
}

void enable_i2c1_pin_mux(void)
{
	configure_module_pin_mux(i2c1_pin_mux);
}


void enable_rmii1_pin_mux(void)
{
	configure_module_pin_mux(rmii1_pin_mux);
}

void enable_board_pin_mux()
{
	/* Do board-specific muxes. */
	configure_module_pin_mux(rev_pin_mux);
	configure_module_pin_mux(i2c1_pin_mux);
	configure_module_pin_mux(mmc0_pin_mux);
	configure_module_pin_mux(nand_pin_mux);
	configure_module_pin_mux(gpio0_2_pin_mux);
	configure_module_pin_mux(gpio2_19_pin_mux);
	configure_module_pin_mux(clkout_pin_mux);
	configure_module_pin_mux(bt_uart_select_pin_mux);

	/* SystemTera.Server.V specific pinmuxes */
	configure_module_pin_mux(knx_pin_mux);
	configure_module_pin_mux(digital_in_pin_mux);
	configure_module_pin_mux(digital_out_pin_mux);
	configure_module_pin_mux(eia485_pin_mux);
	configure_module_pin_mux(gpled_pin_mux);
	configure_module_pin_mux(misc_pin_mux);
}
