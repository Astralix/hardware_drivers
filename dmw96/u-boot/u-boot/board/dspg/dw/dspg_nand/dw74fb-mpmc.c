/*
 * This code is temporary for the broadtile platform, to be removed for DW74
 */

#include "dw_nfls_cntl_pub.h"

#define BROADTILE_MPMC_BASE  0x89100000
#define BROADTILE_SDRAM_BASE 0xe0000000
#define BROADTILE_SDRAM_SIZE 0x04000000

#define mpmc_readl(reg) readl(regs + (reg))
#define mpmc_writel(reg, val) writel((val), regs + (reg))

void *dw74fb_mpmc_init(struct device *dev)
{
	int ret = -ENOMEM;
	volatile int temp;
	void __iomem *mem;
	void __iomem *regs;

#ifdef __LINUX_KERNEL__
	/* map regs */
	if (!request_mem_region(BROADTILE_MPMC_BASE, SZ_4K,
	                        "dw74fb_mpmc regs"))
		return ERR_PTR(-EBUSY);

	regs = ioremap(BROADTILE_MPMC_BASE, SZ_4K);
	if (!regs) {
		dev_err(dev, "cannot map MPMC registers\n");
		goto err_release_regs;
	}
	
	/* map sdram */
	if (!request_mem_region(BROADTILE_SDRAM_BASE, BROADTILE_SDRAM_SIZE,
	                        "dw74fb_mpmc sdram")) {
		ret = -EBUSY;
		goto err_free_regs;		
	}

	mem = ioremap(BROADTILE_SDRAM_BASE, BROADTILE_SDRAM_SIZE);
	if (!mem) {
		dev_err(dev, "cannot map sdram\n");
		goto err_release_mem;
	}
#else
	regs = BROADTILE_MPMC_BASE;
	mem = BROADTILE_SDRAM_BASE;
#endif

	/* initialize sdram */
	mpmc_writel(0x028, 0x0012);
	mpmc_writel(0x024, 0x0009); /* refresh */
	mpmc_writel(0x030, 0x0001);
	mpmc_writel(0x034, 0x0004);
	mpmc_writel(0x038, 0x0007);
	mpmc_writel(0x040, 0x0004);
	mpmc_writel(0x044, 0x0001);
	mpmc_writel(0x048, 0x0006);
	mpmc_writel(0x04C, 0x0006);
	mpmc_writel(0x050, 0x0007);
	mpmc_writel(0x054, 0x0001);
	mpmc_writel(0x058, 0x0002);
	mpmc_writel(0x124, 0x0101); /* Cas and Ras latency */

	/* enable write buffer at ARM port (port 0) */
	mpmc_writel(0x408, mpmc_readl(0x408) | 1);

	/* enable write buffer at ARM port (port 4) */
	mpmc_writel(0x488, mpmc_readl(0x488) | 1);

	mpmc_writel(0x020, 0x0003); /* issue normal operation command */
	mpmc_writel(0x020, 0x0183); /* issue normal operation command */
	mpmc_writel(0x020, 0x0103); /* issue normal operation command */

	mpmc_writel(0x024, 0x0010);

	/* Refresh */
	mdelay(30);

	mpmc_writel(0x120, 0x4500); /* 32bit - Config - (address map) */
	mpmc_writel(0x020, 0x0083); /* issue normal operation command */

	mdelay(30);

	temp = *(volatile int *)(mem + 0x10800);

	mdelay(30);

	mpmc_writel(0x020, 0x0103); /* issue normal operation command */
	mpmc_writel(0x024, 0x0010); /* refresh */

	mdelay(30);

	mpmc_writel(0x020, 0x0003); /* issue normal operation command */
	mpmc_writel(0x480, 0x0001);

	return mem;

#ifdef __LINUX_KERNEL__
err_release_mem:
	release_mem_region(BROADTILE_SDRAM_BASE, BROADTILE_SDRAM_SIZE);

err_free_regs:
	iounmap(regs);
	
err_release_regs:
	release_mem_region(BROADTILE_MPMC_BASE, SZ_4K);

	dev_err(dev, "%s() failed\n", __func__);
	return ERR_PTR(ret);
#endif
}
