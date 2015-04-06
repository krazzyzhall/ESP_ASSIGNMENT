/*
 * Copyright (C) 2005 Stephen Street / StreetFire Sound Labs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/spi/pxa2xx_spi.h>
#include <linux/dma-mapping.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#ifdef CONFIG_INTEL_CLN_SOC
#include <asm/cln.h>
#endif

#include <asm/irq.h>


MODULE_AUTHOR("Stephen Street");
MODULE_DESCRIPTION("PXA2xx SSP SPI Controller");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pxa2xx-spi");

#define MAX_BUSES 3

#define TIMOUT_DFLT		1000

#define DMA_INT_MASK		(DCSR_ENDINTR | DCSR_STARTINTR | DCSR_BUSERR)
#define RESET_DMA_CHANNEL	(DCSR_NODESC | DMA_INT_MASK)
#define IS_DMA_ALIGNED(x)	((((u32)(x)) & 0x07) == 0)
#define MAX_DMA_LEN		8191
#define DMA_ALIGNMENT		8

#define CE5X00_FIFO_DEPTH    32
#define CE4X00_FIFO_DEPTH    4

/*
 * for testing SSCR1 changes that require SSP restart, basically
 * everything except the service and interrupt enables, the pxa270 developer
 * manual says only SSCR1_SCFR, SSCR1_SPH, SSCR1_SPO need to be in this
 * list, but the PXA255 dev man says all bits without really meaning the
 * service and interrupt enables
 */
#define SSCR1_CHANGE_MASK (SSCR1_TTELP | SSCR1_TTE | SSCR1_SCFR \
			   | SSCR1_ECRA | SSCR1_ECRB | SSCR1_SCLKDIR	\
			   | SSCR1_SFRMDIR | SSCR1_RWOT | SSCR1_TRAIL	\
			   | SSCR1_IFS | SSCR1_STRF | SSCR1_EFWR	\
			   | SSCR1_RFT | SSCR1_TFT | SSCR1_MWDS		\
			   | SSCR1_SPH | SSCR1_SPO | SSCR1_LBM)

#define CE5X00_SSCR1_CHANGE_MASK (SSCR1_TTELP | SSCR1_TTE | SSCR1_SCFR	\
				  | SSCR1_ECRA | SSCR1_ECRB | SSCR1_SCLKDIR \
				  | SSCR1_SFRMDIR | SSCR1_RWOT | SSCR1_TRAIL \
				  | SSCR1_IFS | CE5X00_SSCR1_STRF	\
				  | CE5X00_SSCR1_EFWR			\
				  | CE5X00_SSCR1_RFT | CE5X00_SSCR1_TFT \
				  | SSCR1_MWDS				\
				  | SSCR1_SPH | SSCR1_SPO | SSCR1_LBM)


#define DEFINE_SSP_REG(reg, off)				\
	static inline u32 read_##reg(void const __iomem *p)	\
	{ return __raw_readl(p + (off)); }			\
								\
	static inline void write_##reg(u32 v, void __iomem *p)	\
	{ __raw_writel(v, p + (off)); }

DEFINE_SSP_REG(sscr0, 0x00)
DEFINE_SSP_REG(sscr1, 0x04)
DEFINE_SSP_REG(sssr, 0x08)
DEFINE_SSP_REG(ssitr, 0x0c)
DEFINE_SSP_REG(ssdr, 0x10)
#ifdef CONFIG_INTEL_CLN_SOC
DEFINE_SSP_REG(dds_rate, 0x28) /* SSTO unused for clanton */
#endif
DEFINE_SSP_REG(ssto, 0x28)
DEFINE_SSP_REG(sspsp, 0x2c)

#define START_STATE ((void *)0)
#define RUNNING_STATE ((void *)1)
#define DONE_STATE ((void *)2)
#define ERROR_STATE ((void *)-1)

#define QUEUE_RUNNING 0
#define QUEUE_STOPPED 1

#ifdef CONFIG_INTEL_CLN_SOC
#define DDS_MAX     0x800000
#define DDS_666666  0x666666
#define DDS_400000  0x400000
#define DDS_200000  0x200000
#define DDS_100000  0x100000
#define DDS_80000   0x80000
#define DDS_40000   0x40000
#define DDS_20000   0x20000
#define DDS_10000   0x10000
#define DDS_8000    0x8000

#if defined(CONFIG_INTEL_CLN_SOC_FPGAEMU)
/* these values are different on emulations where system clock is 14mhz */
#define FSYS              14 /* mhz */
#define BITRATE_50MHZ     3500000
#define BITRATE_40MHZ     2800000
#define BITRATE_25MHZ     1750000
#define BITRATE_20MHZ     1400000
#define BITRATE_16667KHZ  1167000
#define BITRATE_13333KHZ  933000
#define BITRATE_12500KHZ  875000
#define BITRATE_10MHZ     700000
#define BITRATE_8MHZ      560000
#define BITRATE_6250KHZ   438000
#define BITRATE_5MHZ      350000
#define BITRATE_4MHZ      280000
#define BITRATE_3125KHZ   219000
#define BITRATE_2500KHZ   175000
#define BITRATE_2MHZ      140000
#define BITRATE_1563KHZ   109000
#define BITRATE_1250KHZ   88000
#define BITRATE_1MHZ      70000
#define BITRATE_800KHZ    56000
#define BITRATE_781KHZ    54688
#define BITRATE_625KHZ    43750
#define BITRATE_500KHZ    35000
#define BITRATE_400KHZ    28000
#define BITRATE_390KHZ	  27344
#define BITRATE_250KHZ	  17500
#define BITRATE_200KHZ	  14000
#define BITRATE_195KHZ    13672
#define BITRATE_125KHZ    8750
#define BITRATE_100KHZ    7000
#define BITRATE_50KHZ     3500
#define BITRATE_25KHZ     1750
#define BITRATE_10KHZ     701
#define BITRATE_5KHZ      353
#define BITRATE_1KHZ      70
#define BITRATE_MAX       BITRATE_50MHZ
#define BITRATE_MIN       BITRATE_1KHZ

#else

#define BITRATE_50MHZ     50000000
#define BITRATE_40MHZ     40000000
#define BITRATE_25MHZ     25000000
#define BITRATE_20MHZ     20000000
#define BITRATE_16667KHZ  16667000
#define BITRATE_13333KHZ  13333000
#define BITRATE_12500KHZ  12500000
#define BITRATE_10MHZ     10000000
#define BITRATE_8MHZ      8000000
#define BITRATE_6250KHZ   6250000
#define BITRATE_5MHZ      5000000
#define BITRATE_4MHZ      4000000
#define BITRATE_3125KHZ   3125000
#define BITRATE_2500KHZ   2500000
#define BITRATE_2MHZ      2000000
#define BITRATE_1563KHZ   1563000
#define BITRATE_1250KHZ   1250000
#define BITRATE_1MHZ      1000000
#define BITRATE_800KHZ    800000
#define BITRATE_781KHZ    781250
#define BITRATE_625KHZ    625000
#define BITRATE_500KHZ    500000
#define BITRATE_400KHZ    400000
#define BITRATE_390KHZ    390625
#define BITRATE_250KHZ    250000
#define BITRATE_200KHZ    200000
#define BITRATE_195KHZ    195313
#define BITRATE_125KHZ    125000
#define BITRATE_100KHZ    100000
#define BITRATE_50KHZ     50000
#define BITRATE_25KHZ     25000
#define BITRATE_10KHZ     10016
#define BITRATE_5KHZ      5040
#define BITRATE_1KHZ      1000
#define BITRATE_MIN       BITRATE_1KHZ
#define BITRATE_MAX       BITRATE_50MHZ
#define FSYS              200 /* mhz */

#endif /* #if defined(CONFIG_INTEL_CLN_SOC_FPGAEMU) */

#endif /* CONFIG_INTEL_CLN_SOC */

struct driver_data {
	/* Driver model hookup */
	struct platform_device *pdev;

	/* SSP Info */
	struct ssp_device *ssp;

	/* SPI framework hookup */
	enum pxa_ssp_type ssp_type;
	struct spi_master *master;

	/* PXA hookup */
	struct pxa2xx_spi_master *master_info;

	/* DMA setup stuff */
	int rx_channel;
	int tx_channel;
	u32 *null_dma_buf;

	/* SSP register addresses */
	void __iomem *ioaddr;
	u32 ssdr_physical;

	/* SSP masks*/
	u32 dma_cr1;
	u32 int_cr1;
	u32 clear_sr;
	u32 mask_sr;

	/* Message per-transfer pump */
	struct tasklet_struct pump_transfers;

	/* Current message transfer state info */
	struct spi_message *cur_msg;
	struct spi_transfer *cur_transfer;
	struct chip_data *cur_chip;
	size_t len;
	void *tx;
	void *tx_end;
	void *rx;
	void *rx_end;
	int dma_mapped;
	dma_addr_t rx_dma;
	dma_addr_t tx_dma;
	size_t rx_map_len;
	size_t tx_map_len;
	u8 n_bytes;
	u32 dma_width;
	int (*write)(struct driver_data *drv_data);
	int (*read)(struct driver_data *drv_data);
	irqreturn_t (*transfer_handler)(struct driver_data *drv_data);
	void (*cs_control)(u32 command);
};

struct chip_data {
	u32 cr0;
	u32 cr1;
#ifdef CONFIG_INTEL_CLN_SOC
	u32 dds_rate;
#endif
	u32 psp;
	u32 timeout;
	u8 n_bytes;
	u32 dma_width;
	u32 dma_burst_size;
	u32 threshold;
	u32 dma_threshold;
	u8 enable_dma;
	u8 bits_per_word;
	u32 speed_hz;
	union {
		int gpio_cs;
		unsigned int frm;
	};
	int gpio_cs_inverted;
	int (*write)(struct driver_data *drv_data);
	int (*read)(struct driver_data *drv_data);
	void (*cs_control)(u32 command);
};

static void cs_assert(struct driver_data *drv_data)
{
	struct chip_data *chip = drv_data->cur_chip;

	if (drv_data->ssp_type == CE4100_SSP ||
	    (drv_data->ssp_type == CE5X00_SSP &&
	     (!gpio_is_valid(chip->gpio_cs)))) {
		write_sssr(drv_data->cur_chip->frm, drv_data->ioaddr);
		return;
	}

	if (chip->cs_control) {
		chip->cs_control(PXA2XX_CS_ASSERT);
		return;
	}

	if (gpio_is_valid(chip->gpio_cs))
		gpio_set_value(chip->gpio_cs, chip->gpio_cs_inverted);
}

static void cs_deassert(struct driver_data *drv_data)
{
	struct chip_data *chip = drv_data->cur_chip;

	if (drv_data->ssp_type == CE4100_SSP ||
	    (drv_data->ssp_type == CE5X00_SSP
	     && (!gpio_is_valid(chip->gpio_cs)))) {
		return;
	}
	if (chip->cs_control) {
		chip->cs_control(PXA2XX_CS_DEASSERT);
		return;
	}

	if (gpio_is_valid(chip->gpio_cs))
		gpio_set_value(chip->gpio_cs, !chip->gpio_cs_inverted);
}

static void write_sssr_cs(struct driver_data *drv_data, u32 val)
{
	void __iomem *reg = drv_data->ioaddr;

	if (drv_data->ssp_type == CE4100_SSP
	    || drv_data->ssp_type == CE5X00_SSP)
		val |= read_sssr(reg) & SSSR_ALT_FRM_MASK;

	write_sssr(val, reg);
}

static int pxa25x_ssp_comp(struct driver_data *drv_data)
{
	if (drv_data->ssp_type == PXA25x_SSP)
		return 1;
	if (drv_data->ssp_type == CE4100_SSP)
		return 1;
#ifdef CONFIG_GEN3_SPI
	if (drv_data->ssp_type == CE5X00_SSP)
		return 2;
#endif
	return 0;
}

static int flush(struct driver_data *drv_data)
{
	unsigned long limit = loops_per_jiffy << 1;

	void __iomem *reg = drv_data->ioaddr;

	do {
		while (read_sssr(reg) & SSSR_RNE)
			read_ssdr(reg);
	} while ((read_sssr(reg) & SSSR_BSY) && --limit);
	write_sssr_cs(drv_data, SSSR_ROR);

	return limit;
}

#ifdef CONFIG_GEN3_SPI
static void wait_till_not_busy(struct driver_data *drv_data)
{
	/*
	 * Most cases, the BSY bit will be cleared in very short time.
	 * But if the controller is set to the slowest speed, the BSY
	 * bit waitting time will increase a lot.
	 * Here, we set the wait time to 100ms is to deal with such corner case,
	 * but in real world, there is almost no one will use the slowest
	 * speed to transfer data, which means there is no need to add more
	 * complex code to to deal with the slowest speed case.
	 */
	unsigned long end = jiffies + 1 + usecs_to_jiffies(100000);
	void __iomem *reg = drv_data->ioaddr;
	while (time_before(jiffies, end)) {
		if (!(read_sssr(reg) & SSSR_BSY))
			return;
	}
	pr_err("SPI MASTER keeps busy for 100ms after a read/write!\n");
}
#endif

static int null_writer(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;
	u8 n_bytes = drv_data->n_bytes;

	if ((!(read_sssr(reg) & SSSR_TNF))
	    || (drv_data->tx == drv_data->tx_end))
		return 0;

	write_ssdr(0, reg);
	drv_data->tx += n_bytes;

	return 1;
}

static int null_reader(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;
	u8 n_bytes = drv_data->n_bytes;

	while ((read_sssr(reg) & SSSR_RNE)
		&& (drv_data->rx < drv_data->rx_end)) {
		read_ssdr(reg);
		drv_data->rx += n_bytes;
	}

	return drv_data->rx == drv_data->rx_end;
}

static int u8_writer(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;

	if ((!(read_sssr(reg) & SSSR_TNF))
	    || (drv_data->tx == drv_data->tx_end))
		return 0;

	write_ssdr(*(u8 *)(drv_data->tx), reg);
	++drv_data->tx;

	return 1;
}

static int u8_reader(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;

	while ((read_sssr(reg) & SSSR_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		*(u8 *)(drv_data->rx) = read_ssdr(reg);
		++drv_data->rx;
	}

	return drv_data->rx == drv_data->rx_end;
}

static int u16_writer(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;

	if ((!(read_sssr(reg) & SSSR_TNF))
	    || (drv_data->tx == drv_data->tx_end))
		return 0;

	write_ssdr(*(u16 *)(drv_data->tx), reg);
	drv_data->tx += 2;

	return 1;
}

static int u16_reader(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;

	while ((read_sssr(reg) & SSSR_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		*(u16 *)(drv_data->rx) = read_ssdr(reg);
		drv_data->rx += 2;
	}

	return drv_data->rx == drv_data->rx_end;
}

static int u32_writer(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;

	if ((!(read_sssr(reg) & SSSR_TNF))
	    || (drv_data->tx == drv_data->tx_end))
		return 0;

	write_ssdr(*(u32 *)(drv_data->tx), reg);
	drv_data->tx += 4;

	return 1;
}

static int u32_reader(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;

	while ((read_sssr(reg) & SSSR_RNE)
	       && (drv_data->rx < drv_data->rx_end)) {
		*(u32 *)(drv_data->rx) = read_ssdr(reg);
		drv_data->rx += 4;
	}

	return drv_data->rx == drv_data->rx_end;
}

static void *next_transfer(struct driver_data *drv_data)
{
	struct spi_message *msg = drv_data->cur_msg;
	struct spi_transfer *trans = drv_data->cur_transfer;

	/* Move to next transfer */
	if (trans->transfer_list.next != &msg->transfers) {
		drv_data->cur_transfer =
			list_entry(trans->transfer_list.next,
					struct spi_transfer,
					transfer_list);
		return RUNNING_STATE;
	} else
		return DONE_STATE;
}

static int map_dma_buffers(struct driver_data *drv_data)
{
	struct spi_message *msg = drv_data->cur_msg;
	struct device *dev = &msg->spi->dev;

	if (!drv_data->cur_chip->enable_dma)
		return 0;

	if (msg->is_dma_mapped)
		return  drv_data->rx_dma && drv_data->tx_dma;

	if (!IS_DMA_ALIGNED(drv_data->rx) || !IS_DMA_ALIGNED(drv_data->tx))
		return 0;

	/* Modify setup if rx buffer is null */
	if (drv_data->rx == NULL) {
		*drv_data->null_dma_buf = 0;
		drv_data->rx = drv_data->null_dma_buf;
		drv_data->rx_map_len = 4;
	} else
		drv_data->rx_map_len = drv_data->len;


	/* Modify setup if tx buffer is null */
	if (drv_data->tx == NULL) {
		*drv_data->null_dma_buf = 0;
		drv_data->tx = drv_data->null_dma_buf;
		drv_data->tx_map_len = 4;
	} else
		drv_data->tx_map_len = drv_data->len;

	/* Stream map the tx buffer. Always do DMA_TO_DEVICE first
	 * so we flush the cache *before* invalidating it, in case
	 * the tx and rx buffers overlap.
	 */
	drv_data->tx_dma = dma_map_single(dev, drv_data->tx,
					drv_data->tx_map_len, DMA_TO_DEVICE);
	if (dma_mapping_error(dev, drv_data->tx_dma))
		return 0;

	/* Stream map the rx buffer */
	drv_data->rx_dma = dma_map_single(dev, drv_data->rx,
					drv_data->rx_map_len, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, drv_data->rx_dma)) {
		dma_unmap_single(dev, drv_data->tx_dma,
					drv_data->tx_map_len, DMA_TO_DEVICE);
		return 0;
	}

	return 1;
}

static void unmap_dma_buffers(struct driver_data *drv_data)
{
	struct device *dev;

	if (!drv_data->dma_mapped)
		return;

	if (!drv_data->cur_msg->is_dma_mapped) {
		dev = &drv_data->cur_msg->spi->dev;
		dma_unmap_single(dev, drv_data->rx_dma,
					drv_data->rx_map_len, DMA_FROM_DEVICE);
		dma_unmap_single(dev, drv_data->tx_dma,
					drv_data->tx_map_len, DMA_TO_DEVICE);
	}

	drv_data->dma_mapped = 0;
}

/* caller already set message->status; dma and pio irqs are blocked */
static void giveback(struct driver_data *drv_data)
{
	struct spi_transfer *last_transfer;

	last_transfer = list_entry(drv_data->cur_msg->transfers.prev,
				   struct spi_transfer,
				   transfer_list);

	/* Delay if requested before any change in chip select */
	if (last_transfer->delay_usecs)
		udelay(last_transfer->delay_usecs);

	/* Drop chip select UNLESS cs_change is true or we are returning
	 * a message with an error, or next message is for another chip
	 */
	if (!last_transfer->cs_change)
		cs_deassert(drv_data);
	else {
		struct spi_message *next_msg;

		/* Holding of cs was hinted, but we need to make sure
		 * the next message is for the same chip.  Don't waste
		 * time with the following tests unless this was hinted.
		 *
		 * We cannot postpone this until pump_messages, because
		 * after calling spi_finalize_current_message (below) the
		 * driver that sent the current message could be unloaded,
		 * which could invalidate the cs_control() callback...
		 */
		/* get a pointer to the next message, if any */
		next_msg = spi_get_next_queued_message(drv_data->master);

		/* see if the next and current messages point
		 * to the same chip
		 */
		if (next_msg && next_msg->spi != drv_data->cur_msg->spi)
			next_msg = NULL;
		if (!next_msg || drv_data->cur_msg->state == ERROR_STATE)
			cs_deassert(drv_data);
	}

	drv_data->cur_msg = NULL;
	drv_data->cur_transfer = NULL;
	drv_data->cur_chip = NULL;
	spi_finalize_current_message(drv_data->master);
}

static int wait_ssp_rx_stall(void const __iomem *ioaddr)
{
	unsigned long limit = loops_per_jiffy << 1;

	while ((read_sssr(ioaddr) & SSSR_BSY) && --limit)
		cpu_relax();

	return limit;
}

static int wait_dma_channel_stop(int channel)
{
	unsigned long limit = loops_per_jiffy << 1;

	while (!(DCSR(channel) & DCSR_STOPSTATE) && --limit)
		cpu_relax();

	return limit;
}

static void dma_error_stop(struct driver_data *drv_data, const char *msg)
{
	void __iomem *reg = drv_data->ioaddr;

	/* Stop and reset */
	DCSR(drv_data->rx_channel) = RESET_DMA_CHANNEL;
	DCSR(drv_data->tx_channel) = RESET_DMA_CHANNEL;
	write_sssr_cs(drv_data, drv_data->clear_sr);
	write_sscr1(read_sscr1(reg) & ~drv_data->dma_cr1, reg);
	if (!pxa25x_ssp_comp(drv_data))
		write_ssto(0, reg);
	flush(drv_data);
	write_sscr0(read_sscr0(reg) & ~SSCR0_SSE, reg);

	unmap_dma_buffers(drv_data);

	dev_err(&drv_data->pdev->dev, "%s\n", msg);

	drv_data->cur_msg->state = ERROR_STATE;
	tasklet_schedule(&drv_data->pump_transfers);
}

static void dma_transfer_complete(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;
	struct spi_message *msg = drv_data->cur_msg;

	/* Clear and disable interrupts on SSP and DMA channels*/
	write_sscr1(read_sscr1(reg) & ~drv_data->dma_cr1, reg);
	write_sssr_cs(drv_data, drv_data->clear_sr);
	DCSR(drv_data->tx_channel) = RESET_DMA_CHANNEL;
	DCSR(drv_data->rx_channel) = RESET_DMA_CHANNEL;

	if (wait_dma_channel_stop(drv_data->rx_channel) == 0)
		dev_err(&drv_data->pdev->dev,
			"dma_handler: dma rx channel stop failed\n");

	if (wait_ssp_rx_stall(drv_data->ioaddr) == 0)
		dev_err(&drv_data->pdev->dev,
			"dma_transfer: ssp rx stall failed\n");

	unmap_dma_buffers(drv_data);

	/* update the buffer pointer for the amount completed in dma */
	drv_data->rx += drv_data->len -
			(DCMD(drv_data->rx_channel) & DCMD_LENGTH);

	/* read trailing data from fifo, it does not matter how many
	 * bytes are in the fifo just read until buffer is full
	 * or fifo is empty, which ever occurs first */
	drv_data->read(drv_data);

	/* return count of what was actually read */
	msg->actual_length += drv_data->len -
				(drv_data->rx_end - drv_data->rx);

	/* Transfer delays and chip select release are
	 * handled in pump_transfers or giveback
	 */

	/* Move to next transfer */
	msg->state = next_transfer(drv_data);

	/* Schedule transfer tasklet */
	tasklet_schedule(&drv_data->pump_transfers);
}

static void dma_handler(int channel, void *data)
{
	struct driver_data *drv_data = data;
	u32 irq_status = DCSR(channel) & DMA_INT_MASK;

	if (irq_status & DCSR_BUSERR) {

		if (channel == drv_data->tx_channel)
			dma_error_stop(drv_data,
					"dma_handler: "
					"bad bus address on tx channel");
		else
			dma_error_stop(drv_data,
					"dma_handler: "
					"bad bus address on rx channel");
		return;
	}

	/* PXA255x_SSP has no timeout interrupt, wait for tailing bytes */
	if ((channel == drv_data->tx_channel)
		&& (irq_status & DCSR_ENDINTR)
		&& (drv_data->ssp_type == PXA25x_SSP)) {

		/* Wait for rx to stall */
		if (wait_ssp_rx_stall(drv_data->ioaddr) == 0)
			dev_err(&drv_data->pdev->dev,
				"dma_handler: ssp rx stall failed\n");

		/* finish this transfer, start the next */
		dma_transfer_complete(drv_data);
	}
}

static irqreturn_t dma_transfer(struct driver_data *drv_data)
{
	u32 irq_status;
	void __iomem *reg = drv_data->ioaddr;

	irq_status = read_sssr(reg) & drv_data->mask_sr;
	if (irq_status & SSSR_ROR) {
		dma_error_stop(drv_data, "dma_transfer: fifo overrun");
		return IRQ_HANDLED;
	}

	/* Check for false positive timeout */
	if ((irq_status & SSSR_TINT)
		&& (DCSR(drv_data->tx_channel) & DCSR_RUN)) {
		write_sssr(SSSR_TINT, reg);
		return IRQ_HANDLED;
	}

	if (irq_status & SSSR_TINT || drv_data->rx == drv_data->rx_end) {

		/* Clear and disable timeout interrupt, do the rest in
		 * dma_transfer_complete */
		if (!pxa25x_ssp_comp(drv_data))
			write_ssto(0, reg);

		/* finish this transfer, start the next */
		dma_transfer_complete(drv_data);

		return IRQ_HANDLED;
	}

	/* Opps problem detected */
	return IRQ_NONE;
}

static void reset_sccr1(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;
	struct chip_data *chip = drv_data->cur_chip;
	u32 sccr1_reg;

	sccr1_reg = read_sscr1(reg) & ~drv_data->int_cr1;
	if (drv_data->ssp_type == CE5X00_SSP)
		sccr1_reg &= ~CE5X00_SSCR1_RFT;
	else
		sccr1_reg &= ~SSCR1_RFT;
	sccr1_reg |= chip->threshold;
	write_sscr1(sccr1_reg, reg);
}

static void int_error_stop(struct driver_data *drv_data, const char *msg)
{
	void __iomem *reg = drv_data->ioaddr;

	/* Stop and reset SSP */
	write_sssr_cs(drv_data, drv_data->clear_sr);
	reset_sccr1(drv_data);
	if (!pxa25x_ssp_comp(drv_data))
		write_ssto(0, reg);
	flush(drv_data);
	write_sscr0(read_sscr0(reg) & ~SSCR0_SSE, reg);

	dev_err(&drv_data->pdev->dev, "%s\n", msg);

	drv_data->cur_msg->state = ERROR_STATE;
	tasklet_schedule(&drv_data->pump_transfers);
}

static void int_transfer_complete(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;

	/* Stop SSP */
	write_sssr_cs(drv_data, drv_data->clear_sr);
	reset_sccr1(drv_data);
	if (!pxa25x_ssp_comp(drv_data))
		write_ssto(0, reg);

	/* Update total byte transferred return count actual bytes read */
	drv_data->cur_msg->actual_length += drv_data->len -
				(drv_data->rx_end - drv_data->rx);

	/* Transfer delays and chip select release are
	 * handled in pump_transfers or giveback
	 */

	/* Move to next transfer */
	drv_data->cur_msg->state = next_transfer(drv_data);

	/* Schedule transfer tasklet */
	tasklet_schedule(&drv_data->pump_transfers);
}

#ifndef CONFIG_GEN3_SPI

static irqreturn_t interrupt_transfer(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;

	u32 irq_mask = (read_sscr1(reg) & SSCR1_TIE) ?
			drv_data->mask_sr : drv_data->mask_sr & ~SSSR_TFS;

	u32 irq_status = read_sssr(reg) & irq_mask;

	if (irq_status & SSSR_ROR) {
		int_error_stop(drv_data, "interrupt_transfer: fifo overrun");
		return IRQ_HANDLED;
	}

	if (irq_status & SSSR_TINT) {
		write_sssr_cs(drv_data, SSSR_TINT);
		if (drv_data->read(drv_data)) {
			int_transfer_complete(drv_data);
			return IRQ_HANDLED;
		}
	}

	/* Drain rx fifo, Fill tx fifo and prevent overruns */
	do {
		if (drv_data->read(drv_data)) {
			int_transfer_complete(drv_data);
			return IRQ_HANDLED;
		}
	} while (drv_data->write(drv_data));

	if (drv_data->read(drv_data)) {
		int_transfer_complete(drv_data);
		return IRQ_HANDLED;
	}

	if (drv_data->tx == drv_data->tx_end) {
#ifndef CONFIG_GEN3_SPI
		u32 bytes_left = 0;
#endif
		u32 sccr1_reg;

		sccr1_reg = read_sscr1(reg);
		sccr1_reg &= ~SSCR1_TIE;

		/*
		 * PXA25x_SSP has no timeout, set up rx threshould for the
		 * remaining RX bytes.
		 */
		if (pxa25x_ssp_comp(drv_data)) {
#ifndef CONFIG_GEN3_SPI
			sccr1_reg &= ~SSCR1_RFT;

			bytes_left = drv_data->rx_end - drv_data->rx;
			switch (drv_data->n_bytes) {
			case 4:
				bytes_left >>= 2;
			case 2:
				bytes_left >>= 1;
			}

			if (bytes_left > RX_THRESH_DFLT)
				bytes_left = RX_THRESH_DFLT;

			sccr1_reg |= SSCR1_RxTresh(bytes_left);
#endif
		}
		write_sscr1(sccr1_reg, reg);
#ifdef CONFIG_GEN3_SPI
		if (!wait_ssp_rx_stall(reg)) {
			int_error_stop(drv_data,
				       "interrupt_transfer: rx stall failed");
			return IRQ_HANDLED;
		}

		if (!drv_data->read(drv_data)) {
			int_error_stop(drv_data,
				       "interrupt_transfer: "
				       "trailing byte read failed");
			return IRQ_HANDLED;
		}

		int_transfer_complete(drv_data);
#endif

	}

	/* We did something */
	return IRQ_HANDLED;
}
#else
#ifdef WANT_TO_USE_THIS
static int is_txfifo_empty(struct driver_data *drv_data, u32 tfl_mask)
{
	void __iomem *reg = drv_data->ioaddr;
	u32 sssr = read_sssr(reg);
	u32 tx_fifo_entry = 0;
	tx_fifo_entry = (sssr & tfl_mask)>>8;
	if ((sssr & SSSR_TNF) && (!tx_fifo_entry))
		return 1;
	return 0;
}
#endif

static void pxa2xx_update_tx_threshold(struct driver_data *drv_data,
				       u32 threshold)
{
	void __iomem *reg = drv_data->ioaddr;
	u32 sscr1_reg = 0;
	sscr1_reg = read_sscr1(reg);
	sscr1_reg |= SSCR1_TIE;
	if (drv_data->ssp_type == CE5X00_SSP) {
		sscr1_reg &= ~CE5X00_SSCR1_TFT;
		sscr1_reg |= CE5X00_SSCR1_TxTresh(threshold);
	} else {
		sscr1_reg &= ~SSCR1_TFT;
		sscr1_reg |= SSCR1_TxTresh(threshold);
	}

	write_sscr1(sscr1_reg, reg);
}

static void pxa2xx_mask_intr(void __iomem *reg, u32 intr)
{
	u32 mask;
	mask = read_sscr1(reg) & ~intr;
	write_sscr1(mask, reg);
}

static void pxa2xx_unmask_intr(void __iomem *reg, u32 intr)
{
	u32 mask;
	mask = read_sscr1(reg) | intr;
	write_sscr1(mask, reg);
}

static irqreturn_t interrupt_transfer(struct driver_data *drv_data)
{
	void __iomem *reg = drv_data->ioaddr;

	u32 tx_count;
	u32 irq_mask = (read_sscr1(reg) & SSCR1_TIE) ?
			drv_data->mask_sr : drv_data->mask_sr & ~SSSR_TFS;

	int left;

	u32 tmp_sssr = read_sssr(reg);
	u32 irq_status = tmp_sssr & irq_mask;
	/*
	 * Transmit FIFO Level Depth/MASK/Default value
	 */
	u32 fifo_depth, sssr_tfl_mask, sscr1_tft_mask, tfl_default;

	/*
	 * Check whether the irq is valid spi interrupt
	 */
	if (!(tmp_sssr & (SSSR_TFS | SSSR_RFS | SSSR_ROR)))
		return IRQ_NONE;

	switch (drv_data->ssp_type) {
	case CE4100_SSP:
		fifo_depth = CE4X00_FIFO_DEPTH;
		sssr_tfl_mask = SSSR_TFL_MASK;
		sscr1_tft_mask = SSCR1_TFT;
		tfl_default = TX_THRESH_DFLT;
		break;
	case CE5X00_SSP:
		fifo_depth  = CE5X00_FIFO_DEPTH;
		sssr_tfl_mask = CE5X00_SSSR_TFL_MASK;
		sscr1_tft_mask = CE5X00_SSCR1_TFT;
		tfl_default = TX_THRESH_CE5X00_DFLT;
		break;
	default:
		int_error_stop(drv_data, "Unsupported spi contoller type");
		return IRQ_HANDLED;
	}


	if (irq_status & SSSR_ROR) {
		int_error_stop(drv_data, "interrupt_transfer: fifo overrun");
		return IRQ_HANDLED;
	}


	if (irq_status & SSSR_TINT) {
		write_sssr_cs(drv_data, SSSR_TINT);
		if (drv_data->read(drv_data)) {
			int_transfer_complete(drv_data);
			return IRQ_HANDLED;
		}
	}


	if (irq_status & SSSR_TFS) {

		/*
		 * Mask the Transmit interrupt
		 */
		pxa2xx_mask_intr(reg, SSCR1_TIE);

		left = (drv_data->tx_end - drv_data->tx) / drv_data->n_bytes;

		tmp_sssr = read_sssr(reg);
		tmp_sssr = (tmp_sssr & sssr_tfl_mask)>>8;
		/*
		 * Choose the correct tx_count to fill the Tx fifo and
		 * avoid the Rx Fifo overrun
		 */
		tx_count =  (fifo_depth - 1) - tmp_sssr;

		if (left <= tx_count) {
			if (left > 0) {
				drv_data->read(drv_data);
				pxa2xx_update_tx_threshold(drv_data, 1);
				while ((left) > 0) {
					left--;
					drv_data->write(drv_data);
				}
				pxa2xx_unmask_intr(reg, SSCR1_TIE);
				return IRQ_HANDLED;
			} else {
				/*
				 * Tx transfer is done now,
				 * Read data when controller is not busy.
				 */
				wait_till_not_busy(drv_data);
				drv_data->read(drv_data);
				pxa2xx_unmask_intr(reg, SSCR1_TIE);
				int_transfer_complete(drv_data);
				return IRQ_HANDLED;
			}
		} else {
			left = (left > tx_count) ? tx_count : left;
			drv_data->read(drv_data);
			while ((left) > 0) {
				left--;
				drv_data->write(drv_data);
			}
			pxa2xx_unmask_intr(reg, SSCR1_TIE);
			return IRQ_HANDLED;
		}
	}
	/* We did something */
	return IRQ_HANDLED;
}
#endif

static irqreturn_t ssp_int(int irq, void *dev_id)
{
	struct driver_data *drv_data = dev_id;
	void __iomem *reg = drv_data->ioaddr;
	u32 sccr1_reg = read_sscr1(reg);
	u32 mask = drv_data->mask_sr;
	u32 status;
	irqreturn_t ret;

	status = read_sssr(reg);

	/* Ignore possible writes if we don't need to write */
	if (!(sccr1_reg & SSCR1_TIE))
		mask &= ~SSSR_TFS;

	if (!(status & mask))
		return IRQ_NONE;

#ifdef CONFIG_INTEL_CLN_SOC
	mask_pvm(drv_data->ssp->pcidev);
#endif
	if (!drv_data->cur_msg) {

		write_sscr0(read_sscr0(reg) & ~SSCR0_SSE, reg);
		write_sscr1(read_sscr1(reg) & ~drv_data->int_cr1, reg);
		if (!pxa25x_ssp_comp(drv_data))
			write_ssto(0, reg);
		write_sssr_cs(drv_data, drv_data->clear_sr);

		dev_err(&drv_data->pdev->dev,
			"bad message state in interrupt handler\n");
		/* Never fail */
		return IRQ_HANDLED;
	}
	ret = drv_data->transfer_handler(drv_data);
#ifdef CONFIG_INTEL_CLN_SOC
	unmask_pvm(drv_data->ssp->pcidev);
#endif
	return ret;
}

static int set_dma_burst_and_threshold(struct chip_data *chip,
				struct spi_device *spi,
				u8 bits_per_word, u32 *burst_code,
				u32 *threshold)
{
	struct pxa2xx_spi_chip *chip_info =
			(struct pxa2xx_spi_chip *)spi->controller_data;
	int bytes_per_word;
	int burst_bytes;
	int thresh_words;
	int req_burst_size;
	int retval = 0;

	/* Set the threshold (in registers) to equal the same amount of data
	 * as represented by burst size (in bytes).  The computation below
	 * is (burst_size rounded up to nearest 8 byte, word or long word)
	 * divided by (bytes/register); the tx threshold is the inverse of
	 * the rx, so that there will always be enough data in the rx fifo
	 * to satisfy a burst, and there will always be enough space in the
	 * tx fifo to accept a burst (a tx burst will overwrite the fifo if
	 * there is not enough space), there must always remain enough empty
	 * space in the rx fifo for any data loaded to the tx fifo.
	 * Whenever burst_size (in bytes) equals bits/word, the fifo threshold
	 * will be 8, or half the fifo;
	 * The threshold can only be set to 2, 4 or 8, but not 16, because
	 * to burst 16 to the tx fifo, the fifo would have to be empty;
	 * however, the minimum fifo trigger level is 1, and the tx will
	 * request service when the fifo is at this level, with only 15 spaces.
	 */

	/* find bytes/word */
	if (bits_per_word <= 8)
		bytes_per_word = 1;
	else if (bits_per_word <= 16)
		bytes_per_word = 2;
	else
		bytes_per_word = 4;

	/* use struct pxa2xx_spi_chip->dma_burst_size if available */
	if (chip_info)
		req_burst_size = chip_info->dma_burst_size;
	else {
		switch (chip->dma_burst_size) {
		default:
			/* if the default burst size is not set,
			 * do it now */
			chip->dma_burst_size = DCMD_BURST8;
		case DCMD_BURST8:
			req_burst_size = 8;
			break;
		case DCMD_BURST16:
			req_burst_size = 16;
			break;
		case DCMD_BURST32:
			req_burst_size = 32;
			break;
		}
	}
	if (req_burst_size <= 8) {
		*burst_code = DCMD_BURST8;
		burst_bytes = 8;
	} else if (req_burst_size <= 16) {
		if (bytes_per_word == 1) {
			/* don't burst more than 1/2 the fifo */
			*burst_code = DCMD_BURST8;
			burst_bytes = 8;
			retval = 1;
		} else {
			*burst_code = DCMD_BURST16;
			burst_bytes = 16;
		}
	} else {
		if (bytes_per_word == 1) {
			/* don't burst more than 1/2 the fifo */
			*burst_code = DCMD_BURST8;
			burst_bytes = 8;
			retval = 1;
		} else if (bytes_per_word == 2) {
			/* don't burst more than 1/2 the fifo */
			*burst_code = DCMD_BURST16;
			burst_bytes = 16;
			retval = 1;
		} else {
			*burst_code = DCMD_BURST32;
			burst_bytes = 32;
		}
	}

	thresh_words = burst_bytes / bytes_per_word;

	/* thresh_words will be between 2 and 8 */
	*threshold = (SSCR1_RxTresh(thresh_words) & SSCR1_RFT)
			| (SSCR1_TxTresh(16-thresh_words) & SSCR1_TFT);

	return retval;
}
#ifdef CONFIG_GEN3_SPI
/* this returns the value of DDS_FREQ not Input_Clock
   DDS_FREQ = Input_Clock ( DDS_CLK_RATEdec / 2^24) */
static unsigned long spi_clk_get_rate(int ssp_type)
{
	switch (ssp_type) {
	case CE5X00_SSP:
		/* CE5X00 clk is 10MHZ */
		return 10000000;
	case CE4100_SSP:
		/* CE4X00 clk is 3.684MHZ */
	default:
		return 3686400;
	}

	return 3686400;
}
#endif

#ifdef CONFIG_INTEL_CLN_SOC
/*  see Clanton SPI data sheet for implementation rationale */
u32 cln_set_clk_regvals(u32 rate, u32 *dds, u32 *clk_div)
{
	if (rate <= BITRATE_MAX && rate >= BITRATE_MIN) {
		if (rate >= BITRATE_50MHZ) {
			*dds = DDS_MAX;
			*clk_div = 0;
			return BITRATE_50MHZ;
		} else if (rate >= BITRATE_40MHZ) {
			*dds = DDS_666666;
			*clk_div = 0;
			return BITRATE_40MHZ;
		} else if (rate >= BITRATE_25MHZ) {
			*dds = DDS_400000;
			*clk_div = 0;
			return BITRATE_25MHZ;
		} else if (rate >= BITRATE_20MHZ) {
			*dds = DDS_666666;
			*clk_div = 1;
			return BITRATE_20MHZ;
		} else if (rate >= BITRATE_16667KHZ) {
			*dds = DDS_MAX;
			*clk_div = 2;
			return BITRATE_16667KHZ;
		} else if (rate >= BITRATE_13333KHZ) {
			*dds = DDS_666666;
			*clk_div = 2;
			return BITRATE_13333KHZ;
		} else if (rate >= BITRATE_12500KHZ) {
			*dds = DDS_200000;
			*clk_div = 0;
			return BITRATE_12500KHZ;
		} else if (rate >= BITRATE_10MHZ) {
			*dds = DDS_MAX;
			*clk_div = 4;
			return BITRATE_10MHZ;
		} else if (rate >= BITRATE_8MHZ) {
			*dds = DDS_666666;
			*clk_div = 4;
			return BITRATE_8MHZ;
		} else if (rate >= BITRATE_6250KHZ) {
			*dds = DDS_400000;
			*clk_div = 3;
			return BITRATE_6250KHZ;
		} else if (rate >= BITRATE_5MHZ) {
			*dds = DDS_400000;
			*clk_div = 4;
			return BITRATE_5MHZ;
		} else if (rate >= BITRATE_4MHZ) {
			*dds = DDS_666666;
			*clk_div = 9;
			return BITRATE_4MHZ;
		} else if (rate >= BITRATE_3125KHZ) {
			*dds = DDS_80000;
			*clk_div = 0;
			return BITRATE_3125KHZ;
		} else if (rate >= BITRATE_2500KHZ) {
			*dds = DDS_400000;
			*clk_div = 9;
			return BITRATE_2500KHZ;
		} else if (rate >= BITRATE_2MHZ) {
			*dds = DDS_666666;
			*clk_div = 19;
			return BITRATE_2MHZ;
		} else if (rate >= BITRATE_1563KHZ) {
			*dds = DDS_40000;
			*clk_div = 0;
			return BITRATE_1563KHZ;
		} else if (rate >= BITRATE_1250KHZ) {
			*dds = DDS_200000;
			*clk_div = 9;
			return BITRATE_1250KHZ;
		} else if (rate >= BITRATE_1MHZ) {
			*dds = DDS_400000;
			*clk_div = 24;
			return BITRATE_1MHZ;
		} else if (rate >= BITRATE_800KHZ) {
			*dds = DDS_666666;
			*clk_div = 49;
			return BITRATE_800KHZ;
		} else if (rate >= BITRATE_781KHZ) {
			*dds = DDS_20000;
			*clk_div = 0;
			return BITRATE_781KHZ;
		} else if (rate >= BITRATE_625KHZ) {
			*dds = DDS_200000;
			*clk_div = 19;
			return BITRATE_625KHZ;
		} else if (rate >= BITRATE_500KHZ) {
			*dds = DDS_400000;
			*clk_div = 49;
			return BITRATE_500KHZ;
		} else if (rate >= BITRATE_400KHZ) {
			*dds = DDS_666666;
			*clk_div = 99;
			return BITRATE_400KHZ;
		} else if (rate >= BITRATE_390KHZ) {
			*dds = DDS_10000;
			*clk_div = 0;
			return BITRATE_390KHZ;
		} else if (rate >= BITRATE_250KHZ) {
			*dds = DDS_400000;
			*clk_div = 99;
			return BITRATE_250KHZ;
		} else if (rate >= BITRATE_200KHZ) {
			*dds = DDS_666666;
			*clk_div = 199;
			return BITRATE_200KHZ;
		} else if (rate >= BITRATE_195KHZ) {
			*dds = DDS_8000;
			*clk_div = 0;
			return BITRATE_195KHZ;
		} else if (rate >= BITRATE_125KHZ) {
			*dds = DDS_100000;
			*clk_div = 49;
			return BITRATE_125KHZ;
		} else if (rate >= BITRATE_100KHZ) {
			*dds = DDS_200000;
			*clk_div = 124;
			return BITRATE_100KHZ;
		} else if (rate >= BITRATE_50KHZ) {
			*dds = DDS_100000;
			*clk_div = 124;
			return BITRATE_50KHZ;
		} else if (rate >= BITRATE_25KHZ) {
			*dds = DDS_80000;
			*clk_div = 124;
			return BITRATE_25KHZ;
		} else if (rate >= BITRATE_10KHZ) {
			*dds = DDS_20000;
			*clk_div = 77;
			return BITRATE_10KHZ;
		} else if (rate >= BITRATE_5KHZ) {
			*dds = DDS_20000;
			*clk_div = 154;
			return BITRATE_5KHZ;
		} else if (rate >= BITRATE_1KHZ) {
			*dds = DDS_8000;
			*clk_div = 194;
			return BITRATE_1KHZ;
		}
	} else {
		*dds = DDS_8000;
		*clk_div = 194;
		return BITRATE_MIN;
	}
	return 0;
}
#else
/* this returns the value that SCR needs to be set to
   Bit rate = DDS_FREQ / (2 x (SCR + 1)) */
static unsigned int ssp_get_clk_div(struct ssp_device *ssp, int rate)
{
#ifdef CONFIG_GEN3_SPI
	unsigned long ssp_clk =  spi_clk_get_rate(ssp->type);
#else
	unsigned long ssp_clk = clk_get_rate(ssp->clk);
#endif

	if (ssp->type == PXA25x_SSP || ssp->type == CE4100_SSP
	    || ssp->type == CE5X00_SSP)
		return ((ssp_clk / (2 * rate) - 1) & 0xff) << 8;
	else
		return ((ssp_clk / rate - 1) & 0xfff) << 8;
}
#endif /* CONFIG_INTEL_CLN_SOC */

static void pump_transfers(unsigned long data)
{
	struct driver_data *drv_data = (struct driver_data *)data;
	struct spi_message *message = NULL;
	struct spi_transfer *transfer = NULL;
	struct spi_transfer *previous = NULL;
	struct chip_data *chip = NULL;
#ifndef CONFIG_INTEL_CLN_SOC
	struct ssp_device *ssp = drv_data->ssp;
#else
	u32 actual_speed = 0;
#endif
	void __iomem *reg = drv_data->ioaddr;
	u32 clk_div = 0;
	u8 bits = 0;
	u32 speed = 0;
	u32 cr0;
	u32 cr1;
	u32 dma_thresh = drv_data->cur_chip->dma_threshold;
	u32 dma_burst = drv_data->cur_chip->dma_burst_size;

	/* Get current state information */
	message = drv_data->cur_msg;
	transfer = drv_data->cur_transfer;

	chip = drv_data->cur_chip;

	if (transfer->bits_per_word)
		chip->n_bytes = (transfer->bits_per_word + 7)/8;
	/* Handle for abort */
	if (message->state == ERROR_STATE) {
		message->status = -EIO;
		giveback(drv_data);
		return;
	}

	/* Handle end of message */
	if (message->state == DONE_STATE) {
		message->status = 0;
		giveback(drv_data);
		return;
	}

	/* Delay if requested at end of transfer before CS change */
	if (message->state == RUNNING_STATE) {
		previous = list_entry(transfer->transfer_list.prev,
					struct spi_transfer,
					transfer_list);
		if (previous->delay_usecs)
			udelay(previous->delay_usecs);

		/* Drop chip select only if cs_change is requested */
		if (previous->cs_change)
			cs_deassert(drv_data);
	}

	/* Check for transfers that need multiple DMA segments */
	if (transfer->len > MAX_DMA_LEN && chip->enable_dma) {

		/* reject already-mapped transfers; PIO won't always work */
		if (message->is_dma_mapped
				|| transfer->rx_dma || transfer->tx_dma) {
			dev_err(&drv_data->pdev->dev,
				"pump_transfers: mapped transfer length "
				"of %u is greater than %d\n",
				transfer->len, MAX_DMA_LEN);
			message->status = -EINVAL;
			giveback(drv_data);
			return;
		}

		/* warn ... we force this to PIO mode */
		dev_warn_ratelimited(&message->spi->dev,
				     "pump_transfers: "
				     "DMA disabled for transfer length %ld "
				     "greater than %d\n",
				     (long)drv_data->len, MAX_DMA_LEN);
	}

	/* Setup the transfer state based on the type of transfer */
	if (flush(drv_data) == 0) {
		dev_err(&drv_data->pdev->dev, "pump_transfers: flush failed\n");
		message->status = -EIO;
		giveback(drv_data);
		return;
	}
	drv_data->n_bytes = chip->n_bytes;
	drv_data->dma_width = chip->dma_width;
	drv_data->tx = (void *)transfer->tx_buf;
	drv_data->tx_end = drv_data->tx
		+ (transfer->len/drv_data->n_bytes)*drv_data->n_bytes;
	drv_data->rx = transfer->rx_buf;
	drv_data->rx_end = drv_data->rx
		+ (transfer->len/drv_data->n_bytes)*drv_data->n_bytes;
	drv_data->rx_dma = transfer->rx_dma;
	drv_data->tx_dma = transfer->tx_dma;
	drv_data->len =  (transfer->len/drv_data->n_bytes)
		* drv_data->n_bytes & DCMD_LENGTH;
	drv_data->write = drv_data->tx ? chip->write : null_writer;
	drv_data->read = drv_data->rx ? chip->read : null_reader;

	/* Change speed and bit per word on a per transfer */
	cr0 = chip->cr0;
	if (transfer->speed_hz || transfer->bits_per_word) {
		bits = chip->bits_per_word;
		speed = chip->speed_hz;

		if (transfer->speed_hz)
			speed = transfer->speed_hz;

		if (transfer->bits_per_word)
			bits = transfer->bits_per_word;

#ifdef CONFIG_INTEL_CLN_SOC
		actual_speed = cln_set_clk_regvals
				(speed, &chip->dds_rate, &clk_div);
		clk_div = (clk_div << 8);
#else
		clk_div = ssp_get_clk_div(ssp, speed);
#endif
		if (bits <= 8) {
			drv_data->n_bytes = 1;
			drv_data->dma_width = DCMD_WIDTH1;
			drv_data->read = drv_data->read != null_reader ?
						u8_reader : null_reader;
			drv_data->write = drv_data->write != null_writer ?
						u8_writer : null_writer;
		} else if (bits <= 16) {
			drv_data->n_bytes = 2;
			drv_data->dma_width = DCMD_WIDTH2;
			drv_data->read = drv_data->read != null_reader ?
						u16_reader : null_reader;
			drv_data->write = drv_data->write != null_writer ?
						u16_writer : null_writer;
		} else if (bits <= 32) {
			drv_data->n_bytes = 4;
			drv_data->dma_width = DCMD_WIDTH4;
			drv_data->read = drv_data->read != null_reader ?
						u32_reader : null_reader;
			drv_data->write = drv_data->write != null_writer ?
						u32_writer : null_writer;
		}
		/* if bits/word is changed in dma mode, then must check the
		 * thresholds and burst also */
		if (chip->enable_dma) {
			if (set_dma_burst_and_threshold(chip, message->spi,
							bits, &dma_burst,
							&dma_thresh))
				dev_warn_ratelimited(&message->spi->dev,
						     "pump_transfers: "
						     "DMA burst size reduced to"
						     " match bits_per_word\n");
		}

		switch (drv_data->ssp_type) {
		case CE5X00_SSP:
			chip->cr0 = clk_div
				| CE5X00_SSCR0_Motorola
				| CE5X00_SSCR0_DataSize(bits > 32 ?
							8 : bits)
				| SSCR0_SSE;
			break;
		case CE4100_SSP:
		case PXA25x_SSP:
		default:
			chip->cr0 = clk_div
				| SSCR0_Motorola
				| SSCR0_DataSize(bits > 16 ?
						 bits - 16 : bits)
				| SSCR0_SSE
				| (bits > 16 ? SSCR0_EDSS : 0);
		}
		
		cr0 = chip->cr0;
	}

	message->state = RUNNING_STATE;

	/* Try to map dma buffer and do a dma transfer if successful, but
	 * only if the length is non-zero and less than MAX_DMA_LEN.
	 *
	 * Zero-length non-descriptor DMA is illegal on PXA2xx; force use
	 * of PIO instead.  Care is needed above because the transfer may
	 * have have been passed with buffers that are already dma mapped.
	 * A zero-length transfer in PIO mode will not try to write/read
	 * to/from the buffers
	 *
	 * REVISIT large transfers are exactly where we most want to be
	 * using DMA.  If this happens much, split those transfers into
	 * multiple DMA segments rather than forcing PIO.
	 */
	drv_data->dma_mapped = 0;
	if (drv_data->len > 0 && drv_data->len <= MAX_DMA_LEN)
		drv_data->dma_mapped = map_dma_buffers(drv_data);
	if (drv_data->dma_mapped) {

		/* Ensure we have the correct interrupt handler */
		drv_data->transfer_handler = dma_transfer;

		/* Setup rx DMA Channel */
		DCSR(drv_data->rx_channel) = RESET_DMA_CHANNEL;
		DSADR(drv_data->rx_channel) = drv_data->ssdr_physical;
		DTADR(drv_data->rx_channel) = drv_data->rx_dma;
		if (drv_data->rx == drv_data->null_dma_buf)
			/* No target address increment */
			DCMD(drv_data->rx_channel) = DCMD_FLOWSRC
							| drv_data->dma_width
							| dma_burst
							| drv_data->len;
		else
			DCMD(drv_data->rx_channel) = DCMD_INCTRGADDR
							| DCMD_FLOWSRC
							| drv_data->dma_width
							| dma_burst
							| drv_data->len;

		/* Setup tx DMA Channel */
		DCSR(drv_data->tx_channel) = RESET_DMA_CHANNEL;
		DSADR(drv_data->tx_channel) = drv_data->tx_dma;
		DTADR(drv_data->tx_channel) = drv_data->ssdr_physical;
		if (drv_data->tx == drv_data->null_dma_buf)
			/* No source address increment */
			DCMD(drv_data->tx_channel) = DCMD_FLOWTRG
							| drv_data->dma_width
							| dma_burst
							| drv_data->len;
		else
			DCMD(drv_data->tx_channel) = DCMD_INCSRCADDR
							| DCMD_FLOWTRG
							| drv_data->dma_width
							| dma_burst
							| drv_data->len;

		/* Enable dma end irqs on SSP to detect end of transfer */
		if (drv_data->ssp_type == PXA25x_SSP)
			DCMD(drv_data->tx_channel) |= DCMD_ENDIRQEN;

		/* Clear status and start DMA engine */
		cr1 = chip->cr1 | dma_thresh | drv_data->dma_cr1;
		write_sssr(drv_data->clear_sr, reg);
		DCSR(drv_data->rx_channel) |= DCSR_RUN;
		DCSR(drv_data->tx_channel) |= DCSR_RUN;
	} else {
		/* Ensure we have the correct interrupt handler	*/
		drv_data->transfer_handler = interrupt_transfer;

		/* Clear status  */
		cr1 = chip->cr1 | chip->threshold | drv_data->int_cr1;
		write_sssr_cs(drv_data, drv_data->clear_sr);
	}

	/* see if we need to reload the config registers */
	if (drv_data->ssp_type == CE5X00_SSP) {
#ifdef CONFIG_INTEL_CLN_SOC
		if (read_dds_rate(reg) != chip->dds_rate)
			write_dds_rate(chip->dds_rate, reg);
#endif
		if ((read_sscr0(reg) != cr0)
			|| (read_sscr1(reg) & CE5X00_SSCR1_CHANGE_MASK) !=
				(cr1 & CE5X00_SSCR1_CHANGE_MASK)) {
			/* stop the SSP, and update the other bits */
			write_sscr0(cr0 & ~SSCR0_SSE, reg);
			if (!pxa25x_ssp_comp(drv_data))
				write_ssto(chip->timeout, reg);
			/* first set CR1 w/o interrupt and service enables */
			write_sscr1(cr1 & CE5X00_SSCR1_CHANGE_MASK, reg);
			/* restart the SSP */
			write_sscr0(cr0, reg);

		} else {
			if (!pxa25x_ssp_comp(drv_data))
				write_ssto(chip->timeout, reg);
		}
	} else if ((read_sscr0(reg) != cr0)
		   || (read_sscr1(reg) & SSCR1_CHANGE_MASK) !=
		   (cr1 & SSCR1_CHANGE_MASK)) {
		/* stop the SSP, and update the other bits */
		write_sscr0(cr0 & ~SSCR0_SSE, reg);
		if (!pxa25x_ssp_comp(drv_data))
			write_ssto(chip->timeout, reg);
		/* first set CR1 without interrupt and service enables */
		write_sscr1(cr1 & SSCR1_CHANGE_MASK, reg);
		/* restart the SSP */
		write_sscr0(cr0, reg);

	} else if (!pxa25x_ssp_comp(drv_data)) {
		write_ssto(chip->timeout, reg);
	}
	cs_assert(drv_data);

	/* after chip select, release the data by enabling service
	 * requests and interrupts, without changing any mode bits */
	write_sscr1(cr1, reg);
}

static int transfer_one_message(struct spi_master *master,
				struct spi_message *msg)
{
	struct driver_data *drv_data = spi_master_get_devdata(master);

	/* Initial message state */
	drv_data->cur_msg = msg;
	msg->state = START_STATE;

	drv_data->cur_transfer = list_entry(msg->transfers.next,
					    struct spi_transfer, transfer_list);

	/* prepare to setup the SSP, in pump_transfers, using the per
	 * chip configuration */
	drv_data->cur_chip = spi_get_ctldata(msg->spi);

	/* Launch transfers */
	tasklet_schedule(&drv_data->pump_transfers);

	return 0;
}

static int setup_cs(struct spi_device *spi, struct chip_data *chip,
		    struct pxa2xx_spi_chip *chip_info)
{
	int err = 0;

	if (chip == NULL || chip_info == NULL)
		return 0;

	/* NOTE: setup() can be called multiple times, possibly with
	 * different chip_info, release previously requested GPIO
	 */
	if (gpio_is_valid(chip->gpio_cs))
		gpio_free(chip->gpio_cs);

	/* If (*cs_control) is provided, ignore GPIO chip select */
	if (chip_info->cs_control) {
		chip->cs_control = chip_info->cs_control;
		return 0;
	}

	if (gpio_is_valid(chip_info->gpio_cs)) {
		err = gpio_request(chip_info->gpio_cs, "SPI_CS");
		if (err) {
			dev_err(&spi->dev,
				"failed to request chip select "
				"GPIO%d\n", chip_info->gpio_cs);
			return err;
		}

		chip->gpio_cs = chip_info->gpio_cs;
		chip->gpio_cs_inverted = spi->mode & SPI_CS_HIGH;

		err = gpio_direction_output(chip->gpio_cs,
					!chip->gpio_cs_inverted);
	}

	return err;
}

static int prepare_transfer_hardware(struct spi_master *master)
{
	struct driver_data *drv_data = spi_master_get_devdata(master);

	/*
	 * Just make sure we have all we need to run the transfer by syncing
	 * with the runtime PM framework.
	 */
	pm_runtime_get_sync(&drv_data->pdev->dev);
	return 0;
}

static int unprepare_transfer_hardware(struct spi_master *master)
{
	struct driver_data *drv_data = spi_master_get_devdata(master);

	/* nothing more to do - disable spi/ssp and power off */
	write_sscr0(0, drv_data->ioaddr);
	clk_disable(drv_data->ssp->clk);

	pm_runtime_put(&drv_data->pdev->dev);

	return 0;
}

static int setup(struct spi_device *spi)
{
	struct pxa2xx_spi_chip *chip_info = NULL;
	struct chip_data *chip;
	struct driver_data *drv_data = spi_master_get_devdata(spi->master);
	struct ssp_device *ssp = drv_data->ssp;
	unsigned int clk_div;
#ifdef CONFIG_INTEL_CLN_SOC
	u32 actual_speed;
#endif
	uint tx_thres = TX_THRESH_DFLT;
	uint rx_thres = RX_THRESH_DFLT;
	if (drv_data->ssp_type == CE5X00_SSP) {
		tx_thres = TX_THRESH_CE5X00_DFLT;
		rx_thres = RX_THRESH_CE5X00_DFLT;
	}
	if ((!pxa25x_ssp_comp(drv_data) || pxa25x_ssp_comp(drv_data) == 2)
		&& (spi->bits_per_word < 4 || spi->bits_per_word > 32)) {
		dev_err(&spi->dev,
			"failed setup: ssp_type=%d, bits/wrd=%d "
			"b/w not 4-32 for type non-PXA25x_SSP\n",
			drv_data->ssp_type, spi->bits_per_word);
		return -EINVAL;
	} else if (pxa25x_ssp_comp(drv_data) == 1
			&& (spi->bits_per_word < 4
				|| spi->bits_per_word > 16)) {
		dev_err(&spi->dev,
			"failed setup: ssp_type=%d, bits/wrd=%d "
			"b/w not 4-16 for type PXA25x_SSP\n",
			drv_data->ssp_type, spi->bits_per_word);
		return -EINVAL;
	}

	/* Only alloc on first setup */
	chip = spi_get_ctldata(spi);
	if (!chip) {
		chip = kzalloc(sizeof(struct chip_data), GFP_KERNEL);
		if (!chip) {
			dev_err(&spi->dev,
				"failed setup: can't allocate chip data\n");
			return -ENOMEM;
		}

		if (drv_data->ssp_type == CE4100_SSP) {
			if (spi->chip_select > 4) {
				dev_err(&spi->dev,
					"failed setup: "
					"cs number must not be > 4.\n");
				kfree(chip);
				return -EINVAL;
			}

			chip->frm = spi->chip_select;
		} else
			chip->gpio_cs = -1;

		chip->enable_dma = 0;
		chip->timeout = TIMOUT_DFLT;
		chip->dma_burst_size = drv_data->master_info->enable_dma ?
					DCMD_BURST8 : 0;
	}

	/* protocol drivers may change the chip settings, so...
	 * if chip_info exists, use it */
	chip_info = spi->controller_data;

	/* chip_info isn't always needed */
	chip->cr1 = 0;
	chip->cr0 = 0;
	if (chip_info) {
		if (chip_info->timeout)
			chip->timeout = chip_info->timeout;
		if (chip_info->tx_threshold)
			tx_thres = chip_info->tx_threshold;
		if (chip_info->rx_threshold)
			rx_thres = chip_info->rx_threshold;
		chip->enable_dma = drv_data->master_info->enable_dma;
		chip->dma_threshold = 0;
		if (chip_info->enable_loopback)
			chip->cr1 = SSCR1_LBM;
	}

	/* set dma burst and threshold outside of chip_info path so that if
	 * chip_info goes away after setting chip->enable_dma, the
	 * burst and threshold can still respond to changes in bits_per_word */
	if (chip->enable_dma) {
		/* set up legal burst and threshold for dma */
		if (set_dma_burst_and_threshold(chip, spi, spi->bits_per_word,
						&chip->dma_burst_size,
						&chip->dma_threshold)) {
			dev_warn(&spi->dev,
				 "in setup: DMA burst size reduced "
				 "to match bits_per_word\n");
		}
	}
#ifdef CONFIG_INTEL_CLN_SOC
	actual_speed = cln_set_clk_regvals(spi->max_speed_hz,
						&chip->dds_rate, &clk_div);
	clk_div = (clk_div << 8);
#else
	clk_div = ssp_get_clk_div(ssp, spi->max_speed_hz);
#endif
	chip->speed_hz = spi->max_speed_hz;

	switch (drv_data->ssp_type) {
	case  CE5X00_SSP:
		chip->cr0 = clk_div
			| CE5X00_SSCR0_Motorola
			| CE5X00_SSCR0_DataSize(spi->bits_per_word > 32 ?
						8 : spi->bits_per_word)
			| SSCR0_SSE;
		chip->threshold = (CE5X00_SSCR1_RxTresh(rx_thres)
				   & CE5X00_SSCR1_RFT) |
			(CE5X00_SSCR1_TxTresh(tx_thres) & CE5X00_SSCR1_TFT);
		break;
	case  CE4100_SSP:
	case  PXA25x_SSP:
	default:
		chip->cr0 = clk_div
			| SSCR0_Motorola
			| SSCR0_DataSize(spi->bits_per_word > 16 ?
				spi->bits_per_word - 16 : spi->bits_per_word)
			| SSCR0_SSE
			| (spi->bits_per_word > 16 ? SSCR0_EDSS : 0);
		chip->threshold = (SSCR1_RxTresh(rx_thres) & SSCR1_RFT) |
			(SSCR1_TxTresh(tx_thres) & SSCR1_TFT);
	}

	chip->cr1 &= ~(SSCR1_SPO | SSCR1_SPH);
#ifdef CONFIG_INTEL_CLN_SOC
	chip->cr1 |= (((spi->mode & SPI_CPHA) != 0) ? SSCR1_SPH : 0)
			| (((spi->mode & SPI_CPOL) != 0) ? SSCR1_SPO : 0)
			| (((spi->mode & SPI_LOOP) != 0) ? SSCR1_LBM : 0);
#else
	chip->cr1 |= (((spi->mode & SPI_CPHA) != 0) ? SSCR1_SPH : 0)
			| (((spi->mode & SPI_CPOL) != 0) ? SSCR1_SPO : 0);
#endif

	/* NOTE:  PXA25x_SSP _could_ use external clocking ... */
	if (!pxa25x_ssp_comp(drv_data))
		dev_dbg(&spi->dev,
			"%ld Hz actual, %s\n",
#ifndef CONFIG_GEN3_SPI
			clk_get_rate(ssp->clk)
#else
			spi_clk_get_rate(ssp->type)
#endif
				/ (1 + ((chip->cr0 & SSCR0_SCR(0xfff)) >> 8)),
			chip->enable_dma ? "DMA" : "PIO");
	else
		dev_dbg(&spi->dev,
			"%ld Hz actual, %s\n",
#ifndef CONFIG_GEN3_SPI
			clk_get_rate(ssp->clk) / 2
#else
			spi_clk_get_rate(ssp->type) / 2
#endif
				/ (1 + ((chip->cr0 & SSCR0_SCR(0x0ff)) >> 8)),
			chip->enable_dma ? "DMA" : "PIO");

	if (spi->bits_per_word <= 8) {
		chip->n_bytes = 1;
		chip->dma_width = DCMD_WIDTH1;
		chip->read = u8_reader;
		chip->write = u8_writer;
	} else if (spi->bits_per_word <= 16) {
		chip->n_bytes = 2;
		chip->dma_width = DCMD_WIDTH2;
		chip->read = u16_reader;
		chip->write = u16_writer;
	} else if (spi->bits_per_word <= 32) {
#ifndef CONFIG_GEN3_SPI
		chip->cr0 |= SSCR0_EDSS;
#endif
		chip->n_bytes = 4;
		chip->dma_width = DCMD_WIDTH4;
		chip->read = u32_reader;
		chip->write = u32_writer;
	} else {
		dev_err(&spi->dev, "invalid wordsize\n");
		return -ENODEV;
	}
	chip->bits_per_word = spi->bits_per_word;

	spi_set_ctldata(spi, chip);

	if (drv_data->ssp_type == CE4100_SSP)
		return 0;

	return setup_cs(spi, chip, chip_info);
}

static void cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi_get_ctldata(spi);
	struct driver_data *drv_data = spi_master_get_devdata(spi->master);

	if (!chip)
		return;

	if (drv_data->ssp_type != CE4100_SSP && gpio_is_valid(chip->gpio_cs))
		gpio_free(chip->gpio_cs);

	kfree(chip);
}

static int pxa2xx_spi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pxa2xx_spi_master *platform_info;
	struct spi_master *master;
	struct driver_data *drv_data;
	struct ssp_device *ssp;
	int status;

	platform_info = dev->platform_data;

	ssp = pxa_ssp_request(pdev->id, pdev->name);
	if (ssp == NULL) {
		dev_err(&pdev->dev, "failed to request SSP%d\n", pdev->id);
		return -ENODEV;
	}

	/* Allocate master with space for drv_data and null dma buffer */
	master = spi_alloc_master(dev, sizeof(struct driver_data) + 16);
	if (!master) {
		dev_err(&pdev->dev, "cannot alloc spi_master\n");
		pxa_ssp_free(ssp);
		return -ENOMEM;
	}
	drv_data = spi_master_get_devdata(master);
	drv_data->master = master;
	drv_data->master_info = platform_info;
	drv_data->pdev = pdev;
	drv_data->ssp = ssp;

	master->dev.parent = &pdev->dev;
#ifdef CONFIG_OF
#ifndef CONFIG_GEN3_SPI
	master->dev.of_node = pdev->dev.of_node;
#endif
#endif

	/* the spi->mode bits understood by this driver: */
#ifdef CONFIG_INTEL_CLN_SOC
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LOOP;
#else
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;
#endif
	master->bus_num = pdev->id;
	master->num_chipselect = platform_info->num_chipselect;
	master->dma_alignment = DMA_ALIGNMENT;
	master->cleanup = cleanup;
	master->setup = setup;
	master->prepare_transfer_hardware = prepare_transfer_hardware;
	master->transfer_one_message = transfer_one_message;
	master->unprepare_transfer_hardware = unprepare_transfer_hardware;
	master->rt = true;

	drv_data->ssp_type = ssp->type;
	drv_data->null_dma_buf = (u32 *)ALIGN((u32)(drv_data +
						sizeof(struct driver_data)), 8);

	drv_data->ioaddr = ssp->mmio_base;
	drv_data->ssdr_physical = ssp->phys_base + SSDR;
	if (pxa25x_ssp_comp(drv_data)) {
		drv_data->int_cr1 = SSCR1_TIE | SSCR1_RIE;
		drv_data->dma_cr1 = 0;
		drv_data->clear_sr = SSSR_ROR;
		drv_data->mask_sr = SSSR_RFS | SSSR_TFS | SSSR_ROR;
	} else {
		drv_data->int_cr1 = SSCR1_TIE | SSCR1_RIE | SSCR1_TINTE;
		drv_data->dma_cr1 = SSCR1_TSRE | SSCR1_RSRE | SSCR1_TINTE;
		drv_data->clear_sr = SSSR_ROR | SSSR_TINT;
		drv_data->mask_sr = SSSR_TINT | SSSR_RFS | SSSR_TFS | SSSR_ROR;
	}

	status = request_irq(ssp->irq, ssp_int, IRQF_SHARED, dev_name(dev),
			drv_data);
	if (status < 0) {
		dev_err(&pdev->dev, "cannot get IRQ %d\n", ssp->irq);
		goto out_error_master_alloc;
	}

	/* Setup DMA if requested */
	drv_data->tx_channel = -1;
	drv_data->rx_channel = -1;
	if (platform_info->enable_dma) {

		/* Get two DMA channels	(rx and tx) */
		drv_data->rx_channel = pxa_request_dma("pxa2xx_spi_ssp_rx",
							DMA_PRIO_HIGH,
							dma_handler,
							drv_data);
		if (drv_data->rx_channel < 0) {
			dev_err(dev, "problem (%d) requesting rx channel\n",
				drv_data->rx_channel);
			status = -ENODEV;
			goto out_error_irq_alloc;
		}
		drv_data->tx_channel = pxa_request_dma("pxa2xx_spi_ssp_tx",
							DMA_PRIO_MEDIUM,
							dma_handler,
							drv_data);
		if (drv_data->tx_channel < 0) {
			dev_err(dev, "problem (%d) requesting tx channel\n",
				drv_data->tx_channel);
			status = -ENODEV;
			goto out_error_dma_alloc;
		}

		DRCMR(ssp->drcmr_rx) = DRCMR_MAPVLD | drv_data->rx_channel;
		DRCMR(ssp->drcmr_tx) = DRCMR_MAPVLD | drv_data->tx_channel;
	}

	/* Enable SOC clock */
	clk_enable(ssp->clk);

	/* Load default SSP configuration */
	write_sscr0(0, drv_data->ioaddr);
	switch (drv_data->ssp_type) {
	case CE5X00_SSP:
		write_sscr1(CE5X00_SSCR1_RxTresh(RX_THRESH_CE5X00_DFLT) |
			    CE5X00_SSCR1_TxTresh(TX_THRESH_CE5X00_DFLT),
			    drv_data->ioaddr);
#ifdef CONFIG_INTEL_CLN_SOC
		/* using the Motorola SPI protocol and use 8 bit frame */
		write_sscr0(CE5X00_SSCR0_Motorola
			    | CE5X00_SSCR0_DataSize(8),
			    drv_data->ioaddr);
#else
		/* default using Motorola SPI protocol and use 8 bit frame */
		write_sscr0(SSCR0_SCR(2)
			    | CE5X00_SSCR0_Motorola
			    | CE5X00_SSCR0_DataSize(8),
			    drv_data->ioaddr);
#endif
		break;
	case CE4100_SSP:
	case PXA25x_SSP:
	default:
		write_sscr1(SSCR1_RxTresh(RX_THRESH_DFLT) |
			    SSCR1_TxTresh(TX_THRESH_DFLT),
			    drv_data->ioaddr);
		write_sscr0(SSCR0_SCR(2)
			    | SSCR0_Motorola
			    | SSCR0_DataSize(8),
			    drv_data->ioaddr);
	}
	if (!pxa25x_ssp_comp(drv_data))
		write_ssto(0, drv_data->ioaddr);
#ifndef CONFIG_GEN3_SPI
	/*
	 *  SSPSP register is resrved on the CEXXXX SOCs.
	 */
	write_sspsp(0, drv_data->ioaddr);
#endif

	/* Initialise transfer pump */
	tasklet_init(&drv_data->pump_transfers,
			pump_transfers,	(unsigned long)drv_data);

	/* Register with the SPI framework */
	platform_set_drvdata(pdev, drv_data);
	status = spi_register_master(master);
	if (status != 0) {
		dev_err(&pdev->dev, "problem registering spi master\n");
		goto out_error_clock_enabled;
	}

	return status;

out_error_clock_enabled:
	clk_disable(ssp->clk);

out_error_dma_alloc:
	if (drv_data->tx_channel != -1)
		pxa_free_dma(drv_data->tx_channel);
	if (drv_data->rx_channel != -1)
		pxa_free_dma(drv_data->rx_channel);

out_error_irq_alloc:
	free_irq(ssp->irq, drv_data);

out_error_master_alloc:
	spi_master_put(master);
	pxa_ssp_free(ssp);
	return status;
}

static int pxa2xx_spi_remove(struct platform_device *pdev)
{
	struct driver_data *drv_data = platform_get_drvdata(pdev);
	struct ssp_device *ssp;

	if (!drv_data)
		return 0;
	ssp = drv_data->ssp;

	/* Disable the SSP at the peripheral and SOC level */
	write_sscr0(0, drv_data->ioaddr);
	clk_disable(ssp->clk);

	/* Release DMA */
	if (drv_data->master_info->enable_dma) {
		DRCMR(ssp->drcmr_rx) = 0;
		DRCMR(ssp->drcmr_tx) = 0;
		pxa_free_dma(drv_data->tx_channel);
		pxa_free_dma(drv_data->rx_channel);
	}

	/* Release IRQ */
	free_irq(ssp->irq, drv_data);

	/* Release SSP */
	pxa_ssp_free(ssp);

	/* Disable transfer pump */
	tasklet_disable(&drv_data->pump_transfers);

	/* Disconnect from the SPI framework */
	spi_unregister_master(drv_data->master);

	/* Prevent double remove */
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static void pxa2xx_spi_shutdown(struct platform_device *pdev)
{
	int status = 0;

	status = pxa2xx_spi_remove(pdev);
	if (status != 0)
		dev_err(&pdev->dev, "shutdown failed with %d\n", status);
}

#ifdef CONFIG_PM
static int pxa2xx_spi_suspend(struct device *dev)
{
	struct driver_data *drv_data = dev_get_drvdata(dev);
	struct ssp_device *ssp = drv_data->ssp;
	int status = 0;

	status = spi_master_suspend(drv_data->master);
	if (status) {
		dev_warn(dev, "cannot suspend master\n");
		return status;
	}

	pm_runtime_get_sync(dev);

	write_sscr0(0, drv_data->ioaddr);
	clk_disable(ssp->clk);

	return 0;
}

static int pxa2xx_spi_resume(struct device *dev)
{
	struct driver_data *drv_data = dev_get_drvdata(dev);
	struct ssp_device *ssp = drv_data->ssp;
	int status = 0;

	if (drv_data->rx_channel != -1)
		DRCMR(drv_data->ssp->drcmr_rx) =
			DRCMR_MAPVLD | drv_data->rx_channel;
	if (drv_data->tx_channel != -1)
		DRCMR(drv_data->ssp->drcmr_tx) =
			DRCMR_MAPVLD | drv_data->tx_channel;

	/* Enable the SSP clock */
	clk_enable(ssp->clk);

	pm_runtime_put(dev);

	/* Start the queue running */
	status = spi_master_resume(drv_data->master);
	if (status)
		dev_err(dev, "problem starting queue (%d)\n", status);

	return status;
}

static const struct dev_pm_ops pxa2xx_spi_pm_ops = {
	.suspend	= pxa2xx_spi_suspend,
	.resume		= pxa2xx_spi_resume,
};
#endif

static struct platform_driver driver = {
	.driver = {
		.name	= "pxa2xx-spi",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &pxa2xx_spi_pm_ops,
#endif
	},
	.probe = pxa2xx_spi_probe,
	.remove = pxa2xx_spi_remove,
	.shutdown = pxa2xx_spi_shutdown,
};

static int __init pxa2xx_spi_init(void)
{
	return platform_driver_register(&driver);
}
subsys_initcall(pxa2xx_spi_init);

static void __exit pxa2xx_spi_exit(void)
{
	platform_driver_unregister(&driver);
}
module_exit(pxa2xx_spi_exit);
