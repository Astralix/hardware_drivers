/*
 * Synopsys "DesignWare DWC Mobile Storage Host" driver
 * Copyright (C) 2010 DSPG Technologies GmbH
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <hwconfig.h>
#include <part.h>
#include <malloc.h>
#include <mmc.h>
#include <asm/io.h>

#include <syndwc_mmc.h>

#define SYNDWC_TIMEOUT		5000		/* request timeout in ms */
#define SYNDWC_HARDTIMEOUT	(SYNDWC_TIMEOUT + 1000) /* forced timeout */

#define DWC_CTRL		0x00
#define DWC_PWREN		0x04
#define DWC_CLKDIV		0x08
#define DWC_CLKSRC		0x0c
#define DWC_CLKENA		0x10
#define DWC_TMOUT		0x14
#define DWC_CTYPE		0x18
#define DWC_BLKSIZ		0x1c
#define DWC_BYTCNT		0x20
#define DWC_INTMASK		0x24
#define DWC_CMDARG		0x28
#define DWC_CMD			0x2c
#define DWC_RESP0		0x30
#define DWC_RESP1		0x34
#define DWC_RESP2		0x38
#define DWC_RESP3		0x3c
#define DWC_MINTSTS		0x40
#define DWC_RINTSTS		0x44
#define DWC_STATUS		0x48
#define DWC_FIFOTH		0x4c
#define DWC_CDETECT		0x50
#define DWC_WRTPRT		0x54
#define DWC_GPIO		0x58
#define DWC_TCBCNT		0x5c
#define DWC_TBBCNT		0x60
#define DWC_DEBNCE		0x64
#define DWC_USRID		0x68
#define DWC_VERID		0x6c
#define DWC_HCON		0x70

/* IDMAC */
#define DWC_BMOD		0x80
#define DWC_PLDMND		0x84
#define DWC_DBADDR		0x88
#define DWC_IDSTS		0x8C
#define DWC_IDINTEN		0x90
#define DWC_DSCADDR		0x94
#define DWC_BUFADDR		0x98

/* FIFO */
#define DWC_DATA		0x0100

#define DWC_CMD_START			(1 << 31)
#define DWC_CMD_UPDATE_CLK_REGS_ONLY	(1 << 21)
#define DWC_CMD_INIT			(1 << 15)
#define DWC_CMD_ABORT			(1 << 14)
#define DWC_CMD_WAITPRVCOMPLETE		(1 << 13)
#define DWC_CMD_AUTOSTOP		(1 << 12)
#define DWC_CMD_TRANSFERMODE		(1 << 11)
#define DWC_CMD_WRITE			(1 << 10)
#define DWC_CMD_DATAEXPECTED		(1 << 9)
#define DWC_CMD_CHECKRSPCRC		(1 << 8)
#define DWC_CMD_LONGRSP			(1 << 7)
#define DWC_CMD_RESPEXPECTED		(1 << 6)

#define DWC_IRQ_CARDDETECT		(1 << 0)
#define DWC_IRQ_REPSONSEERROR		(1 << 1)
#define DWC_IRQ_CMDDONE			(1 << 2)
#define DWC_IRQ_DATATRANSFEROVER	(1 << 3)
#define DWC_IRQ_TXFIFOREQ		(1 << 4)
#define DWC_IRQ_RXFIFOREQ		(1 << 5)
#define DWC_IRQ_RESPONSECRCERR		(1 << 6)
#define DWC_IRQ_DATACRCERR		(1 << 7)
#define DWC_IRQ_RESPONSETIMEOUT		(1 << 8)
#define DWC_IRQ_DATAREADTIMEOUT		(1 << 9)
#define DWC_IRQ_DATASTARVATION		(1 << 10)
#define DWC_IRQ_FIFOUNDERUN		(1 << 11)
#define DWC_IRQ_HWLOCKEDWRITEERR	(1 << 12)
#define DWC_IRQ_STARTBITERR		(1 << 13)
#define DWC_IRQ_AUTOCMDDONE		(1 << 14)
#define DWC_IRQ_ENDBITERR		(1 << 15)

#define SYNDWC_PWR_ON			1

/* Control register definitions */
#define CTRL_RESET		0x00000001
#define FIFO_RESET		0x00000002
#define DMA_RESET		0x00000004
#define INT_ENABLE		0x00000010
#define DMA_ENABLE		0x00000020
#define READ_WAIT		0x00000040
#define SEND_IRQ_RESP		0x00000080
#define ABRT_READ_DATA		0x00000100
#define SEND_CCSD		0x00000200
#define SEND_AS_CCSD		0x00000400
#define ENABLE_OD_PULLUP	0x01000000

/*SDIO interrupts are catered from bit 15 through 31*/
#define INTMSK_SDIO_INTR	0xffff0000
#define INTMSK_SDIO_CARD(x)	(1<<(16+x))
#define INTMSK_ALL_ENABLED	0xffffffff

/* CMD Register Defines */
#define CMD_RESP_EXP_BIT	0x00000040
#define CMD_RESP_LENGTH_BIT	0x00000080
#define CMD_CHECK_CRC_BIT	0x00000100
#define CMD_DATA_EXP_BIT	0x00000200
#define CMD_RW_BIT		0x00000400
#define CMD_TRANSMODE_BIT	0x00000800
#define CMD_SENT_AUTO_STOP_BIT	0x00001000
#define CMD_WAIT_PRV_DAT_BIT	0x00002000
#define CMD_ABRT_CMD_BIT	0x00004000
#define CMD_SEND_INIT_BIT	0x00008000
#define CMD_SEND_CLK_ONLY	0x00200000
#define CMD_READ_CEATA		0x00400000
#define CMD_CCS_EXPECTED	0x00800000
#define CMD_DONE_BIT		0x80000000

#define SET_CARD_NUM(x,y) ((x)|=((y)<<16))
#define SET_CMD_INDEX(x,y) ((x)|=(y&0x3f))

/* Status register bits */

#define STATUS_DAT_BUSY_BIT	0x00000200
#define STATUS_FIFO_FULL	0x00000008
#define STATUS_FIFO_EMPTY	0x00000004

struct syndwc_host {
	char *base;
	unsigned int mclk;
	unsigned int cclk;

	struct mmc_cmd *cmd;
	struct mmc_data *data;

	u32 *buffer;
	u32 remain;

	unsigned int card_num;
	syndwc_cd_t card_detect;
};

static int current_card = -1;

static int
syndwc_cd(struct syndwc_host *host)
{
	if (host->card_detect)
		return host->card_detect();
	else
		return !readl(host->base + DWC_CDETECT);
}

static void
syndwc_clear_fifos(struct syndwc_host *host)
{
	writel(readl(host->base + DWC_CTRL) | FIFO_RESET, host->base + DWC_CTRL);
	while (readl(host->base + DWC_CTRL) & FIFO_RESET)
		;
}


static void
syndwc_flush_fifo(struct syndwc_host *host)
{
	while (! (readl(host->base + DWC_STATUS) & STATUS_FIFO_EMPTY))
		readl(host->base + DWC_DATA);
}


static void
syndwc_reset(struct syndwc_host *host)
{
	host->data = NULL;
	host->cmd  = NULL;

	/* reset controller */
	writel(0x3, host->base + DWC_CTRL); /* FIFO & controller reset */
	while (readl(host->base + DWC_CTRL) & 0x3)
		;

	/* enable power */
	writel(0xffff, host->base + DWC_PWREN);
	udelay(2000);

	/* clear interrupts */
	writel(0xffffffff, host->base + DWC_RINTSTS);
	writel(0xffff, host->base + DWC_INTMASK);
	udelay(2000);

	/* reset FIFO */
	syndwc_clear_fifos(host);

	udelay(2000);
}


static int
syndwc_sync(struct syndwc_host *host)
{
	int sync_counter = 0;

	while (readl(host->base + DWC_CMD) & DWC_CMD_START) {
		sync_counter++;
		/* This bit is usually cleared after far less than 50 loops.
		 * Occasionally the controller locks up - so we return with an
		 * error after a reasonable high number of unsuccessful tries
		 */
		if (sync_counter >= 1000) {
			syndwc_reset(host);
			return -1;
		}
	}

	return 0;
}


static void syndwc_set_clock(struct syndwc_host *host, unsigned int clock)
{
	unsigned int clk, value;

	/* remember currently clocked card */
	current_card = host->card_num;

	if (clock == host->mclk) {
		clk = 0;
		host->cclk = host->mclk;
	} else {
		int doubleclk = 2 * clock;

		/*
		 * the clock divider must be rounded or up or we may violate
		 * the cards maximum clock speed
		 */
		clk = (host->mclk + doubleclk - 1) / doubleclk;
		if (clk >= 0xff)
			clk = 0xff;
		if (clk < 1)
			clk = 1;
		host->cclk = host->mclk / (2 * clk);
	}

	/* wait until idle */
	if (syndwc_sync(host))
		return;

	/* stop all clocks */
	writel(0x0, host->base + DWC_CLKENA);
	value = DWC_CMD_START | DWC_CMD_UPDATE_CLK_REGS_ONLY |
		DWC_CMD_WAITPRVCOMPLETE;
	writel(value, host->base + DWC_CMD);
	if (syndwc_sync(host))
		return;

	/* set clock speed */
	writel(0x0, host->base + DWC_CLKSRC);
	writel(clk, host->base + DWC_CLKDIV);
	writel(value, host->base + DWC_CMD);
	if (syndwc_sync(host))
		return;

	/* enable clock */
	writel(1 << host->card_num, host->base + DWC_CLKENA);
	writel(value, host->base + DWC_CMD);
	if (syndwc_sync(host))
		return;

	/* set debounce count */
	writel((100 * host->cclk)/1000, host->base + DWC_DEBNCE);

	/* set timeouts */
	value = SYNDWC_TIMEOUT / 1000;
	value *= host->cclk;
	if (value > 0xfffffful)
		value = 0xfffffful;
	writel(value << 8 | 0x64, host->base + DWC_TMOUT);
}


#define readsl(reg, buf, cnt) \
	do { \
		char *r = (reg); \
		u32 *p = (buf); \
		u32 c = (cnt); \
		while (c--) *p++ = readl(r); \
	} while (0)


#define writesl(reg, buf, cnt) \
	do { \
		char *r = (reg); \
		const u32 *p = (buf); \
		u32 c = (cnt); \
		while (c--) { \
			writel(*p, r); \
			p++; \
		} \
	} while (0)


static void
syndwc_pio_read(struct syndwc_host *host)
{
	u32 status;
	unsigned int count, fifo_count;

	do {
		fifo_count = (readl(host->base + DWC_STATUS) >> 17) & 0x1fff;
		count = MIN(host->remain, fifo_count);

		if (fifo_count <= 0) {
			//printf("no data in fifo but interrupt?\n");
			break;
		}

		if (count <= 0) {
			//printf("count < fifo_count: flushing!\n");
			syndwc_flush_fifo(host);
			break;
		}

		readsl(host->base + DWC_DATA, host->buffer, count);
		host->buffer += count;
		host->remain -= count;

		if (host->remain == 0)
			break;

		status = readl(host->base + DWC_STATUS);
	} while (! (status & STATUS_FIFO_EMPTY));
}


static void
syndwc_pio_write(struct syndwc_host *host)
{
	u32 status;

	status = readl(host->base + DWC_STATUS);

	while (!(status & STATUS_FIFO_FULL) && host->remain) {
		writel(*host->buffer, host->base + DWC_DATA);
		host->buffer++;
		host->remain--;

		status = readl(host->base + DWC_STATUS);
	}
}

static int syndwc_run(struct syndwc_host *host)
{
	ulong timerbase = get_timer(0);
	u32 mask = DWC_IRQ_RXFIFOREQ | DWC_IRQ_TXFIFOREQ |
		DWC_IRQ_DATAREADTIMEOUT | DWC_IRQ_DATACRCERR  |
		DWC_IRQ_DATASTARVATION | DWC_IRQ_STARTBITERR |
		DWC_IRQ_ENDBITERR | DWC_IRQ_DATATRANSFEROVER |
		DWC_IRQ_CMDDONE | DWC_IRQ_AUTOCMDDONE;

	do {
		u32 status;

		status = readl(host->base + DWC_MINTSTS);
		writel(status & mask, host->base + DWC_RINTSTS);

		/* card removed? */
		if (!syndwc_cd(host))
			return -ENOMEDIUM;

		/* IO */
		if (status & DWC_IRQ_RXFIFOREQ)
			syndwc_pio_read(host);

		if (status & DWC_IRQ_TXFIFOREQ)
			syndwc_pio_write(host);

		/* Errors */
		if (status & DWC_IRQ_DATAREADTIMEOUT)
			return TIMEOUT;

		if (status & (DWC_IRQ_DATACRCERR  | DWC_IRQ_DATASTARVATION |
		              DWC_IRQ_STARTBITERR | DWC_IRQ_ENDBITERR)) {
			printf("syndwc %d: data error (status=0x%08x)\n",
				host->card_num, status);
			return -EIO;
		}

		/* Finished? */
		if (status & DWC_IRQ_DATATRANSFEROVER)
			return 0;

		if (status & (DWC_IRQ_CMDDONE | DWC_IRQ_AUTOCMDDONE)) {
			if (status & DWC_IRQ_RESPONSETIMEOUT)
				return TIMEOUT;

			if (host->cmd->resp_type & MMC_RSP_136) {
				host->cmd->response[3] = readl(host->base + DWC_RESP0);
				host->cmd->response[2] = readl(host->base + DWC_RESP1);
				host->cmd->response[1] = readl(host->base + DWC_RESP2);
				host->cmd->response[0] = readl(host->base + DWC_RESP3);
			} else
				host->cmd->response[0] = readl(host->base + DWC_RESP0);

			if (status & DWC_IRQ_RESPONSECRCERR &&
					host->cmd->resp_type & MMC_RSP_CRC) {
				printf("syndwc %d: response error (status=0x%08x)\n",
					host->card_num, status);
				return -EIO;
			}

			if (!host->data)
				return 0;
		}

		if (status)
			timerbase = get_timer(0);
	} while (get_timer(timerbase) < SYNDWC_HARDTIMEOUT);

	printf("syndwc %d: timeout\n", host->card_num);
	return TIMEOUT;
}


static int syndwc_request(struct mmc *mmc, struct mmc_cmd *cmd,
		struct mmc_data *data)
{
	struct syndwc_host *host = mmc->priv;
	unsigned int dwc_cmd;

	/* prepare for the show */
	if (!syndwc_cd(host)) /* inverted register */
		return -ENOMEDIUM;

	syndwc_clear_fifos(host);
	syndwc_flush_fifo(host);
	writel(0xffffffff, host->base + DWC_RINTSTS);  /* clear interrupts */

	/* supply the right card clock */
	if (host->card_num != current_card)
		syndwc_set_clock(host, host->cclk);

	host->cmd = cmd;
	host->data = data;

	/* setup data */
	if (data) {
		/* RX_WMark = (FIFO_DEPTH/2)-1; TX_WMark = FIFO_DEPTH/2 */
		if ((data->flags & MMC_DATA_READ) && (data->blocks * data->blocksize < 16))
			writel(0x10000008, host->base + DWC_FIFOTH);
		else
			writel(0x10070008, host->base + DWC_FIFOTH);

		writel(data->blocksize * data->blocks, host->base + DWC_BYTCNT);
		writel(data->blocksize, host->base + DWC_BLKSIZ);

		writel(cmd->cmdarg, host->base + DWC_CMDARG);

		host->buffer = (u32*)data->dest;
		host->remain = (data->blocksize * data->blocks) >> 2;

		if (data->flags & MMC_DATA_WRITE)
			syndwc_pio_write(host);
	} else {
		writel(cmd->cmdarg, host->base + DWC_CMDARG);
	}

	/* setup command */
	dwc_cmd = cmd->cmdidx;

	SET_CARD_NUM(dwc_cmd, host->card_num);

	dwc_cmd |= DWC_CMD_START;
	if (cmd->resp_type & MMC_RSP_PRESENT)
		dwc_cmd |= DWC_CMD_RESPEXPECTED;
	if (cmd->resp_type & MMC_RSP_CRC)
		dwc_cmd |= DWC_CMD_CHECKRSPCRC;
	if (cmd->resp_type & MMC_RSP_136)
		dwc_cmd |= DWC_CMD_LONGRSP;
	if (data) {
		dwc_cmd |= DWC_CMD_DATAEXPECTED;
		if (data->flags & MMC_DATA_WRITE)
			dwc_cmd |= DWC_CMD_WRITE;
	}

	switch (cmd->cmdidx) {
	case MMC_CMD_GO_IDLE_STATE:
		dwc_cmd |= DWC_CMD_INIT | DWC_CMD_WAITPRVCOMPLETE;
		break;

	case MMC_CMD_STOP_TRANSMISSION:
		dwc_cmd |= DWC_CMD_ABORT;
		break;

	default:
		dwc_cmd |= DWC_CMD_WAITPRVCOMPLETE;
		break;
	}

	/* tell the controller */
	writel(dwc_cmd, host->base + DWC_CMD);
	if (syndwc_sync(host))
		return -EIO;

	/* do it */
	return syndwc_run(host);
}


static void syndwc_set_ios(struct mmc *mmc)
{
	struct syndwc_host *host = mmc->priv;
	unsigned long reg;

	/* Set the clock speed */
	if (mmc->clock)
		syndwc_set_clock(host, mmc->clock);

	/* Set the bus width */
	reg = readl(host->base + DWC_CTYPE) & ~(0x10001ul << host->card_num);
	switch (mmc->bus_width) {
	case 1:
		break;
	case 4:
		reg |= 0x1 << host->card_num;
		break;
	case 8:
		reg |= 0x10000 << host->card_num;
		break;
	}
	writel(reg, host->base + DWC_CTYPE);
}


static int syndwc_init(struct mmc *mmc)
{
	struct syndwc_host *host = mmc->priv;

	syndwc_set_clock(host, 400000);

	return 0;
}


int syndwc_initialize(struct syndwc_pdata *pdata)
{
	struct mmc *mmc;
	struct syndwc_host *host;

	mmc = malloc(sizeof(struct mmc));
	if (!mmc)
		return -ENOMEM;

	mmc->send_cmd = syndwc_request;
	mmc->set_ios = syndwc_set_ios;
	mmc->init = syndwc_init;

	sprintf(mmc->name, "syndwc");
	mmc->host_caps = pdata->host_caps | MMC_MODE_HC;
	mmc->voltages = pdata->voltages;
	mmc->f_min = 400000;
	mmc->f_max = pdata->f_max;

	host = malloc(sizeof(struct syndwc_host));
	if (!host) {
		free(mmc);
		return -ENOMEM;
	}

	host->base = (char *)CONFIG_SYS_SYNDWC_BASE;
	host->card_detect = pdata->card_detect;
	host->mclk = pdata->mclk;
	host->cclk = host->mclk;
	host->card_num = pdata->card_num;

	syndwc_reset(host);

	mmc->priv = host;
	return mmc_register(mmc);
}

