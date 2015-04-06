/*
 * Copyright(c) 2013 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact Information:
 * Intel Corporation
 */
/*
 * Intel Clanton Legacy Platform Data Layout.conf accessor
 *
 * Simple Legacy SPI flash access layer
 *
 * Author : Bryan O'Donoghue <bryan.odonoghue@linux.intel.com> 2013
 */

#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/spi/pxa2xx_spi.h>
#include <linux/spi/spi.h>

#define DRIVER_NAME "cln-plat-kips-bay"
#define GPIO_RESTRICT_NAME "cln-gpio-restrict-sc"

#define CLN_SPI_MAX_CLK_DEFAULT		5000000

static int gpio_cs = 1;

module_param(gpio_cs, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(gpio_cs, "Enable GPIO chip-select for SPI channel 1");


/******************************************************************************
 *             Analog Devices AD7298 SPI Device Platform Data
 ******************************************************************************/
#include "linux/platform_data/ad7298.h"

/* Maximum input voltage allowed for each ADC input, in milliVolts */
#define AD7298_MAX_EXT_VIN 5000

static const struct ad7298_platform_data ad7298_platform_data = {
	.ext_ref = false,
	.ext_vin_max = { AD7298_MAX_EXT_VIN, AD7298_MAX_EXT_VIN,
		AD7298_MAX_EXT_VIN, AD7298_MAX_EXT_VIN,
		AD7298_MAX_EXT_VIN, AD7298_MAX_EXT_VIN,
		AD7298_MAX_EXT_VIN, AD7298_MAX_EXT_VIN }
};

/******************************************************************************
 *                 Intel Clanton SPI Controller Data
 ******************************************************************************/
static struct pxa2xx_spi_chip cln_ffrd_spi_0_cs_0 = {
	.gpio_cs = 8,
};

static struct pxa2xx_spi_chip cln_ffrd_spi_1_cs_0 = {
	.gpio_cs = 10,
};

/**
 * intel_cln_spi_add_onboard_devs
 *
 * @return 0 on success or standard errnos on failure
 *
 * Registers onboard SPI device(s) present on the Kips Bay platform
 */
static int intel_cln_spi_add_onboard_devs(void)
{
	int ret = 0;
	struct spi_board_info spi0_onboard_devs[] = {
		{
			.modalias = "ad7298",
			.max_speed_hz = CLN_SPI_MAX_CLK_DEFAULT,
			.platform_data = &ad7298_platform_data,
			.mode = SPI_MODE_2,
			.bus_num = 0,
			.chip_select = 0,
			.controller_data = &cln_ffrd_spi_0_cs_0,
		}
	};
	struct spi_board_info spi1_onboard_devs_gpiocs[] = {
		{
			.modalias = "spidev",
			.chip_select = 0,
			.controller_data = NULL,
			.max_speed_hz = 50000000,
			.bus_num = 1,
			.controller_data = &cln_ffrd_spi_1_cs_0,
		},
	};
	struct spi_board_info spi1_onboard_devs[] = {
		{
			.modalias = "spidev",
			.chip_select = 0,
			.controller_data = NULL,
			.max_speed_hz = 50000000,
			.bus_num = 1,
		},
	};

	ret = spi_register_board_info(spi0_onboard_devs,
				      ARRAY_SIZE(spi0_onboard_devs));
	if (ret)
		return ret;

	if (gpio_cs)
		return spi_register_board_info(spi1_onboard_devs_gpiocs,
					ARRAY_SIZE(spi1_onboard_devs_gpiocs));
	else
		return spi_register_board_info(spi1_onboard_devs,
					ARRAY_SIZE(spi1_onboard_devs));
}


/**
 * intel_cln_gpio_restrict_probe
 *
 * Make GPIOs pertaining to Firmware inaccessible by requesting them.  The
 * GPIOs are never released nor accessed by this driver.
 */
static int intel_cln_gpio_restrict_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret = intel_cln_spi_add_onboard_devs();

	return ret;
}

static struct platform_driver gpio_restrict_pdriver = {
	.driver		= {
		.name	= GPIO_RESTRICT_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= intel_cln_gpio_restrict_probe,
};

static int intel_cln_plat_kips_bay_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret = platform_driver_register(&gpio_restrict_pdriver);

	return ret;
}

static int intel_cln_plat_kips_bay_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver cln_kips_bay_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= intel_cln_plat_kips_bay_probe,
	.remove		= intel_cln_plat_kips_bay_remove,
};

module_platform_driver(cln_kips_bay_driver);

MODULE_AUTHOR("Bryan O'Donoghue <bryan.odonoghue@intel.com>");
MODULE_DESCRIPTION("Kips Bay BSP Data");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS("platform:"DRIVER_NAME);

