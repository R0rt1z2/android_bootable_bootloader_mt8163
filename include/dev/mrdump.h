#if !defined (__MRDUMP_H__)
#define __MRDUMP_H__

int mrdump_detection(void);

int mrdump_run2(void);

int check_ram_console_is_abnormal_boot(void) __attribute__((weak));

#define AEE_MRDUMP_LK_RSV_SIZE 0x400000

#endif
