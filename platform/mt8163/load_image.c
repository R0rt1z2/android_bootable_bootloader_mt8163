#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/bootimg.h>
#include <platform/errno.h>
#include <printf.h>
#include <string.h>
#include <malloc.h>
#include <platform/mt_gpt.h>
#include <platform/sec_status.h>

#include <platform/mtk_key.h>
#include <target/cust_key.h>
#define MODULE_NAME "LK_BOOT"

// ************************************************************************

#define PUBK_LEN 256
#define BOOT_STATE_GREEN   0x0
#define BOOT_STATE_ORANGE  0x1
#define BOOT_STATE_YELLOW  0x2
#define BOOT_STATE_RED     0x3

#define DEVICE_STATE_UNLOCKED 0x0
#define DEVICE_STATE_LOCKED   0x1

//*********
//* Notice : it's kernel start addr (and not include any debug header)
unsigned int g_kmem_off = 0;

//*********
//* Notice : it's rootfs start addr (and not include any debug header)
unsigned int g_rmem_off = 0;


unsigned int g_bimg_sz = 0;
unsigned int g_rcimg_sz = 0;
unsigned int g_fcimg_sz = 0;
int g_kimg_sz = 0;
int g_rimg_sz = 0;
unsigned int g_boot_state = BOOT_STATE_GREEN;

extern boot_img_hdr *g_boot_hdr;
extern void mtk_wdt_restart(void);

#ifndef MTK_EMMC_SUPPORT
extern unsigned long long get_part_size(char* name);
#define PART_MISC "MISC"
#else
#define PART_MISC "para"
#endif

int print_boot_state(void)
{
	int ret = 0;
	switch (g_boot_state) {
		case BOOT_STATE_ORANGE:
			dprintf(CRITICAL, "boot state: orange\n");
			break;
		case BOOT_STATE_YELLOW:
			dprintf(CRITICAL, "boot state: yellow\n");
			break;
		case BOOT_STATE_RED:
			dprintf(CRITICAL, "boot state: red\n");
			break;
		case BOOT_STATE_GREEN:
			dprintf(CRITICAL, "boot state: green\n");
			break;
		default:
			dprintf(CRITICAL, "boot state: unknown\n");
			break;
	}

	return ret;
}

int yellow_state_warning(void)
{
	const char* title_msg = "yellow state\n\n";
	unsigned char pubk[PUBK_LEN] = {0};
	int ret = 0;

	video_clean_screen();
	video_set_cursor(video_get_rows() / 2, 0);
	video_printf(title_msg);
	video_printf("Your device has loaded a different operating system\n");
	video_printf("ID:\n");

	ret = sec_get_custom_pubk(pubk, PUBK_LEN);
	if (ret) {
		video_printf("Cannot get custom public key, abort in 5 seconds\n");
		mtk_wdt_restart();
		mdelay(5000);
		mtk_wdt_restart();
		return -1;
	}
	video_printf("%x %x %x %x %x %x %x %x\n", pubk[0], pubk[1], pubk[2], pubk[3], pubk[4], pubk[5], pubk[6], pubk[7]);
	video_printf("Yes (Volume UP)   : Confirm and Boot.\n\n");
	video_printf("No  (Volume Down) : Abort.\n\n");

	while (1) {
		mtk_wdt_restart();
		if (mtk_detect_key(MT65XX_MENU_SELECT_KEY)) //VOL_UP
			return 0;
		else if (mtk_detect_key(MT65XX_MENU_OK_KEY)) //VOL_DOWN
			return -1;
		else {
			/* ignore */
		}
	}
}

int orange_state_warning(void)
{
	const char* title_msg = "Orange State\n\n";
	int ret = 0;

	video_clean_screen();
	video_set_cursor(video_get_rows() / 2, 0);
	video_printf(title_msg);
	video_printf("Your device has been unlocked and can't be trusted\n");
	video_printf("Your device will boot in 5 seconds\n");
	mtk_wdt_restart();
	mdelay(5000);
	mtk_wdt_restart();

	return 0;
}

int red_state_warning(void)
{
	const char* title_msg = "Red State\n\n";
	int ret = 0;

	video_clean_screen();
	video_set_cursor(video_get_rows() / 2, 0);
	video_printf(title_msg);
	video_printf("Your device has failed verification and may not\n");
	video_printf("work properly\n");
	video_printf("Your device will boot in 5 seconds\n");
	mtk_wdt_restart();
	mdelay(5000);
	mtk_wdt_restart();

	return -1;
}

int show_warning(void)
{
	int ret = 0;
	switch (g_boot_state) {
		case BOOT_STATE_ORANGE:
			ret = orange_state_warning();
			break;
		case BOOT_STATE_YELLOW:
#ifdef MTK_SECURITY_YELLOW_STATE_SUPPORT
			ret = yellow_state_warning();
			if (0 == ret) /* user confirms to boot into yellow state */
				break;
			/* fall into red state if user refuses to enter yellow state */
#else
			ret = -1;
			/* fall into red state since yellow state is not supported */
#endif
		case BOOT_STATE_RED:
			ret = red_state_warning();
			ret = -1; /* return error */
			break;
		case BOOT_STATE_GREEN:
		default:
			break;
	}

	return ret;
}

int set_boot_state_to_cmdline()
{
	int ret = 0;

	switch (g_boot_state) {
		case BOOT_STATE_ORANGE:
			cmdline_append("androidboot.verifiedbootstate=orange");
			break;
		case BOOT_STATE_YELLOW:
			cmdline_append("androidboot.verifiedbootstate=yellow");
			break;
		case BOOT_STATE_RED:
			cmdline_append("androidboot.verifiedbootstate=red");
			break;
		case BOOT_STATE_GREEN:
			cmdline_append("androidboot.verifiedbootstate=green");
			break;
		default:
			break;
	}

	return ret;
}

int verified_boot_flow(char *part_name)
{
	int ret = 0;
	unsigned int img_vfy_time = 0;
	int lock_state = 0;

	/* if MTK_SECURITY_SW_SUPPORT is not defined, boot state is always green */
#ifdef MTK_SECURITY_SW_SUPPORT
	/* please refer to the following website for verified boot flow */
	/* http://source.android.com/devices/tech/security/verifiedboot/verified-boot.html */
	ret = sec_query_device_lock(&lock_state);
	if (ret) {
		g_boot_state = BOOT_STATE_RED;
		goto _end;
	}

	if (DEVICE_STATE_LOCKED == lock_state) {
		if (g_boot_state != BOOT_STATE_RED)
			goto _end;

		sec_clear_pubk();
		img_vfy_time = get_timer(0);

		ret = android_verified_boot(part_name);
		if (0 == ret)
			g_boot_state = BOOT_STATE_YELLOW;
		else {
			g_boot_state = BOOT_STATE_RED;
			sec_clear_pubk();
		}
		dprintf(INFO, "[SBC] img vfy(%d ms)\n", (unsigned int)get_timer(img_vfy_time));
	}
	else if (DEVICE_STATE_UNLOCKED == lock_state) {
		g_boot_state = BOOT_STATE_ORANGE;
	}
	else {/* unknown lock state*/
		g_boot_state = BOOT_STATE_RED;
	}
#endif //MTK_SECURITY_SW_SUPPORT

_end:
	ret = print_boot_state();
	if (ret)
		return ret;

	ret = show_warning();
	if (ret)
		return ret;

	ret = set_boot_state_to_cmdline();
	if (ret)
		return ret;

	return ret;
}

#if 1
static int mboot_common_load_part_info(part_dev_t *dev, char *part_name, part_hdr_t *part_hdr)
{
	long len;
#if 1  //#ifdef MTK_EMMC_SUPPORT
	u64 addr;
#else
	ulong addr;
#endif
	part_t *part;

	part = mt_part_get_partition(part_name);

#ifdef MTK_EMMC_SUPPORT
	addr = (u64)part->start_sect * BLK_SIZE;
#else
	addr = (u64)part->startblk * BLK_SIZE;
#endif

	//***************
	//* read partition header
	//*
#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, addr, (uchar*)part_hdr, sizeof(part_hdr_t), part->part_id);
#else
	len = dev->read(dev, addr, (uchar*)part_hdr, sizeof(part_hdr_t));
#endif
#else
	len = dev->read(dev, addr, (uchar*)part_hdr, sizeof(part_hdr_t), part->part_id);
#endif

	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s partition read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	dprintf(CRITICAL, "\n=========================================\n");
	dprintf(CRITICAL, "[%s] %s magic number : 0x%x\n",MODULE_NAME,part_name,part_hdr->info.magic);
	part_hdr->info.name[31]='\0'; //append end char
	dprintf(CRITICAL, "[%s] %s name         : %s\n",MODULE_NAME,part_name,part_hdr->info.name);
	dprintf(CRITICAL, "[%s] %s size         : %d\n",MODULE_NAME,part_name,part_hdr->info.dsize);
	dprintf(CRITICAL, "=========================================\n");

	//***************
	//* check partition magic
	//*
	if (part_hdr->info.magic != PART_MAGIC) {
		dprintf(CRITICAL, "[%s] %s partition magic error\n", MODULE_NAME, part_name);
		return -1;
	}

	//***************
	//* check partition name
	//*
	if (strncasecmp(part_hdr->info.name, part_name, sizeof(part_hdr->info.name))) {
		dprintf(CRITICAL, "[%s] %s partition name error\n", MODULE_NAME, part_name);
		return -1;
	}

	//***************
	//* check partition data size
	//*
#ifdef MTK_EMMC_SUPPORT
	if (part_hdr->info.dsize > part->nr_sects * BLK_SIZE) {
#else
	if (part_hdr->info.dsize > part->blknum * BLK_SIZE) {
#endif
		dprintf(CRITICAL, "[%s] %s partition size error\n", MODULE_NAME, part_name);
		return -1;
	}

	return 0;
}


/**********************************************************
 * Routine: mboot_common_load_part
 *
 * Description: common function for loading image from nand flash
 *              this function is called by
 *                  (1) 'mboot_common_load_logo' to display logo
 *
 **********************************************************/
int mboot_common_load_part(char *part_name, unsigned long addr)
{
	long len;
#if 1//#ifdef MTK_EMMC_SUPPORT
	unsigned long long start_addr;
#else
	unsigned long start_addr;
#endif
	part_t *part;
	part_dev_t *dev;
	part_hdr_t *part_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		return -ENOENT;
	}

#ifdef MTK_EMMC_SUPPORT
	start_addr = (u64)part->start_sect * BLK_SIZE;
#else
	start_addr = (u64)part->startblk * BLK_SIZE;
#endif

	part_hdr = (part_hdr_t*)malloc(sizeof(part_hdr_t));


	if (!part_hdr) {
		return -ENOMEM;
	}

	len = mboot_common_load_part_info(dev, part_name, part_hdr);
	if (len < 0) {
		len = -EINVAL;
		goto exit;
	}


	//****************
	//* read image data
	//*
	dprintf(CRITICAL, "read the data of %s\n", part_name);


#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, start_addr + sizeof(part_hdr_t), (uchar*)addr, part_hdr->info.dsize, part->part_id);
#else
	len = dev->read(dev, start_addr + sizeof(part_hdr_t), (uchar*)addr, part_hdr->info.dsize);
#endif
#else
	len = dev->read(dev, start_addr + sizeof(part_hdr_t), (uchar*)addr, part_hdr->info.dsize, part->part_id);
#endif

	if (len < 0) {
		len = -EIO;
		goto exit;
	}


exit:
	if (part_hdr)
		free(part_hdr);

	return len;
}

/**********************************************************
 * Routine: mboot_common_load_logo
 *
 * Description: function to load logo to display
 *
 **********************************************************/
int mboot_common_load_logo(unsigned long logo_addr, char* filename)
{
	int ret;
#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	long len;
#endif

#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	len = file_fat_read(filename, (unsigned char *)logo_addr, 0);

	if (len > 0)
		return (int)len;
#endif

	ret = mboot_common_load_part("logo", logo_addr);

	return ret;
}

/**********************************************************
 * Routine: mboot_android_check_img_info
 *
 * Description: this function is called to
 *              (1) check the header of kernel / rootfs
 *
 * Notice : this function will be called by 'mboot_android_check_bootimg_hdr'
 *
 **********************************************************/
int mboot_android_check_img_info(char *part_name, part_hdr_t *part_hdr)
{
	//***************
	//* check partition magic
	//*
	if (part_hdr->info.magic != PART_MAGIC) {
		dprintf(CRITICAL, "[%s] %s partition magic not match\n", MODULE_NAME, part_name);
		return -1;
	}

	//***************
	//* check partition name
	//*
	if (strncasecmp(part_hdr->info.name, part_name, sizeof(part_hdr->info.name))) {
		dprintf(CRITICAL, "[%s] %s partition name not match\n", MODULE_NAME, part_name);
		return -1;
	}

	dprintf(CRITICAL, "\n=========================================\n");
	dprintf(CRITICAL, "[%s] %s magic number : 0x%x\n",MODULE_NAME,part_name,part_hdr->info.magic);
	dprintf(CRITICAL, "[%s] %s size         : 0x%x\n",MODULE_NAME,part_name,part_hdr->info.dsize);
	dprintf(CRITICAL, "=========================================\n");

	//***************
	//* return the image size
	//*
	return part_hdr->info.dsize;
}

/**********************************************************
 * Routine: mboot_android_check_bootimg_hdr
 *
 * Description: this function is called to
 *              (1) 'read' the header of boot image from nand flash
 *              (2) 'parse' the header of boot image to obtain
 *                  - (a) kernel image size
 *                  - (b) rootfs image size
 *                  - (c) rootfs offset
 *
 * Notice : this function must be read first when doing nand / msdc boot
 *
 **********************************************************/
static int mboot_android_check_bootimg_hdr(part_dev_t *dev, char *part_name, boot_img_hdr *boot_hdr)
{
	int ret;
	long len;
#if 1//#ifdef MTK_EMMC_SUPPORT
	u64 addr;
#else
	ulong addr;
#endif
	part_t *part;


	//**********************************
	// TODO : fix pg_sz assignment
	//**********************************
	unsigned int pg_sz = 2*1024 ;

	part = mt_part_get_partition(part_name);
#ifdef MTK_EMMC_SUPPORT
	addr = (u64)part->start_sect * BLK_SIZE;
#else
	addr = (u64)part->startblk * BLK_SIZE;
#endif

	//***************
	//* read partition header
	//*

	dprintf(CRITICAL, "part page addr is 0x%llx\n", addr);

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr), part->part_id);
#else
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr));
#endif
#else
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr), part->part_id);
#endif
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s boot image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	dprintf(CRITICAL, "\n============================================================\n");
	dprintf(CRITICAL, "[%s] Android Partition Name                : %s\n"    , MODULE_NAME, part_name);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Kernel Size    : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Kernel Address : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_addr);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Rootfs Size    : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Rootfs Address : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Tags Address   : 0x%08X\n", MODULE_NAME, boot_hdr->tags_addr);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Page Size      : 0x%08X\n", MODULE_NAME, boot_hdr->page_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Command Line   : %s\n"    , MODULE_NAME, boot_hdr->cmdline);
	dprintf(CRITICAL, "============================================================\n");

	//***************
	//* check partition magic
	//*
	if (strncmp((char const *)boot_hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)!=0) {
		dprintf(CRITICAL, "[%s] boot image header magic error\n", MODULE_NAME);
		return -1;
	}

	pg_sz = boot_hdr->page_size;

	//***************
	//* follow bootimg.h to calculate the location of rootfs
	//*
	if (len != -1) {
		unsigned int k_pg_cnt = 0;
		unsigned int r_pg_cnt = 0;
		if (g_is_64bit_kernel) {
			g_kmem_off = target_get_scratch_address();
		} else {
			g_kmem_off = boot_hdr->kernel_addr;
		}
		if (boot_hdr->kernel_size % pg_sz == 0) {
			k_pg_cnt = boot_hdr->kernel_size / pg_sz;
		} else {
			k_pg_cnt = (boot_hdr->kernel_size / pg_sz) + 1;
		}

		if (boot_hdr->ramdisk_size % pg_sz == 0) {
			r_pg_cnt = boot_hdr->ramdisk_size / pg_sz;
		} else {
			r_pg_cnt = (boot_hdr->ramdisk_size / pg_sz) + 1;
		}

		dprintf(CRITICAL, " > page count of kernel image = %d\n",k_pg_cnt);
		g_rmem_off = g_kmem_off + k_pg_cnt * pg_sz;

		dprintf(CRITICAL, " > kernel mem offset = 0x%x\n",g_kmem_off);
		dprintf(CRITICAL, " > rootfs mem offset = 0x%x\n",g_rmem_off);


		//***************
		//* specify boot image size
		//*
		g_bimg_sz = (k_pg_cnt + r_pg_cnt + 1)* pg_sz;

		dprintf(CRITICAL, " > boot image size = 0x%x\n",g_bimg_sz);

		ret = verified_boot_flow("boot");
		if (ret)
			g_bimg_sz = -1;
	}

	return 0;
}

/**********************************************************
 * Routine: mboot_android_check_recoveryimg_hdr
 *
 * Description: this function is called to
 *              (1) 'read' the header of boot image from nand flash
 *              (2) 'parse' the header of boot image to obtain
 *                  - (a) kernel image size
 *                  - (b) rootfs image size
 *                  - (c) rootfs offset
 *
 * Notice : this function must be read first when doing nand / msdc boot
 *
 **********************************************************/
static int mboot_android_check_recoveryimg_hdr(part_dev_t *dev, char *part_name, boot_img_hdr *boot_hdr)
{
	int ret;
	long len;
#if 1//#ifdef MTK_EMMC_SUPPORT
	u64 addr;
#else
	ulong addr;
#endif
	part_t *part;

	//**********************************
	// TODO : fix pg_sz assignment
	//**********************************
	unsigned int pg_sz = 2*1024 ;


	part = mt_part_get_partition(part_name);
#ifdef MTK_EMMC_SUPPORT
	addr = (u64)part->start_sect * BLK_SIZE;
#else
	addr = (u64)part->startblk * BLK_SIZE;
#endif

	//***************
	//* read partition header
	//*
#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr), part->part_id);
#else
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr));
#endif
#else
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr), part->part_id);
#endif
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s Recovery image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	dprintf(CRITICAL, "\n============================================================\n");
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Kernel Size    : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Kernel Address : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Rootfs Size    : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Rootfs Address : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Tags Address   : 0x%08X\n", MODULE_NAME, boot_hdr->tags_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Page Size      : 0x%08X\n", MODULE_NAME, boot_hdr->page_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Command Line   : %s\n"    , MODULE_NAME, boot_hdr->cmdline);
	dprintf(CRITICAL, "============================================================\n");

	//***************
	//* check partition magic
	//*
	if (strncmp((char const *)boot_hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)!=0) {
		dprintf(CRITICAL, "[%s] Recovery image header magic error\n", MODULE_NAME);
		return -1;
	}

	pg_sz = boot_hdr->page_size;

	//***************
	//* follow bootimg.h to calculate the location of rootfs
	//*
	if (len != -1) {
		unsigned int k_pg_cnt = 0;
		unsigned int r_pg_cnt = 0;
		if (g_is_64bit_kernel) {
			g_kmem_off = target_get_scratch_address();
		} else {
			g_kmem_off =  boot_hdr->kernel_addr;
		}
		if (boot_hdr->kernel_size % pg_sz == 0) {
			k_pg_cnt = boot_hdr->kernel_size / pg_sz;
		} else {
			k_pg_cnt = (boot_hdr->kernel_size / pg_sz) + 1;
		}

		if (boot_hdr->ramdisk_size % pg_sz == 0) {
			r_pg_cnt = boot_hdr->ramdisk_size / pg_sz;
		} else {
			r_pg_cnt = (boot_hdr->ramdisk_size / pg_sz) + 1;
		}

		dprintf(CRITICAL, " > page count of kernel image = %d\n",k_pg_cnt);
		g_rmem_off = g_kmem_off + k_pg_cnt * pg_sz;

		dprintf(CRITICAL, " > kernel mem offset = 0x%x\n",g_kmem_off);
		dprintf(CRITICAL, " > rootfs mem offset = 0x%x\n",g_rmem_off);


		//***************
		//* specify boot image size
		//*
		//g_rcimg_sz = part->start_sect * BLK_SIZE;
#ifndef MTK_EMMC_SUPPORT
		g_rcimg_sz = (unsigned int)get_part_size(PART_RECOVERY);
#else
		g_rcimg_sz = (k_pg_cnt + r_pg_cnt + 1)* pg_sz;
#endif

		dprintf(CRITICAL, " > Recovery image size = 0x%x\n", g_rcimg_sz);
		ret = verified_boot_flow("recovery");
		if (ret)
			g_rcimg_sz = -1;
	}

	return 0;
}


/**********************************************************
 * Routine: mboot_android_check_factoryimg_hdr
 *
 * Description: this function is called to
 *              (1) 'read' the header of boot image from nand flash
 *              (2) 'parse' the header of boot image to obtain
 *                  - (a) kernel image size
 *                  - (b) rootfs image size
 *                  - (c) rootfs offset
 *
 * Notice : this function must be read first when doing nand / msdc boot
 *
 **********************************************************/
static int mboot_android_check_factoryimg_hdr(char *part_name, boot_img_hdr *boot_hdr)
{
	int ret;
	int len=0;
	//   ulong addr;

	//**********************************
	// TODO : fix pg_sz assignment
	//**********************************
	unsigned int pg_sz = 2*1024 ;

	//***************
	//* read partition header
	//*

#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	len = file_fat_read(part_name, (uchar*) boot_hdr, sizeof(boot_img_hdr));

	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s Factory image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}
#endif

	dprintf(CRITICAL, "\n============================================================\n");
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Kernel Size    : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_size);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Kernel Address : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_addr);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Rootfs Size    : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_size);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Rootfs Address : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Tags Address   : 0x%08X\n", MODULE_NAME, boot_hdr->tags_addr);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Page Size      : 0x%08X\n", MODULE_NAME, boot_hdr->page_size);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Command Line   : %s\n"    , MODULE_NAME, boot_hdr->cmdline);
	dprintf(CRITICAL, "============================================================\n");

	//***************
	//* check partition magic
	//*
	if (strncmp((char const *)boot_hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)!=0) {
		dprintf(CRITICAL, "[%s] Factory image header magic error\n", MODULE_NAME);
		return -1;
	}

	pg_sz = boot_hdr->page_size;

	//***************
	//* follow bootimg.h to calculate the location of rootfs
	//*
	if (len != -1) {
		unsigned int k_pg_cnt = 0;
		if (g_is_64bit_kernel) {
			g_kmem_off = target_get_scratch_address();
		} else {
			g_kmem_off =  boot_hdr->kernel_addr;
		}
		if (boot_hdr->kernel_size % pg_sz == 0) {
			k_pg_cnt = boot_hdr->kernel_size / pg_sz;
		} else {
			k_pg_cnt = (boot_hdr->kernel_size / pg_sz) + 1;
		}

		dprintf(CRITICAL, " > page count of kernel image = %d\n",k_pg_cnt);
		g_rmem_off = g_kmem_off + k_pg_cnt * pg_sz;

		dprintf(CRITICAL, " > kernel mem offset = 0x%x\n",g_kmem_off);
		dprintf(CRITICAL, " > rootfs mem offset = 0x%x\n",g_rmem_off);


		//***************
		//* specify boot image size
		//*
		//g_fcimg_sz = PART_BLKS_RECOVERY * BLK_SIZE;
#ifndef MTK_EMMC_SUPPORT
		g_rcimg_sz = (unsigned int)get_part_size(PART_RECOVERY);
#endif

		dprintf(CRITICAL, " > Factory image size = 0x%x\n", g_rcimg_sz);

		ret = verified_boot_flow("recovery");
		if (ret)
			g_rcimg_sz = -1;
	}

	return 0;
}


/**********************************************************
 * Routine: mboot_android_load_bootimg_hdr
 *
 * Description: this is the entry function to handle boot image header
 *
 **********************************************************/
int mboot_android_load_bootimg_hdr(char *part_name, unsigned long addr)
{
	long len;
//	unsigned long begin;
//	unsigned long start_addr;
	part_t *part;
	part_dev_t *dev;
	boot_img_hdr *boot_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}

//    start_addr = part->start_sect * BLK_SIZE;

	boot_hdr = (boot_img_hdr*)malloc(sizeof(boot_img_hdr));
	if (!boot_hdr) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr, boot_hdr = NULL\n");
		return -ENOMEM;
	}

	g_boot_hdr = boot_hdr;

	len = mboot_android_check_bootimg_hdr(dev, part_name, boot_hdr);

	return len;
}

/**********************************************************
 * Routine: mboot_android_load_recoveryimg_hdr
 *
 * Description: this is the entry function to handle Recovery image header
 *
 **********************************************************/
int mboot_android_load_recoveryimg_hdr(char *part_name, unsigned long addr)
{
	long len;
//	unsigned long begin;
//	unsigned long start_addr;
	part_t *part;
	part_dev_t *dev;
	boot_img_hdr *boot_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_recoveryimg_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_recoveryimg_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}

//    start_addr = part->start_sect * BLK_SIZE;

	boot_hdr = (boot_img_hdr*)malloc(sizeof(boot_img_hdr));
	if (!boot_hdr) {
		dprintf(CRITICAL, "mboot_android_load_bootimg_hdr, boot_hdr = NULL\n");
		return -ENOMEM;
	}

	g_boot_hdr = boot_hdr;

	len = mboot_android_check_recoveryimg_hdr(dev, part_name, boot_hdr);

	return len;
}


/**********************************************************
 * Routine: mboot_android_load_factoryimg_hdr
 *
 * Description: this is the entry function to handle Factory image header
 *
 **********************************************************/
int mboot_android_load_factoryimg_hdr(char *part_name, unsigned long addr)
{
	long len;

	boot_img_hdr *boot_hdr;

	boot_hdr = (boot_img_hdr*)malloc(sizeof(boot_img_hdr));

	if (!boot_hdr) {
		dprintf(CRITICAL, "mboot_android_load_factoryimg_hdr, boot_hdr = NULL\n");
		return -ENOMEM;
	}

	g_boot_hdr = boot_hdr;

	len = mboot_android_check_factoryimg_hdr(part_name, boot_hdr);

	return len;
}


/**********************************************************
 * Routine: mboot_android_load_bootimg
 *
 * Description: main function to load Android Boot Image
 *
 **********************************************************/
int mboot_android_load_bootimg(char *part_name, unsigned long addr)
{
	long len;
#if 1 //#ifdef MTK_EMMC_SUPPORT
	unsigned long long start_addr;
#else
	unsigned long start_addr;
#endif
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , part = NULL\n");
		return -ENOENT;
	}

	//***************
	//* not to include unused header
	//*
#ifdef MTK_EMMC_SUPPORT
	start_addr =(u64)part->start_sect * BLK_SIZE + g_boot_hdr->page_size;
#else
	start_addr = (u64)part->startblk * BLK_SIZE + g_boot_hdr->page_size;
#endif

	/*
	 * check mkimg header
	 */
	dprintf(CRITICAL, "check mkimg header\n");
#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	dev->read(dev, start_addr, (uchar*)addr, MKIMG_HEADER_SZ, part->part_id);
#else
	dev->read(dev, start_addr, (uchar*)addr, MKIMG_HEADER_SZ, part->part_id);
#endif
	// check kernel header
	g_kimg_sz = mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *)addr);
	if (g_kimg_sz == -1) {
		dprintf(CRITICAL, "no mkimg header in kernel image\n");
	} else {
		dprintf(CRITICAL, "mkimg header exist in kernel image\n");
		addr  = addr - MKIMG_HEADER_SZ;
		g_rmem_off = g_rmem_off - MKIMG_HEADER_SZ;
	}

	//***************
	//* read image data
	//*
	dprintf(CRITICAL, "\nread the data of %s (size = 0x%x)\n", part_name, g_bimg_sz);
#ifdef MTK_EMMC_SUPPORT
	dprintf(CRITICAL, " > from - 0x%016llx (skip boot img hdr)\n",start_addr);
#else
	dprintf(CRITICAL, " > from - 0x%x (skip boot img hdr)\n",start_addr);
#endif
	dprintf(CRITICAL, " > to   - 0x%x (starts with kernel img hdr)\n",addr);

#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, start_addr, (uchar*)addr, g_bimg_sz, part->part_id);
#else
	len = dev->read(dev, start_addr, (uchar*)addr, g_bimg_sz, part->part_id);
#endif

	// check ramdisk/rootfs header
	g_rimg_sz = mboot_android_check_img_info(PART_ROOTFS, (part_hdr_t *)g_rmem_off);
	if (g_rimg_sz == -1) {
		dprintf(CRITICAL, "no mkimg header in ramdisk image\n");
		g_rimg_sz = g_boot_hdr->ramdisk_size;
	} else {
		dprintf(CRITICAL, "mkimg header exist in ramdisk image\n");
		g_rmem_off = g_rmem_off + MKIMG_HEADER_SZ;
	}

	if (len < 0) {
		len = -EIO;
	}

	return len;
}

/**********************************************************
 * Routine: mboot_android_load_recoveryimg
 *
 * Description: main function to load Android Recovery Image
 *
 **********************************************************/
int mboot_android_load_recoveryimg(char *part_name, unsigned long addr)
{
	long len;
#if 1//#ifdef MTK_EMMC_SUPPORT
	unsigned long long start_addr;
#else
	unsigned long start_addr;

#endif
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_bootimg , part = NULL\n");
		return -ENOENT;
	}


	//***************
	//* not to include unused header
	//*
#ifdef MTK_EMMC_SUPPORT
	start_addr = (u64)part->start_sect * BLK_SIZE + g_boot_hdr->page_size;
#else
	start_addr = (u64)part->startblk * BLK_SIZE + g_boot_hdr->page_size;
#endif

	/*
	 * check mkimg header
	 */
	dprintf(CRITICAL, "check mkimg header\n");
#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	dev->read(dev, start_addr, (uchar*)addr, MKIMG_HEADER_SZ, part->part_id);
#else
	dev->read(dev, start_addr, (uchar*)addr, MKIMG_HEADER_SZ, part->part_id);
#endif
	// check kernel header
	g_kimg_sz = mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *)addr);
	if (g_kimg_sz == -1) {
		dprintf(CRITICAL, "no mkimg header in kernel image\n");
	} else {
		dprintf(CRITICAL, "mkimg header exist in kernel image\n");
		addr  = addr - MKIMG_HEADER_SZ;
		g_rmem_off = g_rmem_off - MKIMG_HEADER_SZ;
	}

	//***************
	//* read image data
	//*
	dprintf(CRITICAL, "\nread the data of %s (size = 0x%x)\n", part_name, g_rcimg_sz);
#ifdef MTK_EMMC_SUPPORT
	dprintf(CRITICAL, " > from - 0x%016llx (skip recovery img hdr)\n",start_addr);
#else
	dprintf(CRITICAL, " > from - 0x%x (skip recovery img hdr)\n",start_addr);
#endif
	dprintf(CRITICAL, " > to   - 0x%x (starts with kernel img hdr)\n",addr);

#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, start_addr, (uchar*)addr, g_rcimg_sz, part->part_id);
#else
	len = dev->read(dev, start_addr, (uchar*)addr, g_rcimg_sz, part->part_id);
#endif

	// check ramdisk/rootfs header
	g_rimg_sz = mboot_android_check_img_info("recovery", (part_hdr_t *)g_rmem_off);
	if (g_rimg_sz == -1) {
		dprintf(CRITICAL, "no mkimg header in recovery image\n");
		g_rimg_sz = g_boot_hdr->ramdisk_size;
	} else {
		dprintf(CRITICAL, "mkimg header exist in recovery image\n");
		g_rmem_off = g_rmem_off + MKIMG_HEADER_SZ;
	}

	if (len < 0) {
		len = -EIO;
	}

	return len;
}


/**********************************************************
 * Routine: mboot_android_load_factoryimg
 *
 * Description: main function to load Android Factory Image
 *
 **********************************************************/
int mboot_android_load_factoryimg(char *part_name, unsigned long addr)
{
	int len = 0;

	//***************
	//* not to include unused header
	//*
	addr = addr - g_boot_hdr->page_size;

	/*
	 * check mkimg header
	 */
	dprintf(CRITICAL, "check mkimg header\n");
#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	file_fat_read(part_name, (uchar*)addr, MKIMG_HEADER_SZ);
#endif
	// check kernel header
	g_kimg_sz = mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *)addr);
	if (g_kimg_sz == -1) {
		dprintf(CRITICAL, "no mkimg header in kernel image\n");
	} else {
		dprintf(CRITICAL, "mkimg header exist in kernel image\n");
		addr = addr - MKIMG_HEADER_SZ;
		g_rmem_off = g_rmem_off - MKIMG_HEADER_SZ;
	}

#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	len = file_fat_read(part_name, (uchar*)addr, 0);
	dprintf(CRITICAL, "len = %d, addr = 0x%x\n", len, addr);
	dprintf(CRITICAL, "part name = %s \n", part_name);
#endif

	// check ramdisk/rootfs header
	g_rimg_sz = mboot_android_check_img_info(PART_ROOTFS, (part_hdr_t *)g_rmem_off);
	if (g_rimg_sz == -1) {
		dprintf(CRITICAL, "no mkimg header in ramdisk image\n");
		g_rimg_sz = g_boot_hdr->ramdisk_size;
	} else {
		dprintf(CRITICAL, "mkimg header exist in ramdisk image\n");
		g_rmem_off = g_rmem_off + MKIMG_HEADER_SZ;
	}

	if (len < 0) {
		len = -EIO;
	}

	return len;
}


/**********************************************************
 * Routine: mboot_recovery_load_raw_part
 *
 * Description: load raw data for recovery mode support
 *
 **********************************************************/
int mboot_recovery_load_raw_part(char *part_name, unsigned long *addr, unsigned int size)
{
	long len;
	unsigned long begin;

#if 1//#ifdef MTK_EMMC_SUPPORT
	unsigned long long start_addr;
#else
	unsigned long start_addr;
#endif
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		return -ENOENT;
	}
#ifdef MTK_EMMC_SUPPORT
	start_addr = (u64)part->start_sect * BLK_SIZE;
#else
	start_addr = (u64)part->startblk * BLK_SIZE;
#endif
	begin = get_timer(0);

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, start_addr,(uchar*)addr, size, part->part_id);
#else
	len = dev->read(dev, start_addr,(uchar*)addr, size);
#endif
#else
	len = dev->read(dev, start_addr,(uchar*)addr, size, part->part_id);
#endif
	if (len < 0) {
		len = -EIO;
		goto exit;
	}

	dprintf(CRITICAL, "[%s] Load '%s' partition to 0x%08lX (%d bytes in %ld ms)\n", MODULE_NAME, part->name, (unsigned long)addr, size, get_timer(begin));

exit:
	return len;
}

/**********************************************************
 * Routine: mboot_recovery_load_raw_part_offset
 *
 * Description: load partition raw data with offset
 *
 * offset and size must page alignemnt
 **********************************************************/
int mboot_recovery_load_raw_part_offset(char *part_name, unsigned long *addr, unsigned long offset, unsigned int size)
{
	long len;
	unsigned long begin;

#if 1// MTK_EMMC_SUPPORT
	unsigned long long start_addr;
#else
	unsigned long start_addr;
#endif
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		return -ENOENT;
	}
#ifdef MTK_EMMC_SUPPORT
	start_addr = (u64)part->start_sect * BLK_SIZE + ROUNDUP(offset, BLK_SIZE);
#else
	start_addr = part->startblk * BLK_SIZE + ROUNDUP(offset, BLK_SIZE);
#endif
	begin = get_timer(0);

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, start_addr, (uchar*)addr, ROUNDUP(size, BLK_SIZE), part->part_id);
#else
	len = dev->read(dev, start_addr, (uchar*)addr, ROUNDUP(size, BLK_SIZE));
#endif
#else
	len = dev->read(dev, start_addr, (uchar*)addr, ROUNDUP(size, BLK_SIZE), part->part_id);
#endif
	if (len < 0) {
		len = -EIO;
		goto exit;
	}

	dprintf(INFO, "[%s] Load '%s' partition to 0x%08lX (%d bytes in %ld ms)\n",
	        MODULE_NAME, part->name, (unsigned long)addr, size, get_timer(begin));

exit:
	return len;
}

/**********************************************************
 * Routine: mboot_recovery_load_misc
 *
 * Description: load recovery command
 *
 **********************************************************/
int mboot_recovery_load_misc(unsigned long *misc_addr, unsigned int size)
{
	int ret;

	dprintf(CRITICAL, "[mboot_recovery_load_misc]: size is %u\n", size);
	dprintf(CRITICAL, "[mboot_recovery_load_misc]: misc_addr is 0x%x\n", misc_addr);

	ret = mboot_recovery_load_raw_part(PART_MISC, misc_addr, size);

	if (ret < 0)
		return ret;

	return ret;
}

/**********************************************************
 * Routine: mboot_get_inhouse_img_size
 *
 * Description: Get img size from mkimage header (LK,Logo)
                The size include both image and header and the size is align to 4k.
 *
 **********************************************************/
unsigned int mboot_get_inhouse_img_size(char *part_name, unsigned int *size)
{
	int ret = 0;
	long len = 0;
#ifdef MTK_EMMC_SUPPORT
	u64 addr;
#else
	ulong addr;
#endif

	part_t *part;
	part_dev_t *dev;
	part_hdr_t mkimage_hdr;
	part_hdr_t *part_hdr;
	unsigned page_size = 0x1000;

	*size = 0;

	dprintf(CRITICAL, "Get inhouse img size from mkimage header\n");

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_img_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_img_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}

#ifdef MTK_EMMC_SUPPORT
	addr = (u64)part->start_sect  * BLK_SIZE;
#else
	addr = part->startblk * BLK_SIZE;
#endif

	/*Read mkimage header*/
#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, addr,  (uchar*)&mkimage_hdr, sizeof(part_hdr_t), part->part_id);
#else
	len = dev->read(dev, addr, (uchar*)&mkimage_hdr, sizeof(part_hdr_t), part->part_id);
#endif
	/*
	#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)                len = dev->read(dev, addr, (uchar*)&boot_hdr, sizeof(boot_img_hdr), part->part_id);
	#else
	    len = dev->read(dev, addr, (uchar*)&boot_hdr, sizeof(boot_img_hdr));        #endif
	*/
	dprintf(CRITICAL, "\n============================================================\n");
	dprintf(CRITICAL, "[%s] INHOUSE Partition addr             : %x\n", MODULE_NAME, addr);
	dprintf(CRITICAL, "[%s] INHOUSE Partition Name             : %s\n", MODULE_NAME, part_name);
	dprintf(CRITICAL, "[%s] INHOUSE IMG HDR - Magic            : %x\n", MODULE_NAME, mkimage_hdr.info.magic);
	dprintf(CRITICAL, "[%s] INHOUSE IMG size                    : %x\n", MODULE_NAME, mkimage_hdr.info.dsize);
	dprintf(CRITICAL, "[%s] INHOUSE IMG HDR size                : %x\n", MODULE_NAME, sizeof(part_hdr_t));
	dprintf(CRITICAL, "============================================================\n");

	*size =  (((mkimage_hdr.info.dsize + sizeof(part_hdr_t)  + page_size - 1) / page_size) * page_size);
	dprintf(CRITICAL, "[%s] INHOUSE IMG size           : %x\n", MODULE_NAME, *size);

	//mboot_common_load_part_info(dev, part_name, part_hdr);

	return ret;

}

unsigned int mboot_get_img_size(char *part_name, unsigned int *size)
{
	int ret = 0;
	long len = 0;
#if 1//#ifdef MTK_EMMC_SUPPORT
	u64 addr;
#else
	ulong addr;
#endif
	part_t *part;
	part_dev_t *dev;
	boot_img_hdr boot_hdr;
#define BOOT_SIG_HDR_SZ 16
	unsigned char boot_sig_hdr[BOOT_SIG_HDR_SZ] = {0};
	unsigned boot_sig_size = 0;
	unsigned page_size = 0x800; /* used to cache page size in boot image hdr, default 2KB */

	*size = 0;

	dev = mt_part_get_device();
	if (!dev) {
		dprintf(CRITICAL, "mboot_android_load_img_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		dprintf(CRITICAL, "mboot_android_load_img_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}
#ifdef MTK_EMMC_SUPPORT
	addr = (u64)part->start_sect * BLK_SIZE;
#else
	addr = (u64)part->startblk * BLK_SIZE;
#endif

#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, addr, (uchar*)&boot_hdr, sizeof(boot_img_hdr), part->part_id);
#else
	len = dev->read(dev, addr, (uchar*)&boot_hdr, sizeof(boot_img_hdr), part->part_id);
#endif
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s boot image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	dprintf(CRITICAL, "\n============================================================\n");
	boot_hdr.magic[7] = '\0';
	dprintf(CRITICAL, "[%s] Android Partition Name             : %s\n", MODULE_NAME, part_name);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Magic            : %s\n", MODULE_NAME, boot_hdr.magic);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Kernel Size      : 0x%08X\n", MODULE_NAME, boot_hdr.kernel_size);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Kernel Address   : 0x%08X\n", MODULE_NAME, boot_hdr.kernel_addr);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Rootfs Size      : 0x%08X\n", MODULE_NAME, boot_hdr.ramdisk_size);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Rootfs Address   : 0x%08X\n", MODULE_NAME, boot_hdr.ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Tags Address     : 0x%08X\n", MODULE_NAME, boot_hdr.tags_addr);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Page Size        : 0x%08X\n", MODULE_NAME, boot_hdr.page_size);
	dprintf(CRITICAL, "[%s] Android IMG Hdr - Command Line     : %s\n"  , MODULE_NAME, boot_hdr.cmdline);
	dprintf(CRITICAL, "============================================================\n");

	page_size = boot_hdr.page_size;
	*size +=  page_size; /* boot header size is 1 page*/
	*size +=  (((boot_hdr.kernel_size + page_size - 1) / page_size) * page_size);
	*size +=  (((boot_hdr.ramdisk_size + page_size - 1) / page_size) * page_size);
	*size +=  (((boot_hdr.second_size + page_size - 1) / page_size) * page_size);

	/* try to get boot siganture size if it exists */
#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, addr + (u64)(*size), (uchar*)&boot_sig_hdr, BOOT_SIG_HDR_SZ, part->part_id);
#else
	len = dev->read(dev, addr + (ulong)(*size), (uchar*)&boot_sig_hdr, BOOT_SIG_HDR_SZ, part->part_id);
#endif
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s boot sig header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}
#define ASN_ID_SEQUENCE  0x30
	if (boot_sig_hdr[0] == ASN_ID_SEQUENCE) {
		/* boot signature exists */
		unsigned len = 0;
		unsigned len_size = 0;
		if (boot_sig_hdr[1] & 0x80) {
			/* multi-byte length field */
			unsigned int i = 0;
			len_size = 1 + (boot_sig_hdr[1] & 0x7f);
			for (i = 0; i < len_size - 1; i++) {
				len = (len << 8) | boot_sig_hdr[2 + i];
			}
		} else {
			/* single-byte length field */
			len_size = 1;
			len = boot_sig_hdr[1];
		}

		boot_sig_size = 1 + len_size + len;
	}
	*size +=  (((boot_sig_size + page_size - 1) / page_size) * page_size);
	return ret;
}

#endif
