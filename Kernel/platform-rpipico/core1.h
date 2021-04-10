#ifndef USBCONSOLE_H
#define USBCONSOLE_H
#include "config.h"
extern void cdc_task(void);
extern void vga_drain(void);

extern int ypos;
//extern char message_text[25][81];
extern char message_text[26][81];
#endif

