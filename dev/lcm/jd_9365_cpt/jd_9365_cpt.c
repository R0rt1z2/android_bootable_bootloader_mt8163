
#ifdef BUILD_LK
#include <string.h>
#include <mt_gpio.h>
#else
#include <linux/string.h>
#if defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#endif
#endif
#include "lcm_drv.h"
#include <cust_gpio_usage.h>


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)

#define REGFLAG_DELAY             							0XFFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
	
	/*
	Note :

	Data ID will depends on the following rule.
	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/

	//{REGFLAG_DELAY, 200, {}},
	
	{0xE0, 1,{0x00}},
	
	{0xE1, 1,{0x93}},
	
	{0xE2, 1,{0x65}},
	
	{0xE3, 1,{0xF8}},
	

	{0xE0, 1,{0x04}},
	
	{0x2D, 1,{0x03}},
	
	{0xE0, 1,{0x00}},
	
	//{0x80, 1,{0x01}},//03-4lane 02-3lane
		
	{0x70, 1,{0x10}},
	
	{0x71, 1,{0x13}},
	
	{0x72, 1,{0x06}},
	
	{0x75, 1,{0x03}},
	

	{0xE0, 1,{0x01}},
	
	
	{0x00, 1,{0x00}},
	
	{0x01, 1,{0xA0}},
	
	{0x03, 1,{0x00}},
	
	{0x04, 1,{0xA0}},
	
	
	{0x0A, 1,{0x07}},
	
	{0x0C, 1,{0x74}},
	
	
	{0x17, 1,{0x00}},
	
	{0x18, 1,{0xC7}},
	
	{0x19, 1,{0x00}},
	
	{0x1A, 1,{0x00}},
	
	{0x1B, 1,{0xC7}},

	{0x1C, 1,{0x00}},
	
	
	{0x1F, 1,{0x74}},
	
	{0x20, 1,{0x19}},
	
	{0x21, 1,{0x19}},
	
	{0x22, 1,{0x0E}},
	
	{0x27, 1,{0x43}},
	
	
	{0x37, 1,{0x09}},
	
	{0x38, 1,{0x04}},
		
	{0x39, 1,{0x08}},
	
	{0x3A, 1,{0x18}},
	
	{0x3B, 1,{0x18}},
	
	{0x3C, 1,{0x72}},
	
	//{0x3D, 1,{0xFF}}, //???
	
	{0x3E, 1,{0xFF}},
	
	{0x3E, 1,{0xFF}},
	
	{0x3F, 1,{0xFF}},
	
	
	{0x40, 1,{0x04}},
	
	{0x41, 1,{0xA0}},
	
	
	{0x43, 1,{0x08}},
	
	{0x44, 1,{0x07}},
	
	{0x45, 1,{0x30}},
	
	
	{0x55, 1,{0x01}},
	
	{0x56, 1,{0x01}},
	
	{0x57, 1,{0x65}},
	
	{0x58, 1,{0x0A}},
	
	{0x59, 1,{0x0A}},
	
	{0x5A, 1,{0x28}},
	
	{0x5B, 1,{0x0F}},
	
	
	{0x5D, 1,{0x7C}},
	
	{0x5E, 1,{0x5F}},
	
	{0x5F, 1,{0x4D}},
	
	{0x60, 1,{0x3F}},
	
	{0x61, 1,{0x39}},
	
	{0x62, 1,{0x29}},
	
	{0x63, 1,{0x2B}},
	
	{0x64, 1,{0x12}},
	
	{0x65, 1,{0x28}},
	
	{0x66, 1,{0x24}},
	
	{0x67, 1,{0x22}},
	
	{0x68, 1,{0x3E}},
	
	{0x69, 1,{0x2C}},
	
	{0x6A, 1,{0x33}},
	
	{0x6B, 1,{0x26}},
	
	{0x6C, 1,{0x23}},
	
	{0x6D, 1,{0x18}},
	
	{0x6E, 1,{0x09}},
	
	{0x6F, 1,{0x00}},
	
	
	{0x70, 1,{0x7C}},
	
	{0x71, 1,{0x5F}},
	
	{0x72, 1,{0x4D}},
	
	{0x73, 1,{0x3F}},

	{0x74, 1,{0x39}},
	
	{0x75, 1,{0x29}},
	
	{0x76, 1,{0x2B}},
	
	{0x77, 1,{0x12}},
	{0x78, 1,{0x28}},
	
	{0x79, 1,{0x24}},
	
	{0x7A, 1,{0x22}},
	
	{0x7B, 1,{0x3E}},

	{0x7C, 1,{0x2C}},
	
	{0x7D, 1,{0x33}},
	
	{0x7E, 1,{0x26}},
	
	{0x7F, 1,{0x23}},
	
	{0x80, 1,{0x18}},
	
	{0x81, 1,{0x09}},
	
	{0x82, 1,{0x00}},
	
	
	{0xE0, 1,{0x02}},
	
	{0x00, 1,{0x37}},
	{0x01, 1,{0x17}},
	
	{0x02, 1,{0x0A}},
	
	{0x03, 1,{0x06}},
	
	{0x04, 1,{0x08}},
	
	{0x05, 1,{0x04}},
	
	{0x06, 1,{0x00}},
	
	{0x07, 1,{0x1F}},
	
	{0x08, 1,{0x1F}},
	
	{0x09, 1,{0x1F}},
	
	{0x0A, 1,{0x1F}},
	
	{0x0B, 1,{0x1F}},
	
	{0x0C, 1,{0x1F}},
	
	{0x0D, 1,{0x1F}},
	
	{0x0E, 1,{0x1F}},
	
	{0x0F, 1,{0x1F}},
	
	{0x10, 1,{0x3F}},
	
	{0x11, 1,{0x1F}},
	
	{0x12, 1,{0x1F}},
	
	{0x13, 1,{0x1E}},
	
	{0x14, 1,{0x10}},
	
	{0x15, 1,{0x1F}},
	
	
	{0x16, 1,{0x37}},
	
	{0x17, 1,{0x17}},
	
	{0x18, 1,{0x0B}},
	
	{0x19, 1,{0x07}},
	
	{0x1A, 1,{0x09}},
	
	{0x1B, 1,{0x05}},
	
	{0x1C, 1,{0x01}},
	
	{0x1D, 1,{0x1F}},
	
	{0x1E, 1,{0x1F}},
	
	{0x1F, 1,{0x1F}},
	
	{0x20, 1,{0x1F}},
	
	{0x21, 1,{0x1F}},
	
	{0x22, 1,{0x1F}},
	
	{0x23, 1,{0x1F}},
	
	{0x24, 1,{0x1F}},
	
	{0x25, 1,{0x1F}},
	
	{0x26, 1,{0x1F}},
	
	{0x27, 1,{0x1F}},
	
	{0x28, 1,{0x1F}},
	
	{0x29, 1,{0x1E}},
	
	{0x2A, 1,{0x11}},
	
	{0x2B, 1,{0x1F}},
	
	
	{0x2C, 1,{0x37}},
	
	{0x2D, 1,{0x17}},
	
	{0x2E, 1,{0x05}},
	
	{0x2F, 1,{0x09}},
	
	{0x30, 1,{0x07}},
	
	{0x31, 1,{0x0B}},
	
	{0x32, 1,{0x11}},
	
	{0x33, 1,{0x1F}},
	
	{0x34, 1,{0x1F}},
	
	{0x35, 1,{0x1F}},
	
	{0x36, 1,{0x1F}},
	
	{0x37, 1,{0x1F}},
	
	{0x38, 1,{0x1F}},
	
	{0x39, 1,{0x1F}},
	
	{0x3A, 1,{0x1F}},
	
	{0x3B, 1,{0x1F}},
	
	{0x3C, 1,{0x3F}},
	
	{0x3D, 1,{0x1F}},
	
	{0x3E, 1,{0x1E}},
	
	{0x3F, 1,{0x1F}},
	
	{0x40, 1,{0x01}},
	
	{0x41, 1,{0x1F}},
	
	
	{0x42, 1,{0x38}},
	
	{0x43, 1,{0x18}},
	
	{0x44, 1,{0x04}},
	
	{0x45, 1,{0x08}},
	
	{0x46, 1,{0x06}},
	
	{0x47, 1,{0x0A}},
	
	{0x48, 1,{0x10}},
	
	{0x49, 1,{0x1F}},
	
	//{0x49, 1,{0x1F}},
	
	{0x4A, 1,{0x1F}},
	
	{0x4B, 1,{0x1F}},
	
	{0x4C, 1,{0x1F}},
	
	{0x4D, 1,{0x1F}},
	
	{0x4E, 1,{0x1F}},
	
	{0x4F, 1,{0x1F}},
	
	{0x50, 1,{0x1F}},
	
	{0x51, 1,{0x1F}},
	
	{0x52, 1,{0x1F}},
	
	{0x53, 1,{0x1F}},
	
	{0x54, 1,{0x1E}},
	
	{0x55, 1,{0x1F}},
	
	{0x56, 1,{0x00}},
	
	{0x57, 1,{0x1F}},
	
	
	{0x58, 1,{0x10}},
	
	{0x59, 1,{0x00}},
	
	{0x5A, 1,{0x00}},
	
	{0x5B, 1,{0x10}},
	
	{0x5C, 1,{0x01}},
	
	{0x5D, 1,{0x50}},
	
	{0x5E, 1,{0x01}},
	
	{0x5F, 1,{0x02}},
	
	{0x60, 1,{0x30}},
	
	{0x61, 1,{0x01}},
	
	{0x62, 1,{0x02}},
	
	{0x63, 1,{0x06}},
	
	{0x64, 1,{0x6A}},
	
	{0x65, 1,{0x55}},
	
	{0x66, 1,{0x08}},
	
	{0x67, 1,{0x73}},
	
	{0x68, 1,{0x05}},
	
	{0x69, 1,{0x08}},
	
	{0x6A, 1,{0x6E}},
	
	{0x6B, 1,{0x00}},
	
	{0x6C, 1,{0x00}},
	
	{0x6D, 1,{0x00}},
	
	{0x6E, 1,{0x00}},
	
	{0x6F, 1,{0x88}},
	
	{0x70, 1,{0x00}},
	
	{0x71, 1,{0x00}},
	
	{0x72, 1,{0x06}},
	
	{0x73, 1,{0x7B}},
	
	{0x74, 1,{0x00}},
	
	{0x75, 1,{0x80}},
	
	{0x76, 1,{0x00}},
	
	{0x77, 1,{0x0D}},
	
	{0x78, 1,{0x1B}},
	
	{0x79, 1,{0x00}},
	
	{0x7A, 1,{0x00}},
	
	{0x7B, 1,{0x00}},
	
	{0x7C, 1,{0x00}},
	
	{0x7D, 1,{0x03}},
	
	{0x7E, 1,{0x7B}},
	
	
	{0xE0, 1,{0x04}},
	
	{0x09, 1,{0x10}},
	
	{0x04, 1,{0x01}},
		
	{0x0E, 1,{0x38}},
	
	{0x2B, 1,{0x2B}},
	
	{0x2E, 1,{0x44}},


	{0xE0, 1,{0x00}},
	
	{0xE6, 1,{0x02}},
	
	{0xE6, 1,{0x02}},
	
	

	{REGFLAG_DELAY,120, {}},

	{0x11, 0,{0x00}},

	{REGFLAG_DELAY, 120, {}},

	{0x29, 0,{0x00}},

	{REGFLAG_DELAY, 10, {}},

	{0x35, 0,{0x00}},
};



static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 0, {0x00}},
   {REGFLAG_DELAY, 100, {}},
//	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},

    // Sleep Mode On
	{0x10, 0, {0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
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
				//UDELAY(5);//soso add or it will fail to send register
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
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

		// enable tearing-free
		params->dbi.te_mode = LCM_DBI_TE_MODE_DISABLED;
		params->dbi.te_edge_polarity = LCM_POLARITY_RISING;

		params->dsi.mode   = SYNC_EVENT_VDO_MODE;
	
		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.intermediat_buffer_num = 2;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

		params->dsi.vertical_sync_active				= 2;
		params->dsi.vertical_backporch					= 12;
		params->dsi.vertical_frontporch					= 4;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 8;
		params->dsi.horizontal_backporch				= 64;
		params->dsi.horizontal_frontporch				= 64;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		// Bit rate calculation
		//params->dsi.pll_div1=37;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
		//params->dsi.pll_div2=1; 		// div2=0~15: fout=fvo/(2*div2)
		params->dsi.PLL_CLOCK = 220;
}

static unsigned int lcm_compare_id(void);

static void lcm_init(void)
{

	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
	MDELAY(100);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
	MDELAY(100);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
	MDELAY(150);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{

	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
	MDELAY(150);

	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{
	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
	MDELAY(150);

	lcm_init();
	
	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}
static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[3];
	unsigned int array[16];
	
	SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
	SET_RESET_PIN(0);
	MDELAY(6);
	SET_RESET_PIN(1);
	MDELAY(50);

	array[0] = 0x00033700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0x04, buffer, 3);
	id = buffer[0]; //we only need ID

   printf("\n\n[soso]%s, id0 = 0x%x  id1 = 0x%x  id2 = 0x%x\n", __func__, buffer[0],buffer[1],buffer[2]);

    return (id == 0x93)?1:0;
  // return id;
}


LCM_DRIVER jd_9365_cpt = 
{
    .name			= "jd_9365_cpt",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id    = lcm_compare_id,
};

