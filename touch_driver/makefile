# FILE: Makefile
KERNEL_VERSION := $(shell uname -r)
KERNEL_PATH := /lib/modules/$(KERNEL_VERSION)/build

obj-m += ft5426_touch.o

all:
    $(MAKE) -C $(KERNEL_PATH) M=$(PWD) modules

clean:
    $(MAKE) -C $(KERNEL_PATH) M=$(PWD) clean

dtbo:
    dtc -@ -I dts -O dtb -o ft5426.dtbo ft5426-overlay.dts

install:
    sudo cp ft5426.dtbo /boot/overlays/
    sudo dtoverlay ft5426
    sudo insmod ft5426_touch.ko