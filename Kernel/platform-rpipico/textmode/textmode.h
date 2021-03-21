/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
//njh included the above copyright comment just in case its needed
//njh this is my header to make prototype framebuffer available
//njh to FUZIX
#ifndef _TEXTMODE_H
#define _TEXTMODE_H

#include "pico/types.h"

//typedef struct {
//    lv_font_fmt_txt_dsc_t *dsc;
//    uint8_t line_height, base_line;
//} lv_font_t;

//extern const lv_font_t ubuntu_mono8;
//njh make sure to kick off after UART1 setup 
//njh also move sdcard spi pio to pio1
//in FUZIX
void init_for_main(void);
void demo_for_main(void);
int video_main(void);
extern char message_text[32][81];

#endif //_TEXTMODE_H
