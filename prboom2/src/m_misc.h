/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: m_misc.h,v 1.2 2000/05/09 21:45:38 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Colin Phipps (cph@lxdoom.linuxgames.com), 
 *  Jess Haas (JessH@lbjhs.net)
 *  and Florian Schulze (florian.proff.schulze@gmx.net)
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
 *  External non-system-specific stuff, like storing config settings, 
 *  simple file handling, and saving screnshots.
 *    
 *-----------------------------------------------------------------------------*/


#ifndef __M_MISC__
#define __M_MISC__


#include "doomtype.h"
//
// MISC
//

boolean M_WriteFile (char const* name,void* source,int length);

int M_ReadFile (char const* name,byte** buffer);

void M_ScreenShot (void);
void M_DoScreenShot (const char*); // cph

void M_LoadDefaults (void);

void M_SaveDefaults (void);


int M_DrawText (int x,int y,boolean direct,char* string);

// phares 4/21/98:
// Moved from m_misc.c so m_menu.c could see it.

// CPhipps - struct to hold a value in a config file
// Cannot be a union, as it must be initialised
typedef struct
{
  const char* name;
  struct {
    int* pi;
    const char** ppsz;
  } location; // CPhipps - pointer to the value
  struct {
    int i;
    const char* psz;
  } defaultvalue; // CPhipps - default value
  // Limits (for an int)
  int   minvalue;         // jff 3/3/98 minimum allowed value
  int   maxvalue;         // jff 3/3/98 maximum allowed value
  enum {
    def_none, // Dummy entry
    def_str,  // A string 
    def_int,  // Integer
    def_hex,  // Integer (write in hex)
    def_bool = def_int,  // Boolean
    def_key = def_hex,   // Key code (byte)
    def_mouseb = def_int,// Mouse button
    def_colour = def_hex // Colour (256 colour palette entry)
  } type; // CPhipps - type of entry
  int   setupscreen;      // phares 4/19/98: setup screen where this appears
  // cph - removed the help strings from the config file
  // const char* help;       // jff 3/3/98 description of parameter
  // CPhipps - remove unused "lousy hack" code
} default_t;

#define IS_STRING(dv) ((dv).type == def_str)
// CPhipps - What is the max. key code that X will send us?
#define MAX_KEY 65536
#define MAX_MOUSEB 2

#endif
