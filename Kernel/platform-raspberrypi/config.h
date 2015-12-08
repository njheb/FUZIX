#include <stdint.h>

/* Our bigness makes Fuzix uncomfortable. */
#define CONFIG_32BIT

/* Enable to make ^Z dump the inode table for debug */
#undef CONFIG_IDUMP
/* Enable to make ^A drop back into the monitor */
#undef CONFIG_MONITOR
/* Profil syscall support (not yet complete) */
#undef CONFIG_PROFIL
/* Multiple processes in memory at once */
#undef CONFIG_MULTI
#define PTABSIZE 100 /* Actually we can fit 1023 */
#define MAX_SWAPS PTABSIZE

#define CONFIG_USERMEM_DIRECT

/* Simple user copies for now (change when ROM the kernel) */
#define BANK_KERNEL /* */
#define BANK_PROCESS /* */

#define CONFIG_BANK_FIXED
#define CONFIG_BANKS 1
/* Banked Kernel: need to fix GCC first */
#undef CONFIG_BANKED
#undef SWAPDEV

/* Video terminal, not a serial tty */
#undef CONFIG_VT

extern int __user_base;
extern int __user_top;

extern int __swap_base;
extern int __swap_top;
extern int __swap_size_blocks;

#define TICKSPERSEC 64   /* Ticks per second */

extern uint8_t* __progbase;
extern uint8_t* __progtop;

#define PROGBASE    ((uaddr_t)&__progbase)
#define PROGLOAD    PROGBASE /* also data base */
#define PROGTOP     ((uaddr_t)&__progtop)  /* Top of program */
#define PROC_SIZE   ((PROGTOP - PROGBASE) / 1024)

#define MAP_BASE    PROGBASE
#define MAP_SIZE    (PROGTOP - PROGBASE)
#define MAX_MAPS    PTABSIZE

#define BOOT_TTY (512 + 1)   /* Set this to default device for stdio, stderr */
                          /* In this case, the default is the first TTY device */
                            /* Temp FIXME set to serial port for debug ease */

/* We need a tidier way to do this from the loader */
#define CMDLINE	"0"	  /* Location of root dev name */

/* Device parameters */
#define NUM_DEV_TTY 1
#define NDEVS    1        /* Devices 0..NDEVS-1 are capable of being mounted */
                          /*  (add new mountable devices to beginning area.) */
#define TTYDEV   BOOT_TTY /* Device used by kernel for messages, panics */
#define NBUFS    64       /* Number of block buffers */
#define NMOUNTS	 1	      /* Number of mounts at a time */
#define UFTSIZE  15       /* Number of user files */
#define OFTSIZE  15       /* Number of open files */

#define SD_DRIVE_COUNT 1
#define MAX_BLKDEV 1

#define BOOTDEVICE 0x0001 /* hda1 */

extern void platform_discard(void);

