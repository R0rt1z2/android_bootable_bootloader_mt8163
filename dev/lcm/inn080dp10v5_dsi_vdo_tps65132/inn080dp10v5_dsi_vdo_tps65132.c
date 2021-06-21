#include "lcm_drv.h"

#define LCD_DEBUG(fmt)  printf(fmt)
#define TPS65132_DEVICE
#define LCD_CONTROL_PIN

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
//#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (800)
#define FRAME_HEIGHT (1280)

#define REGFLAG_DELAY								0xFE
#define REGFLAG_END_OF_TABLE						0xFF   // END OF REGISTERS MARKER

#define GPIO_OUT_ONE	1
#define GPIO_OUT_ZERO	0
#define GPIO_RST_PIN	83
#define GPIO_PWR_PIN	84

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static LCM_UTIL_FUNCS lcm_util = {
	.set_gpio_out = NULL,
};

#define SET_RESET_PIN(v)					(lcm_util.set_reset_pin((v)))

#define UDELAY(n)							(lcm_util.udelay(n))
#define MDELAY(n)							(lcm_util.mdelay(n))


struct LCM_setting_table {
	unsigned cmd;
	unsigned char count;
	unsigned char para_list[64];
};

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)			lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)		lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)			lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)						lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)					lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define TPS65132_SLAVE_ADDR_WRITE  0x7C
static struct mt_i2c_t TPS65132_i2c;

static int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0] = addr;
	write_data[1] = value;

	TPS65132_i2c.id = 0x3E; /* I2C2; */
	/* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
	TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
	TPS65132_i2c.mode = ST_MODE;
	TPS65132_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&TPS65132_i2c, write_data, len);
	/* printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code); */

	return ret_code;
}

/*****************************************************************************
 * lcm info
 *****************************************************************************/

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
}

static struct LCM_setting_table lcm_initialization_setting[] =
{
	//======Internal setting======
	{0xFF, 4, {0xAA, 0x55, 0xA5, 0x80}},

	//=MIPI ralated timing setting======
	{0x6F, 2, {0x11, 0x00}},
	{0xF7, 2, {0x20, 0x00}},
	//=Improve ESD option======
	{0x6F, 1, {0x06}},
	{0xF7, 1, {0xA0}},
	{0x6F, 1, {0x19}},
	{0xF7, 1, {0x12}},
	{0xF4, 1, {0x03}},

	//=Vcom floating======
	{0x6F, 1, {0x08}},
	{0xFA, 1, {0x40}},
	{0x6F, 1, {0x11}},
	{0xF3, 1, {0x01}},

	//=Page0 relative======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x00}},
	{0xC8, 1, {0x80}},

	//=Set WXGA resolution 0x6C======
	/*
	Tt parameter of below command,
	ol value is 0x07,
	s0x01 to rotate 180 degress,
	ter option value is 0x03 & 0x05.
	*/
	{0xB1, 2, {0x6C, 0x01}},

	//=Set source output hold time======
	{0xB6, 1, {0x08}},

	//=EQ control function======
	{0x6F, 1, {0x02}},
	{0xB8, 1, {0x08}},

	//=Set bias current for GOP and SOP======
	{0xBB, 2, {0x74, 0x44}},

	//=Inversion setting======
	{0xBC, 2, {0x00, 0x00}},

	//=DSP timing Settings update for BIST======
	{0xBD, 5, {0x02, 0xB0, 0x0C, 0x0A, 0x00}},

	//=Page1 relative======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x01}},

	//=Setting AVDD, AVEE clamp======
	{0xB0, 2, {0x05, 0x05}},
	{0xB1, 2, {0x05, 0x05}},

	//=VGMP, VGMN, VGSP, VGSN setting======
	{0xBC, 2, {0x90, 0x01}},
	{0xBD, 2, {0x90, 0x01}},

	//=Gate signal control======
	{0xCA, 1, {0x00}},

	//=Power IC control======
	{0xC0, 1, {0x04}},

	//=VCOM -1.88V======
	{0xBE, 1, {0x29}},

	//=Setting VGH=15V, VGL=-11V======
	{0xB3, 2, {0x37, 0x37}},
	{0xB4, 2, {0x19, 0x19}},

	//=Power control for VGH, VGL======
	{0xB9, 2, {0x44, 0x44}},
	{0xBA, 2, {0x24, 0x24}},

	//=Page2 relative======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x02}},

	//=Gamma control register control======
	{0xEE, 1, {0x01}},
	//=Gradient control for Gamma voltage======
	{0xEF, 4, {0x09, 0x06, 0x15, 0x18}},

	{0xB0, 6, {0x00, 0x00, 0x00, 0x25, 0x00, 0x43}},
	{0x6F, 1, {0x06}},
	{0xB0, 6, {0x00, 0x54, 0x00, 0x68, 0x00, 0xA0}},
	{0x6F, 1, {0x0C}},
	{0xB0, 4, {0x00, 0xC0, 0x01, 0x00}},
	{0xB1, 6, {0x01, 0x30, 0x01, 0x78, 0x01, 0xAE}},
	{0x6F, 1, {0x06}},
	{0xB1, 6, {0x02, 0x08, 0x02, 0x52, 0x02, 0x54}},
	{0x6F, 1, {0x0C}},
	{0xB1, 4, {0x02, 0x99, 0x02, 0xF0}},
	{0xB2, 6, {0x03, 0x20, 0x03, 0x56, 0x03, 0x76}},
	{0x6F, 1, {0x06}},
	{0xB2, 6, {0x03, 0x93, 0x03, 0xA4, 0x03, 0xB9}},
	{0x6F, 1, {0x0C}},
	{0xB2, 4, {0x03, 0xC9, 0x03, 0xE3}},
	{0xB3, 4, {0x03, 0xFC, 0x03, 0xFF}},

	//=GOA relative======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x06}},
	{0xB0, 2, {0x00, 0x10}},
	{0xB1, 2, {0x12, 0x14}},
	{0xB2, 2, {0x16, 0x18}},
	{0xB3, 2, {0x1A, 0x29}},
	{0xB4, 2, {0x2A, 0x08}},
	{0xB5, 2, {0x31, 0x31}},
	{0xB6, 2, {0x31, 0x31}},
	{0xB7, 2, {0x31, 0x31}},
	{0xB8, 2, {0x31, 0x0A}},
	{0xB9, 2, {0x31, 0x31}},
	{0xBA, 2, {0x31, 0x31}},
	{0xBB, 2, {0x0B, 0x31}},
	{0xBC, 2, {0x31, 0x31}},
	{0xBD, 2, {0x31, 0x31}},
	{0xBE, 2, {0x31, 0x31}},
	{0xBF, 2, {0x09, 0x2A}},
	{0xC0, 2, {0x29, 0x1B}},
	{0xC1, 2, {0x19, 0x17}},
	{0xC2, 2, {0x15, 0x13}},
	{0xC3, 2, {0x11, 0x01}},
	{0xE5, 2, {0x31, 0x31}},
	{0xC4, 2, {0x09, 0x1B}},
	{0xC5, 2, {0x19, 0x17}},
	{0xC6, 2, {0x15, 0x13}},
	{0xC7, 2, {0x11, 0x29}},
	{0xC8, 2, {0x2A, 0x01}},
	{0xC9, 2, {0x31, 0x31}},
	{0xCA, 2, {0x31, 0x31}},
	{0xCB, 2, {0x31, 0x31}},
	{0xCC, 2, {0x31, 0x0B}},
	{0xCD, 2, {0x31, 0x31}},
	{0xCE, 2, {0x31, 0x31}},
	{0xCF, 2, {0x0A, 0x31}},
	{0xD0, 2, {0x31, 0x31}},
	{0xD1, 2, {0x31, 0x31}},
	{0xD2, 2, {0x31, 0x31}},
	{0xD3, 2, {0x00, 0x2A}},
	{0xD4, 2, {0x29, 0x10}},
	{0xD5, 2, {0x12, 0x14}},
	{0xD6, 2, {0x16, 0x18}},
	{0xD7, 2, {0x1A, 0x08}},
	{0xE6, 2, {0x31, 0x31}},
	{0xD8, 5, {0x00, 0x00, 0x00, 0x54, 0x00}},
	{0xD9, 5, {0x00, 0x15, 0x00, 0x00, 0x00}},
	{0xE7, 1, {0x00}},

	//=Page3, gate timing control======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x03}},
	{0xB0, 2, {0x20, 0x00}},
	{0xB1, 2, {0x20, 0x00}},
	{0xB2, 5, {0x05, 0x00, 0x00, 0x00, 0x00}},

	{0xB6, 5, {0x05, 0x00, 0x00, 0x00, 0x00}},
	{0xB7, 5, {0x05, 0x00, 0x00, 0x00, 0x00}},

	{0xBA, 5, {0x57, 0x00, 0x00, 0x00, 0x00}},
	{0xBB, 5, {0x57, 0x00, 0x00, 0x00, 0x00}},

	{0xC0, 4, {0x00, 0x00, 0x00, 0x00}},
	{0xC1, 4, {0x00, 0x00, 0x00, 0x00}},

	{0xC4, 1, {0x60}},
	{0xC5, 1, {0x40}},

	//=Page5======
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x05}},
	{0xBD, 5, {0x03, 0x01, 0x03, 0x03, 0x03}},
	{0xB0, 2, {0x17, 0x06}},
	{0xB1, 2, {0x17, 0x06}},
	{0xB2, 2, {0x17, 0x06}},
	{0xB3, 2, {0x17, 0x06}},
	{0xB4, 2, {0x17, 0x06}},
	{0xB5, 2, {0x17, 0x06}},

	{0xB8, 1, {0x00}},
	{0xB9, 1, {0x00}},
	{0xBA, 1, {0x00}},
	{0xBB, 1, {0x02}},
	{0xBC, 1, {0x00}},

	{0xC0, 1, {0x07}},

	{0xC4, 1, {0x80}},
	{0xC5, 1, {0xA4}},

	{0xC8, 2, {0x05, 0x30}},
	{0xC9, 2, {0x01, 0x31}},

	{0xCC, 3, {0x00, 0x00, 0x3C}},
	{0xCD, 3, {0x00, 0x00, 0x3C}},

	{0xD1, 5, {0x00, 0x04, 0xFD, 0x07, 0x10}},
	{0xD2, 5, {0x00, 0x05, 0x02, 0x07, 0x10}},

	{0xE5, 1, {0x06}},
	{0xE6, 1, {0x06}},
	{0xE7, 1, {0x06}},
	{0xE8, 1, {0x06}},
	{0xE9, 1, {0x06}},
	{0xEA, 1, {0x06}},

	{0xED, 1, {0x30}},

	//=Reload setting======
	{0x6F, 1, {0x11}},
	{0xF3, 1, {0x01}},

	//=Normal Display======
	{0x35, 0, {}},
	{0x11, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 0, {}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
	for(i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {
		case REGFLAG_DELAY :
		MDELAY(table[i].count);
		break;

		case REGFLAG_END_OF_TABLE :
		break;

		default:
		dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type		= LCM_TYPE_DSI;
	params->width 	= FRAME_WIDTH;
	params->height 	= FRAME_HEIGHT;
	params->dsi.mode	= BURST_VDO_MODE;

	params->physical_width		= 108;
	params->physical_height	= 172;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	params->dsi.data_format.format			= LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	//params->dsi.packet_size=256;

	// Video mode setting

	//params->dsi.intermediat_buffer_num = 2;
	//params->dsi.word_count=FRAME_WIDTH*3;

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;//LCM_PACKED_PS_18BIT_RGB666;

	params->dsi.vertical_sync_active				= 5;
	params->dsi.vertical_backporch				= 3;
	params->dsi.vertical_frontporch 				= 8;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active			= 5;
	params->dsi.horizontal_backporch			= 59;
	params->dsi.horizontal_frontporch			= 16;
	params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;


	// Bit rate calculation
	params->dsi.PLL_CLOCK = 234;
	//params->dsi.cont_clock = 1;
}

#ifdef LCD_CONTROL_PIN
static void lcm_reset(unsigned char enabled)
{
	if(enabled)
	{
		lcm_set_gpio_output(GPIO_RST_PIN, GPIO_OUT_ONE);
		MDELAY(1);
		lcm_set_gpio_output(GPIO_RST_PIN, !GPIO_OUT_ONE);
		MDELAY(15);
		lcm_set_gpio_output(GPIO_RST_PIN, GPIO_OUT_ONE);
	}else{
		lcm_set_gpio_output(GPIO_RST_PIN, !GPIO_OUT_ONE);
		MDELAY(1);
	}
}

static void lcm_suspend_power(void)
{
	lcm_set_gpio_output(GPIO_PWR_PIN, GPIO_OUT_ZERO);
}

static void lcm_resume_power(void)
{
#ifdef TPS65132_DEVICE
#pragma message("nigger")
	unsigned char cmd = 0x0;
	unsigned char data = 0x0A;
 	int ret=0;
#endif

	lcm_set_gpio_output(GPIO_PWR_PIN, GPIO_OUT_ONE);
	MDELAY(10);

#ifdef TPS65132_DEVICE
	cmd=0x00;
	data=0x12;//DAC -5.8V

	ret=TPS65132_write_byte(cmd,data);
	if(ret<0)
		printf("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printf("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write success-----\n",cmd);

	cmd=0x01;
	data=0x12;//DAC 5.8V

	ret=TPS65132_write_byte(cmd,data);
	if(ret<0)
		printf("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printf("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write success-----\n",cmd);

	cmd=0x03;
	data=0x43;/*Set APPLICATION in Tablet mode (default is 0x03 ,smartphone mode)*/

	ret=TPS65132_write_byte(cmd,data);
	if(ret<0)
		printf("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printf("[KERNEL]inn080dp10v5----tps65132---cmd=%0x-- i2c write success-----\n",cmd);
#endif
}

#endif

static void init_lcm_registers(void)
{
	printf(" %s, kernel\n", __func__);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_init_lcm(void)
{
	printf("[Kernel/LCM] lcm_init() enter\n");
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];

	printf("[Kernel/LCM] lcm_suspend() enter\n");

	data_array[0] = 0x00280500;  //display off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	lcm_reset(0);
	lcm_suspend_power();
	// lcm_vgp_supply_disable();
	MDELAY(1);
}


static void lcm_resume(void)
{
	printf("[Kernel/LCM] lcm_resume() enter\n");

	lcm_set_gpio_output(GPIO_PWR_PIN, GPIO_OUT_ZERO);

	// lcm_vgp_supply_enable();
	MDELAY(5);

	//power on avdd and avee
	lcm_resume_power();

	MDELAY(50);
	lcm_reset(1);
	MDELAY(10);
	lcm_reset(0);
	MDELAY(50);
	lcm_reset(1);
	MDELAY(100);//Must > 120ms

	init_lcm_registers();
}

LCM_DRIVER inn080dp10v5_dsi_vdo_tps65132_lcm_drv =
{
	.name			= "inn080dp10v5_dsi_vdo_tps65132_lcm_drv",
	.set_util_funcs		= lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init			= lcm_init_lcm,
	.suspend		= lcm_suspend,
	.resume			= lcm_resume,
};
