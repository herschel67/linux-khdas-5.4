/*
 * Driver for Amlogic Meson SPI communication controller (SPICC)
 *
 * Copyright (C) BayLibre, SAS
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#ifdef CONFIG_AMLOGIC_MODIFY
#include <linux/of_device.h>
#include <linux/clk-provider.h>
#include <linux/dma-mapping.h>
#include <linux/amlogic/power_domain.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#endif
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/reset.h>
#include <linux/gpio.h>
#include <linux/delay.h>

/*
 * The Meson SPICC controller could support DMA based transfers, but is not
 * implemented by the vendor code, and while having the registers documentation
 * it has never worked on the GXL Hardware.
 * The PIO mode is the only mode implemented, and due to badly designed HW :
 * - all transfers are cutted in 16 words burst because the FIFO hangs on
 *   TX underflow, and there is no TX "Half-Empty" interrupt, so we go by
 *   FIFO max size chunk only
 * - CS management is dumb, and goes UP between every burst, so is really a
 *   "Data Valid" signal than a Chip Select, GPIO link should be used instead
 *   to have a CS go down over the full transfer
 * G12 change list:
 * - add a independent async clk for divider to raise the speed rate
 * - incapable of half-full and overflow interrupts because of the async clk
 * - burst max change from 16 to 15 because of the async clk
 * DMA support:
 * - use bits_per_word of xfer to distinguish what mode the protocol driver
     requested, 64 indicates DMA mode while the others indicate PIO.
 */

#define MESON_SPICC_HW_IF

#ifndef CONFIG_AMLOGIC_MODIFY
#define SPICC_MAX_FREQ	30000000
#endif
#define SPICC_MAX_BURST	128

/* Register Map */
#define SPICC_RXDATA	0x00

#define SPICC_TXDATA	0x04

#define SPICC_CONREG	0x08
#define SPICC_ENABLE		BIT(0)
#define SPICC_MODE_MASTER	BIT(1)
#define SPICC_XCH		BIT(2)
#define SPICC_SMC		BIT(3)
#define SPICC_POL		BIT(4)
#define SPICC_PHA		BIT(5)
#define SPICC_SSCTL		BIT(6)
#define SPICC_SSPOL		BIT(7)
#define SPICC_DRCTL_MASK	GENMASK(9, 8)
#define SPICC_DRCTL_IGNORE	0
#define SPICC_DRCTL_FALLING	1
#define SPICC_DRCTL_LOWLEVEL	2
#define SPICC_CS_MASK		GENMASK(13, 12)
#define SPICC_DATARATE_MASK	GENMASK(18, 16)
#ifdef CONFIG_AMLOGIC_MODIFY
#define SPICC_DATARATE_SHIFT 16
#define SPICC_DATARATE_WIDTH 3
#endif
#define SPICC_DATARATE_DIV4	0
#define SPICC_DATARATE_DIV8	1
#define SPICC_DATARATE_DIV16	2
#define SPICC_DATARATE_DIV32	3
#define SPICC_BITLENGTH_MASK	GENMASK(24, 19)
#define SPICC_BURSTLENGTH_MASK	GENMASK(31, 25)

#define SPICC_INTREG	0x0c
#define SPICC_TE_EN	BIT(0) /* TX FIFO Empty Interrupt */
#define SPICC_TH_EN	BIT(1) /* TX FIFO Half-Full Interrupt */
#define SPICC_TF_EN	BIT(2) /* TX FIFO Full Interrupt */
#define SPICC_RR_EN	BIT(3) /* RX FIFO Ready Interrupt */
#define SPICC_RH_EN	BIT(4) /* RX FIFO Half-Full Interrupt */
#define SPICC_RF_EN	BIT(5) /* RX FIFO Full Interrupt */
#define SPICC_RO_EN	BIT(6) /* RX FIFO Overflow Interrupt */
#define SPICC_TC_EN	BIT(7) /* Transfert Complete Interrupt */

#define SPICC_DMAREG	0x10
#define SPICC_DMA_ENABLE		BIT(0)
#define SPICC_TXFIFO_THRESHOLD_MASK	GENMASK(5, 1)
#define SPICC_TXFIFO_THRESHOLD_DEFAULT	10
#define SPICC_RXFIFO_THRESHOLD_MASK	GENMASK(10, 6)
#define SPICC_READ_BURST_MASK		GENMASK(14, 11)
#define SPICC_WRITE_BURST_MASK		GENMASK(18, 15)
#define DMA_REQ_DEFAULT			8
#define SPICC_DMA_URGENT		BIT(19)
#define SPICC_DMA_THREADID_MASK		GENMASK(25, 20)
#define SPICC_DMA_BURSTNUM_MASK		GENMASK(31, 26)

#define SPICC_STATREG	0x14
#define SPICC_TE	BIT(0) /* TX FIFO Empty Interrupt */
#define SPICC_TH	BIT(1) /* TX FIFO Half-Full Interrupt */
#define SPICC_TF	BIT(2) /* TX FIFO Full Interrupt */
#define SPICC_RR	BIT(3) /* RX FIFO Ready Interrupt */
#define SPICC_RH	BIT(4) /* RX FIFO Half-Full Interrupt */
#define SPICC_RF	BIT(5) /* RX FIFO Full Interrupt */
#define SPICC_RO	BIT(6) /* RX FIFO Overflow Interrupt */
#define SPICC_TC	BIT(7) /* Transfert Complete Interrupt */

#define SPICC_PERIODREG	0x18
#define SPICC_PERIOD	GENMASK(14, 0)	/* Wait cycles */

#define SPICC_TESTREG	0x1c
#define SPICC_TXCNT_MASK	GENMASK(4, 0)	/* TX FIFO Counter */
#define SPICC_RXCNT_MASK	GENMASK(9, 5)	/* RX FIFO Counter */
#define SPICC_SMSTATUS_MASK	GENMASK(12, 10)	/* State Machine Status */
#ifdef CONFIG_AMLOGIC_MODIFY
#define SPICC_LBC		BIT(14) /* Loop Back Control */
#define SPICC_SWAP		BIT(15) /* RX FIFO Data Swap */
#define SPICC_MO_DELAY_MASK	GENMASK(17, 16) /* Master Output Delay */
#define SPICC_MO_NO_DELAY	0
#define SPICC_MO_DELAY_1_CYCLE	1
#define SPICC_MO_DELAY_2_CYCLE	2
#define SPICC_MO_DELAY_3_CYCLE	3
#define SPICC_MI_DELAY_MASK	GENMASK(19, 18) /* Master Input Delay */
#define SPICC_MI_NO_DELAY	0
#define SPICC_MI_DELAY_1_CYCLE	1
#define SPICC_MI_DELAY_2_CYCLE	2
#define SPICC_MI_DELAY_3_CYCLE	3
#define SPICC_MI_DELAY_MIN	SPICC_MI_NO_DELAY
#define SPICC_MI_DELAY_MAX	SPICC_MI_DELAY_3_CYCLE
#define SPICC_MI_CAP_DELAY_MASK	GENMASK(21, 20) /* Master Capture Delay */
#define SPICC_CAP_AHEAD_2_CYCLE	0
#define SPICC_CAP_AHEAD_1_CYCLE	1
#define SPICC_CAP_NO_DELAY	2
#define SPICC_CAP_DELAY_1_CYCLE	3
#define SPICC_CAP_DELAY_MIN	(-2)
#define SPICC_CAP_DELAY_MAX	1
#define SPICC_DELAY_MASK	GENMASK(21, 16)
#define SPICC_FIFORST_MASK	GENMASK(23, 22) /* FIFO Softreset */
#else
#define SPICC_LBC_RO		BIT(13)	/* Loop Back Control Read-Only */
#define SPICC_LBC_W1		BIT(14) /* Loop Back Control Write-Only */
#define SPICC_SWAP_RO		BIT(14) /* RX FIFO Data Swap Read-Only */
#define SPICC_SWAP_W1		BIT(15) /* RX FIFO Data Swap Write-Only */
#define SPICC_DLYCTL_RO_MASK	GENMASK(20, 15) /* Delay Control Read-Only */
#define SPICC_DLYCTL_W1_MASK	GENMASK(21, 16) /* Delay Control Write-Only */
#define SPICC_FIFORST_RO_MASK	GENMASK(22, 21) /* FIFO Softreset Read-Only */
#define SPICC_FIFORST_W1_MASK	GENMASK(23, 22) /* FIFO Softreset Write-Only */
#endif

#define SPICC_DRADDR	0x20	/* Read Address of DMA */

#define SPICC_DWADDR	0x24	/* Write Address of DMA */

#ifdef CONFIG_AMLOGIC_MODIFY
#define SPICC_LD_CNTL0	0x28
#define DMA_READ_COUNTER_EN		BIT(4)
#define DMA_WRITE_COUNTER_EN		BIT(5)

#define SPICC_LD_CNTL1	0x2c
#define DMA_READ_COUNTER		GENMASK(15, 0)
#define DMA_WRITE_COUNTER		GENMASK(31, 16)
#define SMC_REQ_CNT_MAX		0xffff
#define DMA_BURST_MAX		(DMA_REQ_DEFAULT * SMC_REQ_CNT_MAX)

#define SPICC_ENH_CTL0	0x38	/* Enhanced Feature */
#define SPICC_ENH_CLK_CS_DELAY_MASK	GENMASK(15, 0)
#define SPICC_ENH_DATARATE_MASK		GENMASK(23, 16)
#define SPICC_ENH_DATARATE_SHIFT	16
#define SPICC_ENH_DATARATE_WIDTH	8
#define SPICC_ENH_DATARATE_EN		BIT(24)
#define SPICC_ENH_MOSI_OEN		BIT(25)
#define SPICC_ENH_CLK_OEN		BIT(26)
#define SPICC_ENH_CS_OEN		BIT(27)
#define SPICC_ENH_CLK_CS_DELAY_EN	BIT(28)
#define SPICC_ENH_MAIN_CLK_AO		BIT(29)

#define SPICC_ENH_CTL1	0x3c	/* Enhanced Feature 1 */
#define SPICC_ENH_MI_CAP_DELAY_EN	BIT(0)
#define SPICC_ENH_MI_CAP_DELAY_MASK	GENMASK(9, 1)
#define SPICC_ENH_SI_CAP_DELAY_EN	BIT(14)	/* slave mode */
#define SPICC_ENH_DELAY_EN		BIT(15)
#define SPICC_ENH_SI_DELAY_EN		BIT(16)	/* slave mode */
#define SPICC_ENH_SI_DELAY_MASK		GENMASK(19, 17)	/* slave mode */
#define SPICC_ENH_MI_DELAY_EN		BIT(20)
#define SPICC_ENH_MI_DELAY_MASK		GENMASK(23, 21)
#define SPICC_ENH_MO_DELAY_EN		BIT(24)
#define SPICC_ENH_MO_DELAY_MASK		GENMASK(27, 25)
#define SPICC_ENH_MO_OEN_DELAY_EN	BIT(28)
#define SPICC_ENH_MO_OEN_DELAY_MASK	GENMASK(31, 29)

#define SPICC_ENH_CTL2	0x40	/* Enhanced Feature */
#define SPICC_ENH_TT_DELAY_MASK		GENMASK(14, 0)
#define SPICC_ENH_TT_DELAY_EN		BIT(15)
#define SPICC_ENH_TI_DELAY_MASK		GENMASK(30, 16)
#define SPICC_ENH_TI_DELAY_EN		BIT(31)
#endif

#define writel_bits_relaxed(mask, val, addr) \
	writel_relaxed((readl_relaxed(addr) & ~(mask)) | (val), addr)

#ifdef CONFIG_AMLOGIC_MODIFY
static int SPICC_FIFO_SIZE = 16;
#else
#define SPICC_FIFO_SIZE	16
#endif
#define SPICC_FIFO_HALF 10

#ifdef CONFIG_AMLOGIC_MODIFY
struct meson_spicc_data {
	unsigned int			max_speed_hz;
	bool				has_linear_div;
	bool				has_oen;
	bool				has_async_clk;
	bool				is_div_parent_async_clk;
};
#endif

struct meson_spicc_device {
	struct spi_controller		*controller;
	struct platform_device		*pdev;
	void __iomem			*base;
	struct clk			*core;
#ifdef CONFIG_AMLOGIC_MODIFY
	struct clk			*async_clk;
	struct clk			*clk;
	struct meson_spicc_data	*data;
	unsigned int			speed_hz;
	unsigned int			bits_per_word;
	struct				class cls;
	bool				using_dma;
	struct spi_device		*spi;
	bool				is_dma_mapped;
	struct spi_transfer		async_xfer;
	void				(*complete)(void *context);
	void				*context;
	s32				latency;
	unsigned int			cs2clk_ns;
	unsigned int			clk2cs_ns;
	bool				parent_clk_fixed;
	bool				clk_div_none;
	bool				toggle_cs_every_word;
#endif
	//struct spi_message		*message;
	struct spi_transfer		*xfer;
	u8				*tx_buf;
	u8				*rx_buf;
	unsigned int			bytes_per_word;
	unsigned long			tx_remain;
	unsigned long			rx_remain;
	unsigned long			xfer_remain;
	unsigned int			backup_con;
	unsigned int			backup_enh0;
	unsigned int			backup_test;
};

#ifdef CONFIG_AMLOGIC_MODIFY
static int meson_spicc_runtime_suspend(struct device *dev);
static int meson_spicc_runtime_resume(struct device *dev);
static void dirspi_set_cs(struct spi_device *spi, bool enable);

static int xLimitRange(int val, int min, int max)
{
	int ret;

	if (val < min)
		ret = min;
	else if (val > max)
		ret = max;
	else
		ret = val;

	return ret;
}

static void meson_spicc_auto_io_delay(struct meson_spicc_device *spicc)
{
	u32 div, latency;
	int shift, mi_delay, cap_delay;
	u32 conf = 0;
	struct clk *clk;
	u32 period_ns;

	if (spicc->data->has_linear_div)
		conf = readl_relaxed(spicc->base + SPICC_ENH_CTL0);
	if (conf & SPICC_ENH_DATARATE_EN) {
		div = conf & SPICC_ENH_DATARATE_MASK;
		div >>= SPICC_ENH_DATARATE_SHIFT;
		div++;
		div <<= 1;
	} else {
		div = readl_relaxed(spicc->base + SPICC_CONREG);
		div &= SPICC_DATARATE_MASK;
		div >>= SPICC_DATARATE_SHIFT;
		div += 2;
		div = 1 << div;
	}

	if (readl_relaxed(spicc->base + SPICC_TESTREG) & SPICC_LBC)
		latency = -2;
	else
		latency = spicc->latency;

	shift = (div >> 1) - latency;
	mi_delay = xLimitRange(shift, SPICC_MI_DELAY_MIN, SPICC_MI_DELAY_MAX);
	cap_delay = xLimitRange(mi_delay - shift, SPICC_CAP_DELAY_MIN,
			SPICC_CAP_DELAY_MAX);
	cap_delay += 2;

	conf = readl_relaxed(spicc->base + SPICC_TESTREG);
	conf &= ~(SPICC_MO_DELAY_MASK | SPICC_MI_DELAY_MASK
		  | SPICC_MI_CAP_DELAY_MASK);
	conf |= FIELD_PREP(SPICC_MI_DELAY_MASK, mi_delay);
	conf |= FIELD_PREP(SPICC_MI_CAP_DELAY_MASK, cap_delay);
	writel_relaxed(conf, spicc->base + SPICC_TESTREG);

	/* get period of parent clk */
	clk = spicc->data->has_async_clk ? spicc->async_clk : spicc->core;
	period_ns = DIV_ROUND_UP_ULL((u64)1000000000, clk_get_rate(clk));

	/* set cs-clk delay */
	conf = readl_relaxed(spicc->base + SPICC_ENH_CTL0);
	if (spicc->cs2clk_ns) {
		conf &= ~SPICC_ENH_CLK_CS_DELAY_MASK;
		conf |= FIELD_PREP(SPICC_ENH_CLK_CS_DELAY_MASK,
				DIV_ROUND_UP(spicc->cs2clk_ns, period_ns));
		conf |= SPICC_ENH_CLK_CS_DELAY_EN;
	} else {
		conf &= ~SPICC_ENH_CLK_CS_DELAY_EN;
	}
	writel_relaxed(conf, spicc->base + SPICC_ENH_CTL0);

	/* set clk-cs delay */
	conf = readl_relaxed(spicc->base + SPICC_ENH_CTL2);
	if (spicc->clk2cs_ns) {
		conf &= ~SPICC_ENH_TT_DELAY_MASK;
		conf |= FIELD_PREP(SPICC_ENH_TT_DELAY_MASK,
				DIV_ROUND_UP(spicc->clk2cs_ns, period_ns));
		conf |= SPICC_ENH_TT_DELAY_EN;
	} else {
		conf &= ~SPICC_ENH_TT_DELAY_EN;
	}
	writel_relaxed(conf, spicc->base + SPICC_ENH_CTL2);
}

static void meson_spicc_set_width(struct meson_spicc_device *spicc, int width)
{
	if (width && width != spicc->bits_per_word) {
		spicc->bits_per_word = width;
		writel_bits_relaxed(SPICC_BITLENGTH_MASK,
			FIELD_PREP(SPICC_BITLENGTH_MASK, width - 1),
			spicc->base + SPICC_CONREG);
	}
}

static int meson_spicc_set_speed(struct meson_spicc_device *spicc, int hz)
{
	int ret = 0;

	/* Setup clock speed */
	if (!spi_controller_is_slave(spicc->controller) && hz &&
	    spicc->speed_hz != hz) {
		spicc->speed_hz = hz;
		ret = clk_set_rate(spicc->clk, hz);
		if (ret)
			dev_err(&spicc->pdev->dev, "set clk rate failed\n");
		else
			meson_spicc_auto_io_delay(spicc);
	}
	return ret;
}

static int meson_spicc_dma_map(struct meson_spicc_device *spicc,
			       struct spi_transfer *t)
{
	struct device *dev = spicc->controller->dev.parent;

	if (t->tx_buf) {
		t->tx_dma = dma_map_single(dev, (void *)t->tx_buf, t->len,
					   DMA_TO_DEVICE);
		if (dma_mapping_error(dev, t->tx_dma)) {
			dev_err(dev, "tx_dma map failed\n");
			return -ENOMEM;
		}
	}

	if (t->rx_buf) {
		t->rx_dma = dma_map_single(dev, t->rx_buf, t->len,
					   DMA_FROM_DEVICE);
		if (dma_mapping_error(dev, t->rx_dma)) {
			if (t->tx_buf)
				dma_unmap_single(dev, t->tx_dma, t->len,
						 DMA_TO_DEVICE);
			dev_err(dev, "rx_dma map failed\n");
			return -ENOMEM;
		}
	}

	return 0;
}

static void meson_spicc_dma_unmap(struct meson_spicc_device *spicc,
				  struct spi_transfer *t)
{
	struct device *dev = spicc->controller->dev.parent;

	if (t->tx_buf)
		dma_unmap_single(dev, t->tx_dma, t->len, DMA_TO_DEVICE);
	if (t->rx_buf)
		dma_unmap_single(dev, t->rx_dma, t->len, DMA_FROM_DEVICE);
}

static u32 meson_spicc_calc_dma_len(struct meson_spicc_device *spicc, u32 *req)
{
	u32 len = spicc->xfer_remain / spicc->bytes_per_word;
	u32 i;

	if (len <= SPICC_FIFO_SIZE) {
		*req = len;
		return len;
	}

	*req = DMA_REQ_DEFAULT;
	if (len == (DMA_BURST_MAX + 1)) {
		len = DMA_BURST_MAX - DMA_REQ_DEFAULT;
	} else if (len >= DMA_BURST_MAX) {
		len = DMA_BURST_MAX;
	} else {
		/* 1 < len < DMA_BURST_MAX */
		for (i = DMA_REQ_DEFAULT; i > 1; i--) {
			if ((len % i) == 0) {
				*req = i;
				return len;
			}
		}

		if ((len % DMA_REQ_DEFAULT) == 1)
			len -= DMA_REQ_DEFAULT;
		len -= len % DMA_REQ_DEFAULT;
	}

	return len;
}

static void meson_spicc_setup_dma_burst(struct meson_spicc_device *spicc)
{
	unsigned int words, req;
	unsigned int count_en = 0;
	unsigned int txfifo_thres = 0;
	unsigned int read_req = 0;
	unsigned int rxfifo_thres = 31;
	unsigned int write_req = 0;
	unsigned int ld_ctr1 = 0;

	words = meson_spicc_calc_dma_len(spicc, &req);

	/* Setup Xfer variables */
	spicc->xfer_remain -= words * spicc->bytes_per_word;

	words /= req;
	if (spicc->tx_buf) {
		count_en |= DMA_READ_COUNTER_EN;
		txfifo_thres = SPICC_FIFO_SIZE + 1 - req;
		read_req = req - 1;
		ld_ctr1 |= FIELD_PREP(DMA_READ_COUNTER, words);
	}

	if (spicc->rx_buf) {
		count_en |= DMA_WRITE_COUNTER_EN;
		rxfifo_thres = req - 1;
		write_req = req - 1;
		ld_ctr1 |= FIELD_PREP(DMA_WRITE_COUNTER, words);
	}

	/* Enable DMA write/read counter */
	writel_relaxed(count_en, spicc->base + SPICC_LD_CNTL0);
	/* Setup burst length */
	writel_relaxed(ld_ctr1, spicc->base + SPICC_LD_CNTL1);

	writel_relaxed(SPICC_DMA_ENABLE
		    | SPICC_DMA_URGENT
		    | FIELD_PREP(SPICC_TXFIFO_THRESHOLD_MASK, txfifo_thres)
		    | FIELD_PREP(SPICC_READ_BURST_MASK, read_req)
		    | FIELD_PREP(SPICC_RXFIFO_THRESHOLD_MASK, rxfifo_thres)
		    | FIELD_PREP(SPICC_WRITE_BURST_MASK, write_req),
		    spicc->base + SPICC_DMAREG);

	//writel_bits_relaxed(SPICC_SMC, SPICC_SMC, spicc->base + SPICC_CONREG);
}

static void meson_spicc_dma_irq(struct meson_spicc_device *spicc)
{
	if (readl_relaxed(spicc->base + SPICC_DMAREG) & SPICC_DMA_ENABLE)
		return;

	if (!spicc->xfer_remain) {
		/* Disable all IRQs */
		writel_relaxed(0, spicc->base + SPICC_INTREG);
		writel_relaxed(0, spicc->base + SPICC_DMAREG);
		writel_relaxed(0, spicc->base + SPICC_LD_CNTL0);
		if (!spicc->is_dma_mapped)
			meson_spicc_dma_unmap(spicc, spicc->xfer);
		if (spicc->xfer != &spicc->async_xfer) {
			spi_finalize_current_transfer(spicc->controller);
		} else {
			dirspi_set_cs(spicc->spi, false);
			if (spicc->complete)
				spicc->complete(spicc->context);
		}
		spicc->using_dma = 0;
		return;
	}

	/* Setup burst */
	meson_spicc_setup_dma_burst(spicc);
}
#endif

static inline bool meson_spicc_txfull(struct meson_spicc_device *spicc)
{
	return !!FIELD_GET(SPICC_TF,
			   readl_relaxed(spicc->base + SPICC_STATREG));
}

static inline bool meson_spicc_rxready(struct meson_spicc_device *spicc)
{
	return FIELD_GET(SPICC_RH | SPICC_RR | SPICC_RF_EN,
			 readl_relaxed(spicc->base + SPICC_STATREG));
}

static inline u32 meson_spicc_pull_data(struct meson_spicc_device *spicc)
{
	unsigned int bytes = spicc->bytes_per_word;
	unsigned int byte_shift = 0;
	u32 data = 0;
	u8 byte;

	spicc->tx_remain--;
	if (!spicc->tx_buf)
		return 0;

	while (bytes--) {
		byte = *spicc->tx_buf++;
		data |= (byte & 0xff) << byte_shift;
		byte_shift += 8;
	}

	return data;
}

static inline void meson_spicc_push_data(struct meson_spicc_device *spicc,
					 u32 data)
{
	unsigned int bytes = spicc->bytes_per_word;
	unsigned int byte_shift = 0;
	u8 byte;

	spicc->rx_remain--;
	if (!spicc->rx_buf)
		return;

	while (bytes--) {
		byte = (data >> byte_shift) & 0xff;
		*spicc->rx_buf++ = byte;
		byte_shift += 8;
	}
}

static inline void meson_spicc_rx(struct meson_spicc_device *spicc)
{
	/* Empty RX FIFO */
	while (spicc->rx_remain &&
	       meson_spicc_rxready(spicc))
		meson_spicc_push_data(spicc,
				readl_relaxed(spicc->base + SPICC_RXDATA));
}

static inline void meson_spicc_tx(struct meson_spicc_device *spicc)
{
	/* Fill Up TX FIFO */
	while (spicc->tx_remain &&
	       !meson_spicc_txfull(spicc))
		writel_relaxed(meson_spicc_pull_data(spicc),
			       spicc->base + SPICC_TXDATA);
}

static inline void meson_spicc_setup_burst(struct meson_spicc_device *spicc,
					   unsigned int burst_len)
{
	/* Setup Xfer variables */
	spicc->tx_remain = burst_len;
	spicc->rx_remain = burst_len;
	spicc->xfer_remain -= burst_len * spicc->bytes_per_word;

	/* Setup burst length */
	writel_bits_relaxed(SPICC_BURSTLENGTH_MASK,
			FIELD_PREP(SPICC_BURSTLENGTH_MASK, burst_len - 1),
			spicc->base + SPICC_CONREG);

	/* Fill TX FIFO */
	meson_spicc_tx(spicc);
}

#ifdef CONFIG_AMLOGIC_MODIFY
static void meson_spicc_hw_prepare(struct meson_spicc_device *spicc, u16 mode)
#else
static void meson_spicc_hw_prepare(struct meson_spicc_device *spicc,
				   u16 mode,
				   u8 bits_per_word,
				   u32 speed_hz)
#endif
{
	u32 conf = 0;

	/* Disable all IRQs */
	writel_relaxed(0, spicc->base + SPICC_INTREG);
	/* Setup no wait cycles by default */
	writel_relaxed(0, spicc->base + SPICC_PERIODREG);

	conf = readl_relaxed(spicc->base + SPICC_TESTREG);
	conf &= ~SPICC_LBC;
	if (mode & SPI_LOOP)
		conf |= SPICC_LBC;
	writel_relaxed(conf, spicc->base + SPICC_TESTREG);

	conf = readl_relaxed(spicc->base + SPICC_CONREG);
#ifdef CONFIG_AMLOGIC_MODIFY
	/* Setup burst length max */
	conf |= SPICC_BURSTLENGTH_MASK;
	conf &= ~(SPICC_POL | SPICC_PHA | SPICC_SSPOL | SPI_READY
		  | SPICC_SSCTL | SPICC_SMC | SPICC_XCH);
	if (spicc->toggle_cs_every_word)
		conf |= SPICC_SSCTL;
#else
	/* clear all except following */
	conf &= SPICC_ENABLE | SPICC_MODE_MASTER | SPICC_DATARATE_MASK;
#endif

	/* Setup transfer mode */
	if (mode & SPI_CPOL)
		conf |= SPICC_POL;

	if (mode & SPI_CPHA)
		conf |= SPICC_PHA;

	if (mode & SPI_CS_HIGH)
		conf |= SPICC_SSPOL;

	if (mode & SPI_READY)
		conf |= FIELD_PREP(SPICC_DRCTL_MASK, SPICC_DRCTL_LOWLEVEL);

#ifndef CONFIG_AMLOGIC_MODIFY
	/* Default 8bit word */
	if (!bits_per_word)
		bits_per_word = 8;
	conf |= FIELD_PREP(SPICC_BITLENGTH_MASK, bits_per_word - 1);
#endif
	writel_relaxed(conf, spicc->base + SPICC_CONREG);

#ifndef CONFIG_AMLOGIC_MODIFY
	/* Setup clock speed */
	if (speed_hz && spicc->speed_hz != speed_hz) {
		spicc->speed_hz = speed_hz;
		if (clk_set_rate(spicc->clk, speed_hz)) {
			dev_err(&spicc->pdev->dev, "set clk rate failed\n");
			return;
		}
		meson_spicc_auto_io_delay(spicc, !!(mode & SPI_LOOP));
	}
#endif

	/* Reset tx/rx-fifo maybe there are some residues left by DMA */
	writel_bits_relaxed(SPICC_FIFORST_MASK, SPICC_FIFORST_MASK,
			    spicc->base + SPICC_TESTREG);
	/* Clean out rx-fifo */
	while (meson_spicc_rxready(spicc))
		conf = readl_relaxed(spicc->base + SPICC_RXDATA);
}

static irqreturn_t meson_spicc_irq(int irq, void *data)
{
	struct meson_spicc_device *spicc = (void *) data;
	unsigned int burst_len;

	/* Clear TC bit */
	writel_relaxed(SPICC_TC, spicc->base + SPICC_STATREG);

#ifdef CONFIG_AMLOGIC_MODIFY
	if (spicc->using_dma) {
		meson_spicc_dma_irq(spicc);
		return IRQ_HANDLED;
	}
#endif

	if (spicc->clk2cs_ns)
		udelay(DIV_ROUND_UP(spicc->clk2cs_ns, 1000));
	/* Empty RX FIFO */
	meson_spicc_rx(spicc);

	/* Enable TC interrupt since we transferred everything */
	if (!spicc->xfer_remain) {
		if (spicc->xfer != &spicc->async_xfer) {
			spi_finalize_current_transfer(spicc->controller);
		} else {
			dirspi_set_cs(spicc->spi, false);
			if (spicc->complete)
				spicc->complete(spicc->context);
		}
		return IRQ_HANDLED;
	}

	burst_len = min_t(unsigned int,
			  spicc->xfer_remain / spicc->bytes_per_word,
			  SPICC_FIFO_SIZE);

	/* Setup burst */
	meson_spicc_setup_burst(spicc, burst_len);

	/* Restart burst */
	writel_bits_relaxed(SPICC_XCH, SPICC_XCH, spicc->base + SPICC_CONREG);

	return IRQ_HANDLED;
}

static int meson_spicc_transfer_one(struct spi_controller *ctlr,
				    struct spi_device *spi,
				    struct spi_transfer *xfer)
{
	struct meson_spicc_device *spicc = spi_controller_get_devdata(ctlr);
	unsigned int burst_len;

	/* Store current transfer */
	spicc->xfer = xfer;

	/* Setup transfer parameters */
	spicc->tx_buf = (u8 *)xfer->tx_buf;
	spicc->rx_buf = (u8 *)xfer->rx_buf;
	spicc->xfer_remain = xfer->len;

	/* Pre-calculate word size */
	spicc->bytes_per_word =
	   DIV_ROUND_UP(spicc->xfer->bits_per_word, 8);

	/* Setup transfer parameters */
#ifdef CONFIG_AMLOGIC_MODIFY
	meson_spicc_hw_prepare(spicc, spi->mode);
	meson_spicc_set_width(spicc, xfer->bits_per_word);
	if (spicc->xfer != &spicc->async_xfer)
		meson_spicc_set_speed(spicc, xfer->speed_hz);
	if (!xfer->len)
		return 0;
#else
	if (xfer->bits_per_word != spi->bits_per_word ||
	    xfer->speed_hz != spi->max_speed_hz)
		meson_spicc_hw_prepare(spicc, spi->mode, xfer->bits_per_word,
				       xfer->speed_hz);
#endif

	spicc->using_dma = 0;
	writel_relaxed(0, spicc->base + SPICC_DMAREG);
	if (xfer->bits_per_word == 64 && (spicc->is_dma_mapped ||
					  !meson_spicc_dma_map(spicc, xfer))) {
		spicc->using_dma = 1;
		writel_relaxed(xfer->tx_dma, spicc->base + SPICC_DRADDR);
		writel_relaxed(xfer->rx_dma, spicc->base + SPICC_DWADDR);
		writel_relaxed(xfer->speed_hz >> 25,
			       spicc->base + SPICC_PERIODREG);
		meson_spicc_setup_dma_burst(spicc);
		writel_relaxed(SPICC_TE_EN, spicc->base + SPICC_INTREG);
		writel_bits_relaxed(SPICC_SMC, SPICC_SMC,
				    spicc->base + SPICC_CONREG);

	} else {
		burst_len = min_t(unsigned int,
				  spicc->xfer_remain / spicc->bytes_per_word,
				  SPICC_FIFO_SIZE);
		meson_spicc_setup_burst(spicc, burst_len);
		writel_relaxed(spi_controller_is_slave(spicc->controller) ?
			SPICC_RR_EN : SPICC_TC_EN, spicc->base + SPICC_INTREG);
		/* Start burst */
		writel_bits_relaxed(SPICC_XCH, SPICC_XCH,
				    spicc->base + SPICC_CONREG);
	}

	return 1;
}

static int meson_spicc_prepare_message(struct spi_controller *ctlr,
				       struct spi_message *message)
{
	struct meson_spicc_device *spicc = spi_controller_get_devdata(ctlr);
	struct spi_device *spi = message->spi;

	/* Store current message */
	spicc->is_dma_mapped = message->is_dma_mapped;
#ifdef CONFIG_AMLOGIC_MODIFY
	meson_spicc_hw_prepare(spicc, spi->mode);
	meson_spicc_set_width(spicc, spi->bits_per_word);
	return meson_spicc_set_speed(spicc, spi->max_speed_hz);
#else
	meson_spicc_hw_prepare(spicc, spi->mode, spi->bits_per_word,
			       spi->max_speed_hz);
	return 0;
#endif
}

static int meson_spicc_unprepare_transfer(struct spi_controller *ctlr)
{
	struct meson_spicc_device *spicc = spi_controller_get_devdata(ctlr);

	/* Disable all IRQs */
	writel(0, spicc->base + SPICC_INTREG);

#ifndef CONFIG_AMLOGIC_MODIFY
	/* Disable controller */
	writel_bits_relaxed(SPICC_ENABLE, 0, spicc->base + SPICC_CONREG);
#endif

	device_reset_optional(&spicc->pdev->dev);

	return 0;
}

static int meson_spicc_setup(struct spi_device *spi)
{
#ifdef CONFIG_AMLOGIC_MODIFY
	struct meson_spicc_device *spicc = spi_controller_get_devdata(spi->controller);
#endif
	int ret = 0;

	if (!spi->controller_state)
		spi->controller_state = spi_controller_get_devdata(spi->controller);
	else if (gpio_is_valid(spi->cs_gpio))
		goto out_gpio;
	else if (spi->cs_gpio == -ENOENT)
		return 0;

	if (gpio_is_valid(spi->cs_gpio)) {
		ret = gpio_request(spi->cs_gpio, dev_name(&spi->dev));
		if (ret) {
			dev_err(&spi->dev, "failed to request cs gpio\n");
			return ret;
		}
#ifndef CONFIG_AMLOGIC_MODIFY
	}
#else
	} else {
		dev_err(&spi->dev, "cs gpio invalid\n");
		return 0;
	}
#endif

out_gpio:
	ret = gpio_direction_output(spi->cs_gpio,
			!(spi->mode & SPI_CS_HIGH));
#ifdef CONFIG_AMLOGIC_MODIFY
	meson_spicc_set_speed(spicc, spi->max_speed_hz);
#endif

	return ret;
}

static void meson_spicc_cleanup(struct spi_device *spi)
{
	if (gpio_is_valid(spi->cs_gpio))
		gpio_free(spi->cs_gpio);

	spi->controller_state = NULL;
}

#ifdef CONFIG_AMLOGIC_MODIFY
static int meson_spicc_clk_enable(struct meson_spicc_device *spicc)
{
	int ret;

	ret = clk_prepare_enable(spicc->core);
	if (ret)
		return ret;

	ret = clk_prepare_enable(spicc->clk);
	if (ret)
		return ret;

	if (spicc->data->has_async_clk) {
		ret = clk_prepare_enable(spicc->async_clk);
		if (ret)
			return ret;
	}

	return 0;
}

static void meson_spicc_clk_disable(struct meson_spicc_device *spicc)
{
	if (!IS_ERR_OR_NULL(spicc->core))
		clk_disable_unprepare(spicc->core);

	if (!IS_ERR_OR_NULL(spicc->clk))
		clk_disable_unprepare(spicc->clk);

	if (spicc->data->has_async_clk && !IS_ERR_OR_NULL(spicc->async_clk))
		clk_disable_unprepare(spicc->async_clk);
}

static int meson_spicc_hw_init(struct meson_spicc_device *spicc)
{
	/* For GX/AXG, all registers except CONREG are unavailable if
	 * SPICC_ENABLE is 0, which will be also reset to default value
	 * if it changed from 1 to 0.
	 * Set controller mode and enable controller ahead of others here,
	 * and never disable them.
	 */
	if (!spi_controller_is_slave(spicc->controller)) {
		writel_relaxed(SPICC_ENABLE | SPICC_MODE_MASTER,
			       spicc->base + SPICC_CONREG);
		if (spicc->data->has_oen)
			writel_relaxed(0xf020000, spicc->base + SPICC_ENH_CTL0);
	} else {
		writel_relaxed(SPICC_ENABLE, spicc->base + SPICC_CONREG);
		writel_relaxed((spicc->latency > 0) ? 0 :
			SPICC_ENH_DELAY_EN | SPICC_ENH_SI_CAP_DELAY_EN,
			spicc->base + SPICC_ENH_CTL1);
	}

	/* Disable all IRQs */
	writel_relaxed(0, spicc->base + SPICC_INTREG);

	return 0;
}

#ifdef MESON_SPICC_HW_IF
static int dirspi_wait_complete(struct meson_spicc_device *spicc, int times)
{
	u32 sta;

	while (times--) {
		sta = readl_relaxed(spicc->base + SPICC_STATREG);
		sta &= SPICC_TC | SPICC_TE;
		if (sta == (u32)(SPICC_TC | SPICC_TE)) {
			/* set 1 to clear */
			writel_bits_relaxed(SPICC_TC, SPICC_TC,
					    spicc->base + SPICC_STATREG);
			return 0;
		}
	}

	dev_err(&spicc->pdev->dev, "dirspi stareg 0x%x\n",
		readl_relaxed(spicc->base + SPICC_STATREG));
	return -ETIMEDOUT;
}

int dirspi_xfer(struct spi_device *spi, u8 *tx_buf, u8 *rx_buf, int len)
{
	struct meson_spicc_device *spicc = spi_controller_get_devdata(spi->controller);
	unsigned int burst_len, rx_remain;
	int ret = 0;

	spicc->tx_buf = tx_buf;
	spicc->rx_buf = rx_buf;
	spicc->rx_remain = 0;
	spicc->xfer_remain = len;

	/* Pre-calculate word size */
	spicc->bytes_per_word = DIV_ROUND_UP(spi->bits_per_word, 8);

	writel_relaxed(0, spicc->base + SPICC_INTREG);
	writel_relaxed(0, spicc->base + SPICC_DMAREG);

	while (spicc->xfer_remain) {
		rx_remain = spicc->rx_remain;
		burst_len = min_t(unsigned int,
				  spicc->xfer_remain / spicc->bytes_per_word,
				  SPICC_FIFO_SIZE - rx_remain);
		meson_spicc_setup_burst(spicc, burst_len);
		spicc->rx_remain += rx_remain;

		/* Start burst */
		writel_bits_relaxed(SPICC_XCH, SPICC_XCH,
				    spicc->base + SPICC_CONREG);

		ret = dirspi_wait_complete(spicc, burst_len << 13);
		if (ret) {
			dev_err(&spi->dev, "wait TC/TE timeout\n");
			break;
		}

		meson_spicc_rx(spicc);
	}

	return ret;
}
EXPORT_SYMBOL(dirspi_xfer);

int dirspi_write(struct spi_device *spi, u8 *buf, int len)
{
	return dirspi_xfer(spi, buf, 0, len);
}
EXPORT_SYMBOL(dirspi_write);

int dirspi_read(struct spi_device *spi, u8 *buf, int len)
{
	return dirspi_xfer(spi, 0, buf, len);
}
EXPORT_SYMBOL(dirspi_read);

static void dirspi_set_cs(struct spi_device *spi, bool enable)
{
	if (spi->mode & SPI_NO_CS)
		return;

	if (spi->mode & SPI_CS_HIGH)
		enable = !enable;

	if (spi->cs_gpiod)
		gpiod_set_value(spi->cs_gpiod, !enable);
	else if (gpio_is_valid(spi->cs_gpio))
		gpio_set_value(spi->cs_gpio, !enable);
}

void dirspi_start(struct spi_device *spi)
{
#ifndef CONFIG_AMLOGIC_MODIFY
	struct meson_spicc_device *spicc = spi_controller_get_devdata(spi->controller);
#endif

	if (spi->controller->auto_runtime_pm)
		pm_runtime_get_sync(spi->controller->dev.parent);
#ifndef CONFIG_AMLOGIC_MODIFY
	meson_spicc_hw_prepare(spicc, spi->mode, spi->bits_per_word,
			       spi->max_speed_hz);
#endif

	dirspi_set_cs(spi, true);
}
EXPORT_SYMBOL(dirspi_start);

void dirspi_stop(struct spi_device *spi)
{
	dirspi_set_cs(spi, false);
}
EXPORT_SYMBOL(dirspi_stop);

int dirspi_async(struct spi_device *spi, u8 *tx_buf, u8 *rx_buf, int len,
		 void (*complete)(void *context), void *context)
{
	struct meson_spicc_device *spicc = spi_controller_get_devdata(spi->controller);
	struct spi_transfer *t = &spicc->async_xfer;

	spicc->spi = spi;
	spicc->is_dma_mapped = 0;
	spicc->complete = complete;
	spicc->context = context;
	t->bits_per_word = spi->bits_per_word;
	t->speed_hz = spi->max_speed_hz;
	t->tx_buf = (void *)tx_buf;
	t->rx_buf = (void *)rx_buf;
	t->len = len;

	dirspi_start(spi);
	meson_spicc_transfer_one(spi->controller, spi, t);

	return 0;
}
EXPORT_SYMBOL(dirspi_async);

static void dirspi_complete(void *arg)
{
	complete(arg);
}

int dirspi_sync(struct spi_device *spi, u8 *tx_buf, u8 *rx_buf, int len)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int ret;

	dirspi_async(spi, tx_buf, rx_buf, len, dirspi_complete, &done);
	ret = wait_for_completion_timeout(&done, msecs_to_jiffies(200));

	return ret ? 0 : -ETIMEDOUT;
}
EXPORT_SYMBOL(dirspi_sync);

#endif

#define TEST_PARAM_NUM 5
static ssize_t test_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct meson_spicc_device *spicc = dev_get_drvdata(dev);
	unsigned int cs_gpio, speed, mode, bits_per_word, num;
	u8 *tx_buf, *rx_buf;
	unsigned long value;
	char *kstr, *str_temp, *token;
	int i, ret;
	struct spi_transfer t;
	struct spi_message m;

	if (sscanf(buf, "%d%d%x%d%d", &cs_gpio, &speed,
		   &mode, &bits_per_word, &num) != TEST_PARAM_NUM) {
		dev_err(dev, "error format\n");
		return count;
	}

	kstr = kstrdup(buf, GFP_KERNEL);
	tx_buf = kzalloc(num, GFP_KERNEL | GFP_DMA);
	rx_buf = kzalloc(num, GFP_KERNEL | GFP_DMA);
	if (IS_ERR(kstr) || IS_ERR(tx_buf) || IS_ERR(rx_buf)) {
		dev_err(dev, "failed to alloc tx rx buffer\n");
		goto test_end2;
	}

	str_temp = kstr;
	/* pass over "cs_gpio speed mode bits_per_word num" */
	for (i = 0; i < TEST_PARAM_NUM; i++)
		strsep(&str_temp, ", ");

	/* filled tx buffer with user data */
	for (i = 0; i < num; i++) {
		token = strsep(&str_temp, ", ");
		if (token == 0 || kstrtoul(token, 16, &value))
			break;
		tx_buf[i] = (u8)(value & 0xff);
	}

	/* set first tx data default 1 if no any user data */
	if (i == 0) {
		tx_buf[0] = 0x1;
		i++;
	}

	/* filled next buffer incrementally if user data not enough */
	for (; i < num; i++)
		tx_buf[i] = tx_buf[i - 1] + 1;

	spi_message_init(&m);
	m.spi = spi_alloc_device(spicc->controller);
	if (IS_ERR_OR_NULL(m.spi)) {
		dev_err(dev, "spi alloc failed\n");
		goto test_end;
	}

	m.spi->cs_gpio = (cs_gpio > 0) ? cs_gpio : -ENOENT;
	m.spi->max_speed_hz = speed;
	m.spi->mode = mode & 0xffff;
	m.spi->bits_per_word = bits_per_word;
	if (spi_setup(m.spi)) {
		dev_err(dev, "setup failed\n");
		goto test_end;
	}

	memset(&t, 0, sizeof(t));
	t.tx_buf = (void *)tx_buf;
	t.rx_buf = (void *)rx_buf;
	t.len = num;
	spi_message_add_tail(&t, &m);
	ret = spi_sync(m.spi, &m);

	if (!ret && (mode & (SPI_LOOP | (1 << 16)))) {
		ret = 0;
		for (i = 0; i < num; i++) {
			if (tx_buf[i] != rx_buf[i]) {
				ret++;
				pr_info("[%d]: 0x%x, 0x%x\n",
					i, tx_buf[i], rx_buf[i]);
			}
		}
		dev_info(dev, "total %d, failed %d\n", num, ret);
	}
	dev_info(dev, "test end @%d\n", (u32)clk_get_rate(spicc->clk));

test_end:
	meson_spicc_cleanup(m.spi);
	spi_dev_put(m.spi);
test_end2:
	kfree(kstr);
	kfree(tx_buf);
	kfree(rx_buf);
	return count;
}

static DEVICE_ATTR_WO(test);

/*
 * The Clock Mux
 *            x-----------------x   x---------------x    x------\
 *        |---| 0) fixed factor |---| 1) power2 div |----|      |
 *        |   x-----------------x   x---------------x    |      |
 * src ---|                                              |5) mux|-- out
 *        |   x-----------------x   x---------------x    |      |
 *        |---| 2) fixed factor |---| 3) linear div |----|      |
 *            x-----------------x   x---------------x    x------/
 *
 * Clk path for GX series:
 *    src = core clk
 *    src -> 0 -> 1 -> out
 *
 * Clk path for AXG series:
 *    src = core clk
 *    src -> 0 -> 1 -> 5 -> out
 *    src -> 2 -> 3 -> 5 -> out
 *
 * Clk path for G12 series:
 *    src = async clk
 *    src -> 0 -> 1 -> 5 -> out
 *    src -> 2 -> 3 -> 5 -> out
 */
static struct clk_div_table power2_div_table[9] = {
	{0, 4}, {1, 8}, {2, 16}, {3, 32},
	{4, 64}, {5, 128}, {6, 256}, {7, 512}, {0}
};

static struct clk_div_table linear_div_table[255] = {
	[0] = {0},
	[254] = {0}
};

static struct clk *
meson_spicc_divider_clk_get(struct meson_spicc_device *spicc, bool is_linear)
{
	struct device *dev = &spicc->pdev->dev;
	struct clk_init_data init;
	struct clk_divider *div;
	struct clk *clk;
	const char *parent_names[1];
	char name[32], *which;
	bool is_parent_async = spicc->data->is_div_parent_async_clk;

	div = devm_kzalloc(dev, sizeof(*div), GFP_KERNEL);
	if (!div)
		return ERR_PTR(-ENOMEM);

	div->flags = spicc->clk_div_none ? 0 : CLK_DIVIDER_ROUND_CLOSEST;
	if (is_linear) {
		which = "linear";
		div->reg = spicc->base + SPICC_ENH_CTL0;
		div->shift = SPICC_ENH_DATARATE_SHIFT;
		div->width = SPICC_ENH_DATARATE_WIDTH;
		div->table = linear_div_table;
	} else {
		which = "power2";
		div->reg = spicc->base + SPICC_CONREG;
		div->shift = SPICC_DATARATE_SHIFT;
		div->width = SPICC_DATARATE_WIDTH;
		div->table = power2_div_table;
	}

	/* Register clk-divider */
	clk = is_parent_async ? spicc->async_clk : spicc->core;
	parent_names[0] = __clk_get_name(clk);
	snprintf(name, sizeof(name), "%s_%s_div", dev_name(dev), which);
	init.name = name;
	init.ops = &clk_divider_ops;
	init.flags = ((!is_parent_async) || spicc->parent_clk_fixed) ?
		     0 : CLK_SET_RATE_PARENT;
	init.parent_names = parent_names;
	init.num_parents = 1;
	div->hw.init = &init;
	clk = devm_clk_register(dev, &div->hw);

	return clk;
}

static struct clk *
meson_spicc_mux_clk_get(struct meson_spicc_device *spicc,
			const char * const *parent_names)
{
	struct device *dev = &spicc->pdev->dev;
	struct clk_init_data init;
	struct clk_mux *mux;
	struct clk *clk;
	char name[32];

	mux = devm_kzalloc(dev, sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return ERR_PTR(-ENOMEM);

	snprintf(name, sizeof(name), "%s_mux", dev_name(dev));
	init.name = name;
	init.ops = &clk_mux_ops;
	init.parent_names = parent_names;
	init.num_parents = 2;
	init.flags = CLK_SET_RATE_PARENT;

	mux->mask = 0x1;
	mux->shift = 24;
	mux->reg = spicc->base + SPICC_ENH_CTL0;
	mux->hw.init = &init;
	clk = devm_clk_register(dev, &mux->hw);

	return clk;
}

static struct clk *meson_spicc_clk_get(struct meson_spicc_device *spicc)
{
	struct clk *clk;
	const char *parent_names[2];
	int i;

	if (!linear_div_table[0].div)
		for (i = 0; i < ARRAY_SIZE(linear_div_table) - 1; i++) {
			linear_div_table[i].val = i + 2;
			linear_div_table[i].div = (i + 3) * 2;
		}

	/* Get power-of-two divider clk */
	clk = meson_spicc_divider_clk_get(spicc, 0);
	if (!IS_ERR_OR_NULL(clk) && spicc->data->has_linear_div) {
		/* Set power-of-two as the 1st parent */
		parent_names[0] = __clk_get_name(clk);

		/* Get linear divider clk */
		clk = meson_spicc_divider_clk_get(spicc, 1);
		if (!IS_ERR_OR_NULL(clk)) {
			/* Set linear as the 2nd parent */
			parent_names[1] = __clk_get_name(clk);

			/* Get mux clk */
			clk = meson_spicc_mux_clk_get(spicc, parent_names);
		}
	}

	return clk;
}

#ifdef CONFIG_PM_SLEEP
/* The clk data rate setting is handled by clk core. We have to save/restore
 * it when system suspend/resume.
 */
static void meson_spicc_hw_clk_save(struct meson_spicc_device *spicc)
{
	spicc->backup_con = readl_relaxed(spicc->base + SPICC_CONREG) &
			    SPICC_DATARATE_MASK;
	spicc->backup_enh0 = readl_relaxed(spicc->base + SPICC_ENH_CTL0) &
			     (SPICC_ENH_DATARATE_MASK | SPICC_ENH_DATARATE_EN);
	spicc->backup_test = readl_relaxed(spicc->base + SPICC_TESTREG) &
			     SPICC_DELAY_MASK;
}

static void meson_spicc_hw_clk_restore(struct meson_spicc_device *spicc)
{
	writel_bits_relaxed(SPICC_DATARATE_MASK, spicc->backup_con,
			    spicc->base + SPICC_CONREG);
	writel_bits_relaxed(SPICC_ENH_DATARATE_MASK | SPICC_ENH_DATARATE_EN,
			    spicc->backup_enh0, spicc->base + SPICC_ENH_CTL0);
	writel_bits_relaxed(SPICC_DELAY_MASK, spicc->backup_test,
			    spicc->base + SPICC_TESTREG);
}
#endif
#endif

static int meson_spicc_probe(struct platform_device *pdev)
{
	struct spi_controller *ctlr;
	struct meson_spicc_data *match;
	struct meson_spicc_device *spicc;
	int ret, irq, rate;

	ctlr = __spi_alloc_controller(&pdev->dev, sizeof(*spicc),
			of_property_read_bool(pdev->dev.of_node, "slave"));
	if (!ctlr) {
		dev_err(&pdev->dev, "controller allocation failed\n");
		return -ENOMEM;
	}
	spicc = spi_controller_get_devdata(ctlr);
	spicc->controller = ctlr;

	spicc->pdev = pdev;
	platform_set_drvdata(pdev, spicc);

	spicc->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR_OR_NULL(spicc->base)) {
		dev_err(&pdev->dev, "io resource mapping failed\n");
		ret = PTR_ERR(spicc->base);
		goto out_controller;
	}

#ifdef CONFIG_AMLOGIC_MODIFY
	match = (struct meson_spicc_data *)
		of_device_get_match_data(&pdev->dev);
	spicc->data = devm_kzalloc(&pdev->dev, sizeof(*match), GFP_KERNEL);
	spicc->data->max_speed_hz = match->max_speed_hz;
	spicc->data->has_linear_div = match->has_linear_div;
	spicc->data->has_oen = match->has_oen;
	spicc->data->has_async_clk = match->has_async_clk;
	spicc->data->is_div_parent_async_clk = match->is_div_parent_async_clk;

	meson_spicc_hw_init(spicc);
#else
	/* Disable all IRQs */
	writel_relaxed(0, spicc->base + SPICC_INTREG);
#endif

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto out_controller;
	}

	ret = devm_request_irq(&pdev->dev, irq, meson_spicc_irq,
			       0, NULL, spicc);
	if (ret) {
		dev_err(&pdev->dev, "irq request failed\n");
		goto out_controller;
	}

	spicc->core = devm_clk_get(&pdev->dev, "core");
	if (IS_ERR_OR_NULL(spicc->core)) {
		dev_err(&pdev->dev, "core clock request failed\n");
		ret = PTR_ERR(spicc->core);
		goto out_controller;
	}

	ret = clk_prepare_enable(spicc->core);
	if (ret) {
		dev_err(&pdev->dev, "core clock enable failed\n");
		goto out_controller;
	}
	rate = clk_get_rate(spicc->core);

#ifdef CONFIG_AMLOGIC_MODIFY
	spicc->latency = 0;
	spicc->parent_clk_fixed = false;
	spicc->toggle_cs_every_word = false;
	of_property_read_s32(pdev->dev.of_node, "latency", &spicc->latency);
	if (of_property_read_bool(pdev->dev.of_node, "parent_clk_fixed"))
		spicc->parent_clk_fixed = true;
	if (of_property_read_bool(pdev->dev.of_node, "toggle_cs_every_word"))
		spicc->toggle_cs_every_word = true;

	of_property_read_s32(pdev->dev.of_node, "cs2clk-us",
			     &spicc->cs2clk_ns);
	of_property_read_s32(pdev->dev.of_node, "clk2cs-us",
			     &spicc->clk2cs_ns);
	spicc->cs2clk_ns *= 1000;
	spicc->clk2cs_ns *= 1000;

	spicc->clk_div_none = false;
	if (of_property_read_bool(pdev->dev.of_node, "clk_div_none"))
		spicc->clk_div_none = true;

	if (spicc->data->has_async_clk) {
		/* SoCs has async-clk is incapable of using full burst */
		SPICC_FIFO_SIZE = 15;

		spicc->async_clk = devm_clk_get(&pdev->dev, "async");
		if (IS_ERR_OR_NULL(spicc->async_clk)) {
			dev_err(&pdev->dev, "async clock request failed\n");
			ret = PTR_ERR(spicc->async_clk);
			goto out_clk_1;
		}

		ret = clk_prepare_enable(spicc->async_clk);
		if (ret) {
			dev_err(&pdev->dev, "async clock enable failed\n");
			goto out_clk_1;
		}
	}

	if (spi_controller_is_slave(spicc->controller))
		goto dev_test;

	spicc->clk = meson_spicc_clk_get(spicc);
	if (IS_ERR_OR_NULL(spicc->clk)) {
		dev_err(&pdev->dev, "divider clock get failed\n");
		ret = PTR_ERR(spicc->clk);
		goto out_clk_2;
	}

	ret = clk_prepare_enable(spicc->clk);
	if (ret) {
		dev_err(&pdev->dev, "divider clock enable failed\n");
		goto out_clk_2;
	}

dev_test:
	ret = device_create_file(&pdev->dev, &dev_attr_test);
	if (ret) {
		dev_err(&pdev->dev, "Create test attribute failed\n");
		goto out_clk_3;
	}
#endif

	device_reset_optional(&pdev->dev);

	ctlr->num_chipselect = 4;
	ctlr->dev.of_node = pdev->dev.of_node;
#ifdef CONFIG_AMLOGIC_MODIFY
	ctlr->mode_bits = SPI_CPHA | SPI_CPOL | SPI_CS_HIGH | SPI_LOOP;
#else
	ctlr->mode_bits = SPI_CPHA | SPI_CPOL | SPI_CS_HIGH;
	ctlr->bits_per_word_mask = SPI_BPW_MASK(32) |
				     SPI_BPW_MASK(24) |
				     SPI_BPW_MASK(16) |
				     SPI_BPW_MASK(8);
	ctlr->flags = (SPI_CONTROLLER_MUST_RX | SPI_CONTROLLER_MUST_TX);
#endif
	ctlr->min_speed_hz = rate >> 9;
	ctlr->setup = meson_spicc_setup;
	ctlr->cleanup = meson_spicc_cleanup;
	ctlr->prepare_message = meson_spicc_prepare_message;
	ctlr->unprepare_transfer_hardware = meson_spicc_unprepare_transfer;
	ctlr->transfer_one = meson_spicc_transfer_one;
	/* Setup max rate according to the Meson GX datasheet */
#ifdef CONFIG_AMLOGIC_MODIFY
	ctlr->max_speed_hz = spicc->data->max_speed_hz;
#else
	if ((rate >> 2) > SPICC_MAX_FREQ)
		ctlr->max_speed_hz = SPICC_MAX_FREQ;
	else
		ctlr->max_speed_hz = rate >> 2;
#endif

	ret = devm_spi_register_master(&pdev->dev, ctlr);
	if (ret) {
		dev_err(&pdev->dev, "spi controller registration failed\n");
		goto out_clk;
	}

#ifdef CONFIG_AMLOGIC_MODIFY
#ifdef CONFIG_PM_SLEEP
	//pm_runtime_set_autosuspend_delay(&pdev->dev, 500);
	//pm_runtime_use_autosuspend(&pdev->dev);
	ctlr->auto_runtime_pm = false;
	//pm_runtime_enable(&pdev->dev);
	//meson_spicc_hw_clk_save(spicc);
#endif
#endif

	return 0;

out_clk:
#ifdef CONFIG_AMLOGIC_MODIFY
	device_remove_file(&pdev->dev, &dev_attr_test);
out_clk_3:
	clk_disable_unprepare(spicc->clk);
out_clk_2:
	if (spicc->data->has_async_clk)
		clk_disable_unprepare(spicc->async_clk);
out_clk_1:
#endif
	clk_disable_unprepare(spicc->core);

out_controller:
	spi_controller_put(ctlr);

	return ret;
}

static int meson_spicc_remove(struct platform_device *pdev)
{
	struct meson_spicc_device *spicc = platform_get_drvdata(pdev);

	/* Disable SPI */
	writel(0, spicc->base + SPICC_CONREG);

	clk_disable_unprepare(spicc->core);
#ifdef CONFIG_AMLOGIC_MODIFY
	if (spicc->data->has_async_clk)
		clk_disable_unprepare(spicc->async_clk);
	clk_disable_unprepare(spicc->clk);
	device_remove_file(&pdev->dev, &dev_attr_test);
#endif

	return 0;
}

#ifdef CONFIG_AMLOGIC_MODIFY
#ifdef CONFIG_PM_SLEEP
static int meson_spicc_suspend(struct device *dev)
{
	struct meson_spicc_device *spicc = dev_get_drvdata(dev);

	return spi_controller_suspend(spicc->controller);
}

static int meson_spicc_resume(struct device *dev)
{
	struct meson_spicc_device *spicc = dev_get_drvdata(dev);

	return spi_controller_resume(spicc->controller);
}
#endif /* CONFIG_PM_SLEEP */

static int meson_spicc_runtime_suspend(struct device *dev)
{
	struct meson_spicc_device *spicc = dev_get_drvdata(dev);

	meson_spicc_hw_clk_save(spicc);
	meson_spicc_clk_disable(spicc);

	spi_controller_put(spicc->controller);

	return 0;
}

static int meson_spicc_runtime_resume(struct device *dev)
{
	struct meson_spicc_device *spicc = dev_get_drvdata(dev);

	meson_spicc_hw_init(spicc);
	meson_spicc_hw_clk_restore(spicc);

	return meson_spicc_clk_enable(spicc);
}

static const struct dev_pm_ops meson_spicc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(meson_spicc_suspend, meson_spicc_resume)
	SET_RUNTIME_PM_OPS(meson_spicc_runtime_suspend,
			   meson_spicc_runtime_resume,
			   NULL)
};

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
static struct meson_spicc_data meson_spicc_gx_data __initdata = {
	.max_speed_hz = 30000000,
};

static struct meson_spicc_data meson_spicc_axg_data __initdata = {
	.max_speed_hz = 80000000,
	.has_linear_div = true,
	.has_oen = true,
	.has_async_clk = true,
};
#endif

static struct meson_spicc_data meson_spicc_g12_data __initdata = {
	.max_speed_hz = 124000000,
	.has_linear_div = true,
	.has_oen = true,
	.has_async_clk = true,
	.is_div_parent_async_clk = true,
};
#endif

static const struct of_device_id meson_spicc_of_match[] = {
#ifdef CONFIG_AMLOGIC_MODIFY
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	{
		.compatible = "amlogic,meson-gx-spicc",
		.data = &meson_spicc_gx_data,
	},
	{
		.compatible = "amlogic,meson-axg-spicc",
		.data = &meson_spicc_axg_data,
	},
#endif
	{
		.compatible = "amlogic,meson-g12-spicc",
		.data = &meson_spicc_g12_data,
	},
#else
	.compatible = "amlogic,meson-gx-spicc",
	.compatible = "amlogic,meson-axg-spicc"
#endif
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, meson_spicc_of_match);

static struct platform_driver meson_spicc_driver = {
	.probe   = meson_spicc_probe,
	.remove  = meson_spicc_remove,
	.driver  = {
		.name = "meson-spicc",
		.of_match_table = of_match_ptr(meson_spicc_of_match),
#ifdef CONFIG_AMLOGIC_MODIFY
		//.pm = &meson_spicc_pm_ops,
#endif
	},
};

module_platform_driver(meson_spicc_driver);

MODULE_DESCRIPTION("Meson SPI Communication Controller driver");
MODULE_AUTHOR("Neil Armstrong <narmstrong@baylibre.com>");
MODULE_LICENSE("GPL");
