/* BSD 3-Clause License
 *
 * Copyright © 2008-2021, Jice and the libtcod contributors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "renderer_xterm.h"

#include <SDL.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "error.h"

#ifdef _WIN32
static DWORD g_old_mode_stdin = 0;
static DWORD g_old_mode_stdout = 0;
#endif

struct TCOD_RendererXterm {
  TCOD_Console* cache;
};

static char* ucs4_to_utf8(int ucs4, char out[5]) {
  static const unsigned char B10000000 = 128;
  static const unsigned char B11000000 = 192;
  static const unsigned char B11100000 = 224;
  static const unsigned char B11110000 = 240;
  static const int B000_000000_000000_111111 = 63;
  static const int B000_000000_011111_000000 = 1984;
  static const int B000_000000_111111_000000 = 4032;
  static const int B000_001111_000000_000000 = 61440;
  static const int B000_111111_000000_000000 = 258048;
  static const int B111_000000_000000_000000 = 1835008;
  if (ucs4 < 0) {
  } else if (ucs4 <= 0x7F) {
    out[0] = (char)ucs4;
    out[1] = '\0';
    return out;
  } else if (ucs4 <= 0x07FF) {
    out[0] = B11000000 | (unsigned char)((ucs4 & B000_000000_011111_000000) >> 6);
    out[1] = B10000000 | (unsigned char)((ucs4 & B000_000000_000000_111111) >> 0);
    out[2] = '\0';
    return out;
  } else if (ucs4 <= 0xFFFF) {
    out[0] = B11100000 | (unsigned char)((ucs4 & B000_001111_000000_000000) >> 12);
    out[1] = B10000000 | (unsigned char)((ucs4 & B000_000000_111111_000000) >> 6);
    out[2] = B10000000 | (unsigned char)((ucs4 & B000_000000_000000_111111) >> 0);
    out[3] = '\0';
    return out;
  } else if (ucs4 <= 0x10FFFF) {
    out[0] = B11110000 | (unsigned char)((ucs4 & B111_000000_000000_000000) >> 18);
    out[1] = B10000000 | (unsigned char)((ucs4 & B000_111111_000000_000000) >> 12);
    out[2] = B10000000 | (unsigned char)((ucs4 & B000_000000_111111_000000) >> 6);
    out[3] = B10000000 | (unsigned char)((ucs4 & B000_000000_000000_111111) >> 0);
    out[4] = '\0';
    return out;
  }
  out[0] = '?';
  out[1] = '\0';
  return out;
}

static TCOD_Error xterm_present(
    struct TCOD_Context* __restrict self,
    const struct TCOD_Console* __restrict console,
    const struct TCOD_ViewportOptions* __restrict viewport) {
  (void)viewport;
  struct TCOD_RendererXterm* context = self->contextdata_;
  if (context->cache && (context->cache->w != console->w || context->cache->h != console->h)) {
    TCOD_console_delete(context->cache);
    context->cache = NULL;
  }
  if (!context->cache) {
    context->cache = TCOD_console_new(console->w, console->h);
    for (int i = 0; i < context->cache->elements; ++i) context->cache->tiles[i].ch = -1;
  }
  fprintf(stdout, "\x1b[?25l");  // Cursor un-hiding on Windows after window is resized.
  for (int y = 0; y < console->h; ++y) {
    fprintf(stdout, "\x1b[%d;0H", y);  // Move cursor to start of next line.
    int skip_tiles = 0;  // Skip unchanged tiles.
    for (int x = 0; x < console->w; ++x) {
      TCOD_ConsoleTile* prev_tile = &context->cache->tiles[console->w * y + x];
      const TCOD_ConsoleTile* tile = &console->tiles[console->w * y + x];
      if (tile->ch == prev_tile->ch && tile->fg.r == prev_tile->fg.r && tile->fg.g == prev_tile->fg.g &&
          tile->fg.b == prev_tile->fg.b && tile->bg.r == prev_tile->bg.r && tile->bg.g == prev_tile->bg.g &&
          tile->bg.b == prev_tile->bg.b) {
        ++skip_tiles;
        continue;
      }
      if (skip_tiles) {
        fprintf(stdout, "\x1b[%dC", skip_tiles);  // Move cursor forward.
        skip_tiles = 0;
      }
      char utf8[5];
      fprintf(  // Print a character with colors.
          stdout,
          "\x1b[38;2;%u;%u;%u;48;2;%u;%u;%um%s",
          tile->fg.r,
          tile->fg.g,
          tile->fg.b,
          tile->bg.r,
          tile->bg.g,
          tile->bg.b,
          ucs4_to_utf8(tile->ch & 0x10FFFF, utf8));
      *prev_tile = *tile;
    }
  }
  return TCOD_E_OK;
}

static void xterm_cleanup(void) {
  fprintf(
      stdout,
      "\x1b[2J"  // Clear the screen.
      "\x1b[?1049l"  // Disable alternative screen buffer.
      "\x1b[?25h"  // Show cursor.
  );
#ifdef _WIN32
  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), g_old_mode_stdin);
  SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), g_old_mode_stdout);
#endif
}

static void xterm_destructor(struct TCOD_Context* __restrict self) {
  struct TCOD_RendererXterm* context = self->contextdata_;
  if (context) free(context);
  xterm_cleanup();
}

TCOD_Context* TCOD_renderer_init_xterm(const char* window_title) {
  TCOD_Context* context = TCOD_context_new_();
  if (!context) return NULL;
  struct TCOD_RendererXterm* data = context->contextdata_ = calloc(sizeof(*data), 1);
  if (!data) {
    TCOD_context_delete(context);
    TCOD_set_errorv("Could not allocate memory.");
    return NULL;
  }
  context->c_present_ = &xterm_present;
  context->c_destructor_ = &xterm_destructor;
  atexit(&xterm_cleanup);
  setlocale(LC_ALL, ".UTF-8");  // Enable UTF-8.
#ifdef _WIN32
  HANDLE handle_stdin = GetStdHandle(STD_INPUT_HANDLE);
  HANDLE handle_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleMode(handle_stdin, &g_old_mode_stdin);
  GetConsoleMode(handle_stdout, &g_old_mode_stdout);
  SetConsoleMode(handle_stdin, ENABLE_VIRTUAL_TERMINAL_INPUT);
  SetConsoleMode(
      handle_stdout, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
#endif
  fprintf(
      stdout,
      "\x1b[?1049h"  // Enable alternative screen buffer.
      "\x1b[2J"  // Clear the screen.
      "\x1b[?25l"  // Hide cursor.
  );
  if (window_title) fprintf(stdout, "\x1b]0;%s\x07", window_title);
  return context;
}
