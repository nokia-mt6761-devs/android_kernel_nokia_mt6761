// SPDX-License-Identifier: GPL-2.0

#define LOG_TAG "st7703_helitai_video"

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
#include "lcm_bias.h"

#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)

static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define SET_LCD_BIAS_VSPN(en, seq, value)	(lcm_util.set_lcd_bias_vspn(en, seq, value))
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
    {0xB9, 3, {0xF1, 0x12, 0x83}},
    {0xBA, 27, {0x33, 0x81, 0x05, 0xF9, 0x0E, 0x0E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x25, 0x00, 0x90, 0x0A, 0x00, 0x00, 0x00, 0x4F, 0xD1, 0xFF, 0x7F, 0x37}},
    {0xB8, 1, {0x75}},
    {0xBF, 3, {0x02, 0x11, 0x00}},
    {0xB3, 10, {0x07, 0x0B, 0x28, 0x28, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00}},
    {0xC0, 9, {0x73, 0x73, 0x50, 0x50, 0x00, 0x00, 0x08, 0x50, 0x00}},
    {0xBC, 1, {0x46}},
    {0xCC, 1, {0x0A}},
    {0xB4, 1, {0x80}},
    {0xB1, 1, {0x85}},
    {0xB2, 3, {0x04, 0x12, 0xF0}},
    {0xE3, 14, {0x07, 0x07, 0x0B, 0x0B, 0x03, 0x0B, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x80, 0xC0, 0x17}},
    {0xC1, 12, {0x54, 0x00, 0x1E, 0x1E, 0x99, 0xF1, 0xFF, 0xFF, 0xEE, 0xEE, 0x77, 0x77}},
    {0xC6, 6, {0x25, 0x00, 0xFF, 0xDF, 0x20, 0x20}},
    {0xB5, 2, {0x09, 0x09}},
    {0xB6, 2, {0x60, 0x60}},
    {0xE9, 63, {0x02, 0x00, 0x07, 0x05, 0xFD, 0x80, 0x81, 0x12, 0x31, 0x23, 0x77, 0x0B, 0x80, 0x81, 0x47, 0x18, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x4F, 0x88, 0x64, 0x20, 0x08, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x5F, 0x88, 0x75, 0x31, 0x18, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {0xEA, 61, {0x00, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xF8, 0x13, 0x57, 0x58, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x08, 0xF8, 0x02, 0x46, 0x48, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x23, 0x14, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x80, 0x81, 0x00, 0x00, 0x00, 0x00}},
    {0xE0, 34, {0x00, 0x09, 0x0D, 0x2B, 0x2B, 0x3F, 0x3E, 0x31, 0x07, 0x0C, 0x0D, 0x11, 0x12, 0x10, 0x13, 0x16, 0x1C, 0x00, 0x09, 0x0D, 0x2B, 0x2B, 0x3F, 0x3E, 0x31, 0x07, 0x0C, 0x0D, 0x11, 0x12, 0x10, 0x13, 0x16, 0x1C}},
    {0xEF, 3, {0xFF, 0xFF, 0x01}},
    {0xC7, 1, {0x10}},
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 5, {}},
    {REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table suspend_setting[] = {
	{0x28, 0, {}},
	{REGFLAG_DELAY, 20, {}},
	{0x10, 0, {}},
	{REGFLAG_DELAY, 200, {}},
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
	params->dsi.vertical_sync_active = 3;
	params->dsi.vertical_backporch = 10;
	params->dsi.vertical_frontporch = 16;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 50;
	params->dsi.horizontal_frontporch = 50;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	params->dsi.ssc_disable = 0;

	params->dsi.PLL_CLOCK = 241;

	params->dsi.customization_esd_check_enable = 0;
	params->dsi.esd_check_enable = 1;

	params->dsi.lcm_esd_check_table[0].cmd = 0x68;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0xc0;

	params->dsi.lcm_esd_check_table[1].cmd = 0x9;
	params->dsi.lcm_esd_check_table[1].count = 3;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[1].para_list[1] = 0x73;
	params->dsi.lcm_esd_check_table[1].para_list[2] = 0x4;

	params->dsi.lcm_esd_check_table[2].cmd = 0xaf;
	params->dsi.lcm_esd_check_table[2].count = 1;
	params->dsi.lcm_esd_check_table[2].para_list[0] = 0xfd;
}

static void lcm_init(void)
{
	LCM_LOGD("%s +\n", __func__);

	SET_RESET_PIN(1);
  	MDELAY(10);
  	SET_RESET_PIN(0);
  	MDELAY(1);
  	SET_RESET_PIN(1);
	MDELAY(120);
	SET_LCD_BIAS_VSPN(ON, VSP_FIRST_VSN_AFTER, 5500);
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
	SET_LCD_BIAS_VSPN(OFF, VSN_FIRST_VSP_AFTER, 5500);
  	SET_RESET_PIN(0);
  	MDELAY(10);

	LCM_LOGD("%s -\n", __func__);
}

static void lcm_resume(void)
{
	LCM_LOGD("%s +\n", __func__);
	lcm_init();
	LCM_LOGD("%s -\n", __func__);
}

struct LCM_DRIVER st7703_helitai_video_lcm_drv = {
	.name = "ST7703_HELITAI",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
