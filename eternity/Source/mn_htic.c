// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2002 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Heretic-specific menu code
//
// By James Haley
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "d_deh.h"
#include "doomdef.h"
#include "doomstat.h"
#include "c_io.h"
#include "d_gi.h"
#include "c_runcmd.h"
#include "mn_engin.h"
#include "mn_misc.h"
#include "v_video.h"
#include "w_wad.h"

extern int start_episode;

#define FONTB_START '!'
#define FONTB_END   'Z'
#define FONTB_SIZE  (FONTB_END - FONTB_START + 1)

#define NUM_HSKULL 18

int HSkullLumpNums[NUM_HSKULL];

// Heretic FONTB support

patch_t *h_fontb[FONTB_SIZE];

void MN_HBLoadFont(void)
{
   int i, j;
   char tempstr[10];

   // init to NULL first
   for(i = 0; i < FONTB_SIZE; i++)
      h_fontb[i] = NULL;
   
   for(i = 0, j = FONTB_START; i < FONTB_SIZE; i++, j++)
   {      
      sprintf(tempstr, "FONTB%.2d", j - 32);
      h_fontb[i] = W_CacheLumpName(tempstr, PU_STATIC);
   }
}

void MN_HBWriteText(unsigned char *s, int x, int y)
{
   int   w, h;
   unsigned char *ch;
   unsigned int c;
   int   cx;
   int   cy;
   patch_t *patch;
   
   ch = s;
   cx = x;
   cy = y;
   
   while(1)
   {
      c = *ch++;
      if(!c)
         break;
      
      // haleyjd: ignore any color codes
      if(c >= 128)
         continue;
            
      if(c == '\t')
      {
         cx = (cx/40)+1;
         cx = cx*40;
         continue;
      }
      
      if(c == '\n')
      {
         cx = x;
         cy += 8;
         continue;
      }
      
      c = toupper(c) - FONTB_START;
      if(c < 0 || c >= FONTB_SIZE)
      {
         cx += 8; // haleyjd: blank step is 8 for fontb
         continue;
      }
      
      patch = h_fontb[c];
      
      if(!patch)
      {
         cx += 8; // haleyjd: blank step is 8 for fontb
         continue;
      }
      
      w = SHORT(patch->width);
      if(cx < 0 || cx+w > SCREENWIDTH)
         break;
      
      h = SHORT(patch->height);
      if(cy < 0 || cy+h > SCREENHEIGHT)
         break;
      
      V_DrawPatch(cx, cy, &vbscreen, patch);
      
      cx += (w-1);
   }
}

void MN_HBWriteNumText(unsigned char *s, int x, int y)
{
   int   w, h;
   unsigned char *ch;
   unsigned int c;
   int   cx;
   int   cy;
   patch_t *patch;
   
   ch = s;
   cx = x;
   cy = y;
   
   while(1)
   {
      c = *ch++;
      if(!c)
         break;
      
      // haleyjd: ignore anything other than space or a number
      if(c != ' ' && (c < '0' || c > '9'))
         continue;
                  
      c = toupper(c) - FONTB_START;
      if(c < 0 || c >= FONTB_SIZE)
      {
         cx += 12; // haleyjd: blank step is 12 for numbers
         continue;
      }
      
      patch = h_fontb[c];
      
      if(!patch)
      {
         cx += 12; // haleyjd: blank step is 12 for numbers
         continue;
      }
      
      w = SHORT(patch->width);
      if(cx < 0 || cx+w > SCREENWIDTH)
         break;
      
      h = SHORT(patch->height);
      if(cy < 0 || cy+h > SCREENHEIGHT)
         break;
      
      V_DrawPatch(cx + 6 - w/2, cy, &vbscreen, patch);
      
      cx += 12;
   }
}

int MN_HBStringHeight(unsigned char *s)
{
   int height = 20;  // always at least 20
   
   // add an extra 20 for each newline found
   
   while(*s)
   {
      if(*s == '\n') height += 20;
      s++;
   }
   
   return height;
}

int MN_HBStringWidth(unsigned char *s)
{
   int length = 0; // current line width
   int longest_width = 0; // line with longest width so far
   unsigned char c;
   
   for(; *s; s++)
   {
      c = *s;
      // haleyjd: totally ignore any color codes
      if(c >= 128)
         continue;

      if(c == '\n')        // newline
      {
         if(length > longest_width) longest_width = length;
         length = 0; // next line;
         continue;
      }
      c = toupper(c) - FONTB_START;
      length += 
         (c >= FONTB_SIZE || !h_fontb[c]) ? 8 : SHORT(h_fontb[c]->width) - 1;
   }
   
   if(length > longest_width)
      longest_width = length; // check last line
   
   return longest_width;
}

//
// Heretic-Only Menus
//
// Only the menus that need specific restructuring for Heretic
// are here; everything that could be shared has been.
//

// Main Heretic Menu

void MN_HMainMenuDrawer(void);

menu_t menu_hmain =
{
  {
    // 'heretic' title and skulls drawn by the drawer
    
    {it_hruncmd, "new game",             "mn_hnewgame"},
    {it_hruncmd, "options",              "mn_hoptfeat"},
    {it_hruncmd, "load game",            "mn_loadgame"},
    {it_hruncmd, "save game",            "mn_savegame"},
    {it_hruncmd, "end game",             "mn_endgame"},
    {it_hruncmd, "quit game",            "mn_quit"},
    {it_end},
  },
  100, 56,                // x, y offsets
  0,                     // start with 'new game' selected
  mf_skullmenu,          // a skull menu
  MN_HMainMenuDrawer
};

menu_t menu_hoptfeat =
{
   {
      {it_title,  FC_GOLD "options",  NULL },
      {it_gap},
      {it_gap},
      {it_hruncmd, "setup",            "mn_options"   },
      {it_gap},
      {it_hruncmd, "features",         "mn_hfeatures" },
      {it_end},
   },
   100, 15,
   3,
   mf_leftaligned | mf_skullmenu
};

CONSOLE_COMMAND(mn_hoptfeat, 0)
{
   MN_StartMenu(&menu_hoptfeat);
}

void MN_HInitSkull(void)
{
   int i;
   char tempstr[10];

   for(i = 0; i < NUM_HSKULL; i++)
   {
      sprintf(tempstr, "M_SKL%.2d", i);
      HSkullLumpNums[i] = W_GetNumForName(tempstr);         
   }
}

void MN_HMainMenuDrawer(void)
{
   int skullIndex;

   // draw M_HTIC
   V_DrawPatch(88, 0, &vbscreen, W_CacheLumpName("M_HTIC", PU_CACHE));

   // draw spinning skulls
   skullIndex = (menutime / 3) % NUM_HSKULL;

   V_DrawPatch(40, 10, &vbscreen,
      W_CacheLumpNum(HSkullLumpNums[17-skullIndex], PU_CACHE));

   V_DrawPatch(232, 10, &vbscreen,
      W_CacheLumpNum(HSkullLumpNums[skullIndex], PU_CACHE));
}

extern menu_t menu_hepisode;

CONSOLE_COMMAND(mn_hnewgame, 0)
{
   if(netgame && !demoplayback)
   {
      MN_Alert(s_NEWGAME);
      return;
   }

   // chop off SoSR episodes if not present
   if(gamemission != hticsosr)
      menu_hepisode.menuitems[5].type = it_end;
   
   MN_StartMenu(&menu_hepisode);
}


menu_t menu_hepisode =
{
  {
    {it_hinfo,   "which episode?"},
    {it_gap},
    {it_hruncmd, "city of the damned",   "mn_hepis 1"},
    {it_hruncmd, "hell's maw",           "mn_hepis 2"},
    {it_hruncmd, "the dome of d'sparil", "mn_hepis 3"},
    {it_hruncmd, "the ossuary",          "mn_hepis 4"},
    {it_hruncmd, "the stagnant demesne", "mn_hepis 5"},
    {it_end},
  },
  38, 10,               // x,y offsets
  2,                    // starting item: city of the damned
  mf_skullmenu,         // is a skull menu
};

extern menu_t menu_hnewgame;

CONSOLE_COMMAND(mn_hepis, cf_notnet)
{
   if(!c_argc)
   {
      C_Printf("usage: mn_hepis <epinum>\n");
      return;
   }
   
   start_episode = atoi(c_argv[0]);
   
   if((gameModeInfo->flags & GIF_SHAREWARE) && start_episode > 1)
   {
      MN_Alert("only available in the registered version");
      return;
   }
   
   MN_StartMenu(&menu_hnewgame);
}

menu_t menu_hnewgame =
{
  {
    {it_hinfo,   "choose skill level"},
    {it_gap},
    {it_hruncmd, "thou needeth a wet nurse",    "newgame 0"},
    {it_hruncmd, "yellowbellies-r-us",          "newgame 1"},
    {it_hruncmd, "bringest them oneth",         "newgame 2"},
    {it_hruncmd, "thou art a smite-meister",    "newgame 3"},
    {it_hruncmd, "black plague possesses thee", "newgame 4"},
    {it_end},
  },
  38, 10,               // x,y offsets
  4,                    // starting item: hurt me plenty
  mf_skullmenu,         // is a skull menu
};

// Heretic features -- I removed the demo item because internal
// Heretic demos are not supported, and running them could
// potentially crash the game. If any new Eternity demos are 
// recorded, they should be playable via the command line.

menu_t menu_hfeatures =
{
  {
    {it_title, "features"},
    {it_gap},
    {it_gap},
    {it_hruncmd, "player setup",    "mn_player" },
    {it_gap},
    {it_hruncmd, "game settings",   "mn_gset" },
    {it_gap},
    {it_hruncmd, "multiplayer",     "mn_multi" },
    {it_gap},
    {it_hruncmd, "load wad",        "mn_loadwad" },
    {it_gap},
    {it_hruncmd, "about",           "credits" },
    {it_end},
  },
  100, 15,                              // x,y
  3,                                    // start item
  mf_leftaligned | mf_skullmenu         // skull menu
};

CONSOLE_COMMAND(mn_hfeatures, 0)
{
   MN_StartMenu(&menu_hfeatures);
}

void MN_AddHMenus(void)
{
   C_AddCommand(mn_hnewgame);
   C_AddCommand(mn_hoptfeat);
   C_AddCommand(mn_hepis);
   C_AddCommand(mn_hfeatures);
}

// EOF