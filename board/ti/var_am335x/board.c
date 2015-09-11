/*
 * board.c
 *
 * Board functions for TI AM335X based boards
 *
 * Copyright (C) 2011, Texas Instruments, Incorporated - http://www.ti.com/
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

#include <common.h>
#include <errno.h>
#include <spl.h>
#include <asm/arch/cpu.h>
#include <asm/arch/hardware.h>
#include <asm/arch/omap.h>
#include <asm/arch/ddr_defs.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc_host_def.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <asm/emif.h>
#include <asm/gpio.h>
#include <i2c.h>
#include <miiphy.h>
#include <cpsw.h>
#include "board.h"
#include "pmic.h"

#define GPIO_LCD_BACKLIGHT     2
#define GPIO_BT_UART_SELECT    20
#define GPIO_SOM_REV_BIT0_GPIO 77
#define GPIO_SOM_REV_BIT1_GPIO 86
#define GPIO_SOM_REV_BIT2_GPIO 75
#define GPIO_PHY1_RST          83

DECLARE_GLOBAL_DATA_PTR;

static int som_rev = (-1);

static struct wd_timer *wdtimer = (struct wd_timer *)WDT_BASE;
#if defined(CONFIG_SPL_BUILD)
static struct uart_sys *uart_base = (struct uart_sys *)DEFAULT_UART_BASE;
#endif

/* MII mode defines */
#define RMII_MODE_ENABLE	0x5

static struct ctrl_dev *cdev = (struct ctrl_dev *)CTRL_DEVICE_BASE;

/* UART Defines */
#if defined(CONFIG_SPL_BUILD)
/*
 * voltage switching for MPU frequency switching.
 * @module = mpu - 0, core - 1
 * @vddx_op_vol_sel = vdd voltage to set
 */

#define MPU     0
#define CORE    1

int voltage_update(unsigned int module, unsigned char vddx_op_vol_sel)
{
	uchar buf[4];
	unsigned int reg_offset;

	if(module == MPU)
		reg_offset = PMIC_VDD1_OP_REG;
	else
		reg_offset = PMIC_VDD2_OP_REG;

	/* Select VDDx OP   */
	if (i2c_read(PMIC_CTRL_I2C_ADDR, reg_offset, 1, buf, 1))
		return 1;

	buf[0] &= ~PMIC_OP_REG_CMD_MASK;

	if (i2c_write(PMIC_CTRL_I2C_ADDR, reg_offset, 1, buf, 1))
		return 1;

	/* Configure VDDx OP  Voltage */
	if (i2c_read(PMIC_CTRL_I2C_ADDR, reg_offset, 1, buf, 1))
		return 1;

	buf[0] &= ~PMIC_OP_REG_SEL_MASK;
	buf[0] |= vddx_op_vol_sel;

	if (i2c_write(PMIC_CTRL_I2C_ADDR, reg_offset, 1, buf, 1))
		return 1;

	if (i2c_read(PMIC_CTRL_I2C_ADDR, reg_offset, 1, buf, 1))
		return 1;

	if ((buf[0] & PMIC_OP_REG_SEL_MASK ) != vddx_op_vol_sel)
		return 1;

	return 0;
}

#define UART_RESET		(0x1 << 1)
#define UART_CLK_RUNNING_MASK	0x1
#define UART_SMART_IDLE_EN	(0x1 << 0x3)

static void rtc32k_enable(void)
{
	struct rtc_regs *rtc = (struct rtc_regs *)AM335X_RTC_BASE;

	/*
	 * Unlock the RTC's registers.  For more details please see the
	 * RTC_SS section of the TRM.  In order to unlock we need to
	 * write these specific values (keys) in this order.
	 */
	writel(0x83e70b13, &rtc->kick0r);
	writel(0x95a4f1e0, &rtc->kick1r);

	/* Enable the RTC 32K OSC by setting bits 3 and 6. */
	writel((1 << 3) | (1 << 6), &rtc->osc);
}

static const struct ddr_data ddr3_var_am33x_data = {
	.datardsratio0 = MT41K256M16HA125E_RD_DQS,
 	.datawdsratio0 = MT41K256M16HA125E_WR_DQS,
	.datafwsratio0 = MT41K256M16HA125E_PHY_FIFO_WE,
	.datawrsratio0 = MT41K256M16HA125E_PHY_WR_DATA,
	.datadldiff0 = PHY_DLL_LOCK_DIFF,
};

static const struct cmd_control ddr3_var_am33x_cmd_ctrl_data = {
	.cmd0csratio = MT41K256M16HA125E_RATIO,
	.cmd0dldiff = MT41K256M16HA125E_DLL_LOCK_DIFF,
	.cmd0iclkout = MT41K256M16HA125E_INVERT_CLKOUT,

	.cmd1csratio = MT41K256M16HA125E_RATIO,
	.cmd1dldiff = MT41K256M16HA125E_DLL_LOCK_DIFF,
	.cmd1iclkout = MT41K256M16HA125E_INVERT_CLKOUT,

	.cmd2csratio = MT41K256M16HA125E_RATIO,
	.cmd2dldiff = MT41K256M16HA125E_DLL_LOCK_DIFF,
	.cmd2iclkout = MT41K256M16HA125E_INVERT_CLKOUT,
};

static struct emif_regs ddr3_var_am33x_emif_reg_data = {
	.sdram_config = MT41K256M16HA125E_EMIF_SDCFG,
	.ref_ctrl = MT41K256M16HA125E_EMIF_SDREF,
	.sdram_tim1 = MT41K256M16HA125E_EMIF_TIM1,
	.sdram_tim2 = MT41K256M16HA125E_EMIF_TIM2,
	.sdram_tim3 = MT41K256M16HA125E_EMIF_TIM3,
	.zq_config = MT41K256M16HA125E_ZQ_CFG,
	.emif_ddr_phy_ctlr_1 = MT41K256M16HA125E_EMIF_READ_LATENCY,
};

void am33xx_spl_board_init(void)
{
	int mpu_vdd, mpu_pll, sil_rev;
	uchar buf[4];

	/* Assume PG 1.0 */
	mpu_pll = MPUPLL_M_720;

	sil_rev = readl(&cdev->deviceid) >> 28;
	if (sil_rev == 1)
		/* PG 2.0, efuse may not be set. */
		mpu_pll = MPUPLL_M_800;
	else if (sil_rev >= 2) {
		/* Check what the efuse says our max speed is. */
		int efuse_arm_mpu_max_freq;
		efuse_arm_mpu_max_freq = readl(&cdev->efuse_sma);
		if ((efuse_arm_mpu_max_freq & DEVICE_ID_MASK) ==
				AM335X_ZCZ_1000)
			mpu_pll = MPUPLL_M_1000;
		else if ((efuse_arm_mpu_max_freq & DEVICE_ID_MASK) ==
				AM335X_ZCZ_800)
			mpu_pll = MPUPLL_M_800;
	}

	/*
	 * The VAR-AM33-SOM use a TPS65910 PMIC.  For all
	 * MPU frequencies we support we use a CORE voltage of
	 * 1.1375V.  For 1GHz we need to use an MPU voltage of
	 * 1.3250V and for 720MHz or 800MHz we use 1.2625V.
	 */
	if (i2c_probe(PMIC_CTRL_I2C_ADDR))
		return;

	/* VDD1/2 voltage selection register access by control i/f */
	if (i2c_read(PMIC_CTRL_I2C_ADDR, PMIC_DEVCTRL_REG, 1, buf, 1))
		return;

	buf[0] |= PMIC_DEVCTRL_REG_SR_CTL_I2C_SEL_CTL_I2C;

	if (i2c_write(PMIC_CTRL_I2C_ADDR, PMIC_DEVCTRL_REG, 1, buf, 1))
		return;

	/*
	 * Unless we're running at 1GHz we use thesame VDD for
	 * all other frequencies we switch to (currently 720MHz,
	 * 800MHz or 1GHz).
	 */
	if (mpu_pll == MPUPLL_M_1000)
		mpu_vdd = PMIC_OP_REG_SEL_1_3_2_5;
	else
		mpu_vdd = PMIC_OP_REG_SEL_1_2_6;

	if (!voltage_update(CORE, PMIC_OP_REG_SEL_1_1_3))
		core_pll_config(OPP_100);
	if (!voltage_update(MPU, mpu_vdd))
		mpu_pll_config(mpu_pll);
}
#endif

/*
 * early system init of muxing and clocks.
 */
void s_init(void)
{
	/* WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	writel(0xAAAA, &wdtimer->wdtwspr);
	while (readl(&wdtimer->wdtwwps) != 0x0)
		;
	writel(0x5555, &wdtimer->wdtwspr);
	while (readl(&wdtimer->wdtwwps) != 0x0)
		;

#if defined(CONFIG_SPL_BUILD)
	/* Setup the PLLs and the clocks for the peripherals */
	pll_init();

	/* Enable RTC32K clock */
	rtc32k_enable();

	/* UART softreset */
	u32 regVal;

#ifdef CONFIG_SERIAL1
	enable_uart0_pin_mux();
#endif /* CONFIG_SERIAL1 */

	regVal = readl(&uart_base->uartsyscfg);
	regVal |= UART_RESET;
	writel(regVal, &uart_base->uartsyscfg);
	while ((readl(&uart_base->uartsyssts) &
		UART_CLK_RUNNING_MASK) != UART_CLK_RUNNING_MASK)
		;

	/* Disable smart idle */
	regVal = readl(&uart_base->uartsyscfg);
	regVal |= UART_SMART_IDLE_EN;
	writel(regVal, &uart_base->uartsyscfg);

	gd = &gdata;

	preloader_console_init();

	enable_i2c1_pin_mux();
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	enable_board_pin_mux();

	config_ddr(400, MT41K256M16HA125E_IOCTRL_VALUE,
			&ddr3_var_am33x_data,
			&ddr3_var_am33x_cmd_ctrl_data,
			&ddr3_var_am33x_emif_reg_data);

	/* Reset the ethernet chip.
	 */
	gpio_request(GPIO_PHY1_RST, "phy1_rst");
	gpio_direction_output(GPIO_PHY1_RST, 1);
	udelay(10000);
	gpio_set_value(GPIO_PHY1_RST, 0);
	udelay(10000);
	gpio_set_value(GPIO_PHY1_RST, 1);

	enable_rmii1_pin_mux();
#endif
}

/*
 * Basic board specific setup.  Pinmux has been handled already.
 */
int board_init(void)
{
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	gd->bd->bi_boot_params = PHYS_DRAM_1 + 0x100;

	gpmc_init();

	gpio_request(GPIO_SOM_REV_BIT0_GPIO, "som_rev_bit_0");
	gpio_direction_input(GPIO_SOM_REV_BIT0_GPIO);
	gpio_request(GPIO_SOM_REV_BIT1_GPIO, "som_rev_bit_1");
	gpio_direction_input(GPIO_SOM_REV_BIT1_GPIO);
	gpio_request(GPIO_SOM_REV_BIT2_GPIO, "som_rev_bit_2");
	gpio_direction_input(GPIO_SOM_REV_BIT2_GPIO);

	som_rev = (gpio_get_value(GPIO_SOM_REV_BIT0_GPIO) | 
		(gpio_get_value(GPIO_SOM_REV_BIT1_GPIO) << 1)) +
		!gpio_get_value(GPIO_SOM_REV_BIT2_GPIO);

	gpio_free(GPIO_SOM_REV_BIT0_GPIO);
	gpio_free(GPIO_SOM_REV_BIT1_GPIO);
	gpio_free(GPIO_SOM_REV_BIT2_GPIO);

	printf("SystemTera.Server.V by BeKa Software GmbH\n");
	if (som_rev > 0)
		printf("Variscite AM33 SOM revision 1.%d detected\n", 
				som_rev);
	else {
		printf("ERROR: unknown Variscite AM33X SOM revision.\n");
		hang();
	}

	/* Turn off LCD */
	gpio_request(GPIO_LCD_BACKLIGHT, "backlight");
	gpio_direction_output(GPIO_LCD_BACKLIGHT, 0);

	/* mux bluetooth to omap */
	gpio_request(GPIO_BT_UART_SELECT, "bt_uart_select");
	gpio_direction_output(GPIO_BT_UART_SELECT, 1);

	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	return 0;
}
#endif

#if (defined(CONFIG_DRIVER_TI_CPSW) && !defined(CONFIG_SPL_BUILD)) || \
	(defined(CONFIG_SPL_ETH_SUPPORT) && defined(CONFIG_SPL_BUILD))
static void cpsw_control(int enabled)
{
	/* VTP can be added here */

	return;
}

static struct cpsw_slave_data cpsw_slaves[] = {
	{
		.slave_reg_ofs	= 0x208,
		.sliver_reg_ofs	= 0xd80,
		.phy_id		= 0,
	},
	{
		.slave_reg_ofs	= 0x308,
		.sliver_reg_ofs	= 0xdc0,
		.phy_id		= 1,
	},
};

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	if (som_rev < 2) {
		/* Set 50Mhz clock mode */
		int val = phy_read(phydev, MDIO_DEVAD_NONE, 0x1f);
		val |= 0x80;
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, val);
	}

	/* override strap, set RMII mode */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x16, 0x2);
}

static struct cpsw_platform_data cpsw_data = {
	.mdio_base		= AM335X_CPSW_MDIO_BASE,
	.cpsw_base		= AM335X_CPSW_BASE,
	.mdio_div		= 0xff,
	.channels		= 8,
	.cpdma_reg_ofs		= 0x800,
	.slaves			= 1,
	.slave_data		= cpsw_slaves,
	.ale_reg_ofs		= 0xd00,
	.ale_entries		= 1024,
	.host_port_reg_ofs	= 0x108,
	.hw_stats_reg_ofs	= 0x900,
	.mac_control		= (1 << 5),
	.control		= cpsw_control,
	.host_port_num		= 0,
	.version		= CPSW_CTRL_VERSION_2,
};
#endif

#if defined(CONFIG_DRIVER_TI_CPSW) || \
	(defined(CONFIG_USB_ETHER) && defined(CONFIG_MUSB_GADGET))


int board_eth_init(bd_t *bis)
{
	int rv, n = 0;
	uint8_t mac_addr[6];
	uint32_t mac_hi, mac_lo;


	/* try reading mac address from efuse */
	mac_lo = readl(&cdev->macid0l);
	mac_hi = readl(&cdev->macid0h);
	mac_addr[0] = mac_hi & 0xFF;
	mac_addr[1] = (mac_hi & 0xFF00) >> 8;
	mac_addr[2] = (mac_hi & 0xFF0000) >> 16;
	mac_addr[3] = (mac_hi & 0xFF000000) >> 24;
	mac_addr[4] = mac_lo & 0xFF;
	mac_addr[5] = (mac_lo & 0xFF00) >> 8;

#if (defined(CONFIG_DRIVER_TI_CPSW) && !defined(CONFIG_SPL_BUILD)) || \
	(defined(CONFIG_SPL_ETH_SUPPORT) && defined(CONFIG_SPL_BUILD))
	if (!getenv("ethaddr")) {
		printf("<ethaddr> not set. Validating first E-fuse MAC\n");

		if (is_valid_ether_addr(mac_addr))
			eth_setenv_enetaddr("ethaddr", mac_addr);
	}

	if (som_rev < 2)
		writel(RMII_MODE_ENABLE, &cdev->miisel);
	else
		writel(RMII_MODE_ENABLE | 0x40, &cdev->miisel);

	cpsw_slaves[0].phy_if = PHY_INTERFACE_MODE_RMII;
	cpsw_slaves[1].phy_if = PHY_INTERFACE_MODE_RGMII;

	rv = cpsw_register(&cpsw_data);
	if (rv < 0)
		printf("Error %d registering CPSW switch\n", rv);
	else
		n += rv;
#endif
#if defined(CONFIG_USB_ETHER) && \
	(!defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_USBETH_SUPPORT))
	if (is_valid_ether_addr(mac_addr))
		eth_setenv_enetaddr("usbnet_devaddr", mac_addr);

	rv = usb_eth_initialize(bis);
	if (rv < 0)
		printf("Error %d registering USB_ETHER\n", rv);
	else
		n += rv;
#endif
	return n;
}
#endif
