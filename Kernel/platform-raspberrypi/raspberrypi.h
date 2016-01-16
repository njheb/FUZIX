#ifndef RASPBERRYPI_H
#define RASPBERRYPI_H

extern struct
{
	volatile uint32_t ARG2;           /* 00 */
	volatile uint32_t BLKSIZECNT;     /* 04 */
	volatile uint32_t ARG1;           /* 08 */
	volatile uint32_t CMDTM;          /* 0c */
	volatile uint32_t RESP0;          /* 10 */ 
	volatile uint32_t RESP1;          /* 14 */
	volatile uint32_t RESP2;          /* 18 */
	volatile uint32_t RESP3;          /* 1c */
	volatile uint32_t DATA;           /* 20 */
	volatile uint32_t STATUS;         /* 24 */
	volatile uint32_t CONTROL0;       /* 28 */
	volatile uint32_t CONTROL1;       /* 2c */
	volatile uint32_t INTERRUPT;      /* 30 */
	volatile uint32_t IRPT_MASK;      /* 34 */
	volatile uint32_t IRPT_EN;        /* 38 */
	volatile uint32_t CONTROL2;       /* 3c */
	volatile uint32_t CAPABILITIES0;  /* 40 */
	volatile uint32_t CAPABILITIES1;  /* 44 */
	uint32_t padding1[2];             /* 48 4c */
	volatile uint32_t FORCE_IRPT;     /* 50 */
	uint32_t padding2[7];             /* 54 58 5c 60 64 68 6c */
	volatile uint32_t BOOT_TIMEOUT;   /* 70 */
	volatile uint32_t DBG_SEL;        /* 74 */
	uint32_t padding3[2];             /* 78 7c */
	volatile uint32_t EXRDFIFO_CFG;   /* 80 */
	volatile uint32_t EXRDFIFO_EN;    /* 84 */
	volatile uint32_t TUNE_STEP;      /* 88 */
	volatile uint32_t TUNE_STEPS_STD; /* 8c */
	volatile uint32_t TUNE_STEPS_DDR; /* 90 */
	uint32_t padding4[23];            /* 94-ec */
	volatile uint32_t SPI_INT_SPT;    /* f0 */
	uint32_t padding5[2];             /* f4 f8 */
	volatile uint32_t SLOTISR_VER;    /* fc */
}
EMMC;

enum
{
	/* EMMC command flags */

	EMMC_CMD_TYPE_NORMAL  = 0x00000000,
	EMMC_CMD_TYPE_SUSPEND = 0x00400000,
	EMMC_CMD_TYPE_RESUME  = 0x00800000,
	EMMC_CMD_TYPE_ABORT   = 0x00c00000,
	EMMC_CMD_IS_DATA      = 0x00200000,
	EMMC_CMD_IXCHK_EN     = 0x00100000,
	EMMC_CMD_CRCCHK_EN    = 0x00080000,
	EMMC_CMD_RSPNS_NO     = 0x00000000,
	EMMC_CMD_RSPNS_136    = 0x00010000,
	EMMC_CMD_RSPNS_48     = 0x00020000,
	EMMC_CMD_RSPNS_48B    = 0x00030000,
	EMMC_TM_MULTI_BLOCK   = 0x00000020,
	EMMC_TM_DAT_DIR_HC    = 0x00000000,
	EMMC_TM_DAT_DIR_CH    = 0x00000010,
	EMMC_TM_AUTO_CMD23    = 0x00000008,
	EMMC_TM_AUTO_CMD12    = 0x00000004,
	EMMC_TM_BLKCNT_EN     = 0x00000002,
	EMMC_TM_MULTI_DATA    = (EMMC_CMD_IS_DATA | EMMC_TM_MULTI_BLOCK |
                             EMMC_TM_BLKCNT_EN),

	/* INTERRUPT register settings */

	EMMC_INT_AUTO_ERROR   = 0x01000000,
	EMMC_INT_DATA_END_ERR = 0x00400000,
	EMMC_INT_DATA_CRC_ERR = 0x00200000,
	EMMC_INT_DATA_TIMEOUT = 0x00100000,
	EMMC_INT_INDEX_ERROR  = 0x00080000,
	EMMC_INT_END_ERROR    = 0x00040000,
	EMMC_INT_CRC_ERROR    = 0x00020000,
	EMMC_INT_CMD_TIMEOUT  = 0x00010000,
	EMMC_INT_ERR          = 0x00008000,
	EMMC_INT_ENDBOOT      = 0x00004000,
	EMMC_INT_BOOTACK      = 0x00002000,
	EMMC_INT_RETUNE       = 0x00001000,
	EMMC_INT_CARD         = 0x00000100,
	EMMC_INT_READ_RDY     = 0x00000020,
	EMMC_INT_WRITE_RDY    = 0x00000010,
	EMMC_INT_BLOCK_GAP    = 0x00000004,
	EMMC_INT_DATA_DONE    = 0x00000002,
	EMMC_INT_CMD_DONE     = 0x00000001,
	EMMC_INT_ERROR_MASK   = (EMMC_INT_CRC_ERROR | EMMC_INT_END_ERROR | 
	                         EMMC_INT_INDEX_ERROR | EMMC_INT_DATA_TIMEOUT |
							 EMMC_INT_DATA_CRC_ERR | EMMC_INT_DATA_END_ERR |
						     EMMC_INT_ERR | EMMC_INT_AUTO_ERROR),
	EMMC_INT_ALL_MASK     = (EMMC_INT_CMD_DONE | EMMC_INT_DATA_DONE |
	                         EMMC_INT_READ_RDY | EMMC_INT_WRITE_RDY |
						     EMMC_INT_ERROR_MASK),

	/* CONTROL register settings */

	EMMC_C0_SPI_MODE_EN   = 0x00100000,
	EMMC_C0_HCTL_HS_EN    = 0x00000004,
	EMMC_C0_HCTL_DWITDH   = 0x00000002,

	EMMC_C1_SRST_DATA     = 0x04000000,
	EMMC_C1_SRST_CMD      = 0x02000000,
	EMMC_C1_SRST_HC       = 0x01000000,
	EMMC_C1_TOUNIT_DIS    = 0x000f0000,
	EMMC_C1_TOUNIT_MAX    = 0x000e0000,
	EMMC_C1_DATA_TOUNIT   = 0x00010000,
	EMMC_C1_CLK_FREQ8     = 0x00000100,
	EMMC_C1_CLK_FREQ_MS2  = 0x00000040,
	EMMC_C1_CLK_GENSEL    = 0x00000020,
	EMMC_C1_CLK_EN        = 0x00000004,
	EMMC_C1_CLK_STABLE    = 0x00000002,
	EMMC_C1_CLK_INTLEN    = 0x00000001,

	EMMC_FREQ_SETUP       =     400000,  /* 400 Khz */
	EMMC_FREQ_NORMAL      =   25000000,  /* 25 Mhz */

	/* CONTROL2 values */

	EMMC_C2_VDD_18        = 0x00080000,
	EMMC_C2_UHSMODE       = 0x00070000,
	EMMC_C2_UHS_SDR12     = 0x00000000,
	EMMC_C2_UHS_SDR25     = 0x00010000,
	EMMC_C2_UHS_SDR50     = 0x00020000,
	EMMC_C2_UHS_SDR104    = 0x00030000,
	EMMC_C2_UHS_DDR50     = 0x00040000,

	/* SLOTISR_VER values */

	EMMC_HOST_SPEC_NUM        = 0x00ff0000,
	EMMC_HOST_SPEC_NUM_SHIFT  = 16,
	EMMC_HOST_SPEC_V3         = 2,
	EMMC_HOST_SPEC_V2         = 1,
	EMMC_HOST_SPEC_V1         = 0,

	/* STATUS register settings */

	EMMC_SR_DAT_LEVEL1        = 0x1e000000,
	EMMC_SR_CMD_LEVEL         = 0x01000000,
	EMMC_SR_DAT_LEVEL0        = 0x00f00000,
	EMMC_SR_DAT3              = 0x00800000,
	EMMC_SR_DAT2              = 0x00400000,
	EMMC_SR_DAT1              = 0x00200000,
	EMMC_SR_DAT0              = 0x00100000,
	EMMC_SR_WRITE_PROT        = 0x00080000,  /* From SDHC spec v2, BCM says reserved */
	EMMC_SR_READ_AVAILABLE    = 0x00000800,  /* ???? undocumented */
	EMMC_SR_WRITE_AVAILABLE   = 0x00000400,  /* ???? undocumented */
	EMMC_SR_READ_TRANSFER     = 0x00000200,
	EMMC_SR_WRITE_TRANSFER    = 0x00000100,
	EMMC_SR_DAT_ACTIVE        = 0x00000004,
	EMMC_SR_DAT_INHIBIT       = 0x00000002,
	EMMC_SR_CMD_INHIBIT       = 0x00000001,

	/* R1 values */

	EMMC_R1_OUT_OF_RANGE      = 0x80000000,  /* 31   E */ 
	EMMC_R1_ADDRESS_ERROR     = 0x40000000,  /* 30   E */
	EMMC_R1_BLOCK_LEN_ERROR   = 0x20000000,  /* 29   E */
	EMMC_R1_ERASE_SEQ_ERROR   = 0x10000000,  /* 28   E */
	EMMC_R1_ERASE_PARAM_ERROR = 0x08000000,  /* 27   E */
	EMMC_R1_WP_VIOLATION      = 0x04000000,  /* 26   E */
	EMMC_R1_CARD_IS_LOCKED    = 0x02000000,  /* 25   E */
	EMMC_R1_LOCK_UNLOCK_FAIL  = 0x01000000,  /* 24   E */
	EMMC_R1_COM_CRC_ERROR     = 0x00800000,  /* 23   E */
	EMMC_R1_ILLEGAL_COMMAND   = 0x00400000,  /* 22   E */
	EMMC_R1_CARD_ECC_FAILED   = 0x00200000,  /* 21   E */
	EMMC_R1_CC_ERROR          = 0x00100000,  /* 20   E */
	EMMC_R1_ERROR             = 0x00080000,  /* 19   E */
	EMMC_R1_CSD_OVERWRITE     = 0x00010000,  /* 16   E */
	EMMC_R1_WP_ERASE_SKIP     = 0x00008000,  /* 15   E */
	EMMC_R1_CARD_ECC_DISABLED = 0x00004000,  /* 14   E */
	EMMC_R1_ERASE_RESET       = 0x00002000,  /* 13   E */
	EMMC_R1_CARD_STATE        = 0x00001e00,  /* 12:9   */
	EMMC_R1_READY_FOR_DATA    = 0x00000100,  /* 8      */
	EMMC_R1_APP_CMD           = 0x00000020,  /* 5      */
	EMMC_R1_AKE_SEQ_ERROR     = 0x00000004,  /* 3    E */

	EMMC_R1_ERRORS_MASK       = 0xfff9c004,  /* All above bits which indicate errors */
};

extern struct
{
	volatile uint32_t CMD;      /* 00 */
	volatile uint32_t ARG;      /* 04 */
	volatile uint32_t TIMEOUT;  /* 08 */
	volatile uint32_t CLKDIV;   /* 0c */
	volatile uint32_t RSP0;     /* 10 */
	volatile uint32_t RSP1;     /* 14 */
	volatile uint32_t RSP2;     /* 18 */
	volatile uint32_t RSP3;     /* 1c */
	volatile uint32_t STATUS;   /* 20 */
	volatile uint32_t UNK_0x24; /* 24 */
	volatile uint32_t UNK_0x28; /* 28 */
	volatile uint32_t UNK_0x2c; /* 2c */
	volatile uint32_t VDD;      /* 30 */
	volatile uint32_t EDM;      /* 34 */
	volatile uint32_t HOST_CFG; /* 38 */
	volatile uint32_t HBCT;     /* 3c */
	volatile uint32_t DATA;     /* 40 */
	volatile uint32_t UNK_0x44; /* 44 */
	volatile uint32_t UNK_0x48; /* 48 */
	volatile uint32_t UNK_0x4c; /* 4c */
	volatile uint32_t HBLC;     /* 50 */
}
ALTMMC;

enum
{
	/* cmd register */

	ALTMMC_ENABLE = 1<<15,
    ALTMMC_FAIL = 1<<14,
    ALTMMC_BUSY = 1<<11,
    ALTMMC_NO_RSP = 1<<10,
    ALTMMC_LONG_RSP = 1<<9,
    ALTMMC_WRITE = 1<<7,
    ALTMMC_READ = 1<<6,

    /* status register */

    ALTMMC_FIFO_STATUS = 1<<0,

	/* SD card response bits */

	SD_R1_IDLE            = 1<<0,
	SD_R1_ERASE_RESET     = 1<<1,
	SD_R1_ILLEGAL_COMMAND = 1<<2,
	SD_R1_CRC_ERROR       = 1<<3,
	SD_R1_ERASE_ERROR     = 1<<4,
	SD_R1_ADDRESS_ERROR   = 1<<5,
	SD_R1_PARAMETER_ERROR = 1<<6,

	/* SD card OCR register bits */

	SD_OCR_V1_6           = 1<<4,
	SD_OCR_V1_7           = 1<<5,
	SD_OCR_V1_8           = 1<<6,
	SD_OCR_V1_9           = 1<<7,
	SD_OCR_V2_0           = 1<<8,
	SD_OCR_V2_1           = 1<<9,
	SD_OCR_V2_2           = 1<<10,
	SD_OCR_V2_3           = 1<<11,
	SD_OCR_V2_4           = 1<<12,
	SD_OCR_V2_5           = 1<<13,
	SD_OCR_V2_6           = 1<<14,
	SD_OCR_V2_7           = 1<<15,
	SD_OCR_V2_8           = 1<<16,
	SD_OCR_V2_9           = 1<<17,
	SD_OCR_V3_0           = 1<<18,
	SD_OCR_V3_1           = 1<<19,
	SD_OCR_V3_2           = 1<<20,
	SD_OCR_V3_3           = 1<<21,
	SD_OCR_V3_4           = 1<<22,
	SD_OCR_V3_5           = 1<<23,
	SD_OCR_HIGHCAP        = 1<<30,
	SD_OCR_BUSY           = 1<<31,
};

extern struct
{
	volatile uint32_t FSEL0;     /* 00 */
	volatile uint32_t FSEL1;     /* 04 */
	volatile uint32_t FSEL2;     /* 08 */
	volatile uint32_t FSEL3;     /* 0c */
	volatile uint32_t FSEL4;     /* 10 */
	volatile uint32_t FSEL5;     /* 14 */
	uint32_t _padding1;          /* 18 */
	volatile uint32_t SET0;      /* 1c */
	volatile uint32_t SET1;      /* 20 */
	uint32_t _padding2;          /* 24 */
	volatile uint32_t CLR0;      /* 28 */
	volatile uint32_t CLR1;      /* 2c */
	uint32_t _padding3;          /* 30 */
	volatile uint32_t LEV0;      /* 34 */
	volatile uint32_t LEV1;      /* 38 */
	uint32_t _padding4;          /* 3c */
	volatile uint32_t EDS0;      /* 40 */
	volatile uint32_t EDS1;      /* 44 */
	uint32_t _padding5;          /* 48 */
	volatile uint32_t REN0;      /* 4c */
	volatile uint32_t REN1;      /* 50 */
	uint32_t _padding6;          /* 54 */
	volatile uint32_t FEN0;      /* 58 */
	volatile uint32_t FEN1;      /* 5c */
	uint32_t _padding7;          /* 60 */
	volatile uint32_t HEN0;      /* 64 */
	volatile uint32_t HEN1;      /* 68 */
	uint32_t _padding8;          /* 6c */
	volatile uint32_t LEN0;      /* 70 */
	volatile uint32_t LEN1;      /* 74 */
	uint32_t _padding9;          /* 78 */
	volatile uint32_t AREN0;     /* 7c */
	volatile uint32_t AREN1;     /* 80 */
	uint32_t _paddinga;          /* 84 */
	volatile uint32_t AFEN0;     /* 88 */
	volatile uint32_t AFEN1;     /* 8c */
	uint32_t _paddingb;          /* 90 */
	volatile uint32_t PUD;       /* 94 */
	volatile uint32_t PUDCLK0;   /* 98 */
	volatile uint32_t PUDCLK1;   /* 9c */
}
GPIO;

enum
{
	GPIO_FUNC_INPUT = 0,
	GPIO_FUNC_OUTPUT = 1,
	GPIO_FUNC_ALT0 = 4,
	GPIO_FUNC_ALT1 = 5,
	GPIO_FUNC_ALT2 = 6,
	GPIO_FUNC_ALT3 = 7,
	GPIO_FUNC_ALT4 = 3,
	GPIO_FUNC_ALT5 = 2,

	GPIO_PULL_OFF = 0,
	GPIO_PULL_DOWN = 1,
	GPIO_PULL_UP = 2
};

extern struct
{
	volatile uint32_t DR;    /* 00 */
	volatile uint32_t RSRECR;/* 04 */
	uint32_t _padding1;      /* 08 */
	uint32_t _padding2;      /* 0c */
	uint32_t _padding3;      /* 10 */
	uint32_t _padding4;      /* 14 */
	volatile uint32_t FR;    /* 18 */
	uint32_t _padding5;      /* 1c */
	volatile uint32_t ILPR;  /* 20 */
	volatile uint32_t IBRD;  /* 24 */
	volatile uint32_t FBRD;  /* 28 */
	volatile uint32_t LCRH;  /* 2c */
	volatile uint32_t CR;    /* 30 */
	volatile uint32_t IFLS;  /* 34 */
	volatile uint32_t IMSC;  /* 38 */
	volatile uint32_t RIS;   /* 3c */
	volatile uint32_t MIS;   /* 40 */
	volatile uint32_t ICR;   /* 44 */
	volatile uint32_t DMACR; /* 48 */
	uint32_t _padding6;      /* 4c */
	volatile uint32_t ITCR;  /* 80 */
	volatile uint32_t ITIP;  /* 84 */
	volatile uint32_t ITOP;  /* 88 */
	volatile uint32_t TDR;   /* 8c */
}
UART0;

extern struct
{
	volatile uint32_t READ;    /* 00 */
	uint32_t padding1[3];
	volatile uint32_t PEEK;    /* 10 */
	volatile uint32_t SENDER;  /* 14 */
	volatile uint32_t STATUS;  /* 18 */
	volatile uint32_t CONFIG;  /* 1c */
	volatile uint32_t WRITE;   /* 20 */
}
SBM;

enum
{
	MAIL_EMPTY = (1<<30),
	MAIL_FULL = (1<<31)
};

#define invalidate_insn_cache()     mcr(15, 0, 7, 5, 0, 0)
#define flush_prefetch_buffer()     mcr(15, 0, 7, 5, 4, 0)
#define flush_branch_target_cache() mcr(15, 0, 7, 5, 6, 0)
#define invalidate_data_cache()     mcr(15, 0, 7, 6, 0, 0)
#define clean_data_cache()          mcr(15, 0, 7, 10, 0, 0)
#define data_sync_barrier()         mcr(15, 0, 7, 10, 4, 0)
#define data_mem_barrier()          mcr(15, 0, 7, 10, 5, 0)
#define insn_sync_barrier()         flush_prefetch_buffer()
#define insn_mem_barrier()          flush_prefetch_buffer()

#define invalidate_tlb()            mcr(15, 0, 8, 7, 0, 0)

extern void gpio_set_pin_func(int pin, int func, int mode);
extern void gpio_set_output_pin(int pin, bool value);
extern void led_init(void);
extern void led_set(bool value);
extern void led_on(void);
extern void led_off(void);
extern void led_halt_and_blink(int count);

enum
{
	MBOX_PROCESS_REQUEST = 0x00000000,
	MBOX_REQUEST_OK      = 0x80000000,
	MBOX_REQUEST_ERROR   = 0x80000001,

	MBOX_END             = 0,
};

enum
{
	MBOX_PROP_CLOCK_EMMC  = 1,
	MBOX_PROP_CLOCK_UART  = 2,
	MBOX_PROP_CLOCK_ARM   = 3,
	MBOX_PROP_CLOCK_CORE  = 4,
	MBOX_PROP_CLOCK_V3D   = 5,
	MBOX_PROP_CLOCK_H264  = 6,
	MBOX_PROP_CLOCK_ISP   = 7,
	MBOX_PROP_CLOCK_SDRAM = 8,
	MBOX_PROP_CLOCK_PIXEL = 9,
	MBOX_PROP_CLOCK_PWM   = 10,

	MBOX_DEVICE_SDCARD = 0,
	MBOX_DEVICE_UART0  = 1,
	MBOX_DEVICE_UART1  = 2,
	MBOX_DEVICE_USB    = 3,
	MBOX_DEVICE_I2C0   = 4,
	MBOX_DEVICE_I2C1   = 5,
	MBOX_DEVICE_I2C2   = 6,
	MBOX_DEVICE_SPI    = 7,
	MBOX_DEVICE_CCP2TX = 8,

	MBOX_GET_FIRMWARE_REVISION = 0x00000001,
	MBOX_GET_BOARD_MODEL       = 0x00010001,
	MBOX_GET_BOARD_REVISION    = 0x00010002,
	MBOX_GET_MAC_ADDRESS       = 0x00010003,
	MBOX_GET_BOARD_SERIAL      = 0x00010004,
	MBOX_GET_ARM_MEMORY        = 0x00010005,
	MBOX_GET_VC_MEMORY         = 0x00010006,
	MBOX_GET_POWER_STATE       = 0x00020001,
	MBOX_SET_POWER_STATE       = 0x00028001,
	MBOX_GET_CLOCK_STATE       = 0x00030001,
	MBOX_SET_CLOCK_STATE       = 0x00038001,
	MBOX_GET_CLOCK_RATE        = 0x00030002,
	MBOX_SET_CLOCK_RATE        = 0x00038002,
	MBOX_GET_CLOCK_MAX_RATE    = 0x00030004,
	MBOX_GET_CLOCK_MIN_RATE    = 0x00030007,


};

extern uint32_t mbox_get_arm_memory(void);
extern int mbox_get_clock_rate(int clockid);
extern int mbox_get_power_state(int deviceid);
extern void mbox_set_power_state(int deviceid, bool state);

extern struct
{
	volatile uint32_t CS;  /* 00 */
	volatile uint32_t CLO; /* 04 */
	volatile uint32_t CHI; /* 08 */
	volatile uint32_t C0;  /* 0c */
	volatile uint32_t C1;  /* 10 */
	volatile uint32_t C2;  /* 14 */
	volatile uint32_t C3;  /* 18 */
}
SYSTIMER;

enum
{
	SYSTIMER_CS_M0 = 1<<0,
	SYSTIMER_CS_M1 = 1<<1,
	SYSTIMER_CS_M2 = 1<<2,
	SYSTIMER_CS_M3 = 1<<3,
};

extern struct
{
	volatile uint32_t PENDINGB; /* 00 */
	volatile uint32_t PENDING1; /* 04 */
	volatile uint32_t PENDING2; /* 08 */
	volatile uint32_t FIQ;      /* 0c */
	volatile uint32_t ENABLE1;  /* 10 */
	volatile uint32_t ENABLE2;  /* 14 */
	volatile uint32_t ENABLEB;  /* 18 */
	volatile uint32_t DISABLE1; /* 1c */
	volatile uint32_t DISABLE2; /* 20 */
	volatile uint32_t DISABLEB; /* 24 */
}
ARMIC;

enum
{
	ARMIC_IRQ1_TIMER0          = 0,
	ARMIC_IRQ1_TIMER1          = 1,
	ARMIC_IRQ1_TIMER2          = 2,
	ARMIC_IRQ1_TIMER3          = 3,
	ARMIC_IRQ1_CODEC0          = 4,
	ARMIC_IRQ1_CODEC1          = 5,
	ARMIC_IRQ1_CODEC2          = 6,
	ARMIC_IRQ1_JPEG            = 7,
	ARMIC_IRQ1_ISP             = 8,
	ARMIC_IRQ1_USB             = 9,
	ARMIC_IRQ1_3D              = 10,
	ARMIC_IRQ1_TRANSPOSER      = 11,
	ARMIC_IRQ1_MULTICORESYNC0  = 12,
	ARMIC_IRQ1_MULTICORESYNC1  = 13,
	ARMIC_IRQ1_MULTICORESYNC2  = 14,
	ARMIC_IRQ1_MULTICORESYNC3  = 15,
	ARMIC_IRQ1_DMA0            = 16,
	ARMIC_IRQ1_DMA1            = 17,
	ARMIC_IRQ1_DMA2            = 18,
	ARMIC_IRQ1_DMA3            = 19,
	ARMIC_IRQ1_DMA4            = 20,
	ARMIC_IRQ1_DMA5            = 21,
	ARMIC_IRQ1_DMA6            = 22,
	ARMIC_IRQ1_DMA7            = 23,
	ARMIC_IRQ1_DMA8            = 24,
	ARMIC_IRQ1_DMA9            = 25,
	ARMIC_IRQ1_DMA10           = 26,
	ARMIC_IRQ1_DMA11           = 27,
	ARMIC_IRQ1_DMA12           = 28,
	ARMIC_IRQ1_AUX             = 29,
	ARMIC_IRQ1_ARM             = 30,
	ARMIC_IRQ1_VPUDMA          = 31,

	ARMIC_IRQ2_HOSTPORT        = 0,
	ARMIC_IRQ2_VIDEOSCALER     = 1,
	ARMIC_IRQ2_CCP2TX          = 2,
	ARMIC_IRQ2_SDC             = 3,
	ARMIC_IRQ2_DSI0            = 4,
	ARMIC_IRQ2_AVE             = 5,
	ARMIC_IRQ2_CAM0            = 6,
	ARMIC_IRQ2_CAM1            = 7,
	ARMIC_IRQ2_HDMI0           = 8,
	ARMIC_IRQ2_HDMI1           = 9,
	ARMIC_IRQ2_PIXELVALVE1     = 10,
	ARMIC_IRQ2_I2CSPISLV       = 11,
	ARMIC_IRQ2_DSI1            = 12,
	ARMIC_IRQ2_PWA0            = 13,
	ARMIC_IRQ2_PWA1            = 14,
	ARMIC_IRQ2_CPR             = 15,
	ARMIC_IRQ2_SMI             = 16,
	ARMIC_IRQ2_GPIO0           = 17,
	ARMIC_IRQ2_GPIO1           = 18,
	ARMIC_IRQ2_GPIO2           = 19,
	ARMIC_IRQ2_GPIO3           = 20,
	ARMIC_IRQ2_I2C             = 21,
	ARMIC_IRQ2_SPI             = 22,
	ARMIC_IRQ2_I2SPCM          = 23,
	ARMIC_IRQ2_SDIO            = 24,
	ARMIC_IRQ2_UART            = 25,
	ARMIC_IRQ2_SLIMBUS         = 26,
	ARMIC_IRQ2_VEC             = 27,
	ARMIC_IRQ2_CPG             = 28,
	ARMIC_IRQ2_RNG             = 29,
	ARMIC_IRQ2_ARASANSDIO      = 30,
	ARMIC_IRQ2_AVSPMON         = 31,

	ARMIC_IRQB_ARM_TIMER       = 0,
	ARMIC_IRQB_ARM_MAILBOX     = 1,
	ARMIC_IRQB_ARM_DOORBELL_0  = 2,
	ARMIC_IRQB_ARM_DOORBELL_1  = 3,
	ARMIC_IRQB_VPU0_HALTED     = 4,
	ARMIC_IRQB_VPU1_HALTED     = 5,
	ARMIC_IRQB_ILLEGAL_TYPE0   = 6,
	ARMIC_IRQB_ILLEGAL_TYPE1   = 7,
};

#endif


