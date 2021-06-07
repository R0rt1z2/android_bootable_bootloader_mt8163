
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

#define FRAME_WIDTH  										(600)
#define FRAME_HEIGHT 										(1024)

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
//#define SPI_WriteData(x)                    x

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
	
	{0x83, 1,{0X00}},
	
	{0x84, 1,{0X00}},
	
	{0x84, 1,{0X00}},
	
	{0x85, 1,{0X04}},
	
	{0x86, 1,{0X08}},
	
	{0x8c, 1,{0X8e}},
	
	{0xc5, 1,{0X2b}},
			
	{0xc7, 1,{0X2b}},
	
	{0x83, 1,{0Xaa}},
	
	{0x84, 1,{0X11}},
	
	{0xa9, 1,{0X4b}},
	
	{0x83, 1,{0x00}},	
	{0x84, 1,{0x00}},
	
	{0xfd, 1,{0x5B}},
	
	{0xFA, 1,{0x14}},
	
	{0x83, 1,{0xAA}},
	
	
	{0x84, 1,{0x11}},
	
	{0xC0, 1,{0x1F}},
	
	
	{0xC1, 1,{0x22}},
	
	{0xC2, 1,{0x2E}},
	
	{0xC3, 1,{0x3B}},
	
	{0xC4, 1,{0x44}},
	
	{ 0xC5, 1,{0x4D}},

	{0xC6, 1,{0x54}},
	
	
	{0xC7, 1,{0x5B}},
	
	{0xC8, 1,{0x61}},
	
	{0xC9, 1,{0xD0}},
	
	{0xCA, 1,{0xD2}},
	
	{0xCB, 1,{0xEE}},
		
	{0xCC, 1,{0xFB}},
	
	{0xCD, 1,{0x0B}},
		
	{0xCE, 1,{0x0F}},
	
	{0xCF, 1,{0x11}},
	
	{0xD0, 1,{0x13}},
	
	{0xD1, 1,{0x21}},
	
	{0xD2, 1,{0x33}},
	
	{0xD3, 1,{0xFF}},
	
	{0xD4, 1,{0x53}},
		
	{0xD5, 1,{0xB3}},
	
	{0xD6, 1,{0xB9}},
		
	{0xD7, 1,{0xBF}},
	
	{0xD8, 1,{0xC5}},
	
	{0xD9, 1,{0xCE}},
		
	{0xDA, 1,{0xD8}},
	
	{0xDB, 1,{0xE3}},
	
	{0xDC, 1,{0xF1}},
	
	{0xDD, 1,{0xFF}},
	
	{0xDE, 1,{0xF0}},
	
	{0xDF, 1,{0x2F}},
	
	{0xE0, 1,{0x20}},	
	
	{0xE1, 1,{0x23}},
	
	{0xE2, 1,{0x30}},
	
	{0xE3, 1,{0x3E}},
	
	{0xE4, 1,{0x46}},
	
	{0xE5, 1,{0x4E}},
	
	{0xE6, 1,{0x57}},
	
	{0xE7, 1,{0x5E}},
	
	{0xE8, 1,{0x64}},
	
	{0xE9, 1,{0xD7}},
	
	{0xEA, 1,{0xDA}},
	
	{0xEB, 1,{0xF8}},
	
	{0xEC, 1,{0x07}},
	
	{0xED, 1,{0x16}},
	
	{0xEE, 1,{0x1B}},
	
	{0xEF, 1,{0x1D}},
	
	{0xF0, 1,{0x23}},
	
	{0xF1, 1,{0x32}},
	
	{0xF2, 1,{0x44}},
	
	{0xF3, 1,{0x61}},
		
	{0xF4, 1,{0x67}},
	
	{0xF5, 1,{0xBB}},
	
	{0xF6, 1,{0xC2}},
	
	{0xF7, 1,{0xCB}},

	{0xF8, 1,{0xD1}},
	
	{0xF9, 1,{0xDA}},
	
	{0xFA, 1,{0xE4}},
	
	{0xFB, 1,{0xEE}},
	{0xFC, 1,{0xFD}},
	
	{0xFD, 1,{0xFF}},
	
	{0xFE, 1,{0xF8}},
	
	{0xFF, 1,{0x2F}},


	{0x83, 1,{0xBB}},
	
	{0x84, 1,{0x22}},
	
	{0xC0, 1,{0x1F}},
	
	{0xC1, 1,{0x22}},
	
	{0xC2, 1,{0x2E}},
	
	{0xC3, 1,{0x3B}},
	
	{0xC4, 1,{0x44}},
	
	
	{0xC5, 1,{0x4D}},
	
	{0xC6, 1,{0x54}},
	{0xC7, 1,{0x5B}},
	
	{0xC8, 1,{0x61}},
	
	{0xC9, 1,{0xD0}},
	
	{0xCA, 1,{0xD2}},
	
	{0xCB, 1,{0xEE}},
	
	{0xCC, 1,{0xFB}},
	
	{0xCD, 1,{0x0B}},
	
	{0xCE, 1,{0x0F}},
	
	{0xCF, 1,{0x11}},
	
	{0xD0, 1,{0x13}},
	
	{0xD1, 1,{0x21}},
	
	{0xD2, 1,{0x33}},
	
	{0xD3, 1,{0x4D}},
	
	{0xD4, 1,{0x53}},
	
	{0xD5, 1,{0xB3}},
	
	{0xD6, 1,{0xB9}},
	
	{0xD7, 1,{0xBF}},
	
	{0xD8, 1,{0xC5}},
	
	{0xD9, 1,{0xCE}},
	
	{0xDA, 1,{0xD8}},
	
	{0xDB, 1,{0xE3}},
		
	{0xDC, 1,{0xF1}},
	
	{0xDD, 1,{0xFF}},
	
	{0xDE, 1,{0xF0}},
	
	{0xDF, 1,{0x2F}},
	
	{0xE0, 1,{0x20}},
	
	{0xE1, 1,{0x23}},
	
	{0xE2, 1,{0x30}},
	
	{0xE3, 1,{0x3E}},
	
	{0xE4, 1,{0x46}},
	
	{0xE5, 1,{0x4E}},
	
	{0xE6, 1,{0x57}},
	
	{0xE7, 1,{0x5E}},
	
	{0xE8, 1,{0x64}},
	
	{0xE9, 1,{0xD7}},
	
	{0xEA, 1,{0xDA}},
	
	{0xEB, 1,{0xF8}},
	
	{0xEC, 1,{0x07}},
	
	{0xED, 1,{0x16}},
	
	{0xEE, 1,{0x1B}},
	
	{0xEF, 1,{0x1D}},
	
	{0xF0, 1,{0x23}},
	
	{0xF1, 1,{0x32}},
	
	
	{0xF2, 1,{0x44}},
	
	{0xF3, 1,{0x61}},
	
	{0xF4, 1,{0x67}},
	
	{0xF5, 1,{0xBB}},
	
	{0xF6, 1,{0xC2}},
	
	{0xF7, 1,{0xCB}},
	
	{0xF8, 1,{0xD1}},
	
	{0xF9, 1,{0xDA}},
	
	{0xFA, 1,{0xE4}},
	
	{0xFB, 1,{0xEE}},
	
	{0xFC, 1,{0xFD}},
	
	{0xFD, 1,{0xFF}},
	
	{0xFE, 1,{0xF8}},
	
	{0xFF, 1,{0x2F}},
	
	
	{0x83, 1,{0xCC}},
	
	{0x84, 1,{0x33}},
	
	{0x1F, 1,{0x1F}},
	
	{0xC1, 1,{0x22}},
	
	
	{0xC2, 1,{0x2E}},
	
	{0xC3, 1,{0x3B}},
	
	{0xC4, 1,{0x44}},
	
	
	{0xC5, 1,{0x4D}},
	
	{0xC6, 1,{0x54}},
	{0xC7, 1,{0x5B}},
	
	{0xC8, 1,{0x61}},
	
	{0xC9, 1,{0xD0}},
	
	{0xCA, 1,{0xD2}},
	
	{0xCB, 1,{0xEE}},
	
	{0xCC, 1,{0xFB}},
	
	{0xCD, 1,{0x0B}},
	
	{0xCE, 1,{0x0F}},
	
	{0xCF, 1,{0x11}},
	
	{0xD0, 1,{0x13}},
	
	{0xD1, 1,{0x21}},
	
	{0xD2, 1,{0x33}},
	
	{0xD3, 1,{0x4D}},
	
	{0xD4, 1,{0x53}},
	
	{0xD5, 1,{0xB3}},
	
	{0xD6, 1,{0xB9}},
	
	{0xD7, 1,{0xBF}},
	
	{0xD8, 1,{0xC5}},
	
	{0xD9, 1,{0xCE}},
	
	{0xDA, 1,{0xD8}},
	
	{0xDB, 1,{0xE3}},
		
	{0xDC, 1,{0xF1}},
	
	{0xDD, 1,{0xFF}},
	
	{0xDE, 1,{0xF0}},
	
	{0xDF, 1,{0x2F}},
	
	{0xE0, 1,{0x20}},
	
	{0xE1, 1,{0x23}},
	
	{0xE2, 1,{0x30}},
	
	{0xE3, 1,{0x3E}},
	
	{0xE4, 1,{0x46}},
	
	{0xE5, 1,{0x4E}},
	
	{0xE6, 1,{0x57}},
	
	{0xE7, 1,{0x5E}},
	
	{0xE8, 1,{0x64}},
	
	{0xE9, 1,{0xD7}},
	
	{0xEA, 1,{0xDA}},
	
	{0xEB, 1,{0xF8}},
	
	{0xEC, 1,{0x07}},
	
	{0xED, 1,{0x16}},
	
	{0xEE, 1,{0x1B}},
	
	{0xEF, 1,{0x1D}},
	
	{0xF0, 1,{0x23}},
	
	{0xF1, 1,{0x32}},
	
	
	{0xF2, 1,{0x44}},
	
	{0xF3, 1,{0x61}},
	
	{0xF4, 1,{0x67}},
	
	{0xF5, 1,{0xBB}},
	
	{0xF6, 1,{0xC2}},
	
	{0xF7, 1,{0xCB}},
	
	{0xF8, 1,{0xD1}},
	
	{0xF9, 1,{0xDA}},
	
	{0xFA, 1,{0xE4}},
	
	{0xFB, 1,{0xEE}},
	
	{0xFC, 1,{0xFD}},
	
	{0xFD, 1,{0xFF}},
	
	{0xFE, 1,{0xF8}},
	
	{0xFF, 1,{0x2F}},
	

	{0x83, 1,{0x00}},
	{0x84, 1,{0x00}},
	

	//{REGFLAG_DELAY,120, {}},

	{0x11, 0,{0x00}},

	//{REGFLAG_DELAY, 120, {}},

	{0x29, 0,{0x00}},

	//{REGFLAG_DELAY, 10, {}},

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

		params->dsi.mode   =SYNC_PULSE_VDO_MODE; //SYNC_EVENT_VDO_MODE;
	
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
		params->dsi.cont_clock=1;

		// Video mode setting		
		params->dsi.intermediat_buffer_num = 2;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch					=25;// 12;
		params->dsi.vertical_frontporch					= 35;//4;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				=4;
		params->dsi.horizontal_backporch				= 60;//64;
		params->dsi.horizontal_frontporch				= 80;//64;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		// Bit rate calculation
		//params->dsi.pll_div1=37;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
		//params->dsi.pll_div2=1; 		// div2=0~15: fout=fvo/(2*div2)
		params->dsi.PLL_CLOCK = 165;//100
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
    return 1;
 
}


LCM_DRIVER nt51021 = 
{
    .name			= "nt51021",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id    = lcm_compare_id,
};

