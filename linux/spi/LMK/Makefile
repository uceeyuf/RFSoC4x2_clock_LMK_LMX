KERN_DIR := /home/xzy/XzyRepository/RFSoC4x2Radar/petalinux/XZYradar/build/tmp/work/zynqmp_generic_xczu48dr-amd-linux/linux-xlnx/6.12.10+git/linux-zynqmp_generic_xczu48dr-standard-build

obj-m := spi_lmk.o

all:
	make -C $(KERN_DIR) M=$(shell pwd) modules
clean:
	make -C $(KERN_DIR) M=$(shell pwd) clean
