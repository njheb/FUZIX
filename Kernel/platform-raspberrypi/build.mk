$(call find-makefile)

kernelversion.ext = c
kernelversion.srcs = $(abspath $(TOP)/Kernel/makeversion)
$(call build, kernelversion, nop)
$(kernelversion.result):
	@echo MAKEVERSION $@
	$(hide) mkdir -p $(dir $@)
	$(hide) (cd $(dir $@) && $(kernelversion.abssrcs) $(VERSION) $(SUBVERSION))
	$(hide) mv $(dir $@)/version.c $@

elfkernel.srcs = \
	../bankfixed.c \
	../dev/blkdev.c \
	../dev/mbr.c \
	../devio.c \
	../devsys.c \
	../filesys.c \
	../inode.c \
	../kdata.c \
	../malloc.c \
	../mm.c \
	../process.c \
	../start.c \
	../syscall_exec32.c \
	../syscall_fs.c \
	../timer.c \
	../tty.c \
	../usermem.c \
	crt0.S \
	devices.c \
	devsd_altmmc.c \
	devtty.c \
	libc.c \
	main.c \
	raspberrypi.c \
	tricks.s \
	$(kernelversion.result)

elfkernel.includes += \

elfkernel.cflags += \
	-Wno-int-to-pointer-cast \
	-Wno-pointer-to-int-cast \
	-Wno-parentheses \
	-fno-inline \
	-fno-common \
	-mno-unaligned-access \
	-std=gnu99 \
	-g -gdwarf-2 \

elfkernel.asflags += \
	-g -gdwarf-2 \

elfkernel.ldflags += \
	--relax \
	-g

elfkernel.result = $(abspath $(TOP)/kernel-$(PLATFORM).elf)
$(call build, elfkernel, kernel-elf)

kernel.ext = img
kernel.srcs = $(elfkernel.result)
kernel.result = $(TOP)/kernel-$(PLATFORM).img
$(call build, kernel, nop)
$(kernel.result):
	@echo OBJCOPY $@
	$(hide) mkdir -p $(dir $@)
	$(hide) $(TARGETOBJCOPY) -O binary $(elfkernel.result) $(kernel.result)

