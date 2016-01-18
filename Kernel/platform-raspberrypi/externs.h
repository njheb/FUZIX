#ifndef EXTERNS_H
#define EXTERNS_H

extern void tty_rawinit(void);
extern void sd_rawinit(void);
extern void fuzix_main(void);

extern void busy_wait(int delay);

extern void* get_udata_for_page(int page);
extern void* get_svc_stack_for_page(int page);

extern void tty_interrupt(void);

#endif

