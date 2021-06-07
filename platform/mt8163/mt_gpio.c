/******************************************************************************
 * mt_gpio.c - MTKLinux GPIO Device Driver
 * 
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 * 
 * DESCRIPTION:
 *     This file provid the other drivers GPIO relative functions
 *
 ******************************************************************************/

#include <platform/mt_reg_base.h>
#include <platform/mt_gpio.h>
#include <platform/gpio_cfg.h>
#include <debug.h>
/******************************************************************************
 MACRO Definition
******************************************************************************/
//#define  GIO_SLFTEST
#define GPIO_DEVICE "mt-gpio"
#define VERSION     GPIO_DEVICE
/*---------------------------------------------------------------------------*/
#define GPIO_WR32(addr, data)   DRV_WriteReg32(addr,data)
#define GPIO_RD32(addr)         DRV_Reg32(addr)
#define GPIO_SW_SET_BITS(BIT,REG)   GPIO_WR32(REG,GPIO_RD32(REG) | ((unsigned long)(BIT)))
#define GPIO_SET_BITS(BIT,REG)   ((*(volatile u32*)(REG)) = (u32)(BIT))
#define GPIO_CLR_BITS(BIT,REG)   ((*(volatile u32*)(REG)) &= ~((u32)(BIT)))

/*---------------------------------------------------------------------------*/
#define TRUE                   1
#define FALSE                  0
/*---------------------------------------------------------------------------*/
//#define MAX_GPIO_REG_BITS      16
//#define MAX_GPIO_MODE_PER_REG  5
//#define GPIO_MODE_BITS         3
/*---------------------------------------------------------------------------*/
#define GPIOTAG                "[GPIO] "
#define GPIOLOG(fmt, arg...)   dprintf(INFO,GPIOTAG fmt, ##arg)
#define GPIOMSG(fmt, arg...)   dprintf(INFO,fmt, ##arg)
#define GPIOERR(fmt, arg...)   dprintf(INFO,GPIOTAG "%5d: "fmt, __LINE__, ##arg)
#define GPIOFUC(fmt, arg...)   //dprintk(INFO,GPIOTAG "%s\n", __FUNCTION__)
#define GIO_INVALID_OBJ(ptr)   ((ptr) != gpio_obj)
/******************************************************************************
Enumeration/Structure
******************************************************************************/
#if  defined(MACH_FPGA)
		S32 mt_set_gpio_dir(u32 pin, u32 dir)			{return RSUCCESS;}
		S32 mt_get_gpio_dir(u32 pin)				{return GPIO_DIR_UNSUPPORTED;}
		S32 mt_set_gpio_pull_enable(u32 pin, u32 enable)	{return RSUCCESS;}
		S32 mt_get_gpio_pull_enable(u32 pin)			{return GPIO_PULL_EN_UNSUPPORTED;}
		S32 mt_set_gpio_pull_select(u32 pin, u32 select)	{return RSUCCESS;}
		S32 mt_get_gpio_pull_select(u32 pin)			{return GPIO_PULL_UNSUPPORTED;}
		S32 mt_set_gpio_smt(u32 pin, u32 enable)		{return RSUCCESS;}
		S32 mt_get_gpio_smt(u32 pin)				{return GPIO_SMT_UNSUPPORTED;}
		S32 mt_set_gpio_ies(u32 pin, u32 enable)		{return RSUCCESS;}
		S32 mt_get_gpio_ies(u32 pin)				{return GPIO_IES_UNSUPPORTED;}
		S32 mt_set_gpio_out(u32 pin, u32 output)		{return RSUCCESS;}
		S32 mt_get_gpio_out(u32 pin)				{return GPIO_OUT_UNSUPPORTED;}
		S32 mt_get_gpio_in(u32 pin) 				{return GPIO_IN_UNSUPPORTED;}
		S32 mt_set_gpio_mode(u32 pin, u32 mode) 		{return RSUCCESS;}
		S32 mt_get_gpio_mode(u32 pin)				{return GPIO_MODE_UNSUPPORTED;}
#else


/*-------for special kpad pupd-----------*/
struct kpad_pupd {
	unsigned char pin;
	unsigned char reg;
	unsigned char bit;
};
static struct kpad_pupd kpad_pupd_spec[] = {
	{GPIO33, 0, 2},	/* KROW0 */
	{GPIO34, 0, 6},	/* KROW1 */
	{GPIO35, 0, 10},/* KROW2 */
	{GPIO36, 1, 2},	/* KCOL0 */
	{GPIO37, 1, 6},	/* KCOL1 */
	{GPIO38, 1, 10}	/* KCOL2 */
};

/*---------------------------------------*/


struct mt_ies_smt_set {
	unsigned char index_start;
	unsigned char index_end;
	unsigned char reg_index;
	unsigned char bit;
};
static struct mt_ies_smt_set mt_ies_smt_map[] = {
	{GPIO0, GPIO9, 0, 0},    /*IES0/SMT0*/
	{GPIO10, GPIO13, 0, 1},  /*IES1/SMT1*/
	{GPIO14, GPIO28, 0, 2},  /*IES2/SMT2*/
	{GPIO29, GPIO32, 1, 3},  /*IES3/SMT3*/
	{GPIO33, GPIO33, 1, 11}, /*IES27/SMT27*/
	{GPIO34, GPIO38, 0, 10},  /*IES10/SMT10*/
	{GPIO39, GPIO42, 0, 11},  /*IES11/SMT11*/
	{GPIO43, GPIO45, 0, 12},  /*IES12/SMT12*/
	{GPIO46, GPIO49, 0, 13},  /*IES13/SMT13*/
	{GPIO50, GPIO52, 1, 10},  /*IES26/SMT26*/
	{GPIO53, GPIO56, 0, 14},  /*IES14/SMT14*/
	{GPIO57, GPIO58, 1, 0},  /*IES16/SMT16*/
	{GPIO59, GPIO65, 1, 2},  /*IES18/SMT18*/
	{GPIO66, GPIO71, 1, 3}, /*IES19/SMT19*/
	{GPIO72, GPIO74, 1, 4}, /*IES20/SMT20*/
	{GPIO75, GPIO76, 0, 15}, /*IES15/SMT15*/
	{GPIO77, GPIO78, 1, 1},  /*IES17/SMT17*/
	{GPIO79, GPIO82, 1, 5},  /*IES21/SMT21*/
	{GPIO83, GPIO84, 1, 6},  /*IES22/SMT22*/
	{GPIO85, GPIO90, 0, 0},  /*MSDC2*/
	{GPIO91, GPIO100, 0, 0},  /*TDP/TCP/TCN */
	{GPIO101, GPIO116, 0, 0},  /*GPI*/
	{GPIO117, GPIO120, 1, 7},/*IES23/SMT23*/
	{GPIO121, GPIO126, 0, 0},  /*MSDC1*/
	{GPIO127, GPIO137, 0, 0},  /*MSDC0*/
	{GPIO138, GPIO141, 1, 9},/*IES25/SMT25*/
	{GPIO142, GPIO142, 0, 13},/*IES13/SMT13*/
	{GPIO143, GPIO154, 0, 0},  /*MSDC3*/
};


/*-------for msdc pupd-----------*/
struct msdc_pupd {
	unsigned char 	pin;
	unsigned char	reg;
	unsigned char	bit;
};
static struct msdc_pupd msdc_pupd_spec[2][6] = {
    {/*MSDC1*/
	{GPIO121,	1,	8},
	{GPIO122,	0,	8},
	{GPIO123,	3,	0},
	{GPIO124,	3,	4},
	{GPIO125,	3,	8},
	{GPIO126,	3,	12}},
	
	{/*MSDC2*/
	{GPIO85,	1,	8},
	{GPIO86,	0,	8},
	{GPIO87,	3,	0},
	{GPIO88,	3,	4},
	{GPIO89,	3,	8},
	{GPIO90,	3,	12}},
#if 0 /*MSDC0 & MSDC3 controled by IC internal design*/
	{/*MSDC0*/
	{GPIO127,	4,	12},
	{GPIO128,	4,	8},
	{GPIO129,	4,	4},
	{GPIO130,	4,	0},
	{GPIO131,	5,	0},
	{GPIO132,	1,	8},
	{GPIO133,	0,	8},
	{GPIO134,	3,	12},
	{GPIO135,	3,	8},
	{GPIO136,	3,	4}
	{GPIO137,	3,	0}},

	{/*MSDC3*/
	{GPIO143,	4,	12},
	{GPIO144,	4,	8},
	{GPIO145,	4,	4},
	{GPIO146,	4,	0},
	{GPIO147,	5,	0},
	{GPIO148,	1,	8},
	{GPIO149,	0,	8},
	{GPIO150,	3,	12},
	{GPIO151,	3,	8},
	{GPIO152,	3,	4},
	{GPIO153,	3,	0},
	{GPIO154,	5,	4}},
#endif
};
/*---------------------------------------*/



struct mt_gpio_obj {
    GPIO_REGS       *reg;
};
static struct mt_gpio_obj gpio_dat = {
    .reg  = (GPIO_REGS*)(GPIO_BASE),
};
static struct mt_gpio_obj *gpio_obj = &gpio_dat;
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_dir_chip(u32 pin, u32 dir)
{
    u32 pos;
    u32 bit;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (dir >= GPIO_DIR_MAX)
        return -ERINVAL;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    if (dir == GPIO_DIR_IN)
        GPIO_SET_BITS((1L << bit), &obj->reg->dir[pos].rst);
    else
        GPIO_SET_BITS((1L << bit), &obj->reg->dir[pos].set);
    return RSUCCESS;

}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_dir_chip(u32 pin)
{
    u32 pos;
    u32 bit;
    u32 reg;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIO_RD32(&obj->reg->dir[pos].val);
    return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_enable_chip(u32 pin, u32 enable)
{ 
	u32 pos;
    u32 bit;
    u32 i;

    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;
    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;
    if (enable >= GPIO_PULL_EN_MAX)
        return -ERINVAL;
/*for special kpad pupd, NOTE DEFINITION REVERSE!!! */
	/*****************for special kpad pupd, NOTE DEFINITION REVERSE!!!*****************/
	for (i = 0; i < sizeof(kpad_pupd_spec) / sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
			if (enable == GPIO_PULL_DISABLE) {
				GPIO_SET_BITS((3L << (kpad_pupd_spec[i].bit - 2)),
					      &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].rst);
			} else {
				GPIO_SET_BITS((1L << (kpad_pupd_spec[i].bit - 2)), &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].set);	/* single key: 75K */
			}
			return RSUCCESS;
		}
	}

	/********************************* MSDC special *********************************/
	if (pin == GPIO127) {	/* ms0 dat7 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 13, &obj->reg->msdc0_ctrl[4].rst);
		} else {
			GPIO_SET_BITS(1L << 14, &obj->reg->msdc0_ctrl[4].set);	/* 1L:10K */
		}
		return RSUCCESS;
	}else if (pin == GPIO128) {	/* ms0 dat6 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 9, &obj->reg->msdc0_ctrl[4].rst);
		} else {
			GPIO_SET_BITS(1L << 10, &obj->reg->msdc0_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO129) {	/* ms0 dat5*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 5, &obj->reg->msdc0_ctrl[4].rst);
		} else {
			GPIO_SET_BITS(1L << 6, &obj->reg->msdc0_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO130) {	/* ms0 dat4*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 1, &obj->reg->msdc0_ctrl[4].rst);
		} else {
			GPIO_SET_BITS(1L << 2, &obj->reg->msdc0_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO131) {	/* ms0 RST */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 1, &obj->reg->msdc0_ctrl[5].rst);
		} else {
			GPIO_SET_BITS(1L << 2, &obj->reg->msdc0_ctrl[5].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO132) {	/* ms0 cmd */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 9, &obj->reg->msdc0_ctrl[1].rst);
		} else {
			GPIO_SET_BITS(1L << 10, &obj->reg->msdc0_ctrl[1].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO133) {	/* ms0 clk */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 9, &obj->reg->msdc0_ctrl[0].rst);
		} else {
			GPIO_SET_BITS(1L << 10, &obj->reg->msdc0_ctrl[0].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO134) {	/* ms0 dat3 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 13, &obj->reg->msdc0_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 14, &obj->reg->msdc0_ctrl[3].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO135) {	/* ms0 dat2 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 9, &obj->reg->msdc0_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 10, &obj->reg->msdc0_ctrl[3].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO136) {	/* ms0 dat1*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 5, &obj->reg->msdc0_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 6, &obj->reg->msdc0_ctrl[3].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO137) {	/* ms0 dat0 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 1, &obj->reg->msdc0_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 2, &obj->reg->msdc0_ctrl[3].set);
		}
		return RSUCCESS;
	
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO121) {	/* ms1 cmd */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 9, &obj->reg->msdc1_ctrl[1].rst);
		} else {
			GPIO_SET_BITS(1L << 10, &obj->reg->msdc1_ctrl[1].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO122) {	/* ms1 clk */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 9, &obj->reg->msdc1_ctrl[0].rst);
		} else {
			GPIO_SET_BITS(1L << 10, &obj->reg->msdc1_ctrl[0].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO123) {	/* ms1 dat0 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 1, &obj->reg->msdc1_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 2, &obj->reg->msdc1_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO124) {	/* ms1 dat1 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 5), &obj->reg->msdc1_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 6), &obj->reg->msdc1_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO125) {	/* ms1 dat2 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 9), &obj->reg->msdc1_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 10), &obj->reg->msdc1_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO126) {	/* ms1 dat3 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 13), &obj->reg->msdc1_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 14), &obj->reg->msdc1_ctrl[3].set);
		}
		return RSUCCESS;
	 
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO85) {	/* ms2 cmd */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<9, &obj->reg->msdc2_ctrl[1].rst);
		} else {
			GPIO_SET_BITS(1L<<10, &obj->reg->msdc2_ctrl[1].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO86) {	/* ms2 clk */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<9, &obj->reg->msdc2_ctrl[0].rst);
		} else {
			GPIO_SET_BITS(1L<<10, &obj->reg->msdc2_ctrl[0].set);
		}
		return RSUCCESS;	
	}else if (pin == GPIO87) {	/* ms2 dat0 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<1, &obj->reg->msdc2_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L<<2, &obj->reg->msdc2_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO88) {	/* ms2 dat1 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 5), &obj->reg->msdc2_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 6), &obj->reg->msdc2_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO89) {	/* ms2 dat2 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 9), &obj->reg->msdc2_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 10), &obj->reg->msdc2_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO90) {	/* ms2 dat3 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 13), &obj->reg->msdc2_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 14), &obj->reg->msdc2_ctrl[3].set);
		}
		return RSUCCESS;
	
		/* //////////////////////////////////////////////// */
	}else if (pin == GPIO143) {	/* ms3 dat7*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 13, &obj->reg->msdc3_ctrl[4].rst);
		} else {
			GPIO_SET_BITS(1L << 14, &obj->reg->msdc3_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO144) {/* ms3 dat6 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 9), &obj->reg->msdc3_ctrl[4].rst);
		} else {
			GPIO_SET_BITS((1L << 10), &obj->reg->msdc3_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO145) {/* ms3 dat5*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 5), &obj->reg->msdc3_ctrl[4].rst);
		} else {
			GPIO_SET_BITS((1L << 6), &obj->reg->msdc3_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO146) {/* ms3 dat4*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 1), &obj->reg->msdc3_ctrl[4].rst);
		} else {
			GPIO_SET_BITS((1L << 2), &obj->reg->msdc3_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO147) {/* ms3 RST */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<1, &obj->reg->msdc3_ctrl[5].rst);
		} else {
			GPIO_SET_BITS(1L<<2, &obj->reg->msdc3_ctrl[5].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO148) {/* ms3 cmd */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 9, &obj->reg->msdc3_ctrl[1].rst);
		} else {
			GPIO_SET_BITS(1L << 10, &obj->reg->msdc3_ctrl[1].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO149) {	/* ms3 clk */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 9, &obj->reg->msdc3_ctrl[0].rst);
		} else {
			GPIO_SET_BITS(1L << 10, &obj->reg->msdc3_ctrl[0].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO150) {/* ms3 dat3*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L << 13, &obj->reg->msdc3_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 14, &obj->reg->msdc3_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO151) {/* ms3 dat2 */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 9), &obj->reg->msdc3_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 10), &obj->reg->msdc3_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO152) {/* ms3 dat1*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 5), &obj->reg->msdc3_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 6), &obj->reg->msdc3_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO153) {/* ms3 dat0*/
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((3L << 1), &obj->reg->msdc3_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 2), &obj->reg->msdc3_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO154) {/* ms3 DS */
		if (enable == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS(3L<<5, &obj->reg->msdc3_ctrl[5].rst);
		} else {
			GPIO_SET_BITS(1L<<6, &obj->reg->msdc3_ctrl[5].set);	/* 1L:10K */
		}
		return RSUCCESS;
	}

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;

		if (enable == GPIO_PULL_DISABLE)
			GPIO_SET_BITS((1L << bit), &obj->reg->pullen[pos].rst);
		else
			GPIO_SET_BITS((1L << bit), &obj->reg->pullen[pos].set);
	}
	return RSUCCESS;

}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_enable_chip(u32 pin)
{
    u32 pos;
    u32 bit;
 //   u32 reg;
    u32 i;
    u32 data;
	
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;    

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

/*****************for special kpad pupd, NOTE DEFINITION REVERSE!!!*****************/
	for (i = 0; i < sizeof(kpad_pupd_spec) / sizeof(kpad_pupd_spec[0]); i++) {
		if (pin == kpad_pupd_spec[i].pin) {
			return (((GPIO_RD32(&obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].val) &
				  (3L << (kpad_pupd_spec[i].bit - 2))) != 0) ? 1 : 0);
		}
	}

	/********************************* MSDC special *********************************/
	if (pin == GPIO127) {	/* ms0 dat7 */		
		return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (3L << 13)) != 0) ? 1 : 0);
	}else if (pin == GPIO128) {	/* ms0 dat6 */
		return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO129) {	/* ms0 dat5*/
		return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (3L << 5)) != 0) ? 1 : 0);
	} else if (pin == GPIO130) {	/* ms0 dat4*/
		return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO131) {	/* ms0 RST */
		return (((GPIO_RD32(&obj->reg->msdc0_ctrl[5].val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO132) {	/* ms0 cmd */		
		return (((GPIO_RD32(&obj->reg->msdc0_ctrl[1].val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO133) {	/* ms0 clk */		
		return (((GPIO_RD32(&obj->reg->msdc0_ctrl[0].val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO134) {	/* ms0 dat3 */		
		return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (3L << 13)) != 0) ? 1 : 0);
	}else if (pin == GPIO135) {	/* ms0 dat2 */
	    return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (3L << 9)) != 0) ? 1 : 0);
	}else if (pin == GPIO136) {	/* ms0 dat1*/		
		return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (3L << 5)) != 0) ? 1 : 0);
	}else if (pin == GPIO137) {	/* ms0 dat0 */		
		return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (3L << 1)) != 0) ? 1 : 0);
	
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO121) {	/* ms1 cmd */		
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl[1].val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO122) {	/* ms1 clk */	
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl[0].val) & (3L << 9)) != 0) ? 1 : 0);
	}else if (pin == GPIO123) {	/* ms1 dat0 */		
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO124) {	/* ms1 dat1 */		
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (3L << 5)) != 0) ? 1 : 0);
	} else if (pin == GPIO125) {	/* ms1 dat2 */		
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO126) {	/* ms1 dat3 */		
		return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (3L << 13)) != 0) ? 1 : 0);
	 
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO85) {	/* ms2 cmd */		
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl[1].val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO86) {	/* ms2 clk */
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl[0].val) & (3L << 9)) != 0) ? 1 : 0);
	}else if (pin == GPIO87) {	/* ms2 dat0 */
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO88) {	/* ms2 dat1 */
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (3L << 5)) != 0) ? 1 : 0);
	} else if (pin == GPIO89) {	/* ms2 dat2 */
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO90) {	/* ms2 dat3 */
		return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (3L << 13)) != 0) ? 1 : 0);
	
		/* //////////////////////////////////////////////// */
	}else if (pin == GPIO143) {	/* ms3 dat7*/
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (3L << 13)) != 0) ? 1 : 0);
	} else if (pin == GPIO144) {/* ms3 dat6 */
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO145) {/* ms3 dat5*/		
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (3L << 5)) != 0) ? 1 : 0);
	} else if (pin == GPIO146) {/* ms3 dat4*/		
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO147) {/* ms3 RST */
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[5].val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO148) {/* ms3 cmd */		
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[1].val) & (3L << 9)) != 0) ? 1 : 0);
	}else if (pin == GPIO149) {	/* ms3 clk */		
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[0].val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO150) {/* ms3 dat3*/		
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (3L << 13)) != 0) ? 1 : 0);
	} else if (pin == GPIO151) {/* ms3 dat2 */		
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (3L << 9)) != 0) ? 1 : 0);
	} else if (pin == GPIO152) {/* ms3 dat1*/		
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (3L << 5)) != 0) ? 1 : 0);
	} else if (pin == GPIO153) {/* ms3 dat0*/		
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (3L << 1)) != 0) ? 1 : 0);
	} else if (pin == GPIO154) {/* ms3 DS */
		return (((GPIO_RD32(&obj->reg->msdc3_ctrl[5].val) & (3L << 5)) != 0) ? 1 : 0);
	}

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;
		data = GPIO_RD32(&obj->reg->pullen[pos].val);
	}
	return (((data & (1L << bit)) != 0) ? 1 : 0);


}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_smt_chip(u32 pin, u32 enable)
{

       int i = 0;
	
	   struct mt_gpio_obj *obj = gpio_obj;
   
	   for (i = 0; i < 19; i++) {
			   if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
				   if (enable == GPIO_SMT_DISABLE)
					   GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								  &obj->reg->ies[mt_ies_smt_map[i].reg_index].rst);
				   else
					   GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								  &obj->reg->ies[mt_ies_smt_map[i].reg_index].set); 
				   }
		   }
   
   
	   if(i == 19)
		   {
		   /*only set smt,ies not set*/
			 if(pin == GPIO85){ 			   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc2_ctrl[1].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc2_ctrl[1].set);
				  }
			 }else if(pin == GPIO86){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc2_ctrl[0].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc2_ctrl[0].set);
				  }
			 }else if(pin == GPIO87){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc2_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc2_ctrl[3].set);
				  }
			 }else if(pin == GPIO88){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc2_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc2_ctrl[3].set);
				  }
			 }else if(pin == GPIO89){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc2_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc2_ctrl[3].set);
				  }
			 }else if(pin == GPIO90){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc2_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc2_ctrl[3].set);
				  }
			 }
			i++;
			 
		   }
	   i=i+3;/*similar to TDP/RDP/.../not uesd*/
	   if (i == 22) {
			   if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
				   if (enable == GPIO_IES_DISABLE)
					   GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								  &obj->reg->ies[mt_ies_smt_map[i].reg_index].rst);
				   else
					   GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								  &obj->reg->ies[mt_ies_smt_map[i].reg_index].set); 
				   }
		   }
	   i++;
	   if(i == 23)
		   {
		   /*only set smt,ies not set*/
			 if(pin == GPIO121){
			   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc1_ctrl[1].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc1_ctrl[1].set);
				  }
			 }else if(pin == GPIO122){
			   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc1_ctrl[0].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc1_ctrl[0].set);
				  }
			 }else if(pin == GPIO123){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc1_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc1_ctrl[3].set);
				  }
			 }else if(pin == GPIO124){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc1_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc1_ctrl[3].set);
				  }
			 }else if(pin == GPIO125){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc1_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc1_ctrl[3].set);
				  }
			 }else if(pin == GPIO126){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc1_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc1_ctrl[3].set);
				  }
			 }
			i++;
			 
		   }
	   if(i == 24)
		   {
		   /*only set smt,ies not set*/
			 if(pin == GPIO127){
			   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc0_ctrl[4].rst);
				  } else {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc0_ctrl[4].set);
				  }
			 }else if(pin == GPIO128){
			   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc0_ctrl[4].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc0_ctrl[4].set);
				  }
			 }else if(pin == GPIO129){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc0_ctrl[4].rst);
				  } else {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc0_ctrl[4].set);
				  }
			 }else if(pin == GPIO130){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc0_ctrl[4].rst);
				  } else {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc0_ctrl[4].set);
				  }
			 }else if(pin == GPIO131){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc0_ctrl[5].rst);
				  } else {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc0_ctrl[5].set);
				  }
			 }
			 else if(pin == GPIO132){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc0_ctrl[1].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc0_ctrl[1].set);
				  }
			 }else if(pin == GPIO133){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc0_ctrl[0].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc0_ctrl[0].set);
				  }
			 }else if(pin == GPIO134){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc0_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc0_ctrl[3].set);
				  }
			 }else if(pin == GPIO135){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc0_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc0_ctrl[3].set);
				  }
			 }else if(pin == GPIO136){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc0_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc0_ctrl[3].set);
				  }
			 }else if(pin == GPIO137){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc0_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc0_ctrl[3].set);
				  }
			 }
			i++;
			 
		   }
	   for (;i < 27; i++ ) {
			   if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
				   if (enable == GPIO_IES_DISABLE)
					   GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								  &obj->reg->ies[mt_ies_smt_map[i].reg_index].rst);
				   else
					   GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								  &obj->reg->ies[mt_ies_smt_map[i].reg_index].set); 
				   }
		   }
	   if(i == 27)
		   {
		   /*only set smt,ies not set*/
			 if(pin == GPIO143){
			   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc3_ctrl[4].rst);
				  } else {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc3_ctrl[4].set);
				  }
			 }else if(pin == GPIO144){
			   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc3_ctrl[4].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc3_ctrl[4].set);
				  }
			 }else if(pin == GPIO145){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc3_ctrl[4].rst);
				  } else {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc3_ctrl[4].set);
				  }
			 }else if(pin == GPIO146){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc3_ctrl[4].rst);
				  } else {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc3_ctrl[4].set);
				  }
			 }else if(pin == GPIO147){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc3_ctrl[5].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc3_ctrl[5].set);
				  }
			 }else if(pin == GPIO148){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc3_ctrl[1].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc3_ctrl[1].set);
				  }
			 }else if(pin == GPIO149){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc3_ctrl[0].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc3_ctrl[0].set);
				  }
			 }else if(pin == GPIO150){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc3_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 15, &obj->reg->msdc3_ctrl[3].set);
				  }
			 }else if(pin == GPIO151){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc3_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 11, &obj->reg->msdc3_ctrl[3].set);
				  }
			 }else if(pin == GPIO152){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc3_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 7, &obj->reg->msdc3_ctrl[3].set);
				  }
			 }else if(pin == GPIO153){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc3_ctrl[3].rst);
				  } else {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc3_ctrl[3].set);
				  }
			 }else if(pin == GPIO154){		   
				  if (enable == GPIO_SMT_DISABLE) {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc3_ctrl[5].rst);
				  } else {
					  GPIO_SET_BITS(1L << 3, &obj->reg->msdc3_ctrl[5].set);
				  }
			 }		 
			 
		   }	   

	 if (i > ARRAY_SIZE(mt_ies_smt_map)) {
	   return -ERINVAL;
	 }
	 return RSUCCESS;

}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_smt_chip(u32 pin)
{
   int i = 0;
   unsigned long data;
   struct mt_gpio_obj *obj = gpio_obj;

     for (i = 0; i < 19; i++) {
			 if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
				   data = GPIO_RD32(&obj->reg->ies[mt_ies_smt_map[i].reg_index].val);
				   return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
				}
	 }


	if(i == 19)
		{
		/*only set smt,ies not set*/
		  if(pin == GPIO85){				  
			   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[1].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO86){				
			   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[0].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO87){					  
			   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (1L << 3)) != 0) ? 1 : 0);
		  }else if(pin == GPIO88){					  
			   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (1L << 7)) != 0) ? 1 : 0);
		  }else if(pin == GPIO89){				 
			   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO90){				  
			   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (1L << 15)) != 0) ? 1 : 0);
		  }
		 i++;
		  
		}
	i=i+3;/*similar to TDP/RDP/.../not uesd*/
	if (i == 22) {
			if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){					 
				data = GPIO_RD32(&obj->reg->ies[mt_ies_smt_map[i].reg_index].val);
				return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
				}
		}
	i++;
	if(i == 23)
		{
		/*only set smt,ies not set*/
		  if(pin == GPIO121){				   
			   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[1].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO122){ 			
			   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[0].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO123){ 				   
			   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (1L << 3)) != 0) ? 1 : 0);
		  }else if(pin == GPIO124){ 				   
			   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (1L << 7)) != 0) ? 1 : 0);
		  }else if(pin == GPIO125){ 					   
			   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO126){ 				   
			   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (1L << 15)) != 0) ? 1 : 0);
		  }
		 i++;
		  
		}
	if(i == 24)
		{
		/*only set smt,ies not set*/
		  if(pin == GPIO127){				 
			   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (1L << 15)) != 0) ? 1 : 0);
		  }else if(pin == GPIO128){ 			 
				return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO129){ 				
				return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (1L << 7)) != 0) ? 1 : 0);
		  }else if(pin == GPIO130){ 				   
				return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (1L << 3)) != 0) ? 1 : 0);
		  }else if(pin == GPIO131){ 			   
				return (((GPIO_RD32(&obj->reg->msdc0_ctrl[5].val) & (1L << 3)) != 0) ? 1 : 0);
		  }else if(pin == GPIO132){ 			   
				return (((GPIO_RD32(&obj->reg->msdc0_ctrl[1].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO133){ 				   
				return (((GPIO_RD32(&obj->reg->msdc0_ctrl[0].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO134){ 		
				return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (1L << 15)) != 0) ? 1 : 0);
		  }else if(pin == GPIO135){ 			   
			   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO136){ 				
				return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (1L << 7)) != 0) ? 1 : 0);
		  }else if(pin == GPIO137){ 				
				return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (1L << 3)) != 0) ? 1 : 0);
		  }
		 i++;
		  
		}
	for (;i < 27; i++ ) {
			if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
				 
				  data = GPIO_RD32(&obj->reg->ies[mt_ies_smt_map[i].reg_index].val);
				  return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
				}
		}
	if(i == 27)
		{
		/*only set smt,ies not set*/
		  if(pin == GPIO143){				
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (1L << 15)) != 0) ? 1 : 0);
		  }else if(pin == GPIO144){
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO145){ 				  
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (1L << 7)) != 0) ? 1 : 0);
		  }else if(pin == GPIO146){ 		
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (1L << 3)) != 0) ? 1 : 0);
		  }else if(pin == GPIO147){ 		
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[5].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO148){ 		
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[1].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO149){ 		
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[0].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO150){ 		
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (1L << 15)) != 0) ? 1 : 0);
		  }else if(pin == GPIO151){ 		
				return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (1L << 11)) != 0) ? 1 : 0);
		  }else if(pin == GPIO152){ 		
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (1L << 7)) != 0) ? 1 : 0);
		  }else if(pin == GPIO153){ 		
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (1L << 3)) != 0) ? 1 : 0);
		  }else if(pin == GPIO154){ 		
			   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[5].val) & (1L << 3)) != 0) ? 1 : 0);
		  } 	  
		  
		}		

if (i > ARRAY_SIZE(mt_ies_smt_map)) {
	return -ERINVAL;
}

}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_ies_chip(u32 pin, u32 enable)
{
    int i = 0;
	struct mt_gpio_obj *obj = gpio_obj;
	for (i = 0; i < 19; i++) {
			if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
				if (enable == GPIO_IES_DISABLE)
		            GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
			                   &obj->reg->ies[mt_ies_smt_map[i].reg_index].rst);
	            else
		            GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
			                   &obj->reg->ies[mt_ies_smt_map[i].reg_index].set);	
				}
		}
	
	        i=i+4;/*i=19~21,similar to TDP/RDP/.../,ies not uesd*/
			if (i == 22) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
					if (enable == GPIO_IES_DISABLE)
						GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								&obj->reg->ies[mt_ies_smt_map[i].reg_index].rst);
					else
						GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
								&obj->reg->ies[mt_ies_smt_map[i].reg_index].set); 
						}
				}
			i++;
			if(i == 23)
				{
				/*only set MSDC1 ies */
				  if(pin == GPIO121){
					
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc1_ctrl[1].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc1_ctrl[1].set);
					   }
				  }else if(pin == GPIO122){
					
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc1_ctrl[0].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc1_ctrl[0].set);
					   }
				  }else if(pin == GPIO123){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc1_ctrl[2].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc1_ctrl[2].set);
					   }
				  }else if(pin == GPIO124){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 7, &obj->reg->msdc1_ctrl[2].rst);
					   } else {
						   GPIO_SET_BITS(1L << 7, &obj->reg->msdc1_ctrl[2].set);
					   }
				  }else if(pin == GPIO125){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 11, &obj->reg->msdc1_ctrl[2].rst);
					   } else {
						   GPIO_SET_BITS(1L << 11, &obj->reg->msdc1_ctrl[2].set);
					   }
				  }else if(pin == GPIO126){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 15, &obj->reg->msdc1_ctrl[2].rst);
					   } else {
						   GPIO_SET_BITS(1L << 15, &obj->reg->msdc1_ctrl[2].set);
					   }
				  }
				 i++;
				  
				}
			if(i == 24)
				{
				/*only set MSDC0 ies*/
				  if(pin >= GPIO127 && pin <= GPIO131){
					
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[2].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[2].set);
					   }
				  } else if(pin == GPIO132){			
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[1].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[1].set);
					   }
				  }else if(pin == GPIO133){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[0].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[0].set);
					   }
				  }else if(pin >= GPIO134 && pin <= GPIO137){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[2].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[2].set);
					   }
				  }
				 i++;
				  
				}
			for (;i < 27; i++ ) {
					if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
						if (enable == GPIO_IES_DISABLE)
							GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
									   &obj->reg->ies[mt_ies_smt_map[i].reg_index].rst);
						else
							GPIO_SET_BITS((1L << mt_ies_smt_map[i].bit),
									   &obj->reg->ies[mt_ies_smt_map[i].reg_index].set); 
						}
				}
			if(i == 27)
				{
				/*only set smt,ies not set*/
				  if(pin >= GPIO143 && pin <= GPIO147){
					
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc3_ctrl[2].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc3_ctrl[2].set);
					   }
				  }else if(pin == GPIO148){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc3_ctrl[1].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc3_ctrl[1].set);
					   }
				  }else if(pin == GPIO149){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc3_ctrl[0].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc3_ctrl[0].set);
					   }
				  }else if(pin >= GPIO150 && pin <= GPIO153){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc3_ctrl[2].rst);
					   } else {
						   GPIO_SET_BITS(1L << 4, &obj->reg->msdc3_ctrl[2].set);
					   }
				  }else if(pin == GPIO154){ 		
					   if (enable == GPIO_IES_DISABLE) {
						   GPIO_SET_BITS(1L << 7, &obj->reg->msdc3_ctrl[2].rst);
					   } else {
						   GPIO_SET_BITS(1L << 7, &obj->reg->msdc3_ctrl[2].set);
					   }
				  } 	  
				  
				}		

	if (i > ARRAY_SIZE(mt_ies_smt_map)) {
		return -ERINVAL;
	}
	return RSUCCESS;


}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_ies_chip(u32 pin)
{
    int i = 0;
	unsigned long data;
	struct mt_gpio_obj *obj= gpio_obj;

	for (i = 0; i < 19; i++) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
					    data = GPIO_RD32(&obj->reg->ies[mt_ies_smt_map[i].reg_index].val);
	                    return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
					}
			}
		
	i=i+4;/*i=19~21,similar to TDP/RDP/.../,ies not uesd*/
	if (i == 22) {
				if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
						data = GPIO_RD32(&obj->reg->ies[mt_ies_smt_map[i].reg_index].val);
	                    return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
					}
			}
	i++;			
	if(i == 23){/*only set MSDC1 ies */
			          if(pin == GPIO121){
						return (((GPIO_RD32(&obj->reg->msdc1_ctrl[1].val) & (1L << 4)) != 0) ? 1 : 0);						
					  }else if(pin == GPIO122){
						return (((GPIO_RD32(&obj->reg->msdc1_ctrl[0].val) & (1L << 4)) != 0) ? 1 : 0);						   
					  }else if(pin == GPIO123){ 
					    return (((GPIO_RD32(&obj->reg->msdc1_ctrl[2].val) & (1L << 4)) != 0) ? 1 : 0);						  
					  }else if(pin == GPIO124){ 
					    return (((GPIO_RD32(&obj->reg->msdc1_ctrl[2].val) & (1L << 7)) != 0) ? 1 : 0);						   
					  }else if(pin == GPIO125){
					    return (((GPIO_RD32(&obj->reg->msdc1_ctrl[2].val) & (1L << 11)) != 0) ? 1 : 0);						   
					  }else if(pin == GPIO126){ 
					    return (((GPIO_RD32(&obj->reg->msdc1_ctrl[2].val) & (1L << 15)) != 0) ? 1 : 0);						   
					  }
					 i++;
					  
					}
				if(i == 24)
					{
					/*only set MSDC0 ies*/
					  if(pin >= GPIO127 && pin <= GPIO131){
					  	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[2].val) & (1L << 4)) != 0) ? 1 : 0);						  
					  } else if(pin == GPIO132){
					       return (((GPIO_RD32(&obj->reg->msdc0_ctrl[1].val) & (1L << 4)) != 0) ? 1 : 0);						  
					  }else if(pin == GPIO133){ 							  
						   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[0].val) & (1L << 4)) != 0) ? 1 : 0);
					  }else if(pin >= GPIO134 && pin <= GPIO137){						   
						   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[2].val) & (1L << 4)) != 0) ? 1 : 0);
					  }
					 i++;
					  
					}
				for (;i < 27; i++ ) {
						if (pin >= mt_ies_smt_map[i].index_start && pin <= mt_ies_smt_map[i].index_end){
							data = GPIO_RD32(&obj->reg->ies[mt_ies_smt_map[i].reg_index].val);
	                        return (((data & (1L << mt_ies_smt_map[i].bit)) != 0) ? 1 : 0);
							}
					}
				if(i == 27)
					{
					/*only set smt,ies not set*/
					  if(pin >= GPIO143 && pin <= GPIO147){						  
						   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[2].val) & (1L << 4)) != 0) ? 1 : 0);
					  }else if(pin == GPIO148){ 						  
						   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[1].val) & (1L << 4)) != 0) ? 1 : 0);
					  }else if(pin == GPIO149){ 						   
						   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[0].val) & (1L << 4)) != 0) ? 1 : 0);
					  }else if(pin >= GPIO150 && pin <= GPIO153){								  
						   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[2].val) & (1L << 4)) != 0) ? 1 : 0);
					  }else if(pin == GPIO154){ 		
						   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[2].val) & (1L << 7)) != 0) ? 1 : 0);
						 
					  } 	  
					  
					}	
	if (i >= ARRAY_SIZE(mt_ies_smt_map)) {
		return -ERINVAL;
	}



}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_select_chip(u32 pin, u32 select)
{ 

    unsigned long pos;
	unsigned long bit;
	unsigned long i;
	struct mt_gpio_obj *obj = gpio_obj;
	if (!obj)
			return -ERACCESS;
	
	if (pin >= MAX_GPIO_PIN)
			return -ERINVAL;	
	
	if (select >= GPIO_PULL_MAX)
			return -ERINVAL;

	
	/***********************for special kpad pupd, NOTE DEFINITION REVERSE!!!**************************/
		for (i = 0; i < sizeof(kpad_pupd_spec) / sizeof(kpad_pupd_spec[0]); i++) {
			if (pin == kpad_pupd_spec[i].pin) {
				if (select == GPIO_PULL_DOWN)
					GPIO_SET_BITS((1L << kpad_pupd_spec[i].bit),
							  &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].set);
				else
					GPIO_SET_BITS((1L << kpad_pupd_spec[i].bit),
							  &obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].rst);
				return RSUCCESS;
			}
		}


   	/********************************* MSDC special *********************************/
	if (pin == GPIO127) {	/* ms0 dat7 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 12, &obj->reg->msdc0_ctrl[4].rst);
		} else {
			GPIO_SET_BITS(1L << 12, &obj->reg->msdc0_ctrl[4].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO128) {	/* ms0 dat6 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc0_ctrl[4].rst);
		} else {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc0_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO129) {	/* ms0 dat5*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[4].rst);
		} else {
			GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO130) {	/* ms0 dat4*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 0, &obj->reg->msdc0_ctrl[4].rst);
		} else {
			GPIO_SET_BITS(1L << 0, &obj->reg->msdc0_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO131) {	/* ms0 RST */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 0, &obj->reg->msdc0_ctrl[5].rst);
		} else {
			GPIO_SET_BITS(1L << 0, &obj->reg->msdc0_ctrl[5].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO132) {	/* ms0 cmd */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc0_ctrl[1].rst);
		} else {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc0_ctrl[1].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO133) {	/* ms0 clk */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc0_ctrl[0].rst);
		} else {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc0_ctrl[0].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO134) {	/* ms0 dat3 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 12, &obj->reg->msdc0_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 12, &obj->reg->msdc0_ctrl[3].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO135) {	/* ms0 dat2 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc0_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc0_ctrl[3].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO136) {	/* ms0 dat1*/
		if (select== GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 4, &obj->reg->msdc0_ctrl[3].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO137) {	/* ms0 dat0 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 0, &obj->reg->msdc0_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 0, &obj->reg->msdc0_ctrl[3].set);
		}
		return RSUCCESS;
	
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO121) {	/* ms1 cmd */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc1_ctrl[1].rst);
		} else {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc1_ctrl[1].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO122) {	/* ms1 clk */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc1_ctrl[0].rst);
		} else {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc1_ctrl[0].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO123) {	/* ms1 dat0 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 0, &obj->reg->msdc1_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 0, &obj->reg->msdc1_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO124) {	/* ms1 dat1 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 4), &obj->reg->msdc1_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 4), &obj->reg->msdc1_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO125) {	/* ms1 dat2 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 8), &obj->reg->msdc1_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 8), &obj->reg->msdc1_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO126) {	/* ms1 dat3 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 12), &obj->reg->msdc1_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 12), &obj->reg->msdc1_ctrl[3].set);
		}
		return RSUCCESS;
	 
		/* //////////////////////////////////////////////// */
	} else if (pin == GPIO85) {	/* ms2 cmd */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc2_ctrl[1].rst);
		} else {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc2_ctrl[1].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO86) {	/* ms2 clk */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc2_ctrl[0].rst);
		} else {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc2_ctrl[0].set);
		}
		return RSUCCESS;	
	}else if (pin == GPIO87) {	/* ms2 dat0 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 0, &obj->reg->msdc2_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 0, &obj->reg->msdc2_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO88) {	/* ms2 dat1 */
		if (select == GPIO_PULL_DISABLE) {
			GPIO_SET_BITS((1L << 4), &obj->reg->msdc2_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 4), &obj->reg->msdc2_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO89) {	/* ms2 dat2 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 8), &obj->reg->msdc2_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 8), &obj->reg->msdc2_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO90) {	/* ms2 dat3 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 12), &obj->reg->msdc2_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 12), &obj->reg->msdc2_ctrl[3].set);
		}
		return RSUCCESS;
	
		/* //////////////////////////////////////////////// */
	}else if (pin == GPIO143) {	/* ms3 dat7*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 12, &obj->reg->msdc3_ctrl[4].rst);
		} else {
			GPIO_SET_BITS(1L << 12, &obj->reg->msdc3_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO144) {/* ms3 dat6 */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 8), &obj->reg->msdc3_ctrl[4].rst);
		} else {
			GPIO_SET_BITS((1L << 8), &obj->reg->msdc3_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO145) {/* ms3 dat5*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 4), &obj->reg->msdc3_ctrl[4].rst);
		} else {
			GPIO_SET_BITS((1L << 4), &obj->reg->msdc3_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO146) {/* ms3 dat4*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 0), &obj->reg->msdc3_ctrl[4].rst);
		} else {
			GPIO_SET_BITS((1L << 0), &obj->reg->msdc3_ctrl[4].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO147) {/* ms3 RST */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(3L<<1, &obj->reg->msdc3_ctrl[5].rst);
		} else {
			GPIO_SET_BITS(1L<<2, &obj->reg->msdc3_ctrl[5].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO148) {/* ms3 cmd */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc3_ctrl[1].rst);
		} else {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc3_ctrl[1].set);
		}
		return RSUCCESS;
	}else if (pin == GPIO149) {	/* ms3 clk */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc3_ctrl[0].rst);
		} else {
			GPIO_SET_BITS(1L << 8, &obj->reg->msdc3_ctrl[0].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO150) {/* ms3 dat3*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 12, &obj->reg->msdc3_ctrl[3].rst);
		} else {
			GPIO_SET_BITS(1L << 12, &obj->reg->msdc3_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO151) {/* ms3 dat2 */
		if (select== GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 8), &obj->reg->msdc3_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 8), &obj->reg->msdc3_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO152) {/* ms3 dat1*/
		if (select ==GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 4), &obj->reg->msdc3_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 4), &obj->reg->msdc3_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO153) {/* ms3 dat0*/
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS((1L << 0), &obj->reg->msdc3_ctrl[3].rst);
		} else {
			GPIO_SET_BITS((1L << 0), &obj->reg->msdc3_ctrl[3].set);
		}
		return RSUCCESS;
	} else if (pin == GPIO154) {/* ms3 DS */
		if (select == GPIO_PULL_UP) {
			GPIO_SET_BITS(1L << 4, &obj->reg->msdc3_ctrl[5].rst);
		} else {
			GPIO_SET_BITS(1L << 4, &obj->reg->msdc3_ctrl[5].set);	/* 1L:10K */
		}
		return RSUCCESS;
	}

	if (0) {
		return GPIO_PULL_EN_UNSUPPORTED;
	} else {
		pos = pin / MAX_GPIO_REG_BITS;
		bit = pin % MAX_GPIO_REG_BITS;

		if (select == GPIO_PULL_DOWN)
			GPIO_SET_BITS((1L << bit), &obj->reg->pullsel[pos].rst);
		else
			GPIO_SET_BITS((1L << bit), &obj->reg->pullsel[pos].set);
	}
	return RSUCCESS;


}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_select_chip(u32 pin)
{
   unsigned long pos;
   unsigned long bit;
   unsigned long data;
   unsigned long i;
   struct mt_gpio_obj *obj = gpio_obj;

   if (pin >= MAX_GPIO_PIN)
      return -ERINVAL;


   /*********************************for special kpad pupd*********************************/
   for (i = 0; i < sizeof(kpad_pupd_spec) / sizeof(kpad_pupd_spec[0]); i++) {
	   if (pin == kpad_pupd_spec[i].pin) {
		   data = GPIO_RD32(&obj->reg->kpad_ctrl[kpad_pupd_spec[i].reg].val);
		   return (((data & (1L << kpad_pupd_spec[i].bit)) != 0) ? 0 : 1);
	   }
   }

   /********************************* MSDC special *********************************/
   if (pin == GPIO127) {   /* ms0 dat7 */	   
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (1L << 12)) != 0) ? 1 : 0);
   }else if (pin == GPIO128) { /* ms0 dat6 */
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (1L << 8)) != 0) ? 1 : 0);
   } else if (pin == GPIO129) {    /* ms0 dat5*/
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (1L << 4)) != 0) ? 1 : 0);
   } else if (pin == GPIO130) {    /* ms0 dat4*/
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[4].val) & (1L << 0)) != 0) ? 1 : 0);
   } else if (pin == GPIO131) {    /* ms0 RST */
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[5].val) & (1L << 0)) != 0) ? 1 : 0);
   } else if (pin == GPIO132) {    /* ms0 cmd */	   
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[1].val) & (1L << 8)) != 0) ? 1 : 0);
   } else if (pin == GPIO133) {    /* ms0 clk */	   
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[0].val) & (1L << 8)) != 0) ? 1 : 0);
   } else if (pin == GPIO134) {    /* ms0 dat3 */	   
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (1L << 12)) != 0) ? 1 : 0);
   }else if (pin == GPIO135) { /* ms0 dat2 */
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (1L << 8)) != 0) ? 1 : 0);
   }else if (pin == GPIO136) { /* ms0 dat1*/	   
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (1L << 4)) != 0) ? 1 : 0);
   }else if (pin == GPIO137) { /* ms0 dat0 */	   
	   return (((GPIO_RD32(&obj->reg->msdc0_ctrl[3].val) & (1L << 0)) != 0) ? 1 : 0);
   
	   /* //////////////////////////////////////////////// */
   } else if (pin == GPIO121) {    /* ms1 cmd */	   
	   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[1].val) & (1L << 8)) != 0) ? 1 : 0);
   } else if (pin == GPIO122) {    /* ms1 clk */   
	   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[0].val) & (1L << 8)) != 0) ? 1 : 0);
   }else if (pin == GPIO123) { /* ms1 dat0 */	   
	   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (1L << 0)) != 0) ? 1 : 0);
   } else if (pin == GPIO124) {    /* ms1 dat1 */	   
	   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (1L << 4)) != 0) ? 1 : 0);
   } else if (pin == GPIO125) {    /* ms1 dat2 */	   
	   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (1L << 8)) != 0) ? 1 : 0);
   } else if (pin == GPIO126) {    /* ms1 dat3 */	   
	   return (((GPIO_RD32(&obj->reg->msdc1_ctrl[3].val) & (1L << 12)) != 0) ? 1 : 0);
	
	   /* //////////////////////////////////////////////// */
   } else if (pin == GPIO85) { /* ms2 cmd */	   
	   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[1].val) & (1L << 8)) != 0) ? 1 : 0);
   } else if (pin == GPIO86) { /* ms2 clk */
	   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[0].val) & (1L << 8)) != 0) ? 1 : 0);
   }else if (pin == GPIO87) {  /* ms2 dat0 */
	   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (1L << 0)) != 0) ? 1 : 0);
   } else if (pin == GPIO88) { /* ms2 dat1 */
	   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (1L << 4)) != 0) ? 1 : 0);
   } else if (pin == GPIO89) { /* ms2 dat2 */
	   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (1L << 8)) != 0) ? 1 : 0);
   } else if (pin == GPIO90) { /* ms2 dat3 */
	   return (((GPIO_RD32(&obj->reg->msdc2_ctrl[3].val) & (1L << 12)) != 0) ? 1 : 0);
   
	   /* //////////////////////////////////////////////// */
   }else if (pin == GPIO143) { /* ms3 dat7*/
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (1L << 12)) != 0) ? 1 : 0);
   } else if (pin == GPIO144) {/* ms3 dat6 */
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (1L << 8)) != 0) ? 1 : 0);
   } else if (pin == GPIO145) {/* ms3 dat5*/	   
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (1L << 4)) != 0) ? 1 : 0);
   } else if (pin == GPIO146) {/* ms3 dat4*/	   
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[4].val) & (1L << 0)) != 0) ? 1 : 0);
   } else if (pin == GPIO147) {/* ms3 RST */
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[5].val) & (1L << 0)) != 0) ? 1 : 0);
   } else if (pin == GPIO148) {/* ms3 cmd */	   
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[1].val) & (1L << 8)) != 0) ? 1 : 0);
   }else if (pin == GPIO149) { /* ms3 clk */	   
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[0].val) & (1L << 8)) != 0) ? 1 : 0);
   } else if (pin == GPIO150) {/* ms3 dat3*/	   
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (1L << 12)) != 0) ? 1 : 0);
   } else if (pin == GPIO151) {/* ms3 dat2 */	   
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (1L << 8)) != 0) ? 1 : 0);
   } else if (pin == GPIO152) {/* ms3 dat1*/	   
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (1L << 4)) != 0) ? 1 : 0);
   } else if (pin == GPIO153) {/* ms3 dat0*/	   
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[3].val) & (1L << 0)) != 0) ? 1 : 0);
   } else if (pin == GPIO154) {/* ms3 DS */
	   return (((GPIO_RD32(&obj->reg->msdc3_ctrl[5].val) & (1L << 4)) != 0) ? 1 : 0);
   }

   if (0) {
	   return GPIO_PULL_EN_UNSUPPORTED;
   } else {
	   pos = pin / MAX_GPIO_REG_BITS;
	   bit = pin % MAX_GPIO_REG_BITS;
	   data = GPIO_RD32(&obj->reg->pullsel[pos].val);
   }
   return (((data & (1L << bit)) != 0) ? 1 : 0);

}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_out_chip(u32 pin, u32 output)
{

    u32 pos;
    u32 bit;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (output >= GPIO_OUT_MAX)
        return -ERINVAL;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    if (output == GPIO_OUT_ZERO)
        GPIO_SET_BITS((1L << bit), &obj->reg->dout[pos].rst);
    else
        GPIO_SET_BITS((1L << bit), &obj->reg->dout[pos].set);
    return RSUCCESS;



}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_out_chip(u32 pin)
{
    u32 pos;
    u32 bit;
    u32 reg;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIO_RD32(&obj->reg->dout[pos].val);
    return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_in_chip(u32 pin)
{
    u32 pos;
    u32 bit;
    u32 reg;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIO_RD32(&obj->reg->din[pos].val);
    return (((reg & (1L << bit)) != 0)? 1: 0);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_mode_chip(u32 pin, u32 mode)
{
    u32 pos;
    u32 bit;
    u32 reg;
    u32 mask = (1L << GPIO_MODE_BITS) - 1;
   struct mt_gpio_obj *obj = gpio_obj;



    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    if (mode >= GPIO_MODE_MAX)
        return -ERINVAL;

	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;


	reg = GPIO_RD32(&obj->reg->mode[pos].val);

	reg &= ~(mask << (GPIO_MODE_BITS*bit));
	reg |= (mode << (GPIO_MODE_BITS*bit));

	GPIO_WR32(&obj->reg->mode[pos].val, reg);

#if 0
    reg = ((1L << (GPIO_MODE_BITS*bit)) << 3) | (mode << (GPIO_MODE_BITS*bit));

    GPIO_WR32(&obj->reg->mode[pos]._align1, reg);
#endif

    return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_mode_chip(u32 pin)
{
    u32 pos;
    u32 bit;
    u32 reg;
    u32 mask = (1L << GPIO_MODE_BITS) - 1;
    struct mt_gpio_obj *obj = gpio_obj;

    if (!obj)
        return -ERACCESS;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;

	reg = GPIO_RD32(&obj->reg->mode[pos].val);

	return ((reg >> (GPIO_MODE_BITS*bit)) & mask);
}
/*---------------------------------------------------------------------------*/
void mt_gpio_pin_decrypt(u32 *cipher)
{
	//just for debug, find out who used pin number directly
	if((*cipher & (0x80000000)) == 0){
		GPIOERR("GPIO%u HARDCODE warning!!! \n",(unsigned int)*cipher);	
		//dump_stack();
		//return;
	}

	//GPIOERR("Pin magic number is %x\n",*cipher);
	*cipher &= ~(0x80000000);
	return;
}

S32 mt_set_msdc_r0_r1_value(u32 pin, u32 value)
{
	int i = 0, j = 0;
	struct mt_gpio_obj *obj = gpio_obj;
	if (!(((pin >= GPIO85)&&(pin <= GPIO90))||((pin >= GPIO121)&&(pin <= GPIO126))||((pin >= GPIO127)&&(pin <= GPIO137))||((pin >= GPIO143)&&(pin <= GPIO154)))) {
		return -ERINVAL;
	}

	if(value < MSDC_PIN_R0_R1_00 || value > MSDC_PIN_R0_R1_03) {
		return -ERINVAL;
	}

	for(i = 0; i < sizeof(msdc_pupd_spec)/sizeof(msdc_pupd_spec[0]); i++){
		for(j = 0; j < sizeof(msdc_pupd_spec[0])/sizeof(msdc_pupd_spec[0][0]); j++){
			if (pin == msdc_pupd_spec[i][j].pin){
				if (i == 0) {
					switch(value) {
					case MSDC_PIN_R0_R1_00:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc1_ctrl[msdc_pupd_spec[i][j].reg].rst);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc1_ctrl[msdc_pupd_spec[i][j].reg].rst);
						break;

					case MSDC_PIN_R0_R1_01:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc1_ctrl[msdc_pupd_spec[i][j].reg].set);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc1_ctrl[msdc_pupd_spec[i][j].reg].rst);
						break;
					case MSDC_PIN_R0_R1_02:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc1_ctrl[msdc_pupd_spec[i][j].reg].rst);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc1_ctrl[msdc_pupd_spec[i][j].reg].set);
						break;

					case MSDC_PIN_R0_R1_03:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc1_ctrl[msdc_pupd_spec[i][j].reg].set);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc1_ctrl[msdc_pupd_spec[i][j].reg].set);
						break;
					}
				} else if (i == 1) {
					switch(value) {
					case MSDC_PIN_R0_R1_00:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						break;

					case MSDC_PIN_R0_R1_01:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						break;
					case MSDC_PIN_R0_R1_02:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						break;

					case MSDC_PIN_R0_R1_03:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						break;
					}
				}	
             #if 0
				else if (i == 2) {
					switch(value) {
					case MSDC_PIN_R0_R1_00:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						break;
	
					case MSDC_PIN_R0_R1_01:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						break;
					case MSDC_PIN_R0_R1_02:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						break;
	
					case MSDC_PIN_R0_R1_03:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						break;
					}
				}else if (i == 3)  {
					switch(value) {
					case MSDC_PIN_R0_R1_00:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						break;
	
					case MSDC_PIN_R0_R1_01:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						break;
					case MSDC_PIN_R0_R1_02:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].rst);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						break;
	
					case MSDC_PIN_R0_R1_03:
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 1)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						GPIO_SET_BITS((1L << (msdc_pupd_spec[i][j].bit + 2)), &obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].set);
						break;
					}
				}

              #endif
				break;
			}
		}
	}

	return RSUCCESS;
}

S32 mt_get_msdc_r0_r1_value(u32 pin)
{
	int i = 0, j = 0;
	u32 reg = 0;
	struct mt_gpio_obj *obj = gpio_obj;
	if (!(((pin >= GPIO85)&&(pin <= GPIO90))||((pin >= GPIO121)&&(pin <= GPIO126)))) {
		return -ERINVAL;
	}

	for(i = 0; i < sizeof(msdc_pupd_spec)/sizeof(msdc_pupd_spec[0]); i++){
		for(j = 0; j < sizeof(msdc_pupd_spec[0])/sizeof(msdc_pupd_spec[0][0]); j++){
			if (pin == msdc_pupd_spec[i][j].pin){
				if (i == 0) {
					reg = GPIO_RD32(&obj->reg->msdc1_ctrl[msdc_pupd_spec[i][j].reg].val);
				} else if (i == 1) {
					reg = GPIO_RD32(&obj->reg->msdc2_ctrl[msdc_pupd_spec[i][j].reg].val);
				}

				return (reg & (3L << (msdc_pupd_spec[i][j].bit + 1))) >> (msdc_pupd_spec[i][j].bit + 1);
			}
		}
	}
}




//set GPIO function in fact
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_dir(u32 pin, u32 dir)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_set_gpio_dir_chip(pin,dir);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_dir(u32 pin)
{    
	mt_gpio_pin_decrypt(&pin);
    return mt_get_gpio_dir_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_enable(u32 pin, u32 enable)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_set_gpio_pull_enable_chip(pin,enable);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_enable(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_get_gpio_pull_enable_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_pull_select(u32 pin, u32 select)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_set_gpio_pull_select_chip(pin,select);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_pull_select(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_get_gpio_pull_select_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_smt(u32 pin, u32 enable)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_set_gpio_smt_chip(pin,enable);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_smt(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_get_gpio_smt_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_ies(u32 pin, u32 enable)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_set_gpio_ies_chip(pin,enable);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_ies(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_get_gpio_ies_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_out(u32 pin, u32 output)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_set_gpio_out_chip(pin,output);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_out(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_get_gpio_out_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_in(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_get_gpio_in_chip(pin);
}
/*---------------------------------------------------------------------------*/
S32 mt_set_gpio_mode(u32 pin, u32 mode)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_set_gpio_mode_chip(pin,mode);
}
/*---------------------------------------------------------------------------*/
S32 mt_get_gpio_mode(u32 pin)
{
	mt_gpio_pin_decrypt(&pin);
    return mt_get_gpio_mode_chip(pin);
}
/*****************************************************************************/
/* sysfs operation                                                           */
/*****************************************************************************/
#if 0
void mt_gpio_self_test(void)
{
    int i, val;
    for (i = 0; i < GPIO_MAX; i++)
    {
        S32 res,old;
        GPIOMSG("GPIO-%3d test\n", i);
        /*direction test*/
        old = mt_get_gpio_dir(i);
        if (old == 0 || old == 1) {
            GPIOLOG(" dir old = %d\n", old);
        } else {
            GPIOERR(" test dir fail: %d\n", old);
            break;
        }
        if ((res = mt_set_gpio_dir(i, GPIO_DIR_OUT)) != RSUCCESS) {
            GPIOERR(" set dir out fail: %d\n", res);
            break;
        } else if ((res = mt_get_gpio_dir(i)) != GPIO_DIR_OUT) {
            GPIOERR(" get dir out fail: %d\n", res);
            break;
        } else {
            /*output test*/
            S32 out = mt_get_gpio_out(i);
            if (out != 0 && out != 1) {
                GPIOERR(" get out fail = %d\n", old);
                break;
            } 
            for (val = 0; val < GPIO_OUT_MAX; val++) {
                if ((res = mt_set_gpio_out(i,0)) != RSUCCESS) {
                    GPIOERR(" set out[%d] fail: %d\n", val, res);
                    break;
                } else if ((res = mt_get_gpio_out(i)) != 0) {
                    GPIOERR(" get out[%d] fail: %d\n", val, res);
                    break;
                }
            }
            if ((res = mt_set_gpio_out(i,out)) != RSUCCESS)
            {
                GPIOERR(" restore out fail: %d\n", res);
                break;
            }
        }
            
        if ((res = mt_set_gpio_dir(i, GPIO_DIR_IN)) != RSUCCESS) {
            GPIOERR(" set dir in fail: %d\n", res);
            break;
        } else if ((res = mt_get_gpio_dir(i)) != GPIO_DIR_IN) {
            GPIOERR(" get dir in fail: %d\n", res);
            break;
        } else {
            GPIOLOG(" input data = %d\n", res);
        }
        
        if ((res = mt_set_gpio_dir(i, old)) != RSUCCESS) {
            GPIOERR(" restore dir fail: %d\n", res);
            break;
        }
        for (val = 0; val < GPIO_PULL_EN_MAX; val++) {
            if ((res = mt_set_gpio_pull_enable(i,val)) != RSUCCESS) {
                GPIOERR(" set pullen[%d] fail: %d\n", val, res);
                break;
            } else if ((res = mt_get_gpio_pull_enable(i)) != val) {
                GPIOERR(" get pullen[%d] fail: %d\n", val, res);
                break;
            }
        }        
        if ((res = mt_set_gpio_pull_enable(i, old)) != RSUCCESS) {
            GPIOERR(" restore pullen fail: %d\n", res);
            break;
        }

        /*pull select test*/
        old = mt_get_gpio_pull_select(i);
        if (old == 0 || old == 1)
            GPIOLOG(" pullsel old = %d\n", old);
        else {
            GPIOERR(" pullsel fail: %d\n", old);
            break;
        }
        for (val = 0; val < GPIO_PULL_MAX; val++) {
            if ((res = mt_set_gpio_pull_select(i,val)) != RSUCCESS) {
                GPIOERR(" set pullsel[%d] fail: %d\n", val, res);
                break;
            } else if ((res = mt_get_gpio_pull_select(i)) != val) {
                GPIOERR(" get pullsel[%d] fail: %d\n", val, res);
                break;
            }
        } 
        if ((res = mt_set_gpio_pull_select(i, old)) != RSUCCESS)
        {
            GPIOERR(" restore pullsel fail: %d\n", res);
            break;
        }     

        /*mode control*/
		old = mt_get_gpio_mode(i);
		if ((old >= GPIO_MODE_00) && (val < GPIO_MODE_MAX)){
			GPIOLOG(" mode old = %d\n", old);
		}
		else{
			GPIOERR(" get mode fail: %d\n", old);
			break;
		}
		for (val = 0; val < GPIO_MODE_MAX; val++) {
			if ((res = mt_set_gpio_mode(i, val)) != RSUCCESS) {
				GPIOERR("set mode[%d] fail: %d\n", val, res);
				break;
			} else if ((res = mt_get_gpio_mode(i)) != val) {
				GPIOERR("get mode[%d] fail: %d\n", val, res);
				break;
			}            
		}        
		if ((res = mt_set_gpio_mode(i,old)) != RSUCCESS) {
			GPIOERR(" restore mode fail: %d\n", res);
			break;
		}      
    }
    GPIOLOG("GPIO test done\n");
}
/*----------------------------------------------------------------------------*/
void mt_gpio_dump(void) 
{
    GPIO_REGS *regs = (GPIO_REGS*)(GPIO_BASE);
    int idx; 

    GPIOMSG("---# dir #-----------------------------------------------------------------\n");
    for (idx = 0; idx < sizeof(regs->dir)/sizeof(regs->dir[0]); idx++) {
        GPIOMSG("0x%04X ", regs->dir[idx].val);
        if (7 == (idx % 8)) GPIOMSG("\n");
    }
    GPIOMSG("\n---# pullen #--------------------------------------------------------------\n");        
    for (idx = 0; idx < sizeof(regs->pullen)/sizeof(regs->pullen[0]); idx++) {
        GPIOMSG("0x%04X ", regs->pullen[idx].val);    
        if (7 == (idx % 8)) GPIOMSG("\n");
    }
    GPIOMSG("\n---# pullsel #-------------------------------------------------------------\n");   
    for (idx = 0; idx < sizeof(regs->pullsel)/sizeof(regs->pullsel[0]); idx++) {
        GPIOMSG("0x%04X ", regs->pullsel[idx].val);     
        if (7 == (idx % 8)) GPIOMSG("\n");
    }
    GPIOMSG("\n---# dout #----------------------------------------------------------------\n");   
    for (idx = 0; idx < sizeof(regs->dout)/sizeof(regs->dout[0]); idx++) {
        GPIOMSG("0x%04X ", regs->dout[idx].val);     
        if (7 == (idx % 8)) GPIOMSG("\n");
    }
    GPIOMSG("\n---# din  #----------------------------------------------------------------\n");   
    for (idx = 0; idx < sizeof(regs->din)/sizeof(regs->din[0]); idx++) {
        GPIOMSG("0x%04X ", regs->din[idx].val);     
        if (7 == (idx % 8)) GPIOMSG("\n");
    }
    GPIOMSG("\n---# mode #----------------------------------------------------------------\n");   
    for (idx = 0; idx < sizeof(regs->mode)/sizeof(regs->mode[0]); idx++) {
        GPIOMSG("0x%04X ", regs->mode[idx].val);     
        if (7 == (idx % 8)) GPIOMSG("\n");
    } 
	GPIOMSG("sim0 0x%04X ", regs->sim_ctrl[0].val);
	GPIOMSG("sim1 0x%04X ", regs->sim_ctrl[1].val);
	GPIOMSG("sim2 0x%04X ", regs->sim_ctrl[2].val);
	GPIOMSG("sim3 0x%04X ", regs->sim_ctrl[3].val);

	GPIOMSG("keypad0 0x%04X ", regs->kpad_ctrl[0].val);
	GPIOMSG("keypad1 0x%04X ", regs->kpad_ctrl[1].val);

    GPIOMSG("\n---------------------------------------------------------------------------\n");    
}
/*---------------------------------------------------------------------------*/
void mt_gpio_read_pin(GPIO_CFG* cfg, int method)
{
    if (method == 0) {
        GPIO_REGS *cur = (GPIO_REGS*)GPIO_BASE;    
        u32 mask = (1L << GPIO_MODE_BITS) - 1;        
        int num, bit,idx=cfg->no; 
		num = idx / MAX_GPIO_REG_BITS;
		bit = idx % MAX_GPIO_REG_BITS;
		cfg->pullsel= (cur->pullsel[num].val & (1L << bit)) ? (1) : (0);
		cfg->din    = (cur->din[num].val & (1L << bit)) ? (1) : (0);
		cfg->dout   = (cur->dout[num].val & (1L << bit)) ? (1) : (0);
		cfg->pullen = (cur->pullen[num].val & (1L << bit)) ? (1) : (0);
		cfg->dir    = (cur->dir[num].val & (1L << bit)) ? (1) : (0);
		num = idx / MAX_GPIO_MODE_PER_REG;        
		bit = idx % MAX_GPIO_MODE_PER_REG;
		cfg->mode   = (cur->mode[num].val >> (GPIO_MODE_BITS*bit)) & mask;
    } else if (method == 1) {
		int idx=cfg->no; 
        cfg->pullsel= mt_get_gpio_pull_select(idx);
        cfg->din    = mt_get_gpio_in(idx);
        cfg->dout   = mt_get_gpio_out(idx);
        cfg->pullen = mt_get_gpio_pull_enable(idx);
        cfg->dir    = mt_get_gpio_dir(idx);
        cfg->mode   = mt_get_gpio_mode(idx);
    }
}
/*---------------------------------------------------------------------------*/
void mt_gpio_dump_addr(void)
{
    int idx;
    struct mt_gpio_obj *obj = gpio_obj;
    GPIO_REGS *reg = obj->reg;

    GPIOMSG("# direction\n");
    for (idx = 0; idx < sizeof(reg->dir)/sizeof(reg->dir[0]); idx++)
        GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->dir[idx].val, idx, &reg->dir[idx].set, idx, &reg->dir[idx].rst);
    GPIOMSG("# pull enable\n");
    for (idx = 0; idx < sizeof(reg->pullen)/sizeof(reg->pullen[0]); idx++)
        GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->pullen[idx].val, idx, &reg->pullen[idx].set, idx, &reg->pullen[idx].rst);
    GPIOMSG("# pull select\n");
    for (idx = 0; idx < sizeof(reg->pullsel)/sizeof(reg->pullsel[0]); idx++)
        GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->pullsel[idx].val, idx, &reg->pullsel[idx].set, idx, &reg->pullsel[idx].rst);
    GPIOMSG("# data output\n");
    for (idx = 0; idx < sizeof(reg->dout)/sizeof(reg->dout[0]); idx++)
        GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->dout[idx].val, idx, &reg->dout[idx].set, idx, &reg->dout[idx].rst);
    GPIOMSG("# data input\n");
    for (idx = 0; idx < sizeof(reg->din)/sizeof(reg->din[0]); idx++)
        GPIOMSG("val[%2d] %p\n", idx, &reg->din[idx].val);
    GPIOMSG("# mode\n");
    for (idx = 0; idx < sizeof(reg->mode)/sizeof(reg->mode[0]); idx++)
        GPIOMSG("val[%2d] %p\nset[%2d] %p\nrst[%2d] %p\n", idx, &reg->mode[idx].val, idx, &reg->mode[idx].set, idx, &reg->mode[idx].rst);    
}
#endif
#endif
