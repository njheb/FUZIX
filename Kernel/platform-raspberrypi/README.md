To emulate:

Use the patched qemu here:

    https://github.com/0xabu/qemu/tree/raspi

$ qemu-system-arm -M raspi2 -kernel kernel-raspberrypi.elf -s -S -vga none

...and then in another terminal:

$ arm-linux-gnueabi-gdb --ex 'target remote localhost:1234' --ex 'file kernel-raspberrypi.elf' --tui

