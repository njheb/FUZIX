#ifndef USBCONSOLE_H
#define USBCONSOLE_H
#include "config.h"
#ifdef USE_SERIAL_ONLY
extern void core1_init(void);
#else
extern void cdc_task(void);
#endif
extern bool usbconsole_is_readable(void);
extern bool usbconsole_is_writable(void);
extern uint8_t usbconsole_getc_blocking(void);
extern void usbconsole_putc_blocking(uint8_t b);

extern int ypos;
extern char message_text[26][81]; //exploring bug
/*
had not had message_text in the header
when sized [25][81] FUZIX locks up
had the extern declaration in the body of the code of core1 set to [26][81]
by mistake, but that appears to work
*/
#endif

