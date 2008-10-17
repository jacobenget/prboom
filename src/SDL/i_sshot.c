/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2006 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  Screenshot functions, moved out of i_video.c
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "SDL.h"

#ifdef HAVE_LIBPNG
#include <png.h>
#endif

#include "doomstat.h"
#include "doomdef.h"
#include "doomtype.h"
#include "v_video.h"
#include "i_video.h"
#include "z_zone.h"
#include "lprintf.h"

#ifdef HAVE_LIBPNG

//
// Error functions needed by libpng
//

static void error_fn(png_structp p, png_const_charp s)
{
  lprintf(LO_ERROR, "I_ScreenShot: %s\n", s);
}

static void warning_fn(png_structp p, png_const_charp s)
{
  lprintf(LO_WARN, "I_ScreenShot: %s\n", s);
}

//
// 8bpp screenshots (indexed color mode)
//

static int write_png_palette(
    png_struct *png_ptr, png_info *info_ptr, SDL_Surface *scr)
{
  int i, result = -1;
  png_color *palette;

  palette = malloc(sizeof(*palette) * scr->format->palette->ncolors);

  if (palette)
  {
    for (i = 0; i < scr->format->palette->ncolors; i++) {
      palette[i].red   = scr->format->palette->colors[i].r;
      palette[i].green = scr->format->palette->colors[i].g;
      palette[i].blue  = scr->format->palette->colors[i].b;
    }

    png_set_PLTE(png_ptr, info_ptr,
        palette, scr->format->palette->ncolors);

    free(palette);
    result = 0;
  }
  return result;
}

static void fill_buffer_indexed(SDL_Surface *scr, void *buffer)
{
  memcpy(buffer, scr->pixels, SCREENWIDTH * SCREENHEIGHT);
}

static void fill_buffer_hicolor(SDL_Surface *scr, void *buffer)
{
  SDL_PixelFormat *fmt = scr->format;
  png_color *pixel = buffer;
  byte *source = scr->pixels;
  int y;

  // translate SDL pixel format into png_color
  for (y = SCREENWIDTH * SCREENHEIGHT;
      y > 0;
      pixel++, source += fmt->BytesPerPixel, y--)
  {
    Uint32 p = *(Uint32 *)source;
    pixel->red   = (((p & fmt->Rmask)>>fmt->Rshift)<<fmt->Rloss);
    pixel->green = (((p & fmt->Gmask)>>fmt->Gshift)<<fmt->Gloss);
    pixel->blue  = (((p & fmt->Bmask)>>fmt->Bshift)<<fmt->Bloss);
  }
}

enum {
  SCREENSHOT_SDL_INDEXED,
  SCREENSHOT_SDL_HICOLOR,
  SCREENSHOT_SDL_MODES
};

static const struct screenshot_sdl_func {
  const size_t pixel_size;
  void (*fill_buffer)(SDL_Surface *scr, void *buffer);
} screenshot_sdl_funcs[SCREENSHOT_SDL_MODES] = {
  {
    sizeof(char),
    fill_buffer_indexed
  },
  {
    sizeof(png_color),
    fill_buffer_hicolor
  },
};

static int screenshot_indexed(
    png_struct *png_ptr, png_info *info_ptr, SDL_Surface *scr)
{
  int y, result = -1;
  byte *pixel_data;

  pixel_data = malloc(SCREENWIDTH * SCREENHEIGHT);

  if (pixel_data)
  {
    int lock_needed = SDL_MUSTLOCK(scr);
    int lock_was_successful = 0;

    if (!lock_needed || SDL_LockSurface(scr) >= 0)
    {
      lock_was_successful = 1;
      memcpy(pixel_data, scr->pixels, SCREENWIDTH * SCREENHEIGHT);
      if (lock_needed)
        SDL_UnlockSurface(scr);
    }

    if (lock_was_successful)
    {
      png_write_info(png_ptr, info_ptr);
      for (y = 0; y < SCREENHEIGHT; y++)
        png_write_row(png_ptr, pixel_data + y*SCREENWIDTH);
      png_write_end(png_ptr, info_ptr);

      result = 0;
    }

    free(pixel_data);
  }
  return result;
}

//
// 15, 16, 32bpp mode screenshots
//

static int screenshot_hicolor(
    png_struct *png_ptr, png_info *info_ptr, SDL_Surface *scr)
{
  int y, result = -1;
  png_color *pixel_data;

  pixel_data = malloc(sizeof(*pixel_data) * SCREENWIDTH * SCREENHEIGHT);

  if (pixel_data)
  {
    int lock_needed = SDL_MUSTLOCK(scr);
    int lock_was_successful = 0;

    if (!lock_needed || SDL_LockSurface(scr) >= 0)
    {
      SDL_PixelFormat *f = scr->format;
      png_color *pixel = pixel_data;
      byte *source = scr->pixels;

      lock_was_successful = 1;

      for (y = SCREENWIDTH * SCREENHEIGHT;
          y > 0;
          pixel++, source += f->BytesPerPixel, y--)
      {
        Uint32 p = *(Uint32 *)source;
        pixel->red   = (((p & f->Rmask)>>f->Rshift)<<f->Rloss);
        pixel->green = (((p & f->Gmask)>>f->Gshift)<<f->Gloss);
        pixel->blue  = (((p & f->Bmask)>>f->Bshift)<<f->Bloss);
      }
      if (lock_needed)
        SDL_UnlockSurface(scr);
    }

    if (lock_was_successful)
    {
      png_write_info(png_ptr, info_ptr);
      for (y = 0; y < SCREENHEIGHT; y++)
        png_write_row(png_ptr, (png_byte *)(pixel_data + y*SCREENWIDTH));
      png_write_end(png_ptr, info_ptr);

      result = 0;
    }

    free(pixel_data);
  }
  return result;
}

//
// OpenGL mode screenshots
//
#ifdef GL_DOOM
static int screenshot_gl(png_struct *png_ptr, png_info *info_ptr)
{
  unsigned char *pixel_data = gld_ReadScreen();

  if (pixel_data)
  {
    int y;

    png_write_info(png_ptr, info_ptr);
    for (y = 0; y < SCREENHEIGHT; y++)
      png_write_row(png_ptr, pixel_data + y*SCREENWIDTH*3);
    png_write_end(png_ptr, info_ptr);

    free(pixel_data);
    return 0;
  }
  else
    return -1;
}
#endif

//
// I_ScreenShot - PNG version
//

int I_ScreenShot (const char *fname)
{
  int result = -1;
  FILE *fp = fopen(fname, "wb");

  if (fp)
  {
    png_struct *png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING, png_error_ptr_NULL, error_fn, warning_fn);

    if (png_ptr)
    {
      png_info *info_ptr = png_create_info_struct(png_ptr);

      if (info_ptr)
      {
        SDL_Surface *scr = SDL_GetVideoSurface();
        png_time ptime;

        png_set_compression_level(png_ptr, 2);
        png_init_io(png_ptr, fp);
        png_set_IHDR(
            png_ptr, info_ptr, SCREENWIDTH, SCREENHEIGHT, 8,
            (V_GetMode() == VID_MODE8)
              ? PNG_COLOR_TYPE_PALETTE
              : PNG_COLOR_TYPE_RGB,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);

        png_convert_from_time_t(&ptime, time(NULL));
        png_set_tIME(png_ptr, info_ptr, &ptime);

        switch (V_GetMode()) {
#ifdef GL_DOOM
          case VID_MODEGL:
            result = screenshot_gl(png_ptr, info_ptr);
            break;
#endif
          case VID_MODE8:
            if (write_png_palette(png_ptr, info_ptr, scr) >= 0)
              result = screenshot_indexed(png_ptr, info_ptr, scr);
            break;

          case VID_MODE15:
          case VID_MODE16:
          case VID_MODE32:
            result = screenshot_hicolor(png_ptr, info_ptr, scr);
            break;

          default:
            break;
        }
      }
      png_destroy_write_struct(&png_ptr,  png_infopp_NULL);
    }
    fclose(fp);
  }
  return result;
}

#else // !HAVE_LIBPNG

//
// I_ScreenShot - BMP version
//

int I_ScreenShot (const char *fname)
{
#ifdef GL_DOOM
  if (V_GetMode() == VID_MODEGL)
  {
    int result = -1;
    unsigned char *pixel_data = gld_ReadScreen();

    if (pixel_data)
    {
      SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
          pixel_data, SCREENWIDTH, SCREENHEIGHT, 24, SCREENWIDTH*3,
          0x000000ff, 0x0000ff00, 0x00ff0000, 0);

      if (surface)
      {
        result = SDL_SaveBMP(surface, fname);
        SDL_FreeSurface(surface);
      }
      free(pixel_data);
    }
    return result;
  } else
#endif
  return SDL_SaveBMP(SDL_GetVideoSurface(), fname);
}

#endif // HAVE_LIBPNG
