// SPDX-License-Identifier: GPL-2.0

#define LOG_TAG "jd9365_guoxian_video"

#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/fs.h>

#include "lcm_drv.h"

#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)

static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define SET_GPIO_LCD_ENP_BIAS(v)	(lcm_util.set_gpio_lcd_enp_bias((v)))
#define MDELAY(n)	(lcm_util.mdelay(n))
#define UDELAY(n)	(lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) \
	lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define FRAME_WIDTH		(720)
#define FRAME_HEIGHT	(1520)
#define LCM_DENSITY	(320)

#define REGFLAG_DELAY	0xFFFC
#define REGFLAG_UDELAY	0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table init_setting_vdo[] = {
    {0xE0, 1, {0x00}},
    {0xE1, 1, {0x93}},
    {0xE2, 1, {0x65}},
    {0xE3, 1, {0xF8}},
    {0x80, 1, {0x03}},
    {0xE0, 1, {0x01}},
    {0x00, 1, {0x00}},
    {0x01, 1, {0x47}},
    {0x03, 1, {0x00}},
    {0x04, 1, {0x48}},
    {0x17, 1, {0x00}},
    {0x18, 1, {0xCD}},
    {0x19, 1, {0x00}},
    {0x1A, 1, {0x00}},
    {0x1B, 1, {0xCD}},
    {0x1C, 1, {0x00}},
    {0x23, 1, {0x02}},
    {0x24, 1, {0xFE}},
    {0x25, 1, {0x20}},
    {0x27, 1, {0x22}},
    {0x32, 1, {0x23}},
    {0x37, 1, {0x09}},
    {0x38, 1, {0x04}},
    {0x39, 1, {0x08}},
    {0x3A, 1, {0x12}},
    {0x3C, 1, {0x64}},
    {0x3D, 1, {0xFF}},
    {0x3E, 1, {0xFF}},
    {0x3F, 1, {0x64}},
    {0x40, 1, {0x04}},
    {0x41, 1, {0xBE}},
    {0x42, 1, {0x6B}},
    {0x43, 1, {0x12}},
    {0x44, 1, {0x0F}},
    {0x45, 1, {0x28}},
    {0x55, 1, {0x0F}},
    {0x56, 1, {0x01}},
    {0x57, 1, {0x65}},
    {0x58, 1, {0x0A}},
    {0x59, 1, {0x0A}},
    {0x5A, 1, {0x28}},
    {0x5B, 1, {0x10}},
    {0x5D, 1, {0x4C}},
    {0x5E, 1, {0x36}},
    {0x5F, 1, {0x2A}},
    {0x60, 1, {0x20}},
    {0x61, 1, {0x20}},
    {0x62, 1, {0x14}},
    {0x63, 1, {0x1D}},
    {0x64, 1, {0x0B}},
    {0x65, 1, {0x28}},
    {0x66, 1, {0x2A}},
    {0x67, 1, {0x2D}},
    {0x68, 1, {0x4D}},
    {0x69, 1, {0x3D}},
    {0x6A, 1, {0x47}},
    {0x6B, 1, {0x39}},
    {0x6C, 1, {0x39}},
    {0x6D, 1, {0x2E}},
    {0x6E, 1, {0x22}},
    {0x6F, 1, {0x09}},
    {0x70, 1, {0x4C}},
    {0x71, 1, {0x36}},
    {0x72, 1, {0x2A}},
    {0x73, 1, {0x20}},
    {0x74, 1, {0x20}},
    {0x75, 1, {0x14}},
    {0x76, 1, {0x1D}},
    {0x77, 1, {0x0B}},
    {0x78, 1, {0x28}},
    {0x79, 1, {0x2A}},
    {0x7A, 1, {0x2D}},
    {0x7B, 1, {0x4D}},
    {0x7C, 1, {0x3D}},
    {0x7D, 1, {0x47}},
    {0x7E, 1, {0x39}},
    {0x7F, 1, {0x39}},
    {0x80, 1, {0x2E}},
    {0x81, 1, {0x22}},
    {0x82, 1, {0x09}},
    {0xE0, 1, {0x02}},
    {0x00, 1, {0x5E}},
    {0x01, 1, {0x5F}},
    {0x02, 1, {0x57}},
    {0x03, 1, {0x58}},
    {0x04, 1, {0x44}},
    {0x05, 1, {0x46}},
    {0x06, 1, {0x48}},
    {0x07, 1, {0x4A}},
    {0x08, 1, {0x40}},
    {0x09, 1, {0x1D}},
    {0x0A, 1, {0x1D}},
    {0x0B, 1, {0x1D}},
    {0x0C, 1, {0x1D}},
    {0x0D, 1, {0x1D}},
    {0x0E, 1, {0x1D}},
    {0x0F, 1, {0x50}},
    {0x10, 1, {0x5F}},
    {0x11, 1, {0x5F}},
    {0x12, 1, {0x5F}},
    {0x13, 1, {0x5F}},
    {0x14, 1, {0x5F}},
    {0x15, 1, {0x5F}},
    {0x16, 1, {0x5E}},
    {0x17, 1, {0x5F}},
    {0x18, 1, {0x57}},
    {0x19, 1, {0x58}},
    {0x1A, 1, {0x45}},
    {0x1B, 1, {0x47}},
    {0x1C, 1, {0x49}},
    {0x1D, 1, {0x4B}},
    {0x1E, 1, {0x41}},
    {0x1F, 1, {0x1D}},
    {0x20, 1, {0x1D}},
    {0x21, 1, {0x1D}},
    {0x22, 1, {0x1D}},
    {0x23, 1, {0x1D}},
    {0x24, 1, {0x1D}},
    {0x25, 1, {0x51}},
    {0x26, 1, {0x5F}},
    {0x27, 1, {0x5F}},
    {0x28, 1, {0x5F}},
    {0x29, 1, {0x5F}},
    {0x2A, 1, {0x5F}},
    {0x2B, 1, {0x5F}},
    {0x2C, 1, {0x1F}},
    {0x2D, 1, {0x1E}},
    {0x2E, 1, {0x17}},
    {0x2F, 1, {0x18}},
    {0x30, 1, {0x0B}},
    {0x31, 1, {0x09}},
    {0x32, 1, {0x07}},
    {0x33, 1, {0x05}},
    {0x34, 1, {0x11}},
    {0x35, 1, {0x1F}},
    {0x36, 1, {0x1F}},
    {0x37, 1, {0x1F}},
    {0x38, 1, {0x1F}},
    {0x39, 1, {0x1F}},
    {0x3A, 1, {0x1F}},
    {0x3B, 1, {0x01}},
    {0x3C, 1, {0x1F}},
    {0x3D, 1, {0x1F}},
    {0x3E, 1, {0x1F}},
    {0x3F, 1, {0x1F}},
    {0x40, 1, {0x1F}},
    {0x41, 1, {0x1F}},
    {0x42, 1, {0x1F}},
    {0x43, 1, {0x1E}},
    {0x44, 1, {0x17}},
    {0x45, 1, {0x18}},
    {0x46, 1, {0x0A}},
    {0x47, 1, {0x08}},
    {0x48, 1, {0x06}},
    {0x49, 1, {0x04}},
    {0x4A, 1, {0x10}},
    {0x4B, 1, {0x1F}},
    {0x4C, 1, {0x1F}},
    {0x4D, 1, {0x1F}},
    {0x4E, 1, {0x1F}},
    {0x4F, 1, {0x1F}},
    {0x50, 1, {0x1F}},
    {0x51, 1, {0x00}},
    {0x52, 1, {0x1F}},
    {0x53, 1, {0x1F}},
    {0x54, 1, {0x1F}},
    {0x55, 1, {0x1F}},
    {0x56, 1, {0x1F}},
    {0x57, 1, {0x1F}},
    {0x58, 1, {0x40}},
    {0x59, 1, {0x00}},
    {0x5A, 1, {0x00}},
    {0x5B, 1, {0x10}},
    {0x5C, 1, {0x0B}},
    {0x5D, 1, {0x30}},
    {0x5E, 1, {0x01}},
    {0x5F, 1, {0x02}},
    {0x60, 1, {0x30}},
    {0x61, 1, {0x03}},
    {0x62, 1, {0x04}},
    {0x63, 1, {0x1C}},
    {0x64, 1, {0x52}},
    {0x65, 1, {0x56}},
    {0x66, 1, {0x00}},
    {0x67, 1, {0x73}},
    {0x68, 1, {0x0D}},
    {0x69, 1, {0x0D}},
    {0x6A, 1, {0x52}},
    {0x6B, 1, {0x00}},
    {0x6C, 1, {0x00}},
    {0x6D, 1, {0x00}},
    {0x6E, 1, {0x00}},
    {0x6F, 1, {0x88}},
    {0x70, 1, {0x00}},
    {0x71, 1, {0x00}},
    {0x72, 1, {0x06}},
    {0x73, 1, {0x7B}},
    {0x74, 1, {0x00}},
    {0x75, 1, {0xBC}},
    {0x76, 1, {0x00}},
    {0x77, 1, {0x0E}},
    {0x78, 1, {0x11}},
    {0x79, 1, {0x00}},
    {0x7A, 1, {0x00}},
    {0x7B, 1, {0x00}},
    {0x7C, 1, {0x00}},
    {0x7D, 1, {0x03}},
    {0x7E, 1, {0x7B}},
    {0xE0, 1, {0x04}},
    {0x09, 1, {0x11}},
    {0x0E, 1, {0x4A}},
    {0xE0, 1, {0x00}},
    {0x35, 1, {0x00}},
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 5, {}},
    {REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table suspend_setting[] = {
    {REGFLAG_DELAY, 5, {}},
    {0x28, 0, {}},
    {REGFLAG_DELAY, 50, {}},
    {0x10, 0, {}},
    {REGFLAG_DELAY, 120, {}},
};

static void push_table(void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned int cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {
		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				UDELAY(table[i].count * 1000);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count,
				table[i].para_list, force_update);
		}
	}
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->density = LCM_DENSITY;

	params->dsi.mode = SYNC_PULSE_VDO_MODE;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;

	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* video mode timing */
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active = 4;
	params->dsi.vertical_backporch = 12;
	params->dsi.vertical_frontporch = 18;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 20;
	params->dsi.horizontal_backporch = 80;
	params->dsi.horizontal_frontporch = 80;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.ssc_disable = 0;

	params->dsi.PLL_CLOCK = 260;

	params->dsi.customization_esd_check_enable = 1;
	params->dsi.esd_check_enable = 1;

	params->dsi.lcm_esd_check_table[0].cmd = 0xa;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x1c;
}

static void lcm_init(void)
{
	LCM_LOGD("%s +\n", __func__);

	SET_GPIO_LCD_ENP_BIAS(1);
	MDELAY(15);
	SET_RESET_PIN(1);
	MDELAY(2);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(15);

	push_table(NULL, init_setting_vdo, ARRAY_SIZE(init_setting_vdo), true);

	LCM_LOGD("%s -\n", __func__);
}

static void lcm_suspend(void)
{
	LCM_LOGD("%s +\n", __func__);

	push_table(NULL, suspend_setting, ARRAY_SIZE(suspend_setting), true);

  	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(120);
	SET_GPIO_LCD_ENP_BIAS(0);

	LCM_LOGD("%s -\n", __func__);
}

static void lcm_resume(void)
{
	LCM_LOGD("%s +\n", __func__);
	lcm_init();
	LCM_LOGD("%s -\n", __func__);
}

struct LCM_DRIVER jd9365_guoxian_video_lcm_drv = {
	.name = "JD9365_GUOXIAN",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
