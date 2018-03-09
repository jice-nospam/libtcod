/*
* libtcod 1.6.4
* Copyright (c) 2008,2009,2010,2012,2013,2016,2017 Jice & Mingos & rmtew
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * The name of Jice or Mingos may not be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY JICE, MINGOS AND RMTEW ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL JICE, MINGOS OR RMTEW BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _TCOD_BRESENHAM_H
#define _TCOD_BRESENHAM_H

#include "libtcod_portability.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef bool (*TCOD_line_listener_t) (int x, int y);

TCODLIB_API void TCOD_line_init(int xFrom, int yFrom, int xTo, int yTo);
TCODLIB_API bool TCOD_line_step(int *xCur, int *yCur); /* advance one step. returns true if we reach destination */
/* atomic callback function. Stops when the callback returns false */
TCODLIB_API bool TCOD_line(int xFrom, int yFrom, int xTo, int yTo, TCOD_line_listener_t listener);

/* thread-safe versions */
typedef struct {
	int stepx;
	int stepy;
	int e;
	int deltax;
	int deltay;
	int origx;
	int origy;
	int destx;
	int desty;
} TCOD_bresenham_data_t;

TCODLIB_API void TCOD_line_init_mt(int xFrom, int yFrom, int xTo, int yTo, TCOD_bresenham_data_t *data);
TCODLIB_API bool TCOD_line_step_mt(int *xCur, int *yCur, TCOD_bresenham_data_t *data);
TCODLIB_API bool TCOD_line_mt(int xFrom, int yFrom, int xTo, int yTo, TCOD_line_listener_t listener, TCOD_bresenham_data_t *data);
#ifdef __cplusplus
}
#endif
#endif
