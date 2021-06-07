	.section .rodata
	.globl device_tree
	.globl device_tree_size
	.align 4
device_tree:
	.incbin "platform/mt8163_fpga.dtb"

	.align 4
device_tree_size:
	.long device_tree_size - device_tree
