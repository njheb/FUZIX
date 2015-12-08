#ifndef RASPBERRYPI_H
#define RASPBERRYPI_H

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

#endif

