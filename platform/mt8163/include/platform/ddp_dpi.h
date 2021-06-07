#ifndef __DDP_DPI_H__
#define __DDP_DPI_H__



#include "lcm_drv.h"


#ifdef __cplusplus
extern "C" {
#endif

#define DPI_CHECK_RET(expr)             \
    do {                                \
        DPI_STATUS ret = (expr);        \
        ASSERT(DPI_STATUS_OK == ret);   \
    } while (0)

#define DPI_MASKREG32(cmdq, REG, MASK, VALUE)	DISP_REG_MASK((cmdq), (REG), (VALUE), (MASK));
    
#define DPI_OUTREGBIT(cmdq, TYPE, REG, bit, value)  \
        do {\
            TYPE r;\
            TYPE v;\
            if (cmdq)       {*(unsigned int*)(&r) = ((unsigned int)0x00000000); r.bit = ~(r.bit);  *(unsigned int*)(&v) = ((unsigned int)0x00000000); v.bit = value; DISP_REG_MASK(cmdq, &REG, AS_UINT32(&v), AS_UINT32(&r)); }     \
            else            {r = *((TYPE*)&INREG32(&REG)); r.bit = (value);     DISP_REG_SET(cmdq, &REG, AS_UINT32(&r)); }              \
        } while (0);


// for legacy DPI Driver
typedef enum
{
    LCD_IF_PARALLEL_0 = 0,
    LCD_IF_PARALLEL_1 = 1,
    LCD_IF_PARALLEL_2 = 2,
    LCD_IF_SERIAL_0   = 3,
    LCD_IF_SERIAL_1   = 4,

    LCD_IF_ALL = 0xFF,
} LCD_IF_ID;

typedef struct
{
	unsigned rsv_0   :4;
	unsigned addr    :4;
	unsigned rsv_8   :24;
} LCD_REG_CMD_ADDR, *PLCD_REG_CMD_ADDR;

typedef struct
{
	unsigned rsv_0   :4;
	unsigned addr    :4;
	unsigned rsv_8   :24;
} LCD_REG_DAT_ADDR, *PLCD_REG_DAT_ADDR;


typedef enum
{
    LCD_IF_FMT_COLOR_ORDER_RGB = 0,
    LCD_IF_FMT_COLOR_ORDER_BGR = 1,
} LCD_IF_FMT_COLOR_ORDER;


typedef enum
{
    LCD_IF_FMT_TRANS_SEQ_MSB_FIRST = 0,
    LCD_IF_FMT_TRANS_SEQ_LSB_FIRST = 1,
} LCD_IF_FMT_TRANS_SEQ;


typedef enum
{
    LCD_IF_FMT_PADDING_ON_LSB = 0,
    LCD_IF_FMT_PADDING_ON_MSB = 1,
} LCD_IF_FMT_PADDING;


typedef enum
{
    LCD_IF_FORMAT_RGB332 = 0,
    LCD_IF_FORMAT_RGB444 = 1,
    LCD_IF_FORMAT_RGB565 = 2,
    LCD_IF_FORMAT_RGB666 = 3,
    LCD_IF_FORMAT_RGB888 = 4,
} LCD_IF_FORMAT;

typedef enum
{
    LCD_IF_WIDTH_8_BITS  = 0,
    LCD_IF_WIDTH_9_BITS  = 2,
    LCD_IF_WIDTH_16_BITS = 1,
    LCD_IF_WIDTH_18_BITS = 3,
    LCD_IF_WIDTH_24_BITS = 4,
	LCD_IF_WIDTH_32_BITS = 5,
} LCD_IF_WIDTH;


typedef enum
{	
   DPI_STATUS_OK = 0,

   DPI_STATUS_ERROR,
} DPI_STATUS;

typedef enum
{
    DPI_POLARITY_RISING  = 0,
    DPI_POLARITY_FALLING = 1
} DPI_POLARITY;

typedef enum
{
    DPI_RGB_ORDER_RGB = 0,
    DPI_RGB_ORDER_BGR = 1,
} DPI_RGB_ORDER;

typedef enum
{
    DPI_CLK_480p  = 27027,
    DPI_CLK_720p  = 74250,
    DPI_CLK_1080p = 148500
} DPI_CLK_FREQ;

typedef  enum
{
    DPI_VIDEO_720x480p_60Hz=0,  // 0
    DPI_VIDEO_720x576p_50Hz,    // 1
    DPI_VIDEO_1280x720p_60Hz,   // 2
    DPI_VIDEO_1280x720p_50Hz,   // 3
    DPI_VIDEO_1920x1080i_60Hz,  // 4
    DPI_VIDEO_1920x1080i_50Hz,  // 5
    DPI_VIDEO_1920x1080p_30Hz,  // 6
    DPI_VIDEO_1920x1080p_25Hz,  // 7
    DPI_VIDEO_1920x1080p_24Hz,  // 8
    DPI_VIDEO_1920x1080p_23Hz,  // 9
    DPI_VIDEO_1920x1080p_29Hz,  // a
   	DPI_VIDEO_1920x1080p_60Hz,  // b 
    DPI_VIDEO_1920x1080p_50Hz,  // c

    DPI_VIDEO_1280x720p3d_60Hz,   // d
    DPI_VIDEO_1280x720p3d_50Hz,   // e
    DPI_VIDEO_1920x1080i3d_60Hz,  // f
    DPI_VIDEO_1920x1080i3d_50Hz,  // 10
    DPI_VIDEO_1920x1080p3d_24Hz,  // 11
    DPI_VIDEO_1920x1080p3d_23Hz,  // 12

	/*the 2160 mean 3840x2160 */
	DPI_VIDEO_2160P_23_976HZ,
	DPI_VIDEO_2160P_24HZ,
	DPI_VIDEO_2160P_25HZ,
	DPI_VIDEO_2160P_29_97HZ,
	DPI_VIDEO_2160P_30HZ,
	/*the 2161 mean 4096x2160 */
	DPI_VIDEO_2161P_24HZ,

    DPI_VIDEO_RESOLUTION_NUM       
}DPI_VIDEO_RESOLUTION;

typedef struct
{
    unsigned RGB_ORDER      : 1;
    unsigned BYTE_ORDER     : 1;
    unsigned PADDING        : 1;
    unsigned DATA_FMT       : 3;
    unsigned IF_FMT         : 2;
    unsigned COMMAND        : 5;
    unsigned rsv_13         : 2;
    unsigned ENC            : 1;
    unsigned rsv_16         : 8;
    unsigned SEND_RES_MODE  : 1;
    unsigned IF_24          : 1;
	unsigned rsv_6          : 6;
}LCD_REG_WROI_CON, *PLCD_REG_WROI_CON;

int ddp_dpi_stop(DISP_MODULE_ENUM module, void *cmdq_handle);
int ddp_dpi_power_on(DISP_MODULE_ENUM module, void *cmdq_handle);
int ddp_dpi_power_off(DISP_MODULE_ENUM module, void *cmdq_handle);
int ddp_dpi_dump(DISP_MODULE_ENUM module, int level);
int ddp_dpi_start(DISP_MODULE_ENUM module, void *cmdq);
int ddp_dpi_init(DISP_MODULE_ENUM module, void *cmdq);
int ddp_dpi_deinit(DISP_MODULE_ENUM module, void *cmdq_handle);
int ddp_dpi_config(DISP_MODULE_ENUM module, disp_ddp_path_config *config, void *cmdq_handle);
int ddp_dpi_trigger(DISP_MODULE_ENUM module, void *cmdq);
DPI_STATUS ddp_dpi_EnableColorBar(DISP_MODULE_ENUM module);
unsigned int ddp_dpi_get_cur_addr(bool rdma_mode, int layerid );

DPI_STATUS ddp_dpi_yuv422_setting(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 uvsw);
DPI_STATUS ddp_dpi_CLPFSetting(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT8 clpfType, BOOL roundingEnable);
DPI_STATUS ddp_dpi_ConfigHDMI(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 yuv422en, UINT32 rgb2yuven, UINT32 ydfpen, UINT32 r601sel, UINT32 clpfen);
DPI_STATUS ddp_dpi_ConfigVsync_LEVEN(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 pulseWidth, UINT32 backPorch, UINT32 frontPorch, BOOL fgInterlace);
DPI_STATUS ddp_dpi_ConfigVsync_RODD(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 pulseWidth, UINT32 backPorch, UINT32 frontPorch);

#ifdef __cplusplus
}
#endif

#endif // __DPI_DRV_H__