// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/amlogic/gki_module.h>
#include "hdmitx_boot_parameters.h"
#include <hdmitx_log.h>

static struct hdmitx_boot_param tx_params = {
	.fraction_refreshrate = 1,
	.edid_chksum = "invalidcrc",
};

struct hdmitx_boot_param *get_hdmitx_boot_params(void)
{
	return &tx_params;
}

/* besides characters defined in separator, '\"' are used as separator;
 * and any characters in '\"' will not act as separator
 */
static char *next_token_ex(char *separator, char *buf, unsigned int size,
			   unsigned int offset, unsigned int *token_len,
			   unsigned int *token_offset)
{
	char *token = NULL;
	char last_separator = 0;
	char trans_char_flag = 0;

	if (buf) {
		for (; offset < size; offset++) {
			int ii = 0;
		char ch;

		if (buf[offset] == '\\') {
			trans_char_flag = 1;
			continue;
		}
		while (((ch = separator[ii++]) != buf[offset]) && (ch))
			;
		if (ch) {
			if (!token) {
				continue;
		} else {
			if (last_separator != '"') {
				*token_len = (unsigned int)
					(buf + offset - token);
				*token_offset = offset;
				return token;
			}
		}
		} else if (!token) {
			if (trans_char_flag && (buf[offset] == '"'))
				last_separator = buf[offset];
			token = &buf[offset];
		} else if ((trans_char_flag && (buf[offset] == '"')) &&
			   (last_separator == '"')) {
			*token_len = (unsigned int)(buf + offset - token - 2);
			*token_offset = offset + 1;
			return token + 1;
		}
		trans_char_flag = 0;
	}
	if (token) {
		*token_len = (unsigned int)(buf + offset - token);
		*token_offset = offset;
	}
	}
	return token;
}

/* check the colorattribute from uboot */
static int get_hdmitx_color_attr(char *token, char *color_attr)
{
	char attr[16] = {0};
	const char * const cs[] = {
		"444", "422", "rgb", "420", NULL};
	const char * const cd[] = {
		"8bit", "10bit", "12bit", "16bit", NULL};
	int i;
	int ret = -1;

	if (!token)
		return -1;

	for (i = 0; cs[i]; i++) {
		if (strstr(token, cs[i])) {
			if (strlen(cs[i]) < sizeof(attr))
				strcpy(attr, cs[i]);
			strcat(attr, ",");
			break;
		}
	}
	for (i = 0; cd[i]; i++) {
		if (strstr(token, cd[i])) {
			if (strlen(cd[i]) < sizeof(attr))
				if (strlen(cd[i]) <
					(sizeof(attr) - strlen(attr)))
					strcat(attr, cd[i]);

			if (strlen(attr) >= sizeof(attr)) {
				HDMITX_ERROR("get err attr: %zu-%s\n", strlen(attr), attr);
			} else {
				strncpy(color_attr, attr, strlen(attr));
				ret = 0;
			}
			break;
		}
	}

	return ret;
}

static int parse_hdmitx_boot_para(char *s)
{
	char separator[] = {' ', ',', ';', 0x0};
	char *token;
	unsigned int token_len = 0;
	unsigned int token_offset = 0;
	unsigned int offset = 0;
	int size = 0;

	if (!s) {
		memset(tx_params.color_attr, 0, sizeof(tx_params.color_attr));
		tx_params.init_state = 0;
		return -EINVAL;
	}

	size = strlen(s);
	memset(tx_params.color_attr, 0, sizeof(tx_params.color_attr));
	tx_params.init_state = 0;

	do {
		token = next_token_ex(separator, s, size, offset,
				      &token_len, &token_offset);
		if (token) {
			if (token_len == 3 &&
			    strncmp(token, "off", token_len) == 0) {
				tx_params.init_state |= INIT_FLAG_NOT_LOAD;
			}

			if (tx_params.color_attr[0] == 0)
				get_hdmitx_color_attr(token, tx_params.color_attr);
		}
		offset = token_offset;
	} while (token);

	pr_debug("hdmitx_param:[color_attr]=[%s]\n",
		tx_params.color_attr);
	pr_debug("hdmitx_param:[init_state]=[%x]\n",
		tx_params.init_state);

	return 0;
}
__setup("hdmitx=",    parse_hdmitx_boot_para);

static int parse_hdmitx_fraction_rate(char *str)
{
	if (!str) {
		tx_params.fraction_refreshrate = 1;
		return -EINVAL;
	}

	if (strncmp("0", str, 1) == 0)
		tx_params.fraction_refreshrate = 0;
	else
		tx_params.fraction_refreshrate = 1;

	pr_debug("hdmitx_param:[fraction_rate]=[%d]\n",
		tx_params.fraction_refreshrate);

	return 0;
}
__setup("frac_rate_policy=", parse_hdmitx_fraction_rate);

static int parse_hdmitx_hdr_priority(char *str)
{
	int err;
	u32 value;

	if (!str) {
		tx_params.hdr_mask = 0;
		return -EINVAL;
	}

	err = kstrtou32(str, 10, &value);
	if (err) {
		HDMITX_ERROR("%s fail\n", __func__);
		tx_params.hdr_mask = 0;
		return err;
	}

	tx_params.hdr_mask = value;

	pr_debug("hdmitx_param:[hdr_priority]=[%d]\n",
		tx_params.hdr_mask);
	return 0;
}
__setup("hdr_priority=", parse_hdmitx_hdr_priority);

static int parse_hdmitx_checksum(char *str)
{
	if (!str) {
		pr_err("hdmitx_param:[checksum]=[%s]\n", tx_params.edid_chksum);
		return -EINVAL;
	}

	snprintf(tx_params.edid_chksum, sizeof(tx_params.edid_chksum), "%s", str);
	pr_debug("hdmitx_param:[checksum]=[%s]\n", tx_params.edid_chksum);

	return 0;
}
__setup("hdmichecksum=", parse_hdmitx_checksum);

static int hdmitx_config_csc_en(char *str)
{
	if (!str) {
		tx_params.config_csc = false;
		return -EINVAL;
	}

	if (strncmp("1", str, 1) == 0)
		tx_params.config_csc = true;
	else
		tx_params.config_csc = false;
	pr_debug("config_csc_en:[config_csc_en]=[%d]\n", tx_params.config_csc);
	return 0;
}
__setup("config_csc_en=", hdmitx_config_csc_en);

static int hdmitx_boot_edid_check(char *str)
{
	int val = 0;

	if (!str) {
		tx_params.edid_check = 0;
		return -EINVAL;
	}

	if ((strncmp("0", str, 1) == 0) || (strncmp("1", str, 1) == 0) ||
		(strncmp("2", str, 1) == 0) || (strncmp("3", str, 1) == 0)) {
		val = str[0] - '0';
		tx_params.edid_check = val;
		pr_debug("hdmitx_param:[edid_check]=[%d]\n", val);
	}

	return 0;
}
__setup("edid_check=", hdmitx_boot_edid_check);

static int hdmitx_boot_dsc_policy(char *str)
{
	unsigned int val = 0;

	if (!str) {
		tx_params.dsc_policy = 0;
		return -EINVAL;
	}

	if ((strncmp("0", str, 1) == 0) ||
		(strncmp("1", str, 1) == 0) ||
		(strncmp("2", str, 1) == 0) ||
		(strncmp("3", str, 1) == 0) ||
		(strncmp("4", str, 1) == 0)) {
		val = str[0] - '0';
		tx_params.dsc_policy = val;
		pr_debug("hdmitx boot dsc_policy: %d\n", val);
	} else {
		/* default policy */
		tx_params.dsc_policy = 0;
	}
	return 0;
}
__setup("dsc_policy=", hdmitx_boot_dsc_policy);

