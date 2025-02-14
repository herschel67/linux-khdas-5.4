// SPDX-License-Identifier: GPL-2.0-only
/*
 * Amlogic Meson8b, Meson8m2 and GXBB DWMAC glue layer
 *
 * Copyright (C) 2016 Martin Blumenstingl <martin.blumenstingl@googlemail.com>
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/device.h>
#include <linux/ethtool.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_net.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>
#include <linux/stmmac.h>
#include <linux/arm-smccc.h>

#include "stmmac_platform.h"
#include <linux/amlogic/scpi_protocol.h>

#ifdef CONFIG_AMLOGIC_ETH_PRIVE
#include <linux/input.h>
#include <linux/amlogic/pm.h>
#endif

#define PRG_ETH0			0x0
#define PRG_ETH1			0x4

#define PRG_ETH0_RGMII_MODE		BIT(0)

#define PRG_ETH0_EXT_PHY_MODE_MASK	GENMASK(2, 0)
#define PRG_ETH0_EXT_RGMII_MODE		1
#define PRG_ETH0_EXT_RMII_MODE		4

/* mux to choose between fclk_div2 (bit unset) and mpll2 (bit set) */
#define PRG_ETH0_CLK_M250_SEL_SHIFT	4
#define PRG_ETH0_CLK_M250_SEL_MASK	GENMASK(4, 4)

#define PRG_ETH0_TXDLY_SHIFT		5
#define PRG_ETH0_TXDLY_MASK		GENMASK(6, 5)

/* divider for the result of m250_sel */
#define PRG_ETH0_CLK_M250_DIV_SHIFT	7
#define PRG_ETH0_CLK_M250_DIV_WIDTH	3

#define PRG_ETH0_RGMII_TX_CLK_EN	10

#define PRG_ETH0_INVERTED_RMII_CLK	BIT(11)
#define PRG_ETH0_TX_AND_PHY_REF_CLK	BIT(12)

#define MUX_CLK_NUM_PARENTS		2

unsigned int support_mac_wol;
unsigned int support_nfx_doze;

struct meson8b_dwmac;

struct meson8b_dwmac_data {
	int (*set_phy_mode)(struct meson8b_dwmac *dwmac);
};

struct meson8b_dwmac {
	struct device			*dev;
	void __iomem			*regs;

	const struct meson8b_dwmac_data	*data;
	phy_interface_t			phy_mode;
	struct clk			*rgmii_tx_clk;
	u32				tx_delay_ns;
#ifdef CONFIG_AMLOGIC_ETH_PRIVE
	struct input_dev		*input_dev;
#endif
};

struct meson8b_dwmac_clk_configs {
	struct clk_mux		m250_mux;
	struct clk_divider	m250_div;
	struct clk_fixed_factor	fixed_div2;
	struct clk_gate		rgmii_tx_en;
};

static void meson8b_dwmac_mask_bits(struct meson8b_dwmac *dwmac, u32 reg,
				    u32 mask, u32 value)
{
	u32 data;

	data = readl(dwmac->regs + reg);
	data &= ~mask;
	data |= (value & mask);

	writel(data, dwmac->regs + reg);
}

static struct clk *meson8b_dwmac_register_clk(struct meson8b_dwmac *dwmac,
					      const char *name_suffix,
					      const char **parent_names,
					      int num_parents,
					      const struct clk_ops *ops,
					      struct clk_hw *hw)
{
	struct clk_init_data init;
	char clk_name[32];

	snprintf(clk_name, sizeof(clk_name), "%s#%s", dev_name(dwmac->dev),
		 name_suffix);

	init.name = clk_name;
	init.ops = ops;
	init.flags = CLK_SET_RATE_PARENT;
	init.parent_names = parent_names;
	init.num_parents = num_parents;

	hw->init = &init;

	return devm_clk_register(dwmac->dev, hw);
}

static int meson8b_init_rgmii_tx_clk(struct meson8b_dwmac *dwmac)
{
	int i, ret;
	struct clk *clk;
	struct device *dev = dwmac->dev;
	const char *parent_name, *mux_parent_names[MUX_CLK_NUM_PARENTS];
	struct meson8b_dwmac_clk_configs *clk_configs;
	static const struct clk_div_table div_table[] = {
		{ .div = 2, .val = 2, },
		{ .div = 3, .val = 3, },
		{ .div = 4, .val = 4, },
		{ .div = 5, .val = 5, },
		{ .div = 6, .val = 6, },
		{ .div = 7, .val = 7, },
		{ /* end of array */ }
	};

	clk_configs = devm_kzalloc(dev, sizeof(*clk_configs), GFP_KERNEL);
	if (!clk_configs)
		return -ENOMEM;

	/* get the mux parents from DT */
	for (i = 0; i < MUX_CLK_NUM_PARENTS; i++) {
		char name[16];

		snprintf(name, sizeof(name), "clkin%d", i);
		clk = devm_clk_get(dev, name);
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			if (ret != -EPROBE_DEFER)
				dev_err(dev, "Missing clock %s\n", name);
			return ret;
		}

		mux_parent_names[i] = __clk_get_name(clk);
	}

	clk_configs->m250_mux.reg = dwmac->regs + PRG_ETH0;
#ifdef CONFIG_AMLOGIC_ETH_PRIVE
	clk_configs->m250_mux.shift = PRG_ETH0_CLK_M250_SEL_SHIFT;
	clk_configs->m250_mux.mask = PRG_ETH0_CLK_M250_SEL_MASK;
	clk = meson8b_dwmac_register_clk(dwmac, "m250_sel",
					 &mux_parent_names[0],
					 1, &clk_mux_ops,
					 &clk_configs->m250_mux.hw);
#else
	clk_configs->m250_mux.shift = __ffs(PRG_ETH0_CLK_M250_SEL_MASK);
	clk_configs->m250_mux.mask = PRG_ETH0_CLK_M250_SEL_MASK >>
				     clk_configs->m250_mux.shift;
	clk = meson8b_dwmac_register_clk(dwmac, "m250_sel", mux_parent_names,
					 MUX_CLK_NUM_PARENTS, &clk_mux_ops,
					 &clk_configs->m250_mux.hw);
#endif
	if (WARN_ON(IS_ERR(clk)))
		return PTR_ERR(clk);

	parent_name = __clk_get_name(clk);
	clk_configs->m250_div.reg = dwmac->regs + PRG_ETH0;
	clk_configs->m250_div.shift = PRG_ETH0_CLK_M250_DIV_SHIFT;
	clk_configs->m250_div.width = PRG_ETH0_CLK_M250_DIV_WIDTH;
	clk_configs->m250_div.table = div_table;
	clk_configs->m250_div.flags = CLK_DIVIDER_ALLOW_ZERO |
				      CLK_DIVIDER_ROUND_CLOSEST;
	clk = meson8b_dwmac_register_clk(dwmac, "m250_div", &parent_name, 1,
					 &clk_divider_ops,
					 &clk_configs->m250_div.hw);
	if (WARN_ON(IS_ERR(clk)))
		return PTR_ERR(clk);

	parent_name = __clk_get_name(clk);
	clk_configs->fixed_div2.mult = 1;
	clk_configs->fixed_div2.div = 2;
	clk = meson8b_dwmac_register_clk(dwmac, "fixed_div2", &parent_name, 1,
					 &clk_fixed_factor_ops,
					 &clk_configs->fixed_div2.hw);
	if (WARN_ON(IS_ERR(clk)))
		return PTR_ERR(clk);

	parent_name = __clk_get_name(clk);
	clk_configs->rgmii_tx_en.reg = dwmac->regs + PRG_ETH0;
	clk_configs->rgmii_tx_en.bit_idx = PRG_ETH0_RGMII_TX_CLK_EN;
	clk = meson8b_dwmac_register_clk(dwmac, "rgmii_tx_en", &parent_name, 1,
					 &clk_gate_ops,
					 &clk_configs->rgmii_tx_en.hw);
	if (WARN_ON(IS_ERR(clk)))
		return PTR_ERR(clk);

	dwmac->rgmii_tx_clk = clk;

	return 0;
}

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
static int meson8b_set_phy_mode(struct meson8b_dwmac *dwmac)
{
	switch (dwmac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		/* enable RGMII mode */
		meson8b_dwmac_mask_bits(dwmac, PRG_ETH0,
					PRG_ETH0_RGMII_MODE,
					PRG_ETH0_RGMII_MODE);
		break;
	case PHY_INTERFACE_MODE_RMII:
		/* disable RGMII mode -> enables RMII mode */
		meson8b_dwmac_mask_bits(dwmac, PRG_ETH0,
					PRG_ETH0_RGMII_MODE, 0);
		break;
	default:
		dev_err(dwmac->dev, "fail to set phy-mode %s\n",
			phy_modes(dwmac->phy_mode));
		return -EINVAL;
	}

	return 0;
}
#endif

static int meson_axg_set_phy_mode(struct meson8b_dwmac *dwmac)
{
	switch (dwmac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		/* enable RGMII mode */
		meson8b_dwmac_mask_bits(dwmac, PRG_ETH0,
					PRG_ETH0_EXT_PHY_MODE_MASK,
					PRG_ETH0_EXT_RGMII_MODE);
		break;
	case PHY_INTERFACE_MODE_RMII:
		/* disable RGMII mode -> enables RMII mode */
		meson8b_dwmac_mask_bits(dwmac, PRG_ETH0,
					PRG_ETH0_EXT_PHY_MODE_MASK,
					PRG_ETH0_EXT_RMII_MODE);
		break;
	default:
		dev_err(dwmac->dev, "fail to set phy-mode %s\n",
			phy_modes(dwmac->phy_mode));
		return -EINVAL;
	}

	return 0;
}

static int meson8b_init_prg_eth(struct meson8b_dwmac *dwmac)
{
	int ret;
	u8 tx_dly_val = 0;

	switch (dwmac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_RXID:
		/* TX clock delay in ns = "8ns / 4 * tx_dly_val" (where
		 * 8ns are exactly one cycle of the 125MHz RGMII TX clock):
		 * 0ns = 0x0, 2ns = 0x1, 4ns = 0x2, 6ns = 0x3
		 */
		tx_dly_val = dwmac->tx_delay_ns >> 1;
		/* fall through */

	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		/* only relevant for RMII mode -> disable in RGMII mode */
		meson8b_dwmac_mask_bits(dwmac, PRG_ETH0,
					PRG_ETH0_INVERTED_RMII_CLK, 0);

		meson8b_dwmac_mask_bits(dwmac, PRG_ETH0, PRG_ETH0_TXDLY_MASK,
					tx_dly_val << PRG_ETH0_TXDLY_SHIFT);

		/* Configure the 125MHz RGMII TX clock, the IP block changes
		 * the output automatically (= without us having to configure
		 * a register) based on the line-speed (125MHz for Gbit speeds,
		 * 25MHz for 100Mbit/s and 2.5MHz for 10Mbit/s).
		 */
		ret = clk_set_rate(dwmac->rgmii_tx_clk, 125 * 1000 * 1000);
		if (ret) {
			dev_err(dwmac->dev,
				"failed to set RGMII TX clock\n");
			return ret;
		}

		ret = clk_prepare_enable(dwmac->rgmii_tx_clk);
		if (ret) {
			dev_err(dwmac->dev,
				"failed to enable the RGMII TX clock\n");
			return ret;
		}

		devm_add_action_or_reset(dwmac->dev,
					(void(*)(void *))clk_disable_unprepare,
					dwmac->rgmii_tx_clk);
		break;

	case PHY_INTERFACE_MODE_RMII:
		/* invert internal clk_rmii_i to generate 25/2.5 tx_rx_clk */
		meson8b_dwmac_mask_bits(dwmac, PRG_ETH0,
					PRG_ETH0_INVERTED_RMII_CLK,
					PRG_ETH0_INVERTED_RMII_CLK);

		/* TX clock delay cannot be configured in RMII mode */
		meson8b_dwmac_mask_bits(dwmac, PRG_ETH0, PRG_ETH0_TXDLY_MASK,
					0);

		break;

	default:
		dev_err(dwmac->dev, "unsupported phy-mode %s\n",
			phy_modes(dwmac->phy_mode));
		return -EINVAL;
	}

	/* enable TX_CLK and PHY_REF_CLK generator */
	meson8b_dwmac_mask_bits(dwmac, PRG_ETH0, PRG_ETH0_TX_AND_PHY_REF_CLK,
				PRG_ETH0_TX_AND_PHY_REF_CLK);

	return 0;
}

#ifdef CONFIG_AMLOGIC_ETH_PRIVE
struct early_suspend dwmac_early_suspend;
int backup_adv;
static void dwmac_early_suspend_func(struct early_suspend *h)
{
	struct platform_device *pdev = (struct platform_device *)h->param;
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct phy_device *phydev = ndev->phydev;

	if (support_mac_wol) {
		if (phydev->link && phydev->speed != SPEED_10) {
			backup_adv = phy_read(phydev, MII_ADVERTISE);
			phy_write(phydev, MII_ADVERTISE, 0x61);
			genphy_restart_aneg(phydev);
			msleep(3000);
		} else {
			backup_adv = 0;
		}
	}
}

static void dwmac_early_resume_func(struct early_suspend *h)
{
	struct platform_device *pdev = (struct platform_device *)h->param;
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct phy_device *phydev = ndev->phydev;

	if (support_mac_wol) {
		if (backup_adv) {
			phy_write(phydev, MII_ADVERTISE, backup_adv);
			genphy_restart_aneg(phydev);
		}
	}
}

void set_wol_notify_bl31(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(0x8200009D, support_mac_wol,
					0, 0, 0, 0, 0, 0, &res);
}

static void set_wol_notify_bl30(void)
{
	scpi_set_ethernet_wol(support_mac_wol);
}

void __iomem *ee_reset_base;
unsigned int internal_phy;
static int aml_custom_setting(struct platform_device *pdev, struct meson8b_dwmac *dwmac)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	void __iomem *addr = NULL;
	struct resource *res = NULL;
	unsigned int cali_val = 0;
	unsigned int mc_val = 0;

	/*get tx amp setting from tx_amp_src*/
	pr_info("aml_cust_setting\n");

	/*map ETH_RESET address*/
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "eth_reset");
	if (!res) {
		dev_err(&pdev->dev, "Unable to get resource(%d)\n", __LINE__);
		ee_reset_base = NULL;
	} else {
		addr = devm_ioremap(dev, res->start, resource_size(res));
		if (IS_ERR(addr))
			dev_err(&pdev->dev, "Unable to map reset base\n");
		ee_reset_base = addr;
	}

	if (of_property_read_u32(np, "mac_wol", &support_mac_wol) != 0)
		pr_info("no mac_wol\n");

	if (of_property_read_u32(np, "keep-alive", &support_nfx_doze) != 0)
		pr_info("no keep-alive\n");
	/*nfx doze setting ASAP WOL, if not set, do nothing*/
	if (support_nfx_doze)
		support_mac_wol = support_nfx_doze;

	if (of_property_read_u32(np, "internal_phy", &internal_phy) != 0)
		pr_info("use default internal_phy as 0\n");

	if (of_property_read_u32(np, "cali_val", &cali_val) != 0)
		pr_info("set default cali_val as 0\n");
	/*internal_phy 1:inphy;2:exphy; 0 as default*/
	if (internal_phy == 2)
		writel(cali_val, dwmac->regs + PRG_ETH1);

	/*invole mc_val since special bit which not
	 *in normal flow
	 *T3 must set special reg to run ethernet
	 */
	if (of_property_read_u32(np, "mc_val", &mc_val) == 0) {
		pr_info("cover mc_val as 0x%x\n", mc_val);
		writel(mc_val, dwmac->regs + PRG_ETH0);
	}

	return 0;
}

static int dwmac_meson_disable_analog(struct device *dev)
{
	if (support_mac_wol)
		return 0;
	writel(0x00000000, phy_analog_config_addr + 0x0);
	writel(0x003e0000, phy_analog_config_addr + 0x4);
	writel(0x12844008, phy_analog_config_addr + 0x8);
	writel(0x0800a40c, phy_analog_config_addr + 0xc);
	writel(0x00000000, phy_analog_config_addr + 0x10);
	writel(0x031d161c, phy_analog_config_addr + 0x14);
	writel(0x00001683, phy_analog_config_addr + 0x18);
	writel(0x09c0040a, phy_analog_config_addr + 0x44);
	return 0;
}

static int dwmac_meson_recover_analog(struct device *dev)
{
	if (support_mac_wol)
		return 0;
	writel(0x19c0040a, phy_analog_config_addr + 0x44);
	writel(0x0, phy_analog_config_addr + 0x4);
	return 0;
}

extern int stmmac_pltfr_suspend(struct device *dev);
extern void realtek_setup_wol(int enable, bool is_shutdown);
static int aml_dwmac_suspend(struct device *dev)
{
	int ret = 0;

	set_wol_notify_bl31();
	set_wol_notify_bl30();
	/*nfx doze do nothing for suspend*/
	if (support_nfx_doze) {
		pr_info("doze is running\n");
		return 0;
	}
	pr_info("aml_eth_suspend\n");
	ret = stmmac_pltfr_suspend(dev);
	/*internal phy only*/
	if (internal_phy != 2)
		dwmac_meson_disable_analog(dev);

	realtek_setup_wol(1, 0);

	return ret;
}

extern int stmmac_pltfr_resume(struct device *dev);
static int aml_dwmac_resume(struct device *dev)
{
	int ret = 0;
	struct meson8b_dwmac *dwmac = get_stmmac_bsp_priv(dev);
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);

	/*nfx doze do nothing for resume*/
	if (support_nfx_doze) {
		pr_info("doze is running\n");
		return 0;
	}

	pr_info("aml_eth_resume\n");
	if (internal_phy != 2)
		dwmac_meson_recover_analog(dev);

	ret = stmmac_pltfr_resume(dev);
	if (support_mac_wol) {
		if (get_resume_method() == ETH_PHY_WAKEUP) {
			if (!priv->plat->mdns_wkup) {
				pr_info("evan---wol rx--KEY_POWER\n");
				input_event(dwmac->input_dev,
					EV_KEY, KEY_POWER, 1);
				input_sync(dwmac->input_dev);
				input_event(dwmac->input_dev,
					EV_KEY, KEY_POWER, 0);
				input_sync(dwmac->input_dev);
			} else {
				pr_info("evan---wol rx--pm event\n");
				pm_wakeup_event(dev, 2000);
			}
		}
	}

	realtek_setup_wol(0, 0);

	return 0;
}

void meson8b_dwmac_shutdown(struct platform_device *pdev)
{
	pr_info("aml_eth_shutdown\n");
	stmmac_pltfr_suspend(&pdev->dev);
	if (internal_phy != 2)
		dwmac_meson_disable_analog(&pdev->dev);

	realtek_setup_wol(1, 1);
}



#endif
static int meson8b_dwmac_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	struct meson8b_dwmac *dwmac;
#ifdef CONFIG_AMLOGIC_ETH_PRIVE
	struct input_dev *input_dev;
#endif
	int ret;

	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	plat_dat = stmmac_probe_config_dt(pdev, &stmmac_res.mac);
	if (IS_ERR(plat_dat))
		return PTR_ERR(plat_dat);

	dwmac = devm_kzalloc(&pdev->dev, sizeof(*dwmac), GFP_KERNEL);
	if (!dwmac) {
		ret = -ENOMEM;
		goto err_remove_config_dt;
	}

	dwmac->data = (const struct meson8b_dwmac_data *)
		of_device_get_match_data(&pdev->dev);
	if (!dwmac->data) {
		ret = -EINVAL;
		goto err_remove_config_dt;
	}

	dwmac->regs = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(dwmac->regs)) {
		ret = PTR_ERR(dwmac->regs);
		goto err_remove_config_dt;
	}

	dwmac->dev = &pdev->dev;
	dwmac->phy_mode = of_get_phy_mode(pdev->dev.of_node);
	if ((int)dwmac->phy_mode < 0) {
		dev_err(&pdev->dev, "missing phy-mode property\n");
		ret = -EINVAL;
		goto err_remove_config_dt;
	}
#ifdef CONFIG_AMLOGIC_ETH_PRIVE
	/*clear top reg bit13 to disable adj function*/
	writel((readl(dwmac->regs) & (~0x2000)), dwmac->regs);
#endif
	/* use 2ns as fallback since this value was previously hardcoded */
	if (of_property_read_u32(pdev->dev.of_node, "amlogic,tx-delay-ns",
				 &dwmac->tx_delay_ns))
		dwmac->tx_delay_ns = 2;

	ret = meson8b_init_rgmii_tx_clk(dwmac);
	if (ret)
		goto err_remove_config_dt;

	ret = dwmac->data->set_phy_mode(dwmac);
	if (ret)
		goto err_remove_config_dt;

	ret = meson8b_init_prg_eth(dwmac);
	if (ret)
		goto err_remove_config_dt;

	plat_dat->bsp_priv = dwmac;

	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret)
		goto err_remove_config_dt;

#ifdef CONFIG_AMLOGIC_ETH_PRIVE
	aml_custom_setting(pdev, dwmac);
	if (support_mac_wol) {
		if (of_property_read_u32(pdev->dev.of_node, "mdns_wkup", &plat_dat->mdns_wkup) == 0)
			pr_info("feature mdns_wkup\n");

		device_init_wakeup(&pdev->dev, 1);

	/*input device to send virtual pwr key for android*/
		input_dev = input_allocate_device();
		if (!input_dev) {
			pr_err("[abner test]input_allocate_device failed: %d\n", ret);
			return -EINVAL;
		}
		set_bit(EV_KEY,  input_dev->evbit);
		set_bit(KEY_POWER, input_dev->keybit);
		set_bit(133, input_dev->keybit);

		input_dev->name = "input_ethrcu";
		input_dev->phys = "input_ethrcu/input0";
		input_dev->dev.parent = &pdev->dev;
		input_dev->id.bustype = BUS_ISA;
		input_dev->id.vendor = 0x0001;
		input_dev->id.product = 0x0001;
		input_dev->id.version = 0x0100;
		input_dev->rep[REP_DELAY] = 0xffffffff;
		input_dev->rep[REP_PERIOD] = 0xffffffff;
		input_dev->keycodesize = sizeof(unsigned short);
		input_dev->keycodemax = 0x1ff;
		ret = input_register_device(input_dev);
		if (ret < 0) {
			pr_err("[abner test]input_register_device failed: %d\n", ret);
			input_free_device(input_dev);
			return -EINVAL;
		}
		dwmac->input_dev = input_dev;

	}

	dwmac_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	dwmac_early_suspend.suspend = dwmac_early_suspend_func;
	dwmac_early_suspend.resume = dwmac_early_resume_func;
	dwmac_early_suspend.param = pdev;
	register_early_suspend(&dwmac_early_suspend);
#endif
	return 0;

err_remove_config_dt:
	stmmac_remove_config_dt(pdev, plat_dat);

	return ret;
}

#ifdef CONFIG_AMLOGIC_ETH_PRIVE
static int meson8b_dwmac_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	int err;

	struct meson8b_dwmac *dwmac = get_stmmac_bsp_priv(&pdev->dev);

	if (support_mac_wol)
		input_unregister_device(dwmac->input_dev);

	err = stmmac_dvr_remove(&pdev->dev);
	if (err < 0)
		dev_err(&pdev->dev, "failed to remove platform: %d\n", err);

	stmmac_remove_config_dt(pdev, priv->plat);

	return err;
}
#endif
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
static const struct meson8b_dwmac_data meson8b_dwmac_data = {
	.set_phy_mode = meson8b_set_phy_mode,
};
#endif

static const struct meson8b_dwmac_data meson_axg_dwmac_data = {
	.set_phy_mode = meson_axg_set_phy_mode,
};

static const struct of_device_id meson8b_dwmac_match[] = {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	{
		.compatible = "amlogic,meson8b-dwmac",
		.data = &meson8b_dwmac_data,
	},
	{
		.compatible = "amlogic,meson8m2-dwmac",
		.data = &meson8b_dwmac_data,
	},
	{
		.compatible = "amlogic,meson-gxbb-dwmac",
		.data = &meson8b_dwmac_data,
	},
#endif
	{
		.compatible = "amlogic,meson-axg-dwmac",
		.data = &meson_axg_dwmac_data,
	},
	{ }
};
MODULE_DEVICE_TABLE(of, meson8b_dwmac_match);
#ifdef CONFIG_AMLOGIC_ETH_PRIVE
SIMPLE_DEV_PM_OPS(stmmac_meson8b_pm_ops, aml_dwmac_suspend,
		  aml_dwmac_resume);
#endif
static struct platform_driver meson8b_dwmac_driver = {
	.probe  = meson8b_dwmac_probe,
#ifdef CONFIG_AMLOGIC_ETH_PRIVE
	.remove = meson8b_dwmac_remove,
#else
	.remove = stmmac_pltfr_remove,
#endif
	.shutdown = meson8b_dwmac_shutdown,
	.driver = {
		.name           = "meson8b-dwmac",
#ifdef CONFIG_AMLOGIC_ETH_PRIVE
		.pm		= &stmmac_meson8b_pm_ops,
#else
		.pm		= &stmmac_pltfr_pm_ops,
#endif
		.of_match_table = meson8b_dwmac_match,
	},
};
module_platform_driver(meson8b_dwmac_driver);

MODULE_AUTHOR("Martin Blumenstingl <martin.blumenstingl@googlemail.com>");
MODULE_DESCRIPTION("Amlogic Meson8b, Meson8m2 and GXBB DWMAC glue layer");
MODULE_LICENSE("GPL v2");
