KERNEL_VERSION := $(shell uname -r)
KERNEL_PATH := /lib/modules/$(KERNEL_VERSION)/build

obj-m += ft5426_touch.o

.PHONY: help all clean dtbo install

help:
    @echo "Usage:"
    @echo "  make all      - Build the kernel module"
    @echo "  make clean    - Clean the build files"
    @echo "  make dtbo     - Compile the device tree overlay"
    @echo "  make install  - Install the module and overlay"

all:
    $(MAKE) -C $(KERNEL_PATH) M=$(PWD) modules

clean:
    $(MAKE) -C $(KERNEL_PATH) M=$(PWD) clean
    rm -f ft5426.dtbo

dtbo:
    dtc -@ -I dts -O dtb -o ft5426.dtbo ft5426-overlay.dts

install: all dtbo
    sudo cp ft5426.dtbo /boot/overlays/
    sudo bash -c 'echo "dtoverlay=ft5426" >> /boot/config.txt'
    sudo depmod -a
    sudo modprobe ft5426_touch
