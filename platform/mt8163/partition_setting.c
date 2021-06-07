//#include <platform/mt_partition.h>
#include <mt_partition.h>
part_t partition_layout[PART_MAX_COUNT];

struct part_name_map g_part_name_map[PART_MAX_COUNT] = { 
    {"preloader",   "PRELOADER",    "raw data", 0,  0,  0}, 
    {"mbr", "MBR",  "raw data", 1,  0,  0}, 
    {"ebr1",    "EBR1", "raw data", 2,  0,  0}, 
    {"pro_info",    "PRO_INFO", "raw data", 3,  0,  0}, 
    {"nvram",   "NVRAM",    "raw data", 4,  0,  0}, 
    {"protect_f",   "PROTECT_F",    "ext4", 5,  0,  0}, 
    {"protect_s",   "PROTECT_S",    "ext4", 6,  0,  0}, 
    {"persist",   "PERSIST",    "ext4", 7,  0,  0}, 
    {"seccfg",  "SECCFG",   "raw data", 8,  0,  0}, 
    {"uboot",   "UBOOT",    "raw data", 9,  1,  1}, 
    {"boot",    "BOOTIMG",  "raw data", 10,  1,  1}, 
    {"recovery",    "RECOVERY", "raw data", 11, 1,  1}, 
    {"sec_ro",  "SEC_RO",    "ext4", 12, 0,  0}, 
    {"misc",    "MISC", "raw data", 13, 0,  0}, 
    {"logo",    "LOGO", "raw data", 14, 0,  0}, 
    {"ebr2",    "EBR2", "raw data", 15, 0,  0}, 
    {"custom",  "CUSTOM",   "ext4", 16, 0,  0}, 
    {"expdb",   "EXPDB",   "raw data", 17, 0,  0}, 
    {"tee1",   "TEE1",   "raw data", 18, 0,  0}, 
    {"tee2",   "TEE2",   "raw data", 19, 0,  0}, 
    {"system",  "ANDROID",    "ext4", 20, 1,  1}, 
    {"cache",   "CACHE",    "ext4", 21, 1,  1}, 
    {"userdata",    "USRDATA", "ext4", 22, 1,  1}, 
    {"fat", "FAT",  "fat",  23, 0,  0}, 
    {"otp", "OTP",  "raw data",  24, 0,  0}, 
    {"bmtpool", "BMTPOOL",  "raw data",  25, 0,  0}, 
};
