#include "main_thread.h"
#include "doomgeneric/doomgeneric.h"
#include "doom_port_status.h"

void main_thread_entry(void)
{
    static char *argv[] =
    {
        "doomrenesas",
        "-iwad", "doom1.wad",
        "-skill", "1",
        "-warp", "1", "1",
        "-nomonsters",
        "-nosound",
        "-nomusic",
        "-nogui"
    };

    doom_clear_error_msg();
    doom_set_error_literal("stage: main entry");
    doom_set_error_literal("stage: doomgeneric_Create");
    doomgeneric_Create((int) (sizeof(argv) / sizeof(argv[0])), argv);
    doom_set_error_literal("stage: main loop");

    while (1)
    {
        doomgeneric_Tick();
    }
}
