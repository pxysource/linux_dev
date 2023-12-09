KERN_DIR := /home/linux/workspace/zdyz_zynq7020/xenomai_2.6.3_project/build_root/linux

export ARCH=arm
export CROSS_COMPILE=arm-xilinx-linux-gnueabi-

obj-m := key_irq.o

all:
	make ARCH=arm -C $(KERN_DIR) M=`pwd` modules

clean:
	make -C $(KERN_DIR) M=`pwd` clean
