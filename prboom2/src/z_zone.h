/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: z_zone.h,v 1.6 2000/12/30 19:51:28 cph Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
 *      Remark: this was the only stuff that, according
 *       to John Carmack, might have been useful for
 *       Quake.
 *
 * Rewritten by Lee Killough, though, since it was not efficient enough.
 *
 *---------------------------------------------------------------------*/

#ifndef __Z_ZONE__
#define __Z_ZONE__

#ifndef __GNUC__
#define __attribute__(x)
#endif

// Remove all definitions before including system definitions

#undef malloc
#undef free
#undef realloc
#undef calloc
#undef strdup

// Include system definitions so that prototypes become
// active before macro replacements below are in effect.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ZONE MEMORY
// PU - purge tags.

enum {PU_FREE, PU_STATIC, PU_SOUND, PU_MUSIC, PU_LEVEL, PU_LEVSPEC, PU_CACHE,
      /* Must always be last -- killough */ PU_MAX};

#define PU_PURGELEVEL PU_CACHE        /* First purgable tag's level */

#ifdef INSTRUMENTED
#define DA(x,y) ,x,y
#define DAC(x,y) x,y
#else
#define DA(x,y) 
#define DAC(x,y)
#endif

void *(Z_Malloc)(size_t size, int tag, void **ptr DA(const char *, int));
void (Z_Free)(void *ptr DA(const char *, int));
void (Z_FreeTags)(int lowtag, int hightag DA(const char *, int));
void (Z_ChangeTag)(void *ptr, int tag DA(const char *, int));
void (Z_Init)(void);
void Z_Close(void);
void *(Z_Calloc)(size_t n, size_t n2, int tag, void **user DA(const char *, int));
void *(Z_Realloc)(void *p, size_t n, int tag, void **user DA(const char *, int));
char *(Z_Strdup)(const char *s, int tag, void **user DA(const char *, int));
void (Z_CheckHeap)(DAC(const char *,int));   // killough 3/22/98: add file/line info
void Z_DumpHistory(char *);

#ifdef INSTRUMENTED
/* cph - save space if not debugging, don't require file 
 * and line to memory calls */
#define Z_Free(a)          (Z_Free)     (a,      __FILE__,__LINE__)
#define Z_FreeTags(a,b)    (Z_FreeTags) (a,b,    __FILE__,__LINE__)
#define Z_ChangeTag(a,b)   (Z_ChangeTag)(a,b,    __FILE__,__LINE__)
#define Z_Malloc(a,b,c)    (Z_Malloc)   (a,b,c,  __FILE__,__LINE__)
#define Z_Strdup(a,b,c)    (Z_Strdup)   (a,b,c,  __FILE__,__LINE__)
#define Z_Calloc(a,b,c,d)  (Z_Calloc)   (a,b,c,d,__FILE__,__LINE__)
#define Z_Realloc(a,b,c,d) (Z_Realloc)  (a,b,c,d,__FILE__,__LINE__)
#define Z_CheckHeap()      (Z_CheckHeap)(__FILE__,__LINE__)
#endif

#define malloc(n)          Z_Malloc(n,PU_STATIC,0)
#define free(p)            Z_Free(p)
#define realloc(p,n)       Z_Realloc(p,n,PU_STATIC,0)
#define calloc(n1,n2)      Z_Calloc(n1,n2,PU_STATIC,0)
#undef strdup
#ifdef _CRTIMP
_CRTIMP char *strdup(const char *s);
#else
char *strdup(const char *s);
#endif
#define strdup(s)          Z_Strdup(s,PU_STATIC,0)

// Doom-style printf
void doom_printf(const char *, ...) __attribute__((format(printf,1,2)));

void Z_ZoneHistory(char *);

#endif
