#ifndef __FCEU_SDL_H
#define __FCEU_SDL_H

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"

// I'm using this as a #define so the compiler can optimize the
// modulo operation
#define PERIODIC_SAVE_INTERVAL 5000 // milliseconds

const int INVALID_STATE = 99;

extern int noGui;
extern int isloaded;

extern int dendy;
extern int pal_emulation;
extern bool swapDuty;

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);
uint64 FCEUD_GetTime();

#endif
