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
	../devio.c \
	../devsys.c \
	../kdata.c \
	../mm.c \
	../process.c \
	../start.c \
	../tty.c \
	../usermem.c \
	crt0.S \
	devices.c \
	devtty.c \
	libc.c \
	main.c \
	tricks.s \
	$(kernelversion.result)

kernel.includes += \

kernel.cflags += \
	-Wno-int-to-pointer-cast \
	-Wno-pointer-to-int-cast \
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

