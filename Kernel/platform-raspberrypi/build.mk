$(call find-makefile)

kernelversion.ext = c
kernelversion.srcs = $(abspath $(TOP)/Kernel/makeversion)
$(call build, kernelversion, nop)
$(kernelversion.result):
	@echo MAKEVERSION $@
	$(hide) mkdir -p $(dir $@)
	$(hide) (cd $(dir $@) && $(kernelversion.abssrcs) $(VERSION) $(SUBVERSION))
	$(hide) mv $(dir $@)/version.c $@

kernel.srcs = \
	../bankfixed.c \
	../dev/blkdev.c \
	../dev/devsd.c \
	../dev/devsd_discard.c \
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
	tricks.s \
	$(kernelversion.result)

kernel.includes += \

kernel.cflags += \
	-Wno-int-to-pointer-cast \
	-Wno-pointer-to-int-cast \
	-Wno-parentheses \
	-fno-inline \
	-fno-common \
	-g -gdwarf-2 \

kernel.asflags += \
	-g -gdwarf-2 \

kernel.ldflags += \
	--relax \
	-g

kernel.result = $(TOP)/kernel-$(PLATFORM).elf
$(call build, kernel, kernel-elf)

