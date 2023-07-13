// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/clk-provider.h>
#include <linux/init.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/clkdev.h>

#include "clk-pll.h"
#include "clk-regmap.h"
#include "clk-cpu-dyndiv.h"
#include "vid-pll-div.h"
#include "clk-dualdiv.h"
#include "s7.h"
#include <dt-bindings/clock/s7-clkc.h>

#define __MESON_CLK_MUX(_name, _reg, _mask, _shift, _table,		\
			_smcid, _secid, _secid_rd, _dflags,		\
			_ops, _pname, _pdata, _phw, _pnub, _iflags)	\
static struct clk_regmap _name = {					\
	.data = &(struct clk_regmap_mux_data){				\
		.offset = _reg,						\
		.mask = _mask,						\
		.shift = _shift,					\
		.table = _table,					\
		.smc_id = _smcid,					\
		.secid = _secid,					\
		.secid_rd = _secid_rd,					\
		.flags = _dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = _pnub,					\
		.flags = _iflags,					\
	},								\
}

#define __MESON_CLK_DIV(_name, _reg, _shift, _width, _table,		\
			_smcid, _secid, _dflags,			\
			_ops, _pname, _pdata, _phw, _iflags)		\
static struct clk_regmap _name = {					\
	.data = &(struct clk_regmap_div_data){				\
		.offset = _reg,						\
		.shift = _shift,					\
		.width = _width,					\
		.table = _table,					\
		.smc_id = _smcid,					\
		.secid = _secid,					\
		.flags = _dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
}

#define __MESON_CLK_GATE(_name, _reg, _bit, _dflags,			\
			 _ops, _pname, _pdata, _phw, _iflags)		\
static struct clk_regmap _name = {					\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = _reg,						\
		.bit_idx = _bit,					\
		.flags = _dflags,					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
}

#define MEMBER_REG_PARM(_member_name, _reg, _shift, _width)		\
	._member_name = {						\
		.reg_off = _reg,					\
		.shift   = _shift,					\
		.width   = _width,					\
	}

#define __MESON_CLK_DUALDIV(_name, _n1_reg, _n1_shift, _n1_width,	\
			    _n2_reg, _n2_shift, _n2_width,		\
			    _m1_reg, _m1_shift, _m1_width,		\
			    _m2_reg, _m2_shift, _m2_width,		\
			    _d_reg, _d_shift, _d_width, _dualdiv_table,	\
			    _ops, _pname, _pdata, _phw, _iflags)	\
static struct clk_regmap _name = {					\
	.data = &(struct meson_clk_dualdiv_data){			\
		MEMBER_REG_PARM(n1,					\
			_n1_reg, _n1_shift, _n1_width),			\
		MEMBER_REG_PARM(n2,					\
			_n2_reg, _n2_shift, _n2_width),			\
		MEMBER_REG_PARM(m1,					\
			_m1_reg, _m1_shift, _m1_width),			\
		MEMBER_REG_PARM(m2,					\
			_m2_reg, _m2_shift, _m2_width),			\
		MEMBER_REG_PARM(dual,					\
			_d_reg, _d_shift, _d_width),			\
		.table = _dualdiv_table,				\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
}

#define __MESON_CLK_CPU_DYN_SEC(_name, _secid_dyn, _secid_dyn_rd,	\
				_table, _table_cnt,			\
				_ops, _pname, _pdata, _phw, _pnub,	\
				_iflags)				\
static struct clk_regmap _name = {					\
	.data = &(struct meson_clk_cpu_dyn_data){			\
		.table = _table,					\
		.table_cnt = _table_cnt,				\
		.secid_dyn_rd = _secid_dyn_rd,				\
		.secid_dyn = _secid_dyn,				\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = _pnub,					\
		.flags = _iflags,					\
	},								\
}

#define __MESON_CLK_DYN(_name, _offset, _smc_id, _secid_dyn,		\
				_secid_dyn_rd, _table, _table_cnt,	\
				_ops, _pname, _pdata, _phw, _pnub,	\
				_iflags)				\
static struct clk_regmap _name = {					\
	.data = &(struct meson_clk_cpu_dyn_data){			\
		.table = _table,					\
		.offset = _offset,					\
		.smc_id = _smc_id,					\
		.table_cnt = _table_cnt,				\
		.secid_dyn_rd = _secid_dyn_rd,				\
		.secid_dyn = _secid_dyn,				\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = _pnub,					\
		.flags = _iflags,					\
	},								\
}

#define __MESON_CLK_FIXED_FACTOR(_name, _mult, _div, _ops,		\
				 _pname, _pdata, _phw, _iflags)		\
static struct clk_fixed_factor _name = {				\
	.mult = _mult,							\
	.div = _div,							\
	.hw.init = &(struct clk_init_data){				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
}

#ifdef CONFIG_ARM
#define __MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_init_reg, _init_reg_cnt, _range, _table,	\
			_smcid, _secid, _secid_dis, _dflags,		\
			_ops, _pname, _pdata, _phw, _iflags,		\
			_od_reg, _od_shift, _od_width)			\
static struct clk_regmap _name = {					\
	.data = &(struct meson_clk_pll_data){				\
		MEMBER_REG_PARM(en, _en_reg, _en_shift, _en_width),	\
		MEMBER_REG_PARM(m, _m_reg, _m_shift, _m_width),		\
		MEMBER_REG_PARM(frac, _f_reg, _f_shift, _f_width),	\
		MEMBER_REG_PARM(n, _n_reg, _n_shift, _n_width),		\
		MEMBER_REG_PARM(l, _l_reg, _l_shift, _l_width),		\
		MEMBER_REG_PARM(rst, _r_reg, _r_shift, _r_width),	\
		MEMBER_REG_PARM(od, _od_reg, _od_shift, _od_width),	\
		.range = _range,					\
		.table = _table,					\
		.init_regs = _init_reg,					\
		.init_count = _init_reg_cnt,				\
		.smc_id = _smcid,					\
		.secid = _secid,					\
		.secid_disable = _secid_dis,				\
		.flags = _dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
}
#else
#define __MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_init_reg, _init_reg_cnt, _range, _table,	\
			_smcid, _secid, _secid_dis, _dflags,		\
			_ops, _pname, _pdata, _phw, _iflags,		\
			_od_name, _od_reg, _od_shift, _od_width,	\
			_od_table, _od_smcid, _od_secid, _od_dflags,	\
			_od_ops, _od_iflags)				\
static struct clk_regmap _name = {					\
	.data = &(struct meson_clk_pll_data){				\
		MEMBER_REG_PARM(en, _en_reg, _en_shift, _en_width),	\
		MEMBER_REG_PARM(m, _m_reg, _m_shift, _m_width),		\
		MEMBER_REG_PARM(frac, _f_reg, _f_shift, _f_width),	\
		MEMBER_REG_PARM(n, _n_reg, _n_shift, _n_width),		\
		MEMBER_REG_PARM(l, _l_reg, _l_shift, _l_width),		\
		MEMBER_REG_PARM(rst, _r_reg, _r_shift, _r_width),	\
		MEMBER_REG_PARM(od, _od_reg, _od_shift, _od_width),	\
		.range = _range,					\
		.table = _table,					\
		.init_regs = _init_reg,					\
		.init_count = _init_reg_cnt,				\
		.smc_id = _smcid,					\
		.secid = _secid,					\
		.secid_disable = _secid_dis,				\
		.flags = _dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
};									\
static struct clk_regmap _od_name = {					\
	.data = &(struct clk_regmap_div_data){				\
		.offset = _od_reg,					\
		.shift = _od_shift,					\
		.width = _od_width,					\
		.table = _od_table,					\
		.smc_id = _od_smcid,					\
		.secid = _od_secid,					\
		.flags = _od_dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _od_name,					\
		.ops = _od_ops,						\
		.parent_hws = (const struct clk_hw *[]) {		\
			&_name.hw },					\
		.num_parents = 1,					\
		.flags = _od_iflags,					\
	},								\
}
#endif

#define MESON_CLK_MUX_RW(_name, _reg, _mask, _shift, _table, _dflags,	\
			 _pdata, _iflags)				\
	__MESON_CLK_MUX(_name, _reg, _mask, _shift, _table,		\
			0, 0, 0, _dflags,				\
			&clk_regmap_mux_ops, NULL, _pdata, NULL,	\
			ARRAY_SIZE(_pdata), _iflags)

#define MESON_CLK_MUX_RO(_name, _reg, _mask, _shift, _table, _dflags,	\
			 _pdata, _iflags)				\
	__MESON_CLK_MUX(_name, _reg, _mask, _shift, _table,		\
			0, 0, 0, CLK_MUX_READ_ONLY | (_dflags),		\
			&clk_regmap_mux_ro_ops, NULL, _pdata, NULL,	\
			ARRAY_SIZE(_pdata), _iflags)

#define MESON_CLK_MUX_SEC(_name, _reg, _mask, _shift, _table,		\
			  _smcid, _secid, _secid_rd, _dflags,		\
			  _pdata, _iflags)				\
	__MESON_CLK_MUX(_name, _reg, _mask, _shift, _table,		\
			_smcid, _secid, _secid_rd, _dflags,		\
			&clk_regmap_mux_ops, NULL, _pdata, NULL,	\
			ARRAY_SIZE(_pdata), _iflags)

#define MESON_CLK_DIV_RW(_name, _reg, _shift, _width, _table, _dflags,	\
			 _phw, _iflags)					\
	__MESON_CLK_DIV(_name, _reg, _shift, _width, _table,		\
			0, 0, _dflags,					\
			&clk_regmap_divider_ops, NULL, NULL,		\
			(const struct clk_hw *[]) { _phw }, _iflags)

#define MESON_CLK_DIV_RO(_name, _reg, _shift, _width, _table, _dflags,	\
			 _phw, _iflags)					\
	__MESON_CLK_DIV(_name, _reg, _shift, _width, _table,		\
			0, 0, CLK_DIVIDER_READ_ONLY | (_dflags),	\
			&clk_regmap_divider_ro_ops, NULL, NULL,		\
			(const struct clk_hw *[]) { _phw }, _iflags)

#define MESON_CLK_GATE_RW(_name, _reg, _bit, _dflags, _phw, _iflags)	\
	__MESON_CLK_GATE(_name, _reg, _bit, _dflags,			\
			 &clk_regmap_gate_ops, NULL, NULL,		\
			 (const struct clk_hw *[]) { _phw }, _iflags)

#define MESON_CLK_GATE_RO(_name, _reg, _bit, _dflags, _phw, _iflags)	\
	__MESON_CLK_GATE(_name, _reg, _bit, _dflags,			\
			 &clk_regmap_gate_ro_ops, NULL, NULL,		\
			 (const struct clk_hw *[]) { _phw }, _iflags)

#define MESON_CLK_CPU_DYN_SEC_RW(_name, _secid_dyn, _secid_dyn_rd,	\
				 _table, _pdata, _iflags)		\
	__MESON_CLK_CPU_DYN_SEC(_name, _secid_dyn, _secid_dyn_rd,	\
				_table, ARRAY_SIZE(_table),		\
				&meson_clk_cpu_dyn_ops,			\
				NULL, _pdata, NULL, ARRAY_SIZE(_pdata),	\
				_iflags)

#define MESON_CLK_DYN_RW(_name, _offset, _smc_id, _secid_dyn,		\
			 _secid_dyn_rd, _table, _pdata, _iflags)	\
	__MESON_CLK_DYN(_name, _offset, _smc_id, _secid_dyn,		\
			 _secid_dyn_rd, _table, ARRAY_SIZE(_table),	\
			 &meson_clk_cpu_dyn_ops,			\
			 NULL, _pdata, NULL, ARRAY_SIZE(_pdata),	\
			 _iflags)

#define MESON_CLK_FIXED_FACTOR(_name, _mult, _div, _phw, _iflags)	\
	__MESON_CLK_FIXED_FACTOR(_name, _mult, _div,			\
				 &clk_fixed_factor_ops, NULL, NULL,	\
				 (const struct clk_hw *[]) { _phw },	\
				 _iflags)

#define MESON_CLK_DUALDIV_RW(_name, _n1_reg, _n1_shift, _n1_width,	\
			     _n2_reg, _n2_shift, _n2_width,		\
			     _m1_reg, _m1_shift, _m1_width,		\
			     _m2_reg, _m2_shift, _m2_width,		\
			     _d_reg, _d_shift, _d_width, _dualdiv_table,\
			     _phw, _iflags)				\
	__MESON_CLK_DUALDIV(_name, _n1_reg, _n1_shift, _n1_width,	\
			    _n2_reg, _n2_shift, _n2_width,		\
			    _m1_reg, _m1_shift, _m1_width,		\
			    _m2_reg, _m2_shift, _m2_width,		\
			    _d_reg, _d_shift, _d_width, _dualdiv_table,	\
			    &meson_clk_dualdiv_ops, NULL, NULL,		\
			    (const struct clk_hw *[]) { _phw }, _iflags)

#ifdef CONFIG_ARM
#define MESON_CLK_PLL_RW(_name, _en_reg, _en_shift, _en_width,		\
			 _m_reg, _m_shift, _m_width,			\
			 _f_reg, _f_shift, _f_width,			\
			 _n_reg, _n_shift, _n_width,			\
			 _l_reg, _l_shift, _l_width,			\
			 _r_reg, _r_shift, _r_width,			\
			 _init_reg, _range, _table,			\
			 _dflags, _pdata, _iflags,			\
			 _od_reg, _od_shift, _od_width)			\
	__MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_init_reg, ARRAY_SIZE(_init_reg), _range, _table,\
			0, 0, 0, _dflags, &meson_clk_pll_v3_ops,	\
			NULL, _pdata, NULL, _iflags,			\
			_od_reg, _od_shift, _od_width)

#define MESON_CLK_PLL_RO(_name, _en_reg, _en_shift, _en_width,		\
			 _m_reg, _m_shift, _m_width,			\
			 _f_reg, _f_shift, _f_width,			\
			 _n_reg, _n_shift, _n_width,			\
			 _l_reg, _l_shift, _l_width,			\
			 _r_reg, _r_shift, _r_width,			\
			 _range, _table,				\
			 _dflags, _pdata, _iflags,			\
			 _od_reg, _od_shift, _od_width)			\
	__MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			NULL, 0, _range, _table,			\
			0, 0, 0, _dflags, &meson_clk_pll_ro_ops,	\
			NULL, _pdata, NULL, _iflags,			\
			_od_reg, _od_shift, _od_width)

#define MESON_CLK_PLL_SEC(_name, _en_reg, _en_shift, _en_width,		\
			  _m_reg, _m_shift, _m_width,			\
			  _f_reg, _f_shift, _f_width,			\
			  _n_reg, _n_shift, _n_width,			\
			  _l_reg, _l_shift, _l_width,			\
			  _r_reg, _r_shift, _r_width,			\
			  _range, _table,				\
			  _smcid, _secid, _secid_dis, _dflags,		\
			  _pdata, _iflags,				\
			  _od_reg, _od_shift, _od_width)		\
	__MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			NULL, 0, _range, _table,			\
			_smcid, _secid, _secid_dis, _dflags,		\
			&meson_secure_pll_v2_ops,			\
			NULL, _pdata, NULL, _iflags,			\
			_od_reg, _od_shift, _od_width)
#else
#define MESON_CLK_PLL_RW(_name, _en_reg, _en_shift, _en_width,		\
			 _m_reg, _m_shift, _m_width,			\
			 _f_reg, _f_shift, _f_width,			\
			 _n_reg, _n_shift, _n_width,			\
			 _l_reg, _l_shift, _l_width,			\
			 _r_reg, _r_shift, _r_width,			\
			 _init_reg, _range, _table,			\
			 _dflags, _pdata, _iflags,			\
			 _od_reg, _od_shift, _od_width, _od_table,	\
			 _od_dflags)					\
	__MESON_CLK_PLL(_name ## _dco, _en_reg, _en_shift, _en_width,	\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_init_reg, ARRAY_SIZE(_init_reg), _range, _table,\
			0, 0, 0, _dflags,				\
			&meson_clk_pll_v3_ops,				\
			NULL, _pdata, NULL, _iflags,			\
			_name, _od_reg, _od_shift, _od_width,		\
			_od_table, 0, 0, _od_dflags,			\
			&clk_regmap_divider_ops, CLK_SET_RATE_PARENT)

#define MESON_CLK_PLL_RO(_name, _en_reg, _en_shift, _en_width,		\
			 _m_reg, _m_shift, _m_width,			\
			 _f_reg, _f_shift, _f_width,			\
			 _n_reg, _n_shift, _n_width,			\
			 _l_reg, _l_shift, _l_width,			\
			 _r_reg, _r_shift, _r_width,			\
			 _range, _table,				\
			 _dflags, _pdata, _iflags,			\
			 _od_reg, _od_shift, _od_width, _od_table,	\
			 _od_dflags)	\
	__MESON_CLK_PLL(_name ## _dco, _en_reg, _en_shift, _en_width,	\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			NULL, 0, _range, _table,			\
			0, 0, 0, _dflags,				\
			&meson_clk_pll_ro_ops,				\
			NULL, _pdata, NULL, _iflags,			\
			_name, _od_reg, _od_shift, _od_width,		\
			_od_table, 0, 0, _od_dflags,			\
			&clk_regmap_divider_ro_ops, CLK_SET_RATE_PARENT)

#define MESON_CLK_PLL_SEC(_name, _en_reg, _en_shift, _en_width,		\
			 _m_reg, _m_shift, _m_width,			\
			 _f_reg, _f_shift, _f_width,			\
			 _n_reg, _n_shift, _n_width,			\
			 _l_reg, _l_shift, _l_width,			\
			 _r_reg, _r_shift, _r_width,			\
			 _range, _table,				\
			 _smcid, _secid, _secid_dis, _dflags,		\
			 _pdata, _iflags,				\
			 _od_reg, _od_shift, _od_width, _od_table,	\
			 _od_smcid, _od_secid, _od_dflags)		\
	__MESON_CLK_PLL(_name ## _dco, _en_reg, _en_shift, _en_width,	\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_od_reg, _od_shift, _od_width,			\
			NULL, 0, _range, _table,			\
			_smcid, _secid, _secid_dis, _dflags,		\
			&meson_secure_pll_v2_ops,			\
			NULL, _pdata, NULL, _iflags,			\
			_name, _od_reg, _od_shift, _od_width,		\
			_od_table, _od_smcid, _od_secid, _od_dflags,	\
			&clk_regmap_divider_ro_ops, CLK_SET_RATE_PARENT)
#endif

#define __MESON_CLK_COMPOSITE(_m_name, _m_reg, _m_mask, _m_shift,	\
			      _m_table, _m_dflags, _m_ops, _pname,	\
			      _pdata, _phw, _pnub, _m_iflags,		\
			      _d_name, _d_reg, _d_shift, _d_width,	\
			      _d_table, _d_dflags, _d_ops, _d_iflags,	\
			      _g_name, _g_reg, _g_bit, _g_dflags,	\
			      _g_ops, _g_iflags)			\
static struct clk_regmap _m_name = {					\
	.data = &(struct clk_regmap_mux_data){				\
		.offset = _m_reg,					\
		.mask = _m_mask,					\
		.shift = _m_shift,					\
		.table = _m_table,					\
		.flags = _m_dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _m_name,					\
		.ops = _m_ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = _pnub,					\
		.flags = _m_iflags,					\
	},								\
};									\
static struct clk_regmap _d_name = {					\
	.data = &(struct clk_regmap_div_data){				\
		.offset = _d_reg,					\
		.shift = _d_shift,					\
		.width = _d_width,					\
		.table = _d_table,					\
		.flags = _d_dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _d_name,					\
		.ops = _d_ops,						\
		.parent_hws = (const struct clk_hw *[]) {		\
			&_m_name.hw },					\
		.num_parents = 1,					\
		.flags = _d_iflags,					\
	},								\
};									\
static struct clk_regmap _g_name = {					\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = _g_reg,					\
		.bit_idx = _g_bit,					\
		.flags = _g_dflags,					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = # _g_name,					\
		.ops = _g_ops,						\
		.parent_hws = (const struct clk_hw *[]) {		\
			&_d_name.hw },					\
		.num_parents = 1,					\
		.flags = _g_iflags,					\
	},								\
}

#define MESON_CLK_COMPOSITE_RW(_cname, _m_reg, _m_mask, _m_shift,	\
			       _m_table, _m_dflags, _m_pdata, _m_iflags,\
			       _d_reg, _d_shift, _d_width, _d_table,	\
			       _d_dflags, _d_iflags,			\
			       _g_reg, _g_bit, _g_dflags, _g_iflags)	\
	__MESON_CLK_COMPOSITE(_cname ## _mux, _m_reg, _m_mask, _m_shift,\
			      _m_table, _m_dflags, &clk_regmap_mux_ops,	\
			      NULL, _m_pdata, NULL,			\
			      ARRAY_SIZE(_m_pdata), _m_iflags,		\
			      _cname ## _div,				\
			      _d_reg, _d_shift, _d_width, _d_table,	\
			      _d_dflags,				\
			      &clk_regmap_divider_ops, _d_iflags,	\
			      _cname, _g_reg, _g_bit, _g_dflags,	\
			      &clk_regmap_gate_ops, _g_iflags)

#define MESON_CLK_COMPOSITE_RO(_cname, _m_reg, _m_mask, _m_shift,	\
			       _m_table, _m_dflags, _m_pdata, _m_iflags,\
			       _d_reg, _d_shift, _d_width, _d_table,	\
			       _d_dflags, _d_iflags,			\
			       _g_reg, _g_bit, _g_dflags, _g_iflags)	\
	__MESON_CLK_COMPOSITE(_cname ## _mux, _m_reg, _m_mask, _m_shift,\
			      _m_table, CLK_MUX_READ_ONLY | (_m_dflags),\
			      &clk_regmap_mux_ro_ops,			\
			      NULL, _m_pdata, NULL,			\
			      ARRAY_SIZE(_m_pdata), _m_iflags,		\
			      _cname ## _div,				\
			      _d_reg, _d_shift, _d_width, _d_table,	\
			      CLK_DIVIDER_READ_ONLY | (_d_dflags),	\
			      &clk_regmap_divider_ro_ops, _d_iflags,	\
			      _cname, _g_reg, _g_bit, _g_dflags,	\
			      &clk_regmap_gate_ro_ops, _g_iflags)

static const struct clk_parent_data pll_dco_parent = {
	.fw_name = "xtal",
};

#ifdef CONFIG_ARM
static const struct pll_params_table sys_pll_params_table[] = {
	PLL_PARAMS(67, 1, 0), /*DCO=1608M OD=1608MM*/
	PLL_PARAMS(71, 1, 0), /*DCO=1704MM OD=1704M*/
	PLL_PARAMS(75, 1, 0), /*DCO=1800M OD=1800M*/
	PLL_PARAMS(79, 1, 0), /*DCO=1896M OD=1896M*/
	PLL_PARAMS(84, 1, 0), /*DCO=2016M OD=2016M*/
	PLL_PARAMS(100, 1, 1), /*DCO=2400M OD=1200M*/
	PLL_PARAMS(116, 1, 1), /*DCO=2784M OD=1392M*/
	PLL_PARAMS(126, 1, 1), /*DCO=3024M OD=1512M*/
	PLL_PARAMS(133, 1, 1), /*DCO=3192M OD=1596M*/
	{ /* sentinel */ }
};
#else
static const struct pll_params_table sys_pll_params_table[] = {
	PLL_PARAMS(67, 1), /*DCO=1608M*/
	PLL_PARAMS(71, 1), /*DCO=1704M*/
	PLL_PARAMS(75, 1), /*DCO=1800M*/
	PLL_PARAMS(79, 1), /*DCO=1896M*/
	PLL_PARAMS(84, 1), /*DCO=2016M*/
	PLL_PARAMS(100, 1), /*DCO=2400M*/
	PLL_PARAMS(116, 1), /*DCO=2784M*/
	PLL_PARAMS(126, 1), /*DCO=3024M*/
	PLL_PARAMS(133, 1), /*DCO=3192M*/
	{ /* sentinel */ }
};
#endif

MESON_CLK_PLL_SEC(sys_pll, ANACTRL_SYS0PLL_CTRL0, 28, 1,  /* en */
		  ANACTRL_SYS0PLL_CTRL0, 0, 9,  /* m */
		  0, 0, 0,  /* frac */
		  ANACTRL_SYS0PLL_CTRL0, 11, 5,  /* n */
		  ANACTRL_SYS0PLL_CTRL0, 31, 1,  /* lock */
		  ANACTRL_SYS0PLL_CTRL0, 29, 1,  /* rst */
		  //0, 0, 0,  /* th */
		  NULL, sys_pll_params_table,
		  SECURE_PLL_CLK, SECID_SYS0_DCO_PLL, SECID_SYS0_DCO_PLL_DIS,
		  0,
		  &pll_dco_parent, 0,
		  ANACTRL_SYS0PLL_CTRL0, 9, 2   /* od */
#ifdef CONFIG_ARM
		  );
#else
		  , NULL, SECURE_PLL_CLK, SECID_SYS0_PLL_OD,
		  CLK_DIVIDER_POWER_OF_TWO);
#endif

MESON_CLK_PLL_SEC(sys1_pll, ANACTRL_SYS1PLL_CTRL0, 28, 1,  /* en */
		  ANACTRL_SYS1PLL_CTRL0, 0, 9,  /* m */
		  0, 0, 0,  /* frac */
		  ANACTRL_SYS1PLL_CTRL0, 11, 5,  /* n */
		  ANACTRL_SYS1PLL_CTRL0, 31, 1,  /* lock */
		  ANACTRL_SYS1PLL_CTRL0, 29, 1,  /* rst */
		  //0, 0, 0,  /* th */
		  NULL, sys_pll_params_table,
		  SECURE_PLL_CLK, SECID_SYS1_DCO_PLL, SECID_SYS1_DCO_PLL_DIS,
		  0,
		  &pll_dco_parent, 0,
		  ANACTRL_SYS1PLL_CTRL0, 9, 2   /* od */
#ifdef CONFIG_ARM
		  );
#else
		  , NULL, SECURE_PLL_CLK, SECID_SYS1_PLL_OD,
		  CLK_DIVIDER_POWER_OF_TWO);
#endif

MESON_CLK_PLL_RO(fixed_pll, ANACTRL_FIXPLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_FIXPLL_CTRL0, 0,  9,  /* m */
		 0, 0,  0,  /* frac */
		 ANACTRL_FIXPLL_CTRL0, 11, 5,  /* n */
		 ANACTRL_FIXPLL_CTRL0, 31, 1,  /* lock */
		 ANACTRL_FIXPLL_CTRL0, 29, 1,  /* rst */
		 //0, 0, 0,  /* th */
		 NULL, NULL,
		 0, &pll_dco_parent, 0,
		 ANACTRL_FIXPLL_CTRL0, 9, 2     /* od */
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

MESON_CLK_FIXED_FACTOR(fclk_div2_div, 1, 2, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div2, ANACTRL_FIXPLL_CTRL1, 30, 0, &fclk_div2_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div3_div, 1, 3, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div3, ANACTRL_FIXPLL_CTRL1, 26, 0, &fclk_div3_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div4_div, 1, 4, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div4, ANACTRL_FIXPLL_CTRL1, 27, 0, &fclk_div4_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div5_div, 1, 5, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div5, ANACTRL_FIXPLL_CTRL1, 28, 0, &fclk_div5_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div7_div, 1, 7, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div7, ANACTRL_FIXPLL_CTRL1, 29, 0, &fclk_div7_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div2p5_div, 5, 2, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div2p5, ANACTRL_FIXPLL_CTRL0, 25, 0, &fclk_div2p5_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_clk50m_div, 1, 40, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_clk50m, ANACTRL_FIXPLL_CTRL1, 31, 0, &fclk_clk50m_div.hw, 0);

#ifdef CONFIG_ARM
static const struct pll_params_table gp0_pll_table[] = {
	PLL_PARAMS(70, 1, 1), /* DCO = 1680M OD = 2 PLL = 840M */
	PLL_PARAMS(132, 1, 2), /* DCO = 3168M OD = 4 PLL = 792M */
	PLL_PARAMS(128, 1, 2), /* DCO = 3072M OD = 4 PLL = 768M */
	PLL_PARAMS(124, 1, 2), /* DCO = 2976M OD = 4 PLL = 744M */
	PLL_PARAMS(96, 1, 1), /* DCO = 2304M OD = 2 PLL = 1152M */
	{ /* sentinel */  }
};
#else
static const struct pll_params_table gp0_pll_table[] = {
	PLL_PARAMS(70, 1), /* DCO = 1680M */
	PLL_PARAMS(132, 1), /* DCO = 3168M */
	PLL_PARAMS(128, 1), /* DCO = 3072M */
	PLL_PARAMS(124, 1), /* DCO = 2976M */
	PLL_PARAMS(96, 1), /* DCO = 2304M */
	{ /* sentinel */  }
};
#endif

static const struct reg_sequence gp0_init_regs[] = {
	{ .reg = ANACTRL_GP0PLL_CTRL0,	.def = 0x22530b09 },
	{ .reg = ANACTRL_GP0PLL_CTRL1,	.def = 0x09900303 },
	{ .reg = ANACTRL_GP0PLL_CTRL2,	.def = 0x00006666 },
	{ .reg = ANACTRL_GP0PLL_CTRL3,	.def = 0x00000006 },
	{ .reg = ANACTRL_GP0PLL_CTRL0,	.def = 0x32530b09, .delay_us = 20 },
	{ .reg = ANACTRL_GP0PLL_CTRL0,	.def = 0x12530b09, .delay_us = 20 },
	{ .reg = ANACTRL_GP0PLL_CTRL0,	.def = 0x12520b09 },
};

MESON_CLK_PLL_RW(gp0_pll, ANACTRL_GP0PLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_GP0PLL_CTRL0, 0, 9,  /* m */
		 ANACTRL_GP0PLL_CTRL2, 0, 19,  /* frac */
		 ANACTRL_GP0PLL_CTRL0, 11, 5,  /* n */
		 ANACTRL_GP0PLL_CTRL0, 31, 1,  /* lock */
		 ANACTRL_GP0PLL_CTRL0, 29, 1,  /* rst */
		 //0, 0, 0,  /* th */
		 gp0_init_regs, NULL, gp0_pll_table,
		 CLK_MESON_PLL_IGNORE_INIT,
		 &pll_dco_parent, 0,
		 ANACTRL_GP0PLL_CTRL0, 9, 2    /* od */
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

#ifdef CONFIG_ARM
static const struct pll_params_table gp1_pll_param_table[] = {
	PLL_PARAMS(100, 1, 1), /*DCO=2400M OD=1200M*/
	PLL_PARAMS(116, 1, 1), /*DCO=2784 OD=1392M*/
	PLL_PARAMS(125, 1, 1), /*DCO=3000 OD=1500M*/
	PLL_PARAMS(67, 1, 0), /*DCO=1608M OD=1608MM*/
	PLL_PARAMS(71, 1, 0), /*DCO=1704MM OD=1704M*/
	PLL_PARAMS(75, 1, 0), /*DCO=1800M OD=1800M*/
	PLL_PARAMS(79, 1, 0), /*DCO=1896M OD=1896M*/
	PLL_PARAMS(81, 1, 0), /*DCO=1944M OD=1944M*/
	{ /* sentinel */ }
};
#else
static const struct pll_params_table gp1_pll_param_table[] = {
	PLL_PARAMS(100, 1), /* DCO=2400M */
	PLL_PARAMS(116, 1), /* DCO=2784 */
	PLL_PARAMS(125, 1), /* DCO=3000 */
	PLL_PARAMS(67, 1), /* DCO=1608M */
	PLL_PARAMS(71, 1), /* DCO=1704M */
	PLL_PARAMS(75, 1), /* DCO=1800M */
	PLL_PARAMS(79, 1), /* DCO=1896M */
	PLL_PARAMS(81, 1), /* DCO=1944M */
	{ /* sentinel */ }
};
#endif

MESON_CLK_PLL_SEC(gp1_pll, ANACTRL_GP1PLL_CTRL0, 28, 1,  /* en */
		  ANACTRL_GP1PLL_CTRL0, 0, 9,  /* m */
		  0, 0, 0,  /* frac */
		  ANACTRL_GP1PLL_CTRL0, 11, 5,  /* n */
		  ANACTRL_GP1PLL_CTRL0, 31, 1,  /* lock */
		  ANACTRL_GP1PLL_CTRL0, 29, 1,  /* rst */
		  //0, 0, 0,  /* th */
		  NULL, gp1_pll_param_table,
		  SECURE_PLL_CLK, SECID_GP1_DCO_PLL, SECID_GP1_DCO_PLL_DIS,
		  0,
		  &pll_dco_parent, 0,
		  ANACTRL_GP1PLL_CTRL0, 9, 2   /* od */
#ifdef CONFIG_ARM
		  );
#else
		  , NULL, SECURE_PLL_CLK, SECID_GP1_PLL_OD,
		  CLK_DIVIDER_POWER_OF_TWO);
#endif

/*cpu_clk*/
static const struct cpu_dyn_table cpu_dyn_parent_table[] = {
	CPU_LOW_PARAMS(24000000, 0, 0, 0),
	CPU_LOW_PARAMS(100000000, 1, 1, 9),
	CPU_LOW_PARAMS(250000000, 1, 1, 3),
	CPU_LOW_PARAMS(333333333, 2, 1, 1),
	CPU_LOW_PARAMS(500000000, 1, 1, 1),
	CPU_LOW_PARAMS(666666666, 2, 0, 0),
	CPU_LOW_PARAMS(1000000000, 1, 0, 0)
};

static const struct clk_parent_data cpu_dyn_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div2p5.hw }
};

MESON_CLK_CPU_DYN_SEC_RW(cpu_dyn_clk, SECID_CPU_CLK_DYN, SECID_CPU_CLK_RD,
			 cpu_dyn_parent_table, cpu_dyn_parent_data, 0);

static const struct clk_parent_data cpu_parent_data[] = {
	{ .hw = &cpu_dyn_clk.hw },
	{ .hw = &sys_pll.hw }
};

MESON_CLK_MUX_SEC(cpu_clk, CPUCTRL_SYS_CPU_CLK_CTRL, 0x1, 11,
			NULL, SECURE_CPU_CLK, SECID_CPU_CLK_SEL, SECID_CPU_CLK_RD,
			0, cpu_parent_data,
			CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE);

/*dsu_clk*/
static const struct clk_parent_data dsu_dyn_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &sys1_pll.hw }
};

MESON_CLK_CPU_DYN_SEC_RW(dsu_dyn_clk, SECID_DSU_CLK_DYN, SECID_DSU_CLK_RD,
			 cpu_dyn_parent_table, dsu_dyn_parent_data, 0);

static const struct clk_parent_data dsu_parent_data[] = {
	{ .hw = &dsu_dyn_clk.hw },
	{ .hw = &sys_pll.hw }
};

MESON_CLK_MUX_SEC(dsu_clk, CPUCTRL_SYS_CPU_CLK_CTRL5, 0x1, 11,
		  NULL, SECURE_CPU_CLK, SECID_DSU_CLK_SEL, SECID_DSU_CLK_RD,
		  0, dsu_parent_data,
		  CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE);

static const struct clk_parent_data dsu_final_parent_data[] = {
	{ .hw = &dsu_clk.hw },
	{ .hw = &cpu_clk.hw }
};

MESON_CLK_MUX_SEC(dsu_final_clk, CPUCTRL_SYS_CPU_CLK_CTRL6, 0x1, 27,
		  NULL, SECURE_CPU_CLK, SECID_DSU_FINAL_CLK_SEL, SECID_DSU_FINAL_CLK_RD,
		  0, dsu_final_parent_data,
		  CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE);

struct s7_sys_pll_nb_data {
	struct notifier_block nb;
	struct clk_hw *sys_pll;
	struct clk_hw *cpu_clk;
	struct clk_hw *cpu_dyn_clk;
};

static int s7_sys_pll_notifier_cb(struct notifier_block *nb,
				  unsigned long event, void *data)
{
	struct s7_sys_pll_nb_data *nb_data =
		container_of(nb, struct s7_sys_pll_nb_data, nb);

	switch (event) {
	case PRE_RATE_CHANGE:
		/*
		 * This notifier means sys_pll clock will be changed
		 * to feed cpu_clk, this the current path :
		 * cpu_clk
		 *    \- sys_pll
		 *          \- sys_pll_dco
		 */

		/* make sure cpu_clk 1G*/
		if (clk_set_rate(nb_data->cpu_dyn_clk->clk, 1000000000))
			pr_err("%s in %d\n", __func__, __LINE__);
		/* Configure cpu_clk to use cpu_dyn_clk */
		clk_hw_set_parent(nb_data->cpu_clk,
				  nb_data->cpu_dyn_clk);

		/*
		 * Now, cpu_clk uses the dyn path
		 * cpu_clk
		 *    \- cpu_dyn_clk
		 *          \- cpu_clk_dynX
		 *                \- cpu_clk_dynX_mux
		 *		     \- cpu_clk_dynX_div
		 *                      \- xtal/fclk_div2/fclk_div3
		 *                   \- xtal/fclk_div2/fclk_div3
		 */

		return NOTIFY_OK;

	case POST_RATE_CHANGE:
		/*
		 * The sys_pll has ben updated, now switch back cpu_clk to
		 * sys_pll
		 */

		/* Configure cpu_clk to use sys_pll */
		clk_hw_set_parent(nb_data->cpu_clk,
				  nb_data->sys_pll);

		/* new path :
		 * cpu_clk
		 *    \- sys_pll
		 *          \- sys_pll_dco
		 */

		return NOTIFY_OK;

	default:
		return NOTIFY_DONE;
	}
}

static struct s7_sys_pll_nb_data s7_sys_pll_nb_data = {
	.sys_pll = &sys_pll.hw,
	.cpu_clk = &cpu_clk.hw,
	.cpu_dyn_clk = &cpu_dyn_clk.hw,
	.nb.notifier_call = s7_sys_pll_notifier_cb,
};

/* hifi_pll */
static const struct clk_parent_data hifi_pll_dco_parent_data = {
	.fw_name = "xtal",
};

#ifdef CONFIG_ARM
static const struct pll_params_table hifi_pll_table[] = {
	PLL_PARAMS(70, 1, 1), /* DCO = 1680M OD = 2 PLL = 840M */
	PLL_PARAMS(132, 1, 2), /* DCO = 3168M OD = 4 PLL = 792M */
	PLL_PARAMS(128, 1, 2), /* DCO = 3072M OD = 4 PLL = 768M */
	PLL_PARAMS(124, 1, 2), /* DCO = 2976M OD = 4 PLL = 744M */
	PLL_PARAMS(96, 1, 1), /* DCO = 2304M OD = 2 PLL = 1152M */
	{ /* sentinel */  }
};
#else
static const struct pll_params_table hifi_pll_table[] = {
	PLL_PARAMS(70, 1), /* DCO = 1680M */
	PLL_PARAMS(132, 1), /* DCO = 3168M */
	PLL_PARAMS(128, 1), /* DCO = 3072M */
	PLL_PARAMS(124, 1), /* DCO = 2976M */
	PLL_PARAMS(96, 1), /* DCO = 2304M  */
	{ /* sentinel */  }
};
#endif

static const struct reg_sequence hifi_init_regs[] = {
	{ .reg = ANACTRL_HIFI0PLL_CTRL0, .def = 0x22530b09 },
	{ .reg = ANACTRL_HIFI0PLL_CTRL1, .def = 0x09900303 },
	{ .reg = ANACTRL_HIFI0PLL_CTRL2, .def = 0x00004e20 },
	{ .reg = ANACTRL_HIFI0PLL_CTRL3, .def = 0x00000006 },
	{ .reg = ANACTRL_HIFI0PLL_CTRL0, .def = 0x32530b09, .delay_us = 20 },
	{ .reg = ANACTRL_HIFI0PLL_CTRL0, .def = 0x12530b09, .delay_us = 20 },
	{ .reg = ANACTRL_HIFI0PLL_CTRL0, .def = 0x12520b09 }
};

static const struct pll_mult_range hifi_pll_m = {
	.min = 67,
	.max = 133,
};

MESON_CLK_PLL_RW(hifi_pll, ANACTRL_HIFI0PLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_HIFI0PLL_CTRL0, 0, 9,  /* m */
		 ANACTRL_HIFI0PLL_CTRL2, 0, 19,  /* frac */
		 ANACTRL_HIFI0PLL_CTRL0, 11, 5,  /* n */
		 ANACTRL_HIFI0PLL_CTRL0, 31, 0,  /* lock */
		 ANACTRL_HIFI0PLL_CTRL0, 29, 1,  /* rst */
		 //0, 0, 0,  /* th */
		 hifi_init_regs, &hifi_pll_m, hifi_pll_table,
		 CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION,
		 &hifi_pll_dco_parent_data, 0,
		 ANACTRL_HIFI0PLL_CTRL0, 9, 2   /* od */
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

MESON_CLK_PLL_RW(hifi1_pll, ANACTRL_HIFI1PLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_HIFI1PLL_CTRL0, 0, 9,  /* m */
		 ANACTRL_HIFI1PLL_CTRL2, 0, 19,  /* frac */
		 ANACTRL_HIFI1PLL_CTRL0, 11, 5,  /* n */
		 ANACTRL_HIFI1PLL_CTRL0, 31, 0,  /* lock */
		 ANACTRL_HIFI1PLL_CTRL0, 29, 1,  /* rst */
		 //0, 0, 0,  /* th */
		 hifi_init_regs, &hifi_pll_m, hifi_pll_table,
		 CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION,
		 &hifi_pll_dco_parent_data, 0,
		 ANACTRL_HIFI1PLL_CTRL0, 9, 2   /* od */
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

/*
 *rtc 32k clock
 *
 *xtal--GATE------------------GATE---------------------|\
 *	              |  --------                      | \
 *	              |  |      |                      |  \
 *	              ---| DUAL |----------------------|   |
 *	                 |      |                      |   |____GATE__
 *	                 --------                      |   |     rtc_32k_out
 *	   PAD-----------------------------------------|  /
 *	                                               | /
 *	   DUAL function:                              |/
 *	   bit 28 in RTC_BY_OSCIN_CTRL0 control the dual function.
 *	   when bit 28 = 0
 *	         f = 24M/N0
 *	   when bit 28 = 1
 *	         output N1 and N2 in run.
 *	   T = (x*T1 + y*T2)/x+y
 *	   f = (24M/(N0*M0 + N1*M1)) * (M0 + M1)
 *	   f: the frequecy value (HZ)
 *	       |      | |      |
 *	       | Div1 |-| Cnt1 |
 *	      /|______| |______|\
 *	    -|  ______   ______  ---> Out
 *	      \|      | |      |/
 *	       | Div2 |-| Cnt2 |
 *	       |______| |______|
 **/

/*
 * rtc 32k clock in gate
 */
static const struct clk_parent_data rtc_xtal_clkin_parent = {
	.fw_name = "xtal",
};

__MESON_CLK_GATE(rtc_xtal_clkin, CLKCTRL_RTC_BY_OSCIN_CTRL0, 31, 0,
		 &clk_regmap_gate_ops, NULL, &rtc_xtal_clkin_parent, NULL, 0);

static const struct meson_clk_dualdiv_param rtc_32k_div_table[] = {
	{ 733, 732, 8, 11, 1 },
	{ /* sentinel */ }
};

MESON_CLK_DUALDIV_RW(rtc_32k_div, CLKCTRL_RTC_BY_OSCIN_CTRL0, 0,  12,    /* n1 */
		     CLKCTRL_RTC_BY_OSCIN_CTRL0, 12, 12,        /* n2 */
		     CLKCTRL_RTC_BY_OSCIN_CTRL1, 0,  12,        /* m1 */
		     CLKCTRL_RTC_BY_OSCIN_CTRL1, 12, 12,        /* m2 */
		     CLKCTRL_RTC_BY_OSCIN_CTRL0, 28, 1, rtc_32k_div_table,
		     &rtc_xtal_clkin.hw, 0);

static const struct clk_parent_data rtc_32k_mux_parent_data[] = {
	{ .hw = &rtc_32k_div.hw },
	{ .hw = &rtc_xtal_clkin.hw }
};

MESON_CLK_MUX_RW(rtc_32k_mux, CLKCTRL_RTC_BY_OSCIN_CTRL1, 0x1, 24, NULL, 0,
		 rtc_32k_mux_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(rtc_32k, CLKCTRL_RTC_BY_OSCIN_CTRL0, 30, 0,
		  &rtc_32k_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data rtc_clk_mux_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &rtc_32k.hw },
	{ .fw_name = "pad", }
};

MESON_CLK_MUX_RW(rtc_clk, CLKCTRL_RTC_CTRL, 0x3, 0, NULL, 0,
		  rtc_clk_mux_parent_data, CLK_SET_RATE_PARENT);

/* sys clk */
static u32 sys_ab_clk_parent_table[] = { 0, 1, 2, 3, 4, 7 };
static const struct clk_parent_data sys_ab_clk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &rtc_clk.hw }
};

MESON_CLK_COMPOSITE_RW(sysclk_a, CLKCTRL_SYS_CLK_CTRL0, 0x7, 10,
		       sys_ab_clk_parent_table, 0, sys_ab_clk_parent_data, 0,
		       CLKCTRL_SYS_CLK_CTRL0, 0, 10, NULL, 0, 0,
		       CLKCTRL_SYS_CLK_CTRL0, 13, 0, CLK_IGNORE_UNUSED);
MESON_CLK_COMPOSITE_RW(sysclk_b, CLKCTRL_SYS_CLK_CTRL0, 0x7, 26,
		       sys_ab_clk_parent_table, 0, sys_ab_clk_parent_data, 0,
		       CLKCTRL_SYS_CLK_CTRL0, 16, 10, NULL, 0, 0,
		       CLKCTRL_SYS_CLK_CTRL0, 29, 0, CLK_IGNORE_UNUSED);
static const struct clk_parent_data sys_clk_parent_data[] = {
	{ .hw = &sysclk_a.hw },
	{ .hw = &sysclk_b.hw }
};

MESON_CLK_MUX_RW(sys_clk, CLKCTRL_SYS_CLK_CTRL0, 1, 15, NULL, 0,
		 sys_clk_parent_data, CLK_IS_CRITICAL);

/*axi clk*/
static u32 axi_ab_clk_parent_table[] = { 0, 1, 2, 3, 4, 7 };
static const struct clk_parent_data axi_ab_clk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &rtc_clk.hw }
};

MESON_CLK_COMPOSITE_RW(axiclk_a, CLKCTRL_AXI_CLK_CTRL0, 0x7, 10,
		       axi_ab_clk_parent_table, 0, axi_ab_clk_parent_data, 0,
		       CLKCTRL_AXI_CLK_CTRL0, 0, 10, NULL, 0, 0,
		       CLKCTRL_AXI_CLK_CTRL0, 13, 0, CLK_IGNORE_UNUSED);
MESON_CLK_COMPOSITE_RW(axiclk_b, CLKCTRL_AXI_CLK_CTRL0, 0x7, 26,
		       axi_ab_clk_parent_table, 0, axi_ab_clk_parent_data, 0,
		       CLKCTRL_AXI_CLK_CTRL0, 16, 10, NULL, 0, 0,
		       CLKCTRL_AXI_CLK_CTRL0, 29, 0, CLK_IGNORE_UNUSED);
static const struct clk_parent_data axi_clk_parent_data[] = {
	{ .hw = &axiclk_a.hw },
	{ .hw = &axiclk_b.hw }
};

MESON_CLK_MUX_RW(axi_clk, CLKCTRL_AXI_CLK_CTRL0, 1, 15, NULL, 0,
		 axi_clk_parent_data, CLK_IS_CRITICAL);

/* cecb_clk */
static const struct clk_parent_data cecb_xtal_clkin_parent = {
	.fw_name = "xtal",
};

__MESON_CLK_GATE(cecb_xtal_clkin, CLKCTRL_CECB_CTRL0, 31, 0,
		 &clk_regmap_gate_ops, NULL, &cecb_xtal_clkin_parent, NULL, 0);

static const struct meson_clk_dualdiv_param cecb_32k_div_table[] = {
	{ 733, 732, 8, 11, 1 },
	{ /* sentinel */ }
};

MESON_CLK_DUALDIV_RW(cecb_32k_div, CLKCTRL_CECB_CTRL0, 0,  12,
		     CLKCTRL_CECB_CTRL0, 12, 12,
		     CLKCTRL_CECB_CTRL1, 0,  12,
		     CLKCTRL_CECB_CTRL1, 12, 12,
		     CLKCTRL_CECB_CTRL0, 28, 1, cecb_32k_div_table,
		     &cecb_xtal_clkin.hw, 0);

static const struct clk_parent_data cecb_32k_mux_pre_parent_data[] = {
	{ .hw = &cecb_xtal_clkin.hw },
	{ .hw = &cecb_32k_div.hw }
};

MESON_CLK_MUX_RW(cecb_32k_mux_pre, CLKCTRL_CECB_CTRL1, 0x1, 24, NULL, 0,
		 cecb_32k_mux_pre_parent_data, CLK_SET_RATE_PARENT);

static const struct clk_parent_data cecb_32k_mux_parent_data[] = {
	{ .hw = &cecb_32k_mux_pre.hw },
	{ .hw = &rtc_clk.hw }
};

MESON_CLK_MUX_RW(cecb_32k_mux, CLKCTRL_CECB_CTRL1, 0x1, 31, NULL, 0,
		 cecb_32k_mux_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(cecb_32k_clkout, CLKCTRL_CECB_CTRL0, 30, 0,
		  &cecb_32k_mux.hw, CLK_SET_RATE_PARENT);

/*cts_sc_clk*/
static const struct clk_parent_data sc_clk_parent_data[] = {
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw },
	{ .fw_name = "xtal", }
};

MESON_CLK_COMPOSITE_RW(sc_clk, CLKCTRL_SC_CLK_CTRL, 0x3, 9,
				NULL, 0, sc_clk_parent_data, 0,
				CLKCTRL_SC_CLK_CTRL, 0, 8, NULL, 0,
				CLK_SET_RATE_PARENT, CLKCTRL_SC_CLK_CTRL, 8,
				0, CLK_SET_RATE_PARENT);

/*12_24M clk*/
static const struct clk_parent_data clk_12_24m_parent_data = {
	 .fw_name = "xtal",
};

__MESON_CLK_GATE(cts_24M_clk, CLKCTRL_CLK12_24_CTRL, 11, 0,
		 &clk_regmap_gate_ops,
		 NULL, &clk_12_24m_parent_data, NULL, 0);

MESON_CLK_DIV_RW(cts_12M_clk, CLKCTRL_CLK12_24_CTRL, 10, 1, NULL, 0,
		 &cts_24M_clk.hw, 0);

/*vclk*/
static u32 vclk_parent_table[] = { 1, 2, 4, 5, 6, 7 };
static const struct clk_parent_data vclk_parent_data[] = {
	{ &gp1_pll.hw },
	{ &hifi_pll.hw },
	{ &fclk_div3.hw },
	{ &fclk_div4.hw },
	{ &fclk_div5.hw },
	{ &fclk_div7.hw }
};

MESON_CLK_MUX_RW(vclk_mux, CLKCTRL_VID_CLK_CTRL, 0x7, 16,
		 vclk_parent_table, 0, vclk_parent_data, 0);

MESON_CLK_GATE_RW(vclk_input, CLKCTRL_VID_CLK_DIV, 16, 0,
		  &vclk_mux.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_DIV_RW(vclk_div, CLKCTRL_VID_CLK_DIV, 0, 8, NULL, 0,
		 &vclk_input.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk, CLKCTRL_VID_CLK_CTRL, 19, 0,
		  &vclk_div.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_GATE_RW(vclk_div1, CLKCTRL_VID_CLK_CTRL, 0, 0,
		  &vclk.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_GATE_RW(vclk_div2_en, CLKCTRL_VID_CLK_CTRL, 1, 0,
		  &vclk.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_FIXED_FACTOR(vclk_div2, 1, 2, &vclk_div2_en.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk_div4_en, CLKCTRL_VID_CLK_CTRL, 2, 0,
		  &vclk.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_FIXED_FACTOR(vclk_div4, 1, 4, &vclk_div4_en.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk_div6_en, CLKCTRL_VID_CLK_CTRL, 3, 0,
		  &vclk.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_FIXED_FACTOR(vclk_div6, 1, 6, &vclk_div6_en.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk_div12_en, CLKCTRL_VID_CLK_CTRL, 4, 0,
		  &vclk.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_FIXED_FACTOR(vclk_div12, 1, 12, &vclk_div12_en.hw, CLK_SET_RATE_PARENT);

/*vclk2*/
MESON_CLK_MUX_RW(vclk2_mux, CLKCTRL_VIID_CLK_CTRL, 0x7, 16,
		 vclk_parent_table, 0, vclk_parent_data, 0);

MESON_CLK_GATE_RW(vclk2_input, CLKCTRL_VIID_CLK_DIV, 16, 0,
		  &vclk2_mux.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_DIV_RW(vclk2_div, CLKCTRL_VIID_CLK_CTRL, 0, 8, NULL, 0,
		 &vclk2_input.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk2, CLKCTRL_VIID_CLK_CTRL, 19, 0,
		  &vclk2_div.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_GATE_RW(vclk2_div1, CLKCTRL_VIID_CLK_CTRL, 0, 0,
		  &vclk2.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_GATE_RW(vclk2_div2_en, CLKCTRL_VIID_CLK_CTRL, 1, 0,
		  &vclk2.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_FIXED_FACTOR(vclk2_div2, 1, 2, &vclk2_div2_en.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk2_div4_en, CLKCTRL_VIID_CLK_CTRL, 2, 0,
		  &vclk2.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_FIXED_FACTOR(vclk2_div4, 1, 4, &vclk2_div4_en.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk2_div6_en, CLKCTRL_VIID_CLK_CTRL, 3, 0,
		  &vclk2.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_FIXED_FACTOR(vclk2_div6, 1, 6, &vclk2_div6_en.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk2_div12_en, CLKCTRL_VIID_CLK_CTRL, 4, 0,
		  &vclk2.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_FIXED_FACTOR(vclk2_div12, 1, 12, &vclk2_div12_en.hw, CLK_SET_RATE_PARENT);

/*video clk*/
static u32 cts_vld_clk_parent_table[] = { 0, 1, 2, 3, 4, 8, 9, 10, 11, 12 };
static const struct clk_parent_data cts_vid_clk_parent_data[] = {
	{ &vclk_div1.hw },
	{ &vclk_div2.hw },
	{ &vclk_div4.hw },
	{ &vclk_div6.hw },
	{ &vclk_div12.hw },
	{ &vclk2_div1.hw },
	{ &vclk2_div2.hw },
	{ &vclk2_div4.hw },
	{ &vclk2_div6.hw },
	{ &vclk2_div12.hw }
};

MESON_CLK_MUX_RW(cts_enci_mux, CLKCTRL_VID_CLK_DIV, 0xf, 28,
		 cts_vld_clk_parent_table, 0, cts_vid_clk_parent_data, 0);

MESON_CLK_GATE_RW(cts_enci, CLKCTRL_VID_CLK_CTRL2, 0, 0,
		  &cts_enci_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(cts_encp_mux, CLKCTRL_VID_CLK_DIV, 0xf, 24,
		 cts_vld_clk_parent_table, 0, cts_vid_clk_parent_data, 0);

MESON_CLK_GATE_RW(cts_encp, CLKCTRL_VID_CLK_CTRL2, 2, 0,
		  &cts_encp_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(cts_encl_mux, CLKCTRL_VIID_CLK_DIV, 0xf, 12,
		 cts_vld_clk_parent_table, 0, cts_vid_clk_parent_data, 0);

MESON_CLK_GATE_RW(cts_encl, CLKCTRL_VID_CLK_CTRL2, 3, 0,
		  &cts_encl_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(cts_vdac_mux, CLKCTRL_VIID_CLK_DIV, 0xf, 28,
		 cts_vld_clk_parent_table, 0, cts_vid_clk_parent_data, 0);

MESON_CLK_GATE_RW(cts_vdac, CLKCTRL_VID_CLK_CTRL2, 4, 0,
		  &cts_vdac_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(hdmi_tx_pixel_mux, CLKCTRL_HDMI_CLK_CTRL, 0xf, 16,
		 cts_vld_clk_parent_table, 0, cts_vid_clk_parent_data, 0);

MESON_CLK_GATE_RW(hdmi_tx_pixel, CLKCTRL_VID_CLK_CTRL2, 5, 0,
		  &hdmi_tx_pixel_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(hdmi_tx_fe_mux, CLKCTRL_HDMI_CLK_CTRL, 0xf, 20,
		 cts_vld_clk_parent_table, 0, cts_vid_clk_parent_data, 0);

MESON_CLK_GATE_RW(hdmi_tx_fe, CLKCTRL_VID_CLK_CTRL2, 9, 0,
		  &hdmi_tx_fe_mux.hw, CLK_SET_RATE_PARENT);

/*lcd_an_clk_ph*/
static const struct clk_parent_data lcd_an_clk_parent_data[] = {
	{ &vclk_div6.hw },
	{ &vclk_div12.hw }
};

MESON_CLK_MUX_RW(lcd_an_clk_ph23_mux, CLKCTRL_VID_CLK_CTRL, 0x1, 13,
		 NULL, 0, lcd_an_clk_parent_data, 0);

MESON_CLK_GATE_RW(lcd_an_clk_ph23, CLKCTRL_VID_CLK_CTRL, 14, 0,
		  &lcd_an_clk_ph23_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(lcd_an_clk_ph2, CLKCTRL_VID_CLK_CTRL2, 7, 0,
		  &lcd_an_clk_ph23.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(lcd_an_clk_ph3, CLKCTRL_VID_CLK_CTRL2, 6, 0,
		  &lcd_an_clk_ph23.hw, CLK_SET_RATE_PARENT);

/*hdmitx_clk*/
static const struct clk_parent_data hdmitx_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw }
};

MESON_CLK_COMPOSITE_RW(hdmitx_sys, CLKCTRL_HDMI_CLK_CTRL, 0x3, 9,
		       NULL, 0, hdmitx_parent_data, 0,
		       CLKCTRL_HDMI_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_HDMI_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(hdmitx_prif, CLKCTRL_HTX_CLK_CTRL0, 0x3, 9,
		       NULL, 0, hdmitx_parent_data, 0,
		       CLKCTRL_HTX_CLK_CTRL0, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_HTX_CLK_CTRL0, 8,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(hdmitx_200m, CLKCTRL_HTX_CLK_CTRL0, 0x3, 25,
		       NULL, 0, hdmitx_parent_data, 0,
		       CLKCTRL_HTX_CLK_CTRL0, 16, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_HTX_CLK_CTRL0, 24,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(hdmitx_aud, CLKCTRL_HTX_CLK_CTRL1, 0x3, 9,
		       NULL, 0, hdmitx_parent_data, 0,
		       CLKCTRL_HTX_CLK_CTRL1, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_HTX_CLK_CTRL1, 8,
		       0, CLK_SET_RATE_PARENT);

/*vid_lock*/
static const struct clk_parent_data vid_lock_clk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &cts_encl.hw },
	{ .hw = &cts_enci.hw },
	{ .hw = &cts_encp.hw }
};

MESON_CLK_COMPOSITE_RW(vid_lock, CLKCTRL_VID_LOCK_CLK_CTRL, 0x3, 8,
		       NULL, 0, vid_lock_clk_parent_data, 0,
		       CLKCTRL_VID_LOCK_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VID_LOCK_CLK_CTRL, 7,
		       0, CLK_SET_RATE_PARENT);

/*eth_clk_rmii*/
MESON_CLK_DIV_RW(eth_rmii_div, CLKCTRL_ETH_CLK_CTRL, 0, 7, NULL, 0,
		 &fclk_div2.hw, 0);

MESON_CLK_GATE_RW(eth_rmii, CLKCTRL_ETH_CLK_CTRL, 8, 0, &eth_rmii_div.hw,
		  CLK_SET_RATE_PARENT);

MESON_CLK_FIXED_FACTOR(eth_125m_div, 1, 8, &fclk_div2.hw, 0);

MESON_CLK_GATE_RW(eth_125m, CLKCTRL_ETH_CLK_CTRL, 7, 0, &eth_125m_div.hw, 0);

/*ts_clk*/
static const struct clk_parent_data ts_clk_parent_data = {
	.fw_name = "xtal",
};

__MESON_CLK_DIV(ts_clk_div, CLKCTRL_TS_CLK_CTRL, 0, 8, NULL, 0, 0, 0,
		&clk_regmap_divider_ops, NULL, &ts_clk_parent_data, NULL, 0);

MESON_CLK_GATE_RW(ts_clk, CLKCTRL_TS_CLK_CTRL, 8, 0,
					&ts_clk_div.hw, CLK_SET_RATE_PARENT);

/*mali_clk*/
/*
 * The MALI IP is clocked by two identical clocks (mali_0 and mali_1)
 * muxed by a glitch-free switch on Meson8b and Meson8m2 and later.
 *
 * CLK_SET_RATE_PARENT is added for mali_0_mux clock
 * 1.gp0 pll only support the 846M, avoid other rate 500/400M from it
 * 2.hifi pll is used for other module, skip it, avoid some rate from it
 */
static const struct clk_parent_data mali_01_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &gp1_pll.hw },
	{ .hw = &hifi_pll.hw },
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw }
};

MESON_CLK_COMPOSITE_RW(mali_0, CLKCTRL_MALI_CLK_CTRL, 0x7, 9,
		       NULL, 0, mali_01_parent_data, 0,
		       CLKCTRL_MALI_CLK_CTRL, 0, 7, NULL, 0, 0,
		       CLKCTRL_MALI_CLK_CTRL, 8, 0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(mali_1, CLKCTRL_MALI_CLK_CTRL, 0x7, 25,
		       NULL, 0, mali_01_parent_data, 0,
		       CLKCTRL_MALI_CLK_CTRL, 16, 7, NULL, 0, 0,
		       CLKCTRL_MALI_CLK_CTRL, 24, 0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data mali_clk_parent_data[] = {
	{ .hw = &mali_0.hw },
	{ .hw = &mali_1.hw }
};

MESON_CLK_MUX_RW(mali, CLKCTRL_MALI_CLK_CTRL, 1, 31, NULL, 0,
		 mali_clk_parent_data, 0);

/* cts_vdec_clk */
static const struct clk_parent_data vdec_clk_parent_data[] = {
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw },
	{ .hw = &hifi_pll.hw },
	{ .hw = &gp1_pll.hw },
	{ .fw_name = "xtal", }
};

MESON_CLK_COMPOSITE_RW(vdec_p0, CLKCTRL_VDEC_CLK_CTRL, 0x7, 9,
		       NULL, 0, vdec_clk_parent_data, 0,
		       CLKCTRL_VDEC_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VDEC_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(vdec_p1, CLKCTRL_VDEC3_CLK_CTRL, 0x7, 9,
		       NULL, 0, vdec_clk_parent_data, 0,
		       CLKCTRL_VDEC3_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VDEC3_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data vdec_mux_parent_data[] = {
	{ .hw = &vdec_p0.hw },
	{ .hw = &vdec_p1.hw }
};

MESON_CLK_MUX_RW(vdec, CLKCTRL_VDEC3_CLK_CTRL, 0x1, 15, NULL, 0,
		 vdec_mux_parent_data, CLK_SET_RATE_PARENT);

/* cts_hevcf_clk */
MESON_CLK_COMPOSITE_RW(hevcf_p0, CLKCTRL_VDEC2_CLK_CTRL, 0x7, 9,
		       NULL, 0, vdec_clk_parent_data, 0,
		       CLKCTRL_VDEC2_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VDEC2_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(hevcf_p1, CLKCTRL_VDEC4_CLK_CTRL, 0x7, 9,
		       NULL, 0, vdec_clk_parent_data, 0,
		       CLKCTRL_VDEC4_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VDEC4_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data hevcf_parent_data[] = {
	{ .hw = &hevcf_p0.hw },
	{ .hw = &hevcf_p1.hw }
};

MESON_CLK_MUX_RW(hevcf, CLKCTRL_VDEC4_CLK_CTRL, 0x1, 15, NULL, 0,
		 hevcf_parent_data, CLK_SET_RATE_PARENT);

/* cts_vpu_clk */
static u32 vpu_parent_mux_table[] = { 0, 1, 2, 3, 4, 6, 7 };

static const struct clk_parent_data vpu_pre_parent_data[] = {
	{ &fclk_div3.hw },
	{ &fclk_div4.hw },
	{ &fclk_div5.hw },
	{ &fclk_div7.hw },
	{ &fclk_div2.hw },
	{ &hifi_pll.hw },
	{ &gp1_pll.hw }
};

MESON_CLK_COMPOSITE_RW(vpu_0, CLKCTRL_VPU_CLK_CTRL, 0x7, 9,
		       vpu_parent_mux_table, 0, vpu_pre_parent_data, 0,
		       CLKCTRL_VPU_CLK_CTRL, 0, 7, NULL,
		       0, CLKCTRL_VPU_CLK_CTRL,
		       CLKCTRL_VPU_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(vpu_1, CLKCTRL_VPU_CLK_CTRL, 0x7, 25,
		       vpu_parent_mux_table, 0, vpu_pre_parent_data, 0,
		       CLKCTRL_VPU_CLK_CTRL, 16, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VPU_CLK_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data vpu_parent_data[] = {
	{ .hw = &vpu_0.hw },
	{ .hw = &vpu_1.hw }
};

MESON_CLK_MUX_RW(vpu, CLKCTRL_VPU_CLK_CTRL, 0x1, 31, NULL, 0,
		 vpu_parent_data, CLK_SET_RATE_PARENT);

/*cts_vpu_clkb*/
static const struct clk_parent_data vpu_clkb_tmp_parent_data[] = {
	{ .hw = &vpu.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw }
};

MESON_CLK_COMPOSITE_RW(vpu_clkb_tmp, CLKCTRL_VPU_CLKB_CTRL, 0x3, 20,
		       NULL, 0, vpu_clkb_tmp_parent_data, 0,
		       CLKCTRL_VPU_CLKB_CTRL, 16, 4, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VPU_CLKB_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_DIV_RW(vpu_clkb_div, CLKCTRL_VPU_CLKB_CTRL, 0, 8, NULL, 0,
		 &vpu_clkb_tmp.hw, 0);

MESON_CLK_GATE_RW(vpu_clkb, CLKCTRL_VPU_CLKB_CTRL, 8, 0,
		  &vpu_clkb_div.hw, CLK_SET_RATE_PARENT);

/* cts_vpu_clkc */
static u32 vpu_clkc_pre_parent_table[] = { 0, 1, 2, 3, 4, 6, 7 };

static const struct clk_parent_data vpu_clkc_pre_parent_data[] = {
	{ &fclk_div4.hw },
	{ &fclk_div3.hw },
	{ &fclk_div5.hw },
	{ &fclk_div7.hw },
	{ &fclk_div2.hw },
	{ &hifi_pll.hw },
	{ &gp1_pll.hw }
};

MESON_CLK_COMPOSITE_RW(vpu_clkc_p0, CLKCTRL_VPU_CLKC_CTRL, 0x7, 9,
		       vpu_clkc_pre_parent_table, 0, vpu_clkc_pre_parent_data, 0,
		       CLKCTRL_VPU_CLKC_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VPU_CLKC_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(vpu_clkc_p1, CLKCTRL_VPU_CLKC_CTRL, 0x7, 25,
		       vpu_clkc_pre_parent_table, 0, vpu_clkc_pre_parent_data, 0,
		       CLKCTRL_VPU_CLKC_CTRL, 16, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VPU_CLKC_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data vpu_clkc_parent_data[] = {
	{ .hw = &vpu_clkc_p0.hw },
	{ .hw = &vpu_clkc_p1.hw }
};

MESON_CLK_MUX_RW(vpu_clkc, CLKCTRL_VPU_CLKC_CTRL, 0x1, 31, NULL, 0,
		 vpu_clkc_parent_data, CLK_SET_RATE_PARENT);

/*cts_vapb_clk*/
static u32 vapb_pre_parent_table[] = { 0, 1, 2, 3, 4, 6, 7 };

static const struct clk_parent_data vapb_pre_parent_data[] = {
	{ &fclk_div4.hw },
	{ &fclk_div3.hw },
	{ &fclk_div5.hw },
	{ &fclk_div7.hw },
	{ &fclk_div2.hw },
	{ &hifi_pll.hw },
	{ &fclk_div2p5.hw }
};

MESON_CLK_COMPOSITE_RW(vapb_0, CLKCTRL_VAPBCLK_CTRL, 0x7, 9,
		       vapb_pre_parent_table, 0, vapb_pre_parent_data, 0,
		       CLKCTRL_VAPBCLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VAPBCLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(vapb_1, CLKCTRL_VAPBCLK_CTRL, 0x7, 25,
		       vapb_pre_parent_table, 0, vapb_pre_parent_data, 0,
		       CLKCTRL_VAPBCLK_CTRL, 16, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VAPBCLK_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data vapb_parent_data[] = {
	{ .hw = &vapb_0.hw },
	{ .hw = &vapb_1.hw }
};

MESON_CLK_MUX_RW(vapb, CLKCTRL_VAPBCLK_CTRL, 0x1, 31, NULL, 0,
		 vapb_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(ge2d, CLKCTRL_VAPBCLK_CTRL, 30, 0,
		  &vapb.hw, CLK_SET_RATE_PARENT);

/* cts_vdin_meas_clk */
static u32 vdin_meas_pre_parent_table[] = { 0, 1, 2, 3 };

static const struct clk_parent_data vdin_meas_pre_parent_data[]  = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw },
};

MESON_CLK_COMPOSITE_RW(vdin_meas, CLKCTRL_VDIN_MEAS_CLK_CTRL, 0x7, 9,
		       vdin_meas_pre_parent_table, 0, vdin_meas_pre_parent_data, 0,
		       CLKCTRL_VDIN_MEAS_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VDIN_MEAS_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

/* s7 cts_emmc_c(nand)_clk*/
static u32 sd_emmc_parent_table[] = { 0, 1, 2, 3, 4, 6, 7 };

static const struct clk_parent_data sd_emmc_clk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &hifi_pll.hw },
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &gp1_pll.hw },
	{ .hw = &gp0_pll.hw }
};

MESON_CLK_COMPOSITE_RW(sd_emmc_c, CLKCTRL_NAND_CLK_CTRL, 0x7, 9,
		       sd_emmc_parent_table, 0, sd_emmc_clk_parent_data, 0,
		       CLKCTRL_NAND_CLK_CTRL, 0, 7, NULL,
		       0, 0,
		       CLKCTRL_NAND_CLK_CTRL, 7,
		       0, 0);

/*cts_sd_emmc_a/b_clk*/
MESON_CLK_COMPOSITE_RW(sd_emmc_a, CLKCTRL_SD_EMMC_CLK_CTRL, 0x7, 9,
		       sd_emmc_parent_table, 0, sd_emmc_clk_parent_data, 0,
		       CLKCTRL_SD_EMMC_CLK_CTRL, 0, 7, NULL,
		       0, 0,
		       CLKCTRL_SD_EMMC_CLK_CTRL, 7,
		       0, 0);

MESON_CLK_COMPOSITE_RW(sd_emmc_b, CLKCTRL_SD_EMMC_CLK_CTRL, 0x7, 25,
		       sd_emmc_parent_table, 0, sd_emmc_clk_parent_data, 0,
		       CLKCTRL_SD_EMMC_CLK_CTRL, 16, 7, NULL,
		       0, 0,
		       CLKCTRL_SD_EMMC_CLK_CTRL, 23,
		       0, 0);

/*cts_cdac_clk*/
static const struct clk_parent_data cdac_parent_data[]  = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div5.hw }
};

MESON_CLK_COMPOSITE_RW(cdac, CLKCTRL_CDAC_CLK_CTRL, 0x3, 16,
		       NULL, 0, cdac_parent_data, 0,
		       CLKCTRL_CDAC_CLK_CTRL, 0, 16, NULL,
		       0, 0,
		       CLKCTRL_CDAC_CLK_CTRL, 23,
		       0, 0);

/*cts_spicc_0_clk*/
static const struct clk_parent_data spicc_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &sys_clk.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw },
	{ .hw = &gp1_pll.hw }
};

MESON_CLK_COMPOSITE_RW(spicc0, CLKCTRL_SPICC_CLK_CTRL, 0x7, 7,
		       NULL, 0, spicc_parent_data, 0,
		       CLKCTRL_SPICC_CLK_CTRL, 0, 6, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_SPICC_CLK_CTRL, 6,
		       0, CLK_SET_RATE_PARENT);

/*cts_pwm_*_clk*/
static u32 pwm_parent_table[] = { 0, 2, 3 };

static const struct clk_parent_data pwm_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div3.hw }
};

MESON_CLK_COMPOSITE_RW(pwm_a, CLKCTRL_PWM_CLK_AB_CTRL, 0x3, 9,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_AB_CTRL, 0, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_AB_CTRL, 8, 0,
		       CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(pwm_b, CLKCTRL_PWM_CLK_AB_CTRL, 0x3, 25,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_AB_CTRL, 16, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_AB_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(pwm_c, CLKCTRL_PWM_CLK_CD_CTRL, 0x3, 9,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_CD_CTRL, 0, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_CD_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(pwm_d, CLKCTRL_PWM_CLK_CD_CTRL, 0x3, 25,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_CD_CTRL, 16, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_CD_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(pwm_e, CLKCTRL_PWM_CLK_EF_CTRL, 0x3, 9,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_EF_CTRL, 0, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_EF_CTRL, 8, 0,
		       CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(pwm_f, CLKCTRL_PWM_CLK_EF_CTRL, 0x3, 25,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_EF_CTRL, 16, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_EF_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(pwm_g, CLKCTRL_PWM_CLK_GH_CTRL, 0x3, 9,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_GH_CTRL, 0, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_GH_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(pwm_h, CLKCTRL_PWM_CLK_GH_CTRL, 0x3, 25,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_GH_CTRL, 16, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_GH_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(pwm_i, CLKCTRL_PWM_CLK_IJ_CTRL, 0x3, 9,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_IJ_CTRL, 0, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_IJ_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

MESON_CLK_COMPOSITE_RW(pwm_j, CLKCTRL_PWM_CLK_IJ_CTRL, 0x3, 25,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_IJ_CTRL, 16, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_IJ_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

/*cts_sar_adc_clk*/
static const struct clk_parent_data sar_adc_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &sys_clk.hw },
};

MESON_CLK_COMPOSITE_RW(saradc, CLKCTRL_SAR_CLK_CTRL0, 0x3, 9,
		       NULL, 0, sar_adc_parent_data, 0,
		       CLKCTRL_SAR_CLK_CTRL0, 0, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_SAR_CLK_CTRL0, 8,
		       0, CLK_SET_RATE_PARENT);

/* gen clk */
static u32 gen_parent_table[] = { 0, 1, 5, 6, 7, 19, 20, 21, 22, 23, 24 };

static const struct clk_parent_data gen_clk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &rtc_clk.hw },
	{ .hw = &gp0_pll.hw },
	{ .hw = &hifi1_pll.hw },
	{ .hw = &hifi_pll.hw },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw }
};

MESON_CLK_COMPOSITE_RW(gen, CLKCTRL_GEN_CLK_CTRL, 0x1f, 12,
		       gen_parent_table, 0, gen_clk_parent_data, 0,
		       CLKCTRL_GEN_CLK_CTRL, 0, 11, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_GEN_CLK_CTRL, 11,
		       0, CLK_SET_RATE_PARENT);

/* sys_clk_gate */
#define MESON_CLK_GATE_SYS_CLK(_name, _reg, _bit)			\
	MESON_CLK_GATE_RW(_name, _reg, _bit, 0, &sys_clk.hw, CLK_IGNORE_UNUSED)

/* axi_clk_gate */
#define MESON_CLK_GATE_AXI_CLK(_name, _reg, _bit)			\
	MESON_CLK_GATE_RW(_name, _reg, _bit, 0, &axi_clk.hw, CLK_IGNORE_UNUSED)

/*CLKCTRL_SYS_CLK_EN0_REG0*/
MESON_CLK_GATE_SYS_CLK(sys_ddr, CLKCTRL_SYS_CLK_EN0_REG0, 0);
MESON_CLK_GATE_SYS_CLK(sys_dos, CLKCTRL_SYS_CLK_EN0_REG0, 1);
MESON_CLK_GATE_SYS_CLK(sys_ethphy, CLKCTRL_SYS_CLK_EN0_REG0, 4);
MESON_CLK_GATE_SYS_CLK(sys_mali, CLKCTRL_SYS_CLK_EN0_REG0, 6);
MESON_CLK_GATE_SYS_CLK(sys_aocpu, CLKCTRL_SYS_CLK_EN0_REG0, 13);
MESON_CLK_GATE_SYS_CLK(sys_aucpu, CLKCTRL_SYS_CLK_EN0_REG0, 14);
MESON_CLK_GATE_SYS_CLK(sys_cec, CLKCTRL_SYS_CLK_EN0_REG0, 16);
MESON_CLK_GATE_SYS_CLK(sys_sdemmc_a, CLKCTRL_SYS_CLK_EN0_REG0, 24);
MESON_CLK_GATE_SYS_CLK(sys_sdemmc_b, CLKCTRL_SYS_CLK_EN0_REG0, 25);
MESON_CLK_GATE_SYS_CLK(sys_sdemmc_c, CLKCTRL_SYS_CLK_EN0_REG0, 26);
MESON_CLK_GATE_SYS_CLK(sys_smartcard, CLKCTRL_SYS_CLK_EN0_REG0, 27);
MESON_CLK_GATE_SYS_CLK(sys_acodec, CLKCTRL_SYS_CLK_EN0_REG0, 28);
MESON_CLK_GATE_SYS_CLK(sys_msr_clk, CLKCTRL_SYS_CLK_EN0_REG0, 30);
MESON_CLK_GATE_SYS_CLK(sys_ir_ctrl, CLKCTRL_SYS_CLK_EN0_REG0, 31);

/*CLKCTRL_SYS_CLK_EN0_REG1*/
MESON_CLK_GATE_SYS_CLK(sys_audio, CLKCTRL_SYS_CLK_EN0_REG1, 0);
MESON_CLK_GATE_SYS_CLK(sys_eth, CLKCTRL_SYS_CLK_EN0_REG1, 3);
MESON_CLK_GATE_SYS_CLK(sys_uart_a, CLKCTRL_SYS_CLK_EN0_REG1, 5);
MESON_CLK_GATE_SYS_CLK(sys_uart_b, CLKCTRL_SYS_CLK_EN0_REG1, 6);
MESON_CLK_GATE_SYS_CLK(sys_ts_pll, CLKCTRL_SYS_CLK_EN0_REG1, 16);
MESON_CLK_GATE_SYS_CLK(sys_g2d, CLKCTRL_SYS_CLK_EN0_REG1, 20);
MESON_CLK_GATE_SYS_CLK(sys_spicc0, CLKCTRL_SYS_CLK_EN0_REG1, 21);
MESON_CLK_GATE_SYS_CLK(sys_usb, CLKCTRL_SYS_CLK_EN0_REG1, 26);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_a, CLKCTRL_SYS_CLK_EN0_REG1, 30);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_b, CLKCTRL_SYS_CLK_EN0_REG1, 31);

/*CLKCTRL_SYS_CLK_EN0_REG2*/
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_c, CLKCTRL_SYS_CLK_EN0_REG2, 0);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_d, CLKCTRL_SYS_CLK_EN0_REG2, 1);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_e, CLKCTRL_SYS_CLK_EN0_REG2, 2);
MESON_CLK_GATE_SYS_CLK(sys_hdmitx_apb, CLKCTRL_SYS_CLK_EN0_REG2, 4);
MESON_CLK_GATE_SYS_CLK(sys_i2c_s_a, CLKCTRL_SYS_CLK_EN0_REG2, 5);
MESON_CLK_GATE_SYS_CLK(sys_hdmi20_aes, CLKCTRL_SYS_CLK_EN0_REG2, 9);
MESON_CLK_GATE_SYS_CLK(sys_mmc_apb, CLKCTRL_SYS_CLK_EN0_REG2, 11);
MESON_CLK_GATE_SYS_CLK(sys_cpu_apb, CLKCTRL_SYS_CLK_EN0_REG2, 19);
MESON_CLK_GATE_SYS_CLK(sys_vpu_intr, CLKCTRL_SYS_CLK_EN0_REG2, 25);
MESON_CLK_GATE_SYS_CLK(sys_sar_adc, CLKCTRL_SYS_CLK_EN0_REG2, 28);
MESON_CLK_GATE_SYS_CLK(sys_pwm_j, CLKCTRL_SYS_CLK_EN0_REG2, 29);
MESON_CLK_GATE_SYS_CLK(sys_gic, CLKCTRL_SYS_CLK_EN0_REG2, 30);
MESON_CLK_GATE_SYS_CLK(sys_pwm_i, CLKCTRL_SYS_CLK_EN0_REG2, 31);

/*CLKCTRL_SYS_CLK_EN0_REG3*/
MESON_CLK_GATE_SYS_CLK(sys_pwm_h, CLKCTRL_SYS_CLK_EN0_REG3, 0);
MESON_CLK_GATE_SYS_CLK(sys_pwm_g, CLKCTRL_SYS_CLK_EN0_REG3, 1);
MESON_CLK_GATE_SYS_CLK(sys_pwm_f, CLKCTRL_SYS_CLK_EN0_REG3, 2);
MESON_CLK_GATE_SYS_CLK(sys_pwm_e, CLKCTRL_SYS_CLK_EN0_REG3, 3);
MESON_CLK_GATE_SYS_CLK(sys_pwm_d, CLKCTRL_SYS_CLK_EN0_REG3, 4);
MESON_CLK_GATE_SYS_CLK(sys_pwm_c, CLKCTRL_SYS_CLK_EN0_REG3, 5);
MESON_CLK_GATE_SYS_CLK(sys_pwm_b, CLKCTRL_SYS_CLK_EN0_REG3, 6);
MESON_CLK_GATE_SYS_CLK(sys_pwm_a, CLKCTRL_SYS_CLK_EN0_REG3, 7);

/*CLKCTRL_AXI_CLK_EN0*/
MESON_CLK_GATE_AXI_CLK(axi_sys_nic, CLKCTRL_AXI_CLK_EN0, 0);
MESON_CLK_GATE_AXI_CLK(axi_sram, CLKCTRL_AXI_CLK_EN0, 0);
MESON_CLK_GATE_AXI_CLK(axi_dev0_mmc, CLKCTRL_AXI_CLK_EN0, 0);

/* Array of all clocks provided by this provider */
static struct clk_hw_onecell_data s7_hw_onecell_data = {
	.hws = {
#ifndef CONFIG_ARM
		[CLKID_FIXED_PLL_DCO]		= &fixed_pll_dco.hw,
#endif
		[CLKID_FIXED_PLL]		    = &fixed_pll.hw,
#ifndef CONFIG_ARM
		[CLKID_SYS0_PLL_DCO]		= &sys_pll_dco.hw,
#endif
		[CLKID_SYS0_PLL]			= &sys_pll.hw,
#ifndef CONFIG_ARM
		[CLKID_SYS1_PLL_DCO]		= &sys1_pll_dco.hw,
#endif
		[CLKID_SYS1_PLL]			= &sys1_pll.hw,
		[CLKID_FCLK_DIV2_DIV]		= &fclk_div2_div.hw,
		[CLKID_FCLK_DIV2]		    = &fclk_div2.hw,
		[CLKID_FCLK_DIV3_DIV]		= &fclk_div3_div.hw,
		[CLKID_FCLK_DIV3]		    = &fclk_div3.hw,
		[CLKID_FCLK_DIV4_DIV]		= &fclk_div4_div.hw,
		[CLKID_FCLK_DIV4]		    = &fclk_div4.hw,
		[CLKID_FCLK_DIV5_DIV]		= &fclk_div5_div.hw,
		[CLKID_FCLK_DIV5]		    = &fclk_div5.hw,
		[CLKID_FCLK_DIV7_DIV]		= &fclk_div7_div.hw,
		[CLKID_FCLK_DIV7]		    = &fclk_div7.hw,
		[CLKID_FCLK_DIV2P5_DIV]		= &fclk_div2p5_div.hw,
		[CLKID_FCLK_DIV2P5]		    = &fclk_div2p5.hw,
		[CLKID_FCLK_CLK50M_DIV]		= &fclk_clk50m_div.hw,
		[CLKID_FCLK_CLK50M]		    = &fclk_clk50m.hw,

#ifndef CONFIG_ARM
		[CLKID_GP0_PLL_DCO]		= &gp0_pll_dco.hw,
#endif
		[CLKID_GP0_PLL]			= &gp0_pll.hw,
#ifndef CONFIG_ARM
		[CLKID_GP1_PLL_DCO]		= &gp1_pll_dco.hw,
#endif
		[CLKID_GP1_PLL]			= &gp1_pll.hw,

		[CLKID_CPU_DYN_CLK]		= &cpu_dyn_clk.hw,
		[CLKID_CPU_CLK]			= &cpu_clk.hw,

		[CLKID_DSU_DYN_CLK]		= &dsu_dyn_clk.hw,
		[CLKID_DSU_CLK]			= &dsu_clk.hw,
		[CLKID_DSU_FINAL_CLK]	= &dsu_final_clk.hw,

#ifndef CONFIG_ARM
		[CLKID_HIFI_PLL_DCO]	= &hifi_pll_dco.hw,
#endif
		[CLKID_HIFI_PLL]		= &hifi_pll.hw,
#ifndef CONFIG_ARM
		[CLKID_HIFI1_PLL_DCO]	= &hifi1_pll_dco.hw,
#endif
		[CLKID_HIFI1_PLL]		= &hifi1_pll.hw,

		[CLKID_RTC_32K_CLKIN]	= &rtc_xtal_clkin.hw,
		[CLKID_RTC_32K_DIV]		= &rtc_32k_div.hw,
		[CLKID_RTC_32K_MUX]		= &rtc_32k_mux.hw,
		[CLKID_RTC_32K]		    = &rtc_32k.hw,
		[CLKID_RTC_CLK]			= &rtc_clk.hw,

		[CLKID_SYS_CLK_B_MUX]		= &sysclk_b_mux.hw,
		[CLKID_SYS_CLK_B_DIV]		= &sysclk_b_div.hw,
		[CLKID_SYS_CLK_B_GATE]		= &sysclk_b.hw,
		[CLKID_SYS_CLK_A_MUX]		= &sysclk_a_mux.hw,
		[CLKID_SYS_CLK_A_DIV]		= &sysclk_a_div.hw,
		[CLKID_SYS_CLK_A_GATE]		= &sysclk_a.hw,
		[CLKID_SYS_CLK]			    = &sys_clk.hw,

		[CLKID_AXI_CLK_B_MUX]		= &axiclk_b_mux.hw,
		[CLKID_AXI_CLK_B_DIV]		= &axiclk_b_div.hw,
		[CLKID_AXI_CLK_B_GATE]		= &axiclk_b.hw,
		[CLKID_AXI_CLK_A_MUX]		= &axiclk_a_mux.hw,
		[CLKID_AXI_CLK_A_DIV]		= &axiclk_a_div.hw,
		[CLKID_AXI_CLK_A_GATE]		= &axiclk_a.hw,
		[CLKID_AXI_CLK]			    = &axi_clk.hw,


		[CLKID_CECB_32K_CLKIN]		= &cecb_xtal_clkin.hw,
		[CLKID_CECB_32K_DIV]		= &cecb_32k_div.hw,
		[CLKID_CECB_32K_MUX_PRE]	= &cecb_32k_mux_pre.hw,
		[CLKID_CECB_32K_MUX]		= &cecb_32k_mux.hw,
		[CLKID_CECB_32K_CLKOUT]		= &cecb_32k_clkout.hw,

		[CLKID_SC_CLK_MUX]		    = &sc_clk_mux.hw,
		[CLKID_SC_CLK_DIV]		    = &sc_clk_div.hw,
		[CLKID_SC_CLK_GATE]		    = &sc_clk.hw,

		[CLKID_24M_CLK_GATE]		= &cts_24M_clk.hw,
		[CLKID_12M_CLK_GATE]		= &cts_12M_clk.hw,

		[CLKID_VCLK_MUX]		    = &vclk_mux.hw,
		[CLKID_VCLK2_MUX]		    = &vclk2_mux.hw,
		[CLKID_VCLK_INPUT]		    = &vclk_input.hw,
		[CLKID_VCLK2_INPUT]		    = &vclk2_input.hw,
		[CLKID_VCLK_DIV]		    = &vclk_div.hw,
		[CLKID_VCLK2_DIV]		    = &vclk2_div.hw,
		[CLKID_VCLK]			    = &vclk.hw,
		[CLKID_VCLK2]			    = &vclk2.hw,
		[CLKID_VCLK_DIV1]		    = &vclk_div1.hw,
		[CLKID_VCLK_DIV2_EN]		= &vclk_div2_en.hw,
		[CLKID_VCLK_DIV4_EN]		= &vclk_div4_en.hw,
		[CLKID_VCLK_DIV6_EN]		= &vclk_div6_en.hw,
		[CLKID_VCLK_DIV12_EN]		= &vclk_div12_en.hw,
		[CLKID_VCLK2_DIV1]		    = &vclk2_div1.hw,
		[CLKID_VCLK2_DIV2_EN]		= &vclk2_div2_en.hw,
		[CLKID_VCLK2_DIV4_EN]		= &vclk2_div4_en.hw,
		[CLKID_VCLK2_DIV6_EN]		= &vclk2_div6_en.hw,
		[CLKID_VCLK2_DIV12_EN]		= &vclk2_div12_en.hw,
		[CLKID_VCLK_DIV2]		    = &vclk_div2.hw,
		[CLKID_VCLK_DIV4]		    = &vclk_div4.hw,
		[CLKID_VCLK_DIV6]		    = &vclk_div6.hw,
		[CLKID_VCLK_DIV12]		    = &vclk_div12.hw,
		[CLKID_VCLK2_DIV2]		    = &vclk2_div2.hw,
		[CLKID_VCLK2_DIV4]		    = &vclk2_div4.hw,
		[CLKID_VCLK2_DIV6]		    = &vclk2_div6.hw,
		[CLKID_VCLK2_DIV12]		    = &vclk2_div12.hw,
		[CLKID_CTS_ENCI_MUX]		= &cts_enci_mux.hw,
		[CLKID_CTS_ENCL_MUX]		= &cts_encl_mux.hw,
		[CLKID_CTS_ENCP_MUX]		= &cts_encp_mux.hw,
		[CLKID_CTS_VDAC_MUX]		= &cts_vdac_mux.hw,
		[CLKID_HDMI_TX_PIXEL_MUX]	= &hdmi_tx_pixel_mux.hw,
		[CLKID_HDMI_TX_FE_MUX]		= &hdmi_tx_fe_mux.hw,
		[CLKID_CTS_ENCI]		    = &cts_enci.hw,
		[CLKID_CTS_ENCL]		    = &cts_encl.hw,
		[CLKID_CTS_ENCP]		    = &cts_encp.hw,
		[CLKID_CTS_VDAC]		    = &cts_vdac.hw,
		[CLKID_HDMI_TX_PIXEL]		= &hdmi_tx_pixel.hw,
		[CLKID_HDMI_TX_FE]			= &hdmi_tx_fe.hw,
		[CLKID_LCD_AN_CLK_PH23_SEL]	= &lcd_an_clk_ph23_mux.hw,
		[CLKID_LCD_AN_CLK_PH23_GATE]	= &lcd_an_clk_ph23.hw,
		[CLKID_LCD_AN_CLK_PH2]		= &lcd_an_clk_ph2.hw,
		[CLKID_LCD_AN_CLK_PH3]		= &lcd_an_clk_ph3.hw,

		[CLKID_HDMITX_SYS_MUX]		= &hdmitx_sys_mux.hw,
		[CLKID_HDMITX_SYS_DIV]		= &hdmitx_sys_div.hw,
		[CLKID_HDMITX_SYS]			= &hdmitx_sys.hw,
		[CLKID_HDMITX_PRIF_MUX]		= &hdmitx_prif_mux.hw,
		[CLKID_HDMITX_PRIF_DIV]		= &hdmitx_prif_div.hw,
		[CLKID_HDMITX_PRIF]			= &hdmitx_prif.hw,
		[CLKID_HDMITX_200M_MUX]		= &hdmitx_200m_mux.hw,
		[CLKID_HDMITX_200M_DIV]		= &hdmitx_200m_div.hw,
		[CLKID_HDMITX_200M]			= &hdmitx_200m.hw,
		[CLKID_HDMITX_AUD_MUX]		= &hdmitx_aud_mux.hw,
		[CLKID_HDMITX_AUD_DIV]		= &hdmitx_aud_div.hw,
		[CLKID_HDMITX_AUD]			= &hdmitx_aud.hw,
		[CLKID_VID_LOCK_MUX]	= &vid_lock_mux.hw,
		[CLKID_VID_LOCK_DIV]	= &vid_lock_div.hw,
		[CLKID_VID_LOCK]		= &vid_lock.hw,
		[CLKID_ETH_RMII_DIV]	= &eth_rmii_div.hw,
		[CLKID_ETH_RMII]		= &eth_rmii.hw,
		[CLKID_ETH_125M_DIV]	= &eth_125m_div.hw,
		[CLKID_ETH_125M]		= &eth_125m.hw,

		[CLKID_TS_CLK_DIV]		= &ts_clk_div.hw,
		[CLKID_TS_CLK_GATE]		= &ts_clk.hw,

		[CLKID_MALI_0_SEL]		= &mali_0_mux.hw,
		[CLKID_MALI_0_DIV]		= &mali_0_div.hw,
		[CLKID_MALI_0]			= &mali_0.hw,
		[CLKID_MALI_1_SEL]		= &mali_1_mux.hw,
		[CLKID_MALI_1_DIV]		= &mali_1_div.hw,
		[CLKID_MALI_1]			= &mali_1.hw,
		[CLKID_MALI_MUX]		= &mali.hw,

		[CLKID_VDEC_P0_MUX]		= &vdec_p0_mux.hw,
		[CLKID_VDEC_P0_DIV]		= &vdec_p0_div.hw,
		[CLKID_VDEC_P0]			= &vdec_p0.hw,
		[CLKID_VDEC_P1_MUX]		= &vdec_p1_mux.hw,
		[CLKID_VDEC_P1_DIV]		= &vdec_p1_div.hw,
		[CLKID_VDEC_P1]			= &vdec_p1.hw,
		[CLKID_VDEC_MUX]		= &vdec.hw,

		[CLKID_HEVCF_P0_MUX]	= &hevcf_p0_mux.hw,
		[CLKID_HEVCF_P0_DIV]	= &hevcf_p0_div.hw,
		[CLKID_HEVCF_P0]		= &hevcf_p0.hw,
		[CLKID_HEVCF_P1_MUX]	= &hevcf_p1_mux.hw,
		[CLKID_HEVCF_P1_DIV]	= &hevcf_p1_div.hw,
		[CLKID_HEVCF_P1]		= &hevcf_p1.hw,
		[CLKID_HEVCF_MUX]		= &hevcf.hw,

		[CLKID_VPU_0_MUX]		= &vpu_0_mux.hw,
		[CLKID_VPU_0_DIV]		= &vpu_0_div.hw,
		[CLKID_VPU_0]			= &vpu_0.hw,
		[CLKID_VPU_1_MUX]		= &vpu_1_mux.hw,
		[CLKID_VPU_1_DIV]		= &vpu_1_div.hw,
		[CLKID_VPU_1]			= &vpu_1.hw,
		[CLKID_VPU]			    = &vpu.hw,

		[CLKID_VPU_CLKB_TMP_MUX]	= &vpu_clkb_tmp_mux.hw,
		[CLKID_VPU_CLKB_TMP_DIV]	= &vpu_clkb_tmp_div.hw,
		[CLKID_VPU_CLKB_TMP]		= &vpu_clkb_tmp.hw,
		[CLKID_VPU_CLKB_DIV]		= &vpu_clkb_div.hw,
		[CLKID_VPU_CLKB]		    = &vpu_clkb.hw,
		[CLKID_VPU_CLKC_P0_MUX]		= &vpu_clkc_p0_mux.hw,
		[CLKID_VPU_CLKC_P0_DIV]		= &vpu_clkc_p0_div.hw,
		[CLKID_VPU_CLKC_P0]		    = &vpu_clkc_p0.hw,
		[CLKID_VPU_CLKC_P1_MUX]		= &vpu_clkc_p1_mux.hw,
		[CLKID_VPU_CLKC_P1_DIV]		= &vpu_clkc_p1_div.hw,
		[CLKID_VPU_CLKC_P1]		    = &vpu_clkc_p1.hw,
		[CLKID_VPU_CLKC_MUX]		= &vpu_clkc.hw,

		[CLKID_VAPB_0_MUX]		= &vapb_0_mux.hw,
		[CLKID_VAPB_0_DIV]		= &vapb_0_div.hw,
		[CLKID_VAPB_0]			= &vapb_0.hw,
		[CLKID_VAPB_1_MUX]		= &vapb_1_mux.hw,
		[CLKID_VAPB_1_DIV]		= &vapb_1_div.hw,
		[CLKID_VAPB_1]			= &vapb_1.hw,
		[CLKID_VAPB]			= &vapb.hw,

		[CLKID_GE2D]			= &ge2d.hw,

		[CLKID_VDIN_MEAS_MUX]		= &vdin_meas_mux.hw,
		[CLKID_VDIN_MEAS_DIV]		= &vdin_meas_div.hw,
		[CLKID_VDIN_MEAS_GATE]		= &vdin_meas.hw,

		[CLKID_SD_EMMC_C_CLK_MUX]	= &sd_emmc_c_mux.hw,
		[CLKID_SD_EMMC_C_CLK_DIV]	= &sd_emmc_c_div.hw,
		[CLKID_SD_EMMC_C_CLK]		= &sd_emmc_c.hw,
		[CLKID_SD_EMMC_A_CLK_MUX]	= &sd_emmc_a_mux.hw,
		[CLKID_SD_EMMC_A_CLK_DIV]	= &sd_emmc_a_div.hw,
		[CLKID_SD_EMMC_A_CLK]		= &sd_emmc_a.hw,
		[CLKID_SD_EMMC_B_CLK_MUX]	= &sd_emmc_b_mux.hw,
		[CLKID_SD_EMMC_B_CLK_DIV]	= &sd_emmc_b_div.hw,
		[CLKID_SD_EMMC_B_CLK]		= &sd_emmc_b.hw,

		[CLKID_CDAC_MUX]		= &cdac_mux.hw,
		[CLKID_CDAC_DIV]		= &cdac_div.hw,
		[CLKID_CDAC]		    = &cdac.hw,

		[CLKID_SPICC0_MUX]		= &spicc0_mux.hw,
		[CLKID_SPICC0_DIV]		= &spicc0_div.hw,
		[CLKID_SPICC0_GATE]		= &spicc0.hw,

		[CLKID_PWM_A_MUX]		= &pwm_a_mux.hw,
		[CLKID_PWM_A_DIV]		= &pwm_a_div.hw,
		[CLKID_PWM_A_GATE]		= &pwm_a.hw,
		[CLKID_PWM_B_MUX]		= &pwm_b_mux.hw,
		[CLKID_PWM_B_DIV]		= &pwm_b_div.hw,
		[CLKID_PWM_B_GATE]		= &pwm_b.hw,
		[CLKID_PWM_C_MUX]		= &pwm_c_mux.hw,
		[CLKID_PWM_C_DIV]		= &pwm_c_div.hw,
		[CLKID_PWM_C_GATE]		= &pwm_c.hw,
		[CLKID_PWM_D_MUX]		= &pwm_d_mux.hw,
		[CLKID_PWM_D_DIV]		= &pwm_d_div.hw,
		[CLKID_PWM_D_GATE]		= &pwm_d.hw,
		[CLKID_PWM_E_MUX]		= &pwm_e_mux.hw,
		[CLKID_PWM_E_DIV]		= &pwm_e_div.hw,
		[CLKID_PWM_E_GATE]		= &pwm_e.hw,
		[CLKID_PWM_F_MUX]		= &pwm_f_mux.hw,
		[CLKID_PWM_F_DIV]		= &pwm_f_div.hw,
		[CLKID_PWM_F_GATE]		= &pwm_f.hw,
		[CLKID_PWM_G_MUX]		= &pwm_g_mux.hw,
		[CLKID_PWM_G_DIV]		= &pwm_g_div.hw,
		[CLKID_PWM_G_GATE]		= &pwm_g.hw,
		[CLKID_PWM_H_MUX]		= &pwm_h_mux.hw,
		[CLKID_PWM_H_DIV]		= &pwm_h_div.hw,
		[CLKID_PWM_H_GATE]		= &pwm_h.hw,
		[CLKID_PWM_I_MUX]		= &pwm_i_mux.hw,
		[CLKID_PWM_I_DIV]		= &pwm_i_div.hw,
		[CLKID_PWM_I_GATE]		= &pwm_i.hw,
		[CLKID_PWM_J_MUX]		= &pwm_j_mux.hw,
		[CLKID_PWM_J_DIV]		= &pwm_j_div.hw,
		[CLKID_PWM_J_GATE]		= &pwm_j.hw,

		[CLKID_SARADC_MUX]		= &saradc_mux.hw,
		[CLKID_SARADC_DIV]		= &saradc_div.hw,
		[CLKID_SARADC_GATE]		= &saradc.hw,

		[CLKID_GEN_MUX]			= &gen_mux.hw,
		[CLKID_GEN_DIV]			= &gen_div.hw,
		[CLKID_GEN_GATE]		= &gen.hw,

		[CLKID_DDR]			    = &sys_ddr.hw,
		[CLKID_DOS]			    = &sys_dos.hw,
		[CLKID_ETHPHY]			= &sys_ethphy.hw,
		[CLKID_MALI_GATE]		= &sys_mali.hw,
		[CLKID_AOCPU]			= &sys_aocpu.hw,
		[CLKID_AUCPU]			= &sys_aucpu.hw,
		[CLKID_CEC]			    = &sys_cec.hw,
		[CLKID_SD_EMMC_A]		= &sys_sdemmc_a.hw,
		[CLKID_SD_EMMC_B]		= &sys_sdemmc_b.hw,
		[CLKID_SD_EMMC_C]		= &sys_sdemmc_c.hw,
		[CLKID_SMARTCARD]		= &sys_smartcard.hw,
		[CLKID_ACODEC]			= &sys_acodec.hw,
		[CLKID_MSR_CLK]			= &sys_msr_clk.hw,
		[CLKID_IR_CTRL]			= &sys_ir_ctrl.hw,

		[CLKID_AUDIO]			= &sys_audio.hw,
		[CLKID_ETH]			    = &sys_eth.hw,
		[CLKID_UART_A]			= &sys_uart_a.hw,
		[CLKID_UART_B]			= &sys_uart_b.hw,
		[CLKID_TS_PLL]			= &sys_ts_pll.hw,
		[CLKID_G2D]			    = &sys_g2d.hw,
		[CLKID_SPICC0]			= &sys_spicc0.hw,
		[CLKID_USB]			    = &sys_usb.hw,
		[CLKID_I2C_M_A]			= &sys_i2c_m_a.hw,
		[CLKID_I2C_M_B]			= &sys_i2c_m_b.hw,
		[CLKID_I2C_M_C]			= &sys_i2c_m_c.hw,
		[CLKID_I2C_M_D]			= &sys_i2c_m_d.hw,
		[CLKID_I2C_M_E]			= &sys_i2c_m_e.hw,
		[CLKID_HDMITX_APB]		= &sys_hdmitx_apb.hw,
		[CLKID_I2C_S_A]			= &sys_i2c_s_a.hw,
		[CLKID_HDMI20_AES]		= &sys_hdmi20_aes.hw,
		[CLKID_MMC_APB]			= &sys_mmc_apb.hw,
		[CLKID_CPU_APB]			= &sys_cpu_apb.hw,
		[CLKID_VPU_INTR]		= &sys_vpu_intr.hw,
		[CLKID_SAR_ADC]			= &sys_sar_adc.hw,
		[CLKID_PWM_J]			= &sys_pwm_j.hw,
		[CLKID_GIC]				= &sys_gic.hw,
		[CLKID_PWM_I]			= &sys_pwm_i.hw,
		[CLKID_PWM_H]			= &sys_pwm_h.hw,
		[CLKID_PWM_G]			= &sys_pwm_g.hw,
		[CLKID_PWM_F]			= &sys_pwm_f.hw,
		[CLKID_PWM_E]			= &sys_pwm_e.hw,
		[CLKID_PWM_D]			= &sys_pwm_d.hw,
		[CLKID_PWM_C]			= &sys_pwm_c.hw,
		[CLKID_PWM_B]			= &sys_pwm_b.hw,
		[CLKID_PWM_A]			= &sys_pwm_a.hw,

		[CLKID_SYS_NIC]			= &axi_sys_nic.hw,
		[CLKID_SRAM]			= &axi_sram.hw,
		[CLKID_DEV0_MMC]		= &axi_dev0_mmc.hw,

		[NR_CLKS]			= NULL
	},
	.num = NR_CLKS,
};

/* Convenience table to populate regmap in .probe */
static struct clk_regmap *const s7_clk_regmaps[] = {
	&rtc_xtal_clkin,
	&rtc_32k_div,
	&rtc_32k_mux,
	&rtc_32k,
	&rtc_clk,

	&sysclk_b_mux,
	&sysclk_b_div,
	&sysclk_b,
	&sysclk_a_mux,
	&sysclk_a_div,
	&sysclk_a,
	&sys_clk,

	&axiclk_b_mux,
	&axiclk_b_div,
	&axiclk_b,
	&axiclk_a_mux,
	&axiclk_a_div,
	&axiclk_a,
	&axi_clk,

	&cecb_xtal_clkin,
	&cecb_32k_div,
	&cecb_32k_mux_pre,
	&cecb_32k_mux,
	&cecb_32k_clkout,

	&sc_clk_mux,
	&sc_clk_div,
	&sc_clk,

	&cts_24M_clk,
	&cts_12M_clk,
	&vclk_mux,
	&vclk2_mux,
	&vclk_input,
	&vclk2_input,
	&vclk_div,
	&vclk2_div,
	&vclk,
	&vclk2,
	&vclk_div1,
	&vclk_div2_en,
	&vclk_div4_en,
	&vclk_div6_en,
	&vclk_div12_en,
	&vclk2_div1,
	&vclk2_div2_en,
	&vclk2_div4_en,
	&vclk2_div6_en,
	&vclk2_div12_en,
	&cts_enci_mux,
	&cts_encl_mux,
	&cts_encp_mux,
	&cts_vdac_mux,
	&hdmi_tx_pixel_mux,
	&hdmi_tx_fe_mux,
	&cts_enci,
	&cts_encl,
	&cts_encp,
	&cts_vdac,
	&hdmi_tx_pixel,
	&hdmi_tx_fe,

	&lcd_an_clk_ph23_mux,
	&lcd_an_clk_ph23,
	&lcd_an_clk_ph2,
	&lcd_an_clk_ph3,

	&hdmitx_sys_mux,
	&hdmitx_sys_div,
	&hdmitx_sys,
	&hdmitx_prif_mux,
	&hdmitx_prif_div,
	&hdmitx_prif,
	&hdmitx_200m_mux,
	&hdmitx_200m_div,
	&hdmitx_200m,
	&hdmitx_aud_mux,
	&hdmitx_aud_div,
	&hdmitx_aud,

	&vid_lock_mux,
	&vid_lock_div,
	&vid_lock,
	&eth_rmii_div,
	&eth_rmii,
	&eth_125m,
	&ts_clk_div,
	&ts_clk,

	&mali_0_mux,
	&mali_0_div,
	&mali_0,
	&mali_1_mux,
	&mali_1_div,
	&mali_1,
	&mali,

	&vdec_p0_mux,
	&vdec_p0_div,
	&vdec_p0,
	&vdec_p1_mux,
	&vdec_p1_div,
	&vdec_p1,
	&vdec,

	&hevcf_p0_mux,
	&hevcf_p0_div,
	&hevcf_p0,
	&hevcf_p1_mux,
	&hevcf_p1_div,
	&hevcf_p1,
	&hevcf,

	&vpu_0_mux,
	&vpu_0_div,
	&vpu_0,
	&vpu_1_mux,
	&vpu_1_div,
	&vpu_1,
	&vpu,
	&vpu_clkb_tmp_mux,
	&vpu_clkb_tmp_div,
	&vpu_clkb_tmp,
	&vpu_clkb_div,
	&vpu_clkb,
	&vpu_clkc_p0_mux,
	&vpu_clkc_p0_div,
	&vpu_clkc_p0,
	&vpu_clkc_p1_mux,
	&vpu_clkc_p1_div,
	&vpu_clkc_p1,
	&vpu_clkc,

	&vapb_0_mux,
	&vapb_0_div,
	&vapb_0,
	&vapb_1_mux,
	&vapb_1_div,
	&vapb_1,
	&vapb,

	&ge2d,

	&vdin_meas_mux,
	&vdin_meas_div,
	&vdin_meas,

	&sd_emmc_c_mux,
	&sd_emmc_c_div,
	&sd_emmc_c,
	&sd_emmc_a_mux,
	&sd_emmc_a_div,
	&sd_emmc_a,
	&sd_emmc_b_mux,
	&sd_emmc_b_div,
	&sd_emmc_b,

	&cdac_mux,
	&cdac_div,
	&cdac,

	&spicc0_mux,
	&spicc0_div,
	&spicc0,

	&pwm_a_mux,
	&pwm_a_div,
	&pwm_a,
	&pwm_b_mux,
	&pwm_b_div,
	&pwm_b,
	&pwm_c_mux,
	&pwm_c_div,
	&pwm_c,
	&pwm_d_mux,
	&pwm_d_div,
	&pwm_d,
	&pwm_e_mux,
	&pwm_e_div,
	&pwm_e,
	&pwm_f_mux,
	&pwm_f_div,
	&pwm_f,
	&pwm_g_mux,
	&pwm_g_div,
	&pwm_g,
	&pwm_h_mux,
	&pwm_h_div,
	&pwm_h,
	&pwm_i_mux,
	&pwm_i_div,
	&pwm_i,
	&pwm_j_mux,
	&pwm_j_div,
	&pwm_j,

	&saradc_mux,
	&saradc_div,
	&saradc,

	&gen_mux,
	&gen_div,
	&gen,

	&sys_ddr,
	&sys_dos,
	&sys_ethphy,
	&sys_mali,
	&sys_aocpu,
	&sys_aucpu,
	&sys_cec,
	&sys_sdemmc_a,
	&sys_sdemmc_b,
	&sys_sdemmc_c,
	&sys_smartcard,
	&sys_acodec,
	&sys_msr_clk,
	&sys_ir_ctrl,
	&sys_audio,
	&sys_eth,
	&sys_uart_a,
	&sys_uart_b,
	&sys_ts_pll,
	&sys_g2d,
	&sys_spicc0,
	&sys_usb,
	&sys_i2c_m_a,
	&sys_i2c_m_b,
	&sys_i2c_m_c,
	&sys_i2c_m_d,
	&sys_i2c_m_e,
	&sys_hdmitx_apb,
	&sys_i2c_s_a,
	&sys_hdmi20_aes,
	&sys_mmc_apb,
	&sys_cpu_apb,
	&sys_vpu_intr,
	&sys_sar_adc,
	&sys_pwm_j,
	&sys_gic,
	&sys_pwm_i,
	&sys_pwm_h,
	&sys_pwm_g,
	&sys_pwm_f,
	&sys_pwm_e,
	&sys_pwm_d,
	&sys_pwm_c,
	&sys_pwm_b,
	&sys_pwm_a,
	&axi_sys_nic,
	&axi_sram,
	&axi_dev0_mmc,
};

static struct clk_regmap *const s7_cpu_clk_regmaps[] = {
	&cpu_dyn_clk,
	&cpu_clk,

	&dsu_dyn_clk,
	&dsu_clk,
	&dsu_final_clk
};

static struct clk_regmap *const s7_pll_clk_regmaps[] = {
#ifndef CONFIG_ARM
	&fixed_pll_dco,
#endif
	&fixed_pll,
#ifndef CONFIG_ARM
	&sys_pll_dco,
#endif
	&sys_pll,
#ifndef CONFIG_ARM
	&sys1_pll_dco,
#endif
	&sys1_pll,
	&fclk_div2,
	&fclk_div3,
	&fclk_div4,
	&fclk_div5,
	&fclk_div7,
	&fclk_div2p5,
	&fclk_clk50m,
#ifndef CONFIG_ARM
	&gp0_pll_dco,
#endif
	&gp0_pll,
#ifndef CONFIG_ARM
	&gp1_pll_dco,
#endif
	&gp1_pll,

#ifndef CONFIG_ARM
	&hifi_pll_dco,
#endif
	&hifi_pll,
#ifndef CONFIG_ARM
	&hifi1_pll_dco,
#endif
	&hifi1_pll
};

static int meson_s7_dvfs_setup(struct platform_device *pdev)
{
	int ret;

	/*for pxp*/

	/* Setup clock notifier for sys_pll */
	ret = clk_notifier_register(sys_pll.hw.clk,
				    &s7_sys_pll_nb_data.nb);
	if (ret) {
		dev_err(&pdev->dev, "failed to register sys_pll notifier\n");
		return ret;
	}

	return 0;
}

static struct regmap_config clkc_regmap_config = {
	.reg_bits       = 32,
	.val_bits       = 32,
	.reg_stride     = 4,
};

static struct regmap *s7_regmap_resource(struct device *dev, char *name)
{
	struct resource res;
	void __iomem *base;
	int i;
	struct device_node *node = dev->of_node;

	i = of_property_match_string(node, "reg-names", name);
	if (of_address_to_resource(node, i, &res))
		return ERR_PTR(-ENOENT);

	base = devm_ioremap_resource(dev, &res);
	if (IS_ERR(base))
		return ERR_CAST(base);

	clkc_regmap_config.max_register = resource_size(&res) - 4;
	clkc_regmap_config.name = devm_kasprintf(dev, GFP_KERNEL,
						 "%s-%s", node->name,
						 name);
	if (!clkc_regmap_config.name)
		return ERR_PTR(-ENOMEM);

	return devm_regmap_init_mmio(dev, base, &clkc_regmap_config);
}

static int meson_s7_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *basic_map;
	struct regmap *pll_map;
	struct regmap *cpu_clk_map;
	const struct clk_hw_onecell_data *hw_onecell_data;
	struct clk *clk;
	int ret, i;

	hw_onecell_data = of_device_get_match_data(&pdev->dev);
	if (!hw_onecell_data)
		return -EINVAL;
	pr_info("");
	clk = devm_clk_get(dev, "xtal");
	if (IS_ERR(clk)) {
		pr_err("%s: clock source xtal not found\n", dev_name(&pdev->dev));
		return PTR_ERR(clk);
	}

#ifdef CONFIG_AMLOGIC_CLK_DEBUG
		ret = devm_clk_hw_register_clkdev(dev, __clk_get_hw(clk),
						  NULL,
						  __clk_get_name(clk));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
#endif

	basic_map = s7_regmap_resource(dev, "basic");
	if (IS_ERR(basic_map)) {
		dev_err(dev, "basic clk registers not found\n");
		return PTR_ERR(basic_map);
	}

	pll_map = s7_regmap_resource(dev, "pll");
	if (IS_ERR(pll_map)) {
		dev_err(dev, "pll clk registers not found\n");
		return PTR_ERR(pll_map);
	}

	cpu_clk_map = s7_regmap_resource(dev, "cpu_clk");
	if (IS_ERR(cpu_clk_map)) {
		dev_err(dev, "cpu clk registers not found\n");
		return PTR_ERR(cpu_clk_map);
	}

	/* Populate regmap for the regmap backed clocks */
	for (i = 0; i < ARRAY_SIZE(s7_clk_regmaps); i++)
		s7_clk_regmaps[i]->map = basic_map;

	for (i = 0; i < ARRAY_SIZE(s7_cpu_clk_regmaps); i++)
		s7_cpu_clk_regmaps[i]->map = cpu_clk_map;

	for (i = 0; i < ARRAY_SIZE(s7_pll_clk_regmaps); i++)
		s7_pll_clk_regmaps[i]->map = pll_map;

	for (i = 0; i < hw_onecell_data->num; i++) {
		/* array might be sparse */
		if (!hw_onecell_data->hws[i])
			continue;

		//dev_err(dev, "register %d  %s\n", i,
		//		hw_onecell_data->hws[i]->init->name);

		ret = devm_clk_hw_register(dev, hw_onecell_data->hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}

#ifdef CONFIG_AMLOGIC_CLK_DEBUG
		ret = devm_clk_hw_register_clkdev(dev, hw_onecell_data->hws[i],
						  NULL,
						  clk_hw_get_name(hw_onecell_data->hws[i]));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
#endif
	}

	meson_s7_dvfs_setup(pdev);

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   (void *)hw_onecell_data);
}

static const struct of_device_id clkc_match_table[] = {
	{
		.compatible = "amlogic,s7-clkc",
		.data = &s7_hw_onecell_data
	},
	{}
};

static struct platform_driver s7_driver = {
	.probe		= meson_s7_probe,
	.driver		= {
		.name	= "s7-clkc",
		.of_match_table = clkc_match_table,
	},
};

#ifndef CONFIG_AMLOGIC_MODIFY
builtin_platform_driver(s7_driver);
#else
#ifndef MODULE
static int __init s7_clkc_init(void)
{
	return platform_driver_register(&s7_driver);
}
arch_initcall_sync(s7_clkc_init);
#else
int __init meson_s7_clkc_init(void)
{
	return platform_driver_register(&s7_driver);
}
module_init(meson_s7_clkc_init);
#endif
#endif

MODULE_LICENSE("GPL v2");
