#ifndef RASPBERRYPI_H
#define RASPBERRYPI_H

extern struct
{
	volatile uint32_t EMMC_ARG2;
	volatile uint32_t EMMC_BLKSIZECNT;
	volatile uint32_t EMMC_ARG1;
	volatile uint32_t EMMC_CMDTM;
	volatile uint32_t EMMC_RESP0;
	volatile uint32_t EMMC_RESP1;
	volatile uint32_t EMMC_RESP2;
	volatile uint32_t EMMC_RESP3;
	volatile uint32_t EMMC_DATA;
	volatile uint32_t EMMC_STATUS;
	volatile uint32_t EMMC_CONTROL0;
	volatile uint32_t EMMC_CONTROL1;
	volatile uint32_t EMMC_INTERRUPT;
	volatile uint32_t EMMC_IRPT_MASK;
	volatile uint32_t EMMC_IRPT_EN;
	volatile uint32_t EMMC_CONTROL2;
	volatile uint32_t EMMC_HOST_CAPS;
	uint32_t padding1[3];
	volatile uint32_t EMMC_BOOT_TIMEOUT;
	uint32_t padding2[4];
	volatile uint32_t EMMC_EXRDFIFO_EN;
	uint32_t padding3[28];
	volatile uint32_t EMMC_SPI_INT_SPT;
	uint32_t padding4[2];
	volatile uint32_t EMMC_SLOTISR_VER;
}
EMMC;

enum
{
	/* EMMC command flags */

	CMD_TYPE_NORMAL  = 0x00000000,
	CMD_TYPE_SUSPEND = 0x00400000,
	CMD_TYPE_RESUME  = 0x00800000,
	CMD_TYPE_ABORT   = 0x00c00000,
	CMD_IS_DATA      = 0x00200000,
	CMD_IXCHK_EN     = 0x00100000,
	CMD_CRCCHK_EN    = 0x00080000,
	CMD_RSPNS_NO     = 0x00000000,
	CMD_RSPNS_136    = 0x00010000,
	CMD_RSPNS_48     = 0x00020000,
	CMD_RSPNS_48B    = 0x00030000,
	TM_MULTI_BLOCK   = 0x00000020,
	TM_DAT_DIR_HC    = 0x00000000,
	TM_DAT_DIR_CH    = 0x00000010,
	TM_AUTO_CMD23    = 0x00000008,
	TM_AUTO_CMD12    = 0x00000004,
	TM_BLKCNT_EN     = 0x00000002,
	TM_MULTI_DATA    = (CMD_IS_DATA|TM_MULTI_BLOCK|TM_BLKCNT_EN),

	/* INTERRUPT register settings */

	INT_AUTO_ERROR   = 0x01000000,
	INT_DATA_END_ERR = 0x00400000,
	INT_DATA_CRC_ERR = 0x00200000,
	INT_DATA_TIMEOUT = 0x00100000,
	INT_INDEX_ERROR  = 0x00080000,
	INT_END_ERROR    = 0x00040000,
	INT_CRC_ERROR    = 0x00020000,
	INT_CMD_TIMEOUT  = 0x00010000,
	INT_ERR          = 0x00008000,
	INT_ENDBOOT      = 0x00004000,
	INT_BOOTACK      = 0x00002000,
	INT_RETUNE       = 0x00001000,
	INT_CARD         = 0x00000100,
	INT_READ_RDY     = 0x00000020,
	INT_WRITE_RDY    = 0x00000010,
	INT_BLOCK_GAP    = 0x00000004,
	INT_DATA_DONE    = 0x00000002,
	INT_CMD_DONE     = 0x00000001,
	INT_ERROR_MASK   = (INT_CRC_ERROR|INT_END_ERROR|INT_INDEX_ERROR|
						INT_DATA_TIMEOUT|INT_DATA_CRC_ERR|INT_DATA_END_ERR|
						INT_ERR|INT_AUTO_ERROR),
	INT_ALL_MASK     = (INT_CMD_DONE|INT_DATA_DONE|INT_READ_RDY|INT_WRITE_RDY|
						INT_ERROR_MASK),

	/* CONTROL register settings */

	C0_SPI_MODE_EN   = 0x00100000,
	C0_HCTL_HS_EN    = 0x00000004,
	C0_HCTL_DWITDH   = 0x00000002,

	C1_SRST_DATA     = 0x04000000,
	C1_SRST_CMD      = 0x02000000,
	C1_SRST_HC       = 0x01000000,
	C1_TOUNIT_DIS    = 0x000f0000,
	C1_TOUNIT_MAX    = 0x000e0000,
	C1_CLK_GENSEL    = 0x00000020,
	C1_CLK_EN        = 0x00000004,
	C1_CLK_STABLE    = 0x00000002,
	C1_CLK_INTLEN    = 0x00000001,

	FREQ_SETUP       =     400000,  /* 400 Khz */
	FREQ_NORMAL      =   25000000,  /* 25 Mhz */

	/* CONTROL2 values */

	C2_VDD_18        = 0x00080000,
	C2_UHSMODE       = 0x00070000,
	C2_UHS_SDR12     = 0x00000000,
	C2_UHS_SDR25     = 0x00010000,
	C2_UHS_SDR50     = 0x00020000,
	C2_UHS_SDR104    = 0x00030000,
	C2_UHS_DDR50     = 0x00040000,

	/* SLOTISR_VER values */

	HOST_SPEC_NUM              = 0x00ff0000,
	HOST_SPEC_NUM_SHIFT        = 16,
	HOST_SPEC_V3               = 2,
	HOST_SPEC_V2               = 1,
	HOST_SPEC_V1               = 0,

	/* STATUS register settings */

	SR_DAT_LEVEL1        = 0x1e000000,
	SR_CMD_LEVEL         = 0x01000000,
	SR_DAT_LEVEL0        = 0x00f00000,
	SR_DAT3              = 0x00800000,
	SR_DAT2              = 0x00400000,
	SR_DAT1              = 0x00200000,
	SR_DAT0              = 0x00100000,
	SR_WRITE_PROT        = 0x00080000,  /* From SDHC spec v2, BCM says reserved */
	SR_READ_AVAILABLE    = 0x00000800,  /* ???? undocumented */
	SR_WRITE_AVAILABLE   = 0x00000400,  /* ???? undocumented */
	SR_READ_TRANSFER     = 0x00000200,
	SR_WRITE_TRANSFER    = 0x00000100,
	SR_DAT_ACTIVE        = 0x00000004,
	SR_DAT_INHIBIT       = 0x00000002,
	SR_CMD_INHIBIT       = 0x00000001,

};

extern struct
{
	volatile uint32_t CMD;     /* 00 */
	volatile uint32_t ARG;     /* 04 */
	volatile uint32_t TIMEOUT; /* 08 */
	volatile uint32_t CLKDIV;  /* 0c */
	volatile uint32_t RSP0;    /* 10 */
	volatile uint32_t RSP1;    /* 14 */
	volatile uint32_t RSP2;    /* 18 */
	volatile uint32_t RSP3;    /* 1c */
	volatile uint32_t STATUS;  /* 20 */
	volatile uint32_t UNK_0x24;
	volatile uint32_t UNK_0x28;
	volatile uint32_t UNK_0x2c;
	volatile uint32_t VDD;
	volatile uint32_t EDM;
	volatile uint32_t HOST_CFG;
	volatile uint32_t HBCT;
	volatile uint32_t DATA;
	volatile uint32_t UNK_0x44;
	volatile uint32_t UNK_0x48;
	volatile uint32_t UNK_0x4c;
	volatile uint32_t HBLC;
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

    ALTMMC_FIFO_STATUS = 1<<0
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
	uint32_t padding2[3];
}
SBM;

enum
{
	MAIL_EMPTY = (1<<31),
	MAIL_FULL = (1<<30)
};

extern void gpio_set_pin_func(int pin, int func, int mode);
extern void mbox_write(int channel, uint32_t value);
extern uint32_t mbox_read(int channel);

#endif


