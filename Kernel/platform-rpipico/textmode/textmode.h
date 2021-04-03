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

void init_for_main(void);
void demo_for_main(void);
int video_main(void);

//extern char message_text[25][81];

#endif //_TEXTMODE_H
