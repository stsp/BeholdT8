/*
	SAA7231xx PCI/PCI Express bridge driver
	Copyright (C) Manu Abraham <abraham.manu@gmail.com>

	Copyright (C) NXP Semiconductor

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/delay.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>

#include "saa7231_priv.h"

#include "saa7231_mod.h"
#include "saa7231_mmu_reg.h"
#include "saa7231_s2d_reg.h"
#include "saa7231_cgu.h"
#include "saa7231_msi.h"
#include "saa7231_str_reg.h"

#include "saa7231_dmabuf.h"
#include "saa7231_dtl.h"
#include "saa7231_stream.h"
#include "saa7231_ring.h"
#include "saa7231_xs2dtl.h"


/*
 * The physical address is filled to every PT used
 * for DMA. The PTAs are then set to the MMU.
 */
static int saa7231_init_ptables(struct saa7231_stream *stream)
{
	struct saa7231_ring *ring	= stream->ring;
	struct saa7231_dmabuf *dmabuf	= ring->dmabuf;
	struct saa7231_dev *saa7231 = dmabuf->saa7231;

	int port = stream->port_id;
	int channel = DMACH(port);

	BUG_ON(!ring);
	BUG_ON(!dmabuf);
	BUG_ON(!saa7231);

	dprintk(SAA7231_DEBUG, 1, "DEBUG: Initializing PORT:%d DMA_CH:%d with %d Buffers",
		port,
		channel,
		XS2D_BUFFERS);

	/* Update MMU */
	SAA7231_WR(XS2D_BUFFERS - 1, SAA7231_BAR0, MMU, MMU_DMA_CONFIG(channel));

	SAA7231_WR(PTA_LSB(dmabuf[0].pt_phys), SAA7231_BAR0, MMU, MMU_PTA0_LSB(channel)); /* Low */
	SAA7231_WR(PTA_MSB(dmabuf[0].pt_phys), SAA7231_BAR0, MMU, MMU_PTA0_MSB(channel)); /* High */
	SAA7231_WR(PTA_LSB(dmabuf[1].pt_phys), SAA7231_BAR0, MMU, MMU_PTA1_LSB(channel)); /* Low */
	SAA7231_WR(PTA_MSB(dmabuf[1].pt_phys), SAA7231_BAR0, MMU, MMU_PTA1_MSB(channel)); /* High */
	SAA7231_WR(PTA_LSB(dmabuf[2].pt_phys), SAA7231_BAR0, MMU, MMU_PTA2_LSB(channel)); /* Low */
	SAA7231_WR(PTA_MSB(dmabuf[2].pt_phys), SAA7231_BAR0, MMU, MMU_PTA2_MSB(channel)); /* High */
	SAA7231_WR(PTA_LSB(dmabuf[3].pt_phys), SAA7231_BAR0, MMU, MMU_PTA3_LSB(channel)); /* Low */
	SAA7231_WR(PTA_MSB(dmabuf[3].pt_phys), SAA7231_BAR0, MMU, MMU_PTA3_MSB(channel)); /* High */
	SAA7231_WR(PTA_LSB(dmabuf[4].pt_phys), SAA7231_BAR0, MMU, MMU_PTA4_LSB(channel)); /* Low */
	SAA7231_WR(PTA_MSB(dmabuf[4].pt_phys), SAA7231_BAR0, MMU, MMU_PTA4_MSB(channel)); /* High */
	SAA7231_WR(PTA_LSB(dmabuf[5].pt_phys), SAA7231_BAR0, MMU, MMU_PTA5_LSB(channel)); /* Low */
	SAA7231_WR(PTA_MSB(dmabuf[5].pt_phys), SAA7231_BAR0, MMU, MMU_PTA5_MSB(channel)); /* High */
	SAA7231_WR(PTA_LSB(dmabuf[6].pt_phys), SAA7231_BAR0, MMU, MMU_PTA6_LSB(channel)); /* Low */
	SAA7231_WR(PTA_MSB(dmabuf[6].pt_phys), SAA7231_BAR0, MMU, MMU_PTA6_MSB(channel)); /* High */
	SAA7231_WR(PTA_LSB(dmabuf[7].pt_phys), SAA7231_BAR0, MMU, MMU_PTA7_LSB(channel)); /* Low */
	SAA7231_WR(PTA_MSB(dmabuf[7].pt_phys), SAA7231_BAR0, MMU, MMU_PTA7_MSB(channel)); /* High */

	stream->index_w += 8;
	return 0;
}

static int saa7231_xs2dtl_setparams(struct saa7231_stream *stream)
{
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	u32 module			= dtl->module;
	struct stream_params *params	= &stream->params;
	struct saa7231_ring *ring	= stream->ring;
	struct saa7231_dmabuf *dmabuf	= ring->dmabuf;

	int id, ret;

	u32 buf_size	= params->pitch * params->lines ;
	u32 stride	= 0;
	u32 x_length	= 0;
	u32 y_length	= 0;

	u32 ctrl_config		= 0;
	u32 block_control	= 0;
	u32 channel_control	= 0;
	u32 size_test		= 0;

	/* Activate Gating */
	switch (stream->port_id) {
	case STREAM_PORT_DS2D_AVIS:
		SAA7231_WR(0x5, SAA7231_BAR0, STREAM, STREAM_RA_VBI_LOC_CLGATE); /* 54MHz */
		break;
	case STREAM_PORT_DS2D_ITU:
		SAA7231_WR(0x5, SAA7231_BAR0, STREAM, STREAM_RA_VIDEO_EXT_CLGATE); /* 54MHz */
		break;
	case STREAM_PORT_AS2D_LOCAL:
		SAA7231_WR(0x1, SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_LOC_CLGATE); /* 27MHz */
		/* Japan MODE */
		SAA7231_WR((stream->config & 0x1), SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_LOC_IISJAPAN);
		/* I2S mode */
		SAA7231_WR(((stream->config >> 1) & 0x1), SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_LOC_IISMODE);
		/* SPDIF or I2S */
		SAA7231_WR(((stream->config >> 2) & 0x1), SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_LOC_STREAM_SEL);
		SAA7231_WR(0x0, SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_LOC_IISJAPANWIDTH); /* 16bit */
		break;
	case STREAM_PORT_AS2D_EXTERN:
		SAA7231_WR(0x1, SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_EXT_CLGATE); /* 27MHz */
		/* Japan MODE */
		SAA7231_WR((stream->config & 0x1), SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_LOC_IISJAPAN);
		/* I2S mode */
		SAA7231_WR(((stream->config >> 1) & 0x1), SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_EXT_IISMODE);
		/* SPDIF or I2S */
		SAA7231_WR(((stream->config >> 2) & 0x1), SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_EXT_STREAM_SEL);
		SAA7231_WR(0x0, SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_EXT_IISJAPANWIDTH); /* 16bit */
		break;

	default:
		dprintk(SAA7231_ERROR, 1, "Invalid port: 0x%02x", stream->port_id);
		return -EINVAL;
	}

	/* reset S2D block */
	SAA7231_WR(0x1, SAA7231_BAR0, module, S2D_SW_RST);

	/* set page tables to defined state */
	saa7231_init_ptables(stream);

	/* read the module Id and the version */
	id = SAA7231_RD(SAA7231_BAR0, module, S2D_MODULE_ID);

	/* calculate stream type dependence setup for S2D */
	switch (params->type) {
	case STREAM_AUDIO:
		/*
		 * calculate setup with the condition that the audio
		 * preprocessing generates a sync on every 192 frames
		 */
		stride		= 192 * params->pitch;
		x_length	= stride;
		y_length	= buf_size / x_length;
		size_test	= buf_size % x_length;
		if (size_test) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Bad Audio Buffer size, %d bytes empty", size_test);
			return -EINVAL;
		}

		block_control	= 2;
		channel_control	= 0x29c;

		if ((id & ~0x00000F00) != 0xA0AF1000)
			dprintk(SAA7231_ERROR, 1, "ERROR: AUDIO Module ID: 0x%02x unsupported", id);
		break;

	case STREAM_TS:
		/*
		 * x_length is bytes per packet frame
		 * y_length is packets per buffer
		 * stride not used, set to bytes per packet
		 */
		stride		= params->pitch;
		x_length	= params->pitch;
		y_length	= params->lines;

		block_control	= 2;
		channel_control	= 0x29c;

		if ((id & ~0x00000F00) != 0xA0B11000)
			dprintk(SAA7231_ERROR, 1, "ERROR: TS Module ID: 0x%02x unsupported",id);
		break;

	case STREAM_VBI:
		ctrl_config = 0x2C; /* with resync on z */

		if (!params->lines)
			params->lines = 1; /* division by zero */

		stride		= params->pitch;
		x_length	= buf_size / params->lines;
		y_length	= params->lines;

		block_control	= 2;
		channel_control	= 0x290;

		if ((id & ~0x00000F00) != 0xA0B01000)
			dprintk(SAA7231_ERROR, 1, "ERROR: VBI Module ID: 0x%02x unsupported", id);
		break;

	default:
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid Stream type=0x%02x", params->type);
		return -EINVAL;
	}

	SAA7231_WR(0x0, SAA7231_BAR0, module, S2D_S2D_CTRL);

	/* Init S2D Block */
	SAA7231_WR(0x0, SAA7231_BAR0, module, S2D_CFG_CTRL);
	SAA7231_WR((XS2D_BUFFERS - 1), SAA7231_BAR0, module, S2D_S2D_MAX_B_START);

	SAA7231_WR(0x3, SAA7231_BAR0, module, S2D_CLK_GATING); /* 27 MHz */

	/* program channel block 0 related registers */
	SAA7231_WR(block_control, SAA7231_BAR0, module, S2D_CHx_B0_CTRL(0));	// 0x20
	SAA7231_WR(OFFSET(dmabuf, 0), SAA7231_BAR0, module, S2D_CHx_B0_B_START_ADDRESS(0));// 0x24
	SAA7231_WR(buf_size, SAA7231_BAR0, module, S2D_CHx_B0_B_LENGTH(0));	// 0x28
	SAA7231_WR(stride, SAA7231_BAR0, module, S2D_CHx_B0_STRIDE(0));		// 0x2c
	SAA7231_WR(x_length, SAA7231_BAR0, module, S2D_CHx_B0_X_LENGTH(0));	// 0x30
	SAA7231_WR(y_length, SAA7231_BAR0, module, S2D_CHx_B0_Y_LENGTH(0));	// 0x34

	/* program channel block 1 related registers */
	SAA7231_WR(block_control, SAA7231_BAR0, module, S2D_CHx_B1_CTRL(0));	// 0x40
	SAA7231_WR(OFFSET(dmabuf, 1), SAA7231_BAR0, module, S2D_CHx_B1_B_START_ADDRESS(0));// 0x44
	SAA7231_WR(buf_size, SAA7231_BAR0, module, S2D_CHx_B1_B_LENGTH(0));	// 0x48
	SAA7231_WR(stride, SAA7231_BAR0, module, S2D_CHx_B1_STRIDE(0));		// 0x4c
	SAA7231_WR(x_length, SAA7231_BAR0, module, S2D_CHx_B1_X_LENGTH(0));	// 0x50
	SAA7231_WR(y_length, SAA7231_BAR0, module, S2D_CHx_B1_Y_LENGTH(0));	// 0x54

	/* program other channel related registers */
	SAA7231_WR(OFFSET(dmabuf, 2), SAA7231_BAR0, module,  S2D_CHx_B2_B_START_ADDRESS(0));
	SAA7231_WR(OFFSET(dmabuf, 3), SAA7231_BAR0, module,  S2D_CHx_B3_B_START_ADDRESS(0));
	SAA7231_WR(OFFSET(dmabuf, 4), SAA7231_BAR0, module,  S2D_CHx_B4_B_START_ADDRESS(0));
	SAA7231_WR(OFFSET(dmabuf, 5), SAA7231_BAR0, module,  S2D_CHx_B5_B_START_ADDRESS(0));
	SAA7231_WR(OFFSET(dmabuf, 6), SAA7231_BAR0, module,  S2D_CHx_B6_B_START_ADDRESS(0));
	SAA7231_WR(OFFSET(dmabuf, 7), SAA7231_BAR0, module,  S2D_CHx_B7_B_START_ADDRESS(0));

	SAA7231_WR(channel_control, SAA7231_BAR0, module, S2D_CHx_CTRL(0));

	return 0;
}

static int saa7231_xs2dtl_acquire(struct saa7231_stream *stream)
{
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	struct stream_params *params	= &stream->params;
	int ret = 0;

	if (!params->pitch || !params->lines || !params->bps || !params->spl) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid Parameter");
		ret = -EINVAL;
		goto err;
	}
#if 0
	if (stream->port_hw != stream->port_id) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid stream type, type=0x%02x", stream->port_id);
		ret = -EINVAL;
		goto err;
	}
//	if (TM_FALSE == m_ucUseCaseActive) {
		// enable use case
		ret = m_pClockShop->tmIActivateSystemUseCase(m_tSystemUseCase, m_dwDMAChannel);
		if (TM_OK != tmStatus) {
			tmDBGPRINTEx(0,("Error: tmIActivateSystemUseCase %d failed!", m_tSystemUseCase ));
			ret = -EINVAL;
			goto err;
		}
		m_ucUseCaseActive = TM_TRUE;
//	}
#endif

	mutex_lock(&saa7231->dev_lock);

	if (!dtl->clock_run) {
		dprintk(SAA7231_DEBUG, 1, "Activating clock .. mode=0x%02x, port_id=0x%02x",
			stream->mode,
			stream->port_id);

		ret = saa7231_activate_clocks(saa7231, stream->mode, stream->port_id);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "Activating Clock for failed!");
			ret = -EINVAL;
			goto err;
		}
		dtl->clock_run = 1;
	}

	/* initialize hardware */
	ret = saa7231_xs2dtl_setparams(stream);
	if (ret < 0) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Setup Parameters failed!, err = %d", ret);
		ret = -EINVAL;
		goto err;
	}
err:
	mutex_unlock(&saa7231->dev_lock);
	return 0;
}

static int saa7231_xs2dtl_run(struct saa7231_stream *stream)
{
	unsigned long run = 0;
	u32 delay;

	u32 reg;
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	struct saa7231_ring *ring	= stream->ring;
	struct saa7231_dmabuf *dmabuf	= ring->dmabuf;
	u32 module			= dtl->module;

	SAA7231_WR(OFFSET(dmabuf, 0), SAA7231_BAR0, module, S2D_CHx_B0_B_START_ADDRESS(0));
	SAA7231_WR(OFFSET(dmabuf, 1), SAA7231_BAR0, module, S2D_CHx_B1_B_START_ADDRESS(0));

	/* tell the device that the mmu got updated */
	reg = SAA7231_RD(SAA7231_BAR0, MMU, MMU_DMA_CONFIG(stream->port_id));
	reg &= ~0x40;
	SAA7231_WR(reg, SAA7231_BAR0, MMU, MMU_DMA_CONFIG(stream->port_id));
	SAA7231_WR((reg | 0x40), SAA7231_BAR0, MMU, MMU_DMA_CONFIG(stream->port_id));

	dtl->addr_prev = 0xffffff;

	/**
	 * monitor PT load operation but wait a short time before
	 * checking the PT valid. Otherwise we will see the old state.
	 */
	delay = 1000;
	do {
		if (!run) {
			msleep(10);
			delay--;
		}
		run = SAA7231_RD(SAA7231_BAR0, MMU, MMU_DMA_CONFIG(stream->port_id)) & 0x80;
	} while (!run && delay);

	/* check status of the PTA load operation */
	if (!run) {
		dprintk(SAA7231_ERROR, 1, "ERROR, Preload PTA failed");
		return -EINVAL;
	}

	/* read channel control register, enable channel */
	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_CHx_CTRL(0));
	SAA7231_WR(reg | 0x1, SAA7231_BAR0, module, S2D_CHx_CTRL(0));
	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_S2D_CTRL);

	/* clear all pending interrupts */
	SAA7231_WR(0x3fff, SAA7231_BAR0, module, S2D_INT_CLR_STATUS(0));

	/* enable interrupts */
	SAA7231_WR(0x1800, SAA7231_BAR0, module, S2D_INT_SET_ENABLE(0));

	/* enable capture */
	reg |= 0x1;
	SAA7231_WR(reg, SAA7231_BAR0, module, S2D_S2D_CTRL);
	dtl->stream_run = 1;

	return 0;
}

static int saa7231_xs2dtl_pause(struct saa7231_stream *stream)
{
	int ret;
	u32 reg;
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	u32 module			= dtl->module;

	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_S2D_CTRL);

	/* check whether block is running */
	if (reg & 0x01) {
		/* disable capture bit */
		SAA7231_WR(reg & 0xfffffffe, SAA7231_BAR0, module, S2D_S2D_CTRL);
		/* TODO! print all errors */
		msleep(25);
	}

	/* RESET Stream State */
	dtl->stream_run = 0;

	/*
	 * configure hardware in the transition from run to pause.
	 * initial setup is done in acquire function.
	 */
	if (reg & 0x01) {

		/* clear all enabled interrupts */
		reg = SAA7231_RD(SAA7231_BAR0, module, S2D_INT_ENABLE(0));
		SAA7231_WR(reg, SAA7231_BAR0, module, S2D_INT_CLR_ENABLE(0));

		/* clear all pending interrupts */
		reg = SAA7231_RD(SAA7231_BAR0, module, S2D_INT_STATUS(0));
		SAA7231_WR(reg, SAA7231_BAR0, module, S2D_INT_CLR_STATUS(0));

		/* It's now safe to restore settings */
		ret = saa7231_xs2dtl_setparams(stream);
		if (ret != 0) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Setting parameters, err=%d", ret);
			return -EINVAL;
		}
	}

	/* initialize debug members */
	dtl->x_length = 0;
	dtl->y_length = 0;
	dtl->b_length = 0;
	dtl->data_loss = 0;
	dtl->wr_error = 0;
	dtl->tag_error = 0;
	dtl->dtl_halt = 0;

	return 0;
}

static int saa7231_xs2dtl_stop(struct saa7231_stream *stream)
{
	struct saa7231_dtl *dtl = &stream->dtl;

#if 0
	if (m_ucUseCaseActive) {
		tmErrorCode_t  tmStatus = TM_OK;

		// enable use case
		tmStatus = m_pClockShop->tmIDisableSystemUseCase(m_tSystemUseCase, m_dwDMAChannel);
		if (TM_OK != tmStatus)
			tmDBGPRINTEx(2,("Warning: tmIDisableSystemUseCase %d failed!", m_tSystemUseCase ));
		else
			m_ucUseCaseActive = TM_FALSE;
	}
	// release DMA controller members
#endif
	if (dtl->clock_run) {

		dtl->clock_run = 0;
	}

	return 0;
}

static int saa7231_xs2dtl_set_buffer(struct saa7231_stream *stream,
				     u32 index,
				     struct saa7231_dmabuf *dmabuf)
{
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	u32 module			= dtl->module;

	/* update buffer index */
	switch (index) {
	case 0:
		SAA7231_WR(OFFSET(dmabuf, 0), SAA7231_BAR0, module, S2D_CHx_B0_B_START_ADDRESS(0));
		break;
	case 1:
		SAA7231_WR(OFFSET(dmabuf, 1), SAA7231_BAR0, module, S2D_CHx_B1_B_START_ADDRESS(0));
		break;
	case 2:
		SAA7231_WR(OFFSET(dmabuf, 2), SAA7231_BAR0, module, S2D_CHx_B2_B_START_ADDRESS(0));
		break;
	case 3:
		SAA7231_WR(OFFSET(dmabuf, 3), SAA7231_BAR0, module, S2D_CHx_B3_B_START_ADDRESS(0));
		break;
	case 4:
		SAA7231_WR(OFFSET(dmabuf, 4), SAA7231_BAR0, module, S2D_CHx_B4_B_START_ADDRESS(0));
		break;
	case 5:
		SAA7231_WR(OFFSET(dmabuf, 5), SAA7231_BAR0, module, S2D_CHx_B5_B_START_ADDRESS(0));
		break;
	case 6:
		SAA7231_WR(OFFSET(dmabuf, 6), SAA7231_BAR0, module, S2D_CHx_B6_B_START_ADDRESS(0));
		break;
	case 7:
		SAA7231_WR(OFFSET(dmabuf, 7), SAA7231_BAR0, module, S2D_CHx_B7_B_START_ADDRESS(0));
		break;
	default:
		break;
	}
#if 0
	// set odd/even field flag (needed for VBI)
	if (m_tempBuffer[index].pdwIndexInfo)
		// the oslayer will check against %2
		*(m_tempBuffer[index].pdwIndexInfo) = index;
#endif

	return 0;
}

static int saa7231_xs2dtl_get_buffer(struct saa7231_stream *stream, u32 *index, u32 *fid)
{
	u32 reg, stat, index_prev, addr_curr;

	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	u32 module			= dtl->module;

	if (!dtl->stream_run) {
		*index = 0;
		return 0;
	}
	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_CHx_CURRENT_ADDRESS(0));
	stat = SAA7231_RD(SAA7231_BAR0, module, S2D_INT_STATUS(0));

	if (stat) {
		/* Clear all pending interrupts */
		SAA7231_WR(stat, SAA7231_BAR0, module, S2D_INT_CLR_STATUS(0));

		/* update debug members on occurance */
		if (stat & 0x4)
			dtl->x_length++;

		if (stat & 0x8)
			dtl->y_length++;

		if (stat & 0x10)
			dtl->b_length++;

		if (stat & 0x20)
			dtl->data_loss++;

		if (stat & 0x40)
			dtl->wr_error++;

		if (stat & 0x80)
			dtl->tag_error++;

		if (stat & 0x100)
			dtl->dtl_halt++;
	}

	if (reg == 0xffffffff) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid stream: %d", stream->port_id);
		return -EINVAL;
	}
	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_CHx_CURRENT_ADDRESS(0));
	*index =  INDEX(reg);

	/**
	 * PHPURU#180
	 * Check if the write index has been changed, if not re-read the status.
	 * It might be the case that the index hasn't been updated in time.
	 */
	index_prev = INDEX(dtl->addr_prev);

	if (index_prev == *index) {
		msleep(1);
		addr_curr = SAA7231_RD(SAA7231_BAR0, module, S2D_CHx_CURRENT_ADDRESS(0));
		*index = INDEX(addr_curr);
		reg = addr_curr;
	}

	dtl->addr_prev = reg;

	return 0;
}

static int saa7231_xs2dtl_exit(struct saa7231_stream *stream)
{
	int i;
	struct saa7231_ring *ring = stream->ring;
	struct saa7231_dmabuf *dmabuf = ring->dmabuf;
	struct saa7231_dev *saa7231 = dmabuf->saa7231;

	dprintk(SAA7231_DEBUG, 1, "Free XS2DTL engine ..");

	for (i = 0; i < RINGSIZ; i++) {
		dmabuf = &ring->dmabuf[i];
		saa7231_dmabuf_free(dmabuf);
	}
	dmabuf = &ring->dmabuf[0];
	kfree(dmabuf);
	saa7231_ring_exit(ring);
	return 0;
}

int saa7231_xs2dtl_init(struct saa7231_stream *stream, int pages)
{
	int i, ret = 0;
	struct stream_ops *ops		= &stream->ops;
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dmabuf *dmabuf;
	struct saa7231_ring *ring;

#if 0
	m_dwNumOfInputs   = 6;
	m_dwCurrentInput  = 5;
#endif

	dprintk(SAA7231_DEBUG, 1, "XS2DTL engine Initializing .....");
	mutex_lock(&saa7231->dev_lock);

	switch (stream->port_id) {
	case STREAM_PORT_DS2D_AVIS:
	case STREAM_PORT_DS2D_ITU:
		stream->port_hw = STREAM_VBI;
		stream->config	= 0;
		break;

	case STREAM_PORT_AS2D_LOCAL:
	case STREAM_PORT_AS2D_EXTERN:
		stream->port_hw = STREAM_AUDIO;
		stream->config	= 0x4; /* IIS stream */
		break;

	default:
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid DMA setup!");
		stream->config	= 0;
		return -EINVAL;
	}
	/* Allocate space for n XS2D Buffers */
	dmabuf = kzalloc(sizeof (struct saa7231_dmabuf) * RINGSIZ, GFP_KERNEL);
	if (!dmabuf) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Allocating %d XS2D Buffers", RINGSIZ);
		ret = -ENOMEM;
		goto err;
	}
	dprintk(SAA7231_DEBUG, 1, "Allocated %d XS2DTL Buffer @0x%p", XS2D_BUFFERS, dmabuf);

	dprintk(SAA7231_DEBUG, 1, "Allocating DMA Buffers ...");
	for (i = 0; i < RINGSIZ; i++) {
		dprintk(SAA7231_DEBUG, 1, "Allocating DMA buffer:%d @0x%p..", i, &dmabuf[i]);
		ret = saa7231_dmabuf_alloc(saa7231, &dmabuf[i], pages);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Failed allocating DMA buffers, error=%d", ret);
			ret = -ENOMEM;
			goto err;
		}
		dmabuf[i].count = i;
	}
	ring = saa7231_ring_init(dmabuf, RINGSIZ);
	if (!ring) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Allocating TS2D Ring");
		ret = -ENOMEM;
		goto err;
	}
	stream->ring = ring;

	dprintk(SAA7231_DEBUG, 1, "Initializing PTA ...");
	saa7231_init_ptables(stream);

	dprintk(SAA7231_DEBUG, 1, "Setting up PORT Gates ...");

	/* deactivate gating */
	switch (stream->port_id) {
	case STREAM_PORT_DS2D_AVIS:
		SAA7231_WR(0, SAA7231_BAR0, STREAM, STREAM_RA_VBI_LOC_CLGATE);
		break;
	case STREAM_PORT_DS2D_ITU:
		SAA7231_WR(0, SAA7231_BAR0, STREAM, STREAM_RA_VIDEO_EXT_CLGATE);
		break;
	case STREAM_PORT_AS2D_LOCAL:
		SAA7231_WR(0, SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_LOC_CLGATE);
		break;
	case STREAM_PORT_AS2D_EXTERN:
		SAA7231_WR(0, SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_EXT_CLGATE);
		break;

		/* TODO! disable clock usage here */
		break;
	default:
		ret = -EINVAL;
		goto err;
	}

	ops->acquire	= saa7231_xs2dtl_acquire;
	ops->run	= saa7231_xs2dtl_run;
	ops->pause	= saa7231_xs2dtl_pause;
	ops->stop	= saa7231_xs2dtl_stop;

	ops->set_buffer = saa7231_xs2dtl_set_buffer;
	ops->get_buffer = saa7231_xs2dtl_get_buffer;

	ops->exit 	= saa7231_xs2dtl_exit;

	return 0;
err:
	mutex_unlock(&saa7231->dev_lock);
	return ret;
}
