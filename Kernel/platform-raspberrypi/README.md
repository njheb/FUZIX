To emulate:

Use the patched qemu here:

    https://github.com/0xabu/qemu/tree/raspi

$ qemu-system-arm -M raspi2 -kernel kernel-raspberrypi.elf -s -S -vga none

...and then in another terminal:

$ arm-linux-gnueabi-gdb --ex 'target remote localhost:1234' --ex 'file kernel-raspberrypi.elf' --tui

Essential reading:

http://infocenter.arm.com/help/topic/com.arm.doc.ddi0301h/DDI0301H_arm1176jzfs_r0p7_trm.pdf
https://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf

