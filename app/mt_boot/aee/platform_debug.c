#include <dev/aee_platform_debug.h>

int dfd_get(void **data, int *len) __attribute__((weak));

int dfd_get(void **data, int *len)
{
	return 0;
}

int plat_dram_debug_get(void **data, int *len) __attribute__((weak));

int plat_dram_debug_get(void **data, int *len)
{
	return 0;
}
