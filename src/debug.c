#include "debug.h"

#ifdef DEBUG


#include <exec/types.h>
#include <exec/execbase.h>
#include <libraries/dos.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <devices/serial.h>

#include <stdio.h>
#include <stdarg.h>

static struct IOExtSer *serialIO = NULL;
static struct MsgPort *serialMP = NULL;
char buffer[1024];

void dbg_print(const char *fmt, ...) {

    va_list args;   
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    serialIO->IOSer.io_Command = CMD_WRITE;
    serialIO->IOSer.io_Length = len;
    serialIO->IOSer.io_Data = (APTR)buffer;
    DoIO((struct IORequest *)serialIO);
}

void dbg_init() {
    // Create a message port for serial device
    if (!(serialMP = CreatePort(0, 0))) {
        printf("Failed to create message port\n");
    }

    // Create an extended IORequest for serial device
    if (!(serialIO = (struct IOExtSer *)CreateExtIO(serialMP, sizeof(struct IOExtSer)))) {
        printf("Failed to create IORequest\n");
        dbg_shutdown();
    }

    // Open the serial device
    BYTE error = OpenDevice("serial.device", 0, (struct IORequest *)serialIO, 0);
    if (error) {
        printf("Failed to open serial device: %ld\n", error);
        dbg_shutdown();
    }

    // Set up the serial parameters (19200 baud, 8N1)
    serialIO->IOSer.io_Command = SDCMD_SETPARAMS;
    serialIO->io_Baud = 19200;
    serialIO->io_ReadLen = 8;
    serialIO->io_WriteLen = 8;
    serialIO->io_StopBits = 1;
    serialIO->io_SerFlags = 0;
    DoIO((struct IORequest *)serialIO);

}

void dbg_shutdown() {
    if (serialIO) {
        CloseDevice((struct IORequest *)serialIO);
        DeleteExtIO((struct IORequest *)serialIO);
        serialIO = NULL;
    }
    if (serialMP) {
        DeletePort(serialMP);
        serialMP = NULL;
    }
}


#endif

