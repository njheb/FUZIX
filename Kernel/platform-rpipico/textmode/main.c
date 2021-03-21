#include <stdio.h>
#include <stdlib.h>
#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "font.h"

#include "textmode.h"


int main(void) {

    init_for_main();
//    return video_main();
    (void)video_main(); //kick of display generation
    demo_for_main();
}
