// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "../tvin_global.h"
#include "vdin_regs_t3x.h"
#include "vdin_drv.h"

void vdin_wr(struct vdin_dev_s *devp, u32 reg, const u32 val)
{
	if (devp->flags & VDIN_FLAG_ISR_EN &&
	    devp->flags & VDIN_FLAG_RDMA_ENABLE)
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
		rdma_write_reg(devp->rdma_handle, reg, val);
	else
#endif
		wr(0, reg, val);
}

void vdin_wr_bits(struct vdin_dev_s *devp, u32 reg, const u32 val, const u32 start, const u32 len)
{
	if (devp->flags & VDIN_FLAG_ISR_EN &&
	    devp->flags & VDIN_FLAG_RDMA_ENABLE)
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
		rdma_write_reg_bits(devp->rdma_handle, reg, val, start, len);
	else
#endif
		wr_bits(0, reg, val, start, len);
}

