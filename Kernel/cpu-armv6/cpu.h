#include <stddef.h>
#include <stdint.h>

typedef uint16_t irqflags_t;

typedef int32_t arg_t;
typedef uint32_t uarg_t;		/* Holds arguments */
typedef uint32_t usize_t;		/* Largest value passed by userspace */
typedef int32_t susize_t;
typedef uint32_t uaddr_t;
typedef uint32_t uptr_t;		/* User pointer equivalent */

#define uputp  uputl			/* Copy user pointer type */
#define ugetp  ugetl			/* between user and kernel */
extern uint32_t ugetl(void *uaddr, int *err);
extern int uputl(uint32_t val, void *uaddr);

extern void ei(void);
extern irqflags_t di(void);
extern void irqrestore(irqflags_t f);

extern void *memcpy(void *, const void  *, size_t);
extern void *memset(void *, int, size_t);
extern int memcmp(const void *, const void *, size_t);
extern size_t strlen(const char *);

#define brk_limit() ((udata.u_syscall_sp) - 512)

#define staticfast

/* User's structure for times() system call */
typedef unsigned long clock_t;

typedef struct {
  uint32_t low;
  uint32_t high;
} time_t;

typedef union {            /* this structure is endian dependent */
    clock_t  full;         /* 32-bit count of ticks since boot */
    struct {
      uint16_t high;
      uint16_t low;         /* 16-bit count of ticks since boot */
    } h;
} ticks_t;

/* We don't need any banking bits really */
#define CODE1
#define CODE2
#define COMMON
#define VIDEO
#define DISCARD

/* Pointers are 32bit */
#define POINTER32

/* ARM requires aligned accesses (but it least it traps if you get it wrong).
 */

#define ALIGNUP(v)   alignup(v, 4)
#define ALIGNDOWN(v) aligndown(v, 4)

/* Sane behaviour for unused parameters */
#define used(x)

#define cpu_to_le16(x)	(x)
#define le16_to_cpu(x)	(x)
#define cpu_to_le32(x)  (x)
#define le32_to_cpu(x)  (x)

#define __fastcall__

#define mcr(proc, op1, crn, crm, op2, value) \
	asm volatile("mcr p" #proc "," #op1 ", %0, c" #crn ",c" #crm "," #op2 \
	: : "r" (value))
	
#define mrc(proc, op1, crn, crm, op2) \
	({ uint32_t result; \
	   asm volatile("mrc p" #proc "," #op1 ", %0, c" #crn ",c" #crm "," #op2 \
	   : "=r" (result)); \
	   result; \
	})

