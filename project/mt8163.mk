LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := bq8163_tb_m

MODULES += \
	app/mt_boot \
	dev/lcm

# EMMC
MTK_EMMC_SUPPORT = yes
DEFINES += MTK_NEW_COMBO_EMMC_SUPPORT

# Power Off Charging
MTK_KERNEL_POWER_OFF_CHARGING = yes

# Power
DEFINES += SWCHR_POWER_PATH
DEFINES += MTK_BQ24261_SUPPORT

# LCM
MTK_LCM_PHYSICAL_ROTATION = 180
CUSTOM_LK_LCM = "inn080dp10v5_dsi_vdo_tps65132"

# Security
MTK_SECURITY_SW_SUPPORT = yes
MTK_VERIFIED_BOOT_SUPPORT = yes
MTK_SEC_FASTBOOT_UNLOCK_SUPPORT = yes

# Misc
BOOT_LOGO = wxga
DEFINES += WITH_DEBUG_UART=1