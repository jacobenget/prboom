// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
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
// Heads up display
//
// Re-written. Displays the messages, etc
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include <stdio.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_net.h"
#include "d_deh.h"
#include "d_event.h"
#include "g_game.h"
#include "hu_frags.h"
#include "hu_stuff.h"
#include "hu_over.h"
#include "p_info.h"
#include "p_map.h"
#include "p_setup.h"
#include "p_spec.h"
#include "r_draw.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"
#include "am_map.h"
#include "d_gi.h"
#include "m_qstr.h"
#include "a_small.h"

void HU_WarningsInit(void);
void HU_WarningsDrawer(void);

void HU_WidgetsInit(void);
void HU_WidgetsTick(void);
void HU_WidgetsDraw(void);
void HU_WidgetsErase(void);

void HU_MessageTick(void);
void HU_MessageDraw(void);
void HU_MessageClear(void);
void HU_MessageErase(void);

void HU_CenterMessageClear(void);
boolean HU_ChatRespond(event_t *ev);

// the global widget list

char *chat_macros[10];
const char* shiftxform;
const char english_shiftxform[];
//boolean chat_on;
boolean chat_active = false;
int obituaries = 0;
int obcolour = CR_BRICK;       // the colour of death messages
int showMessages;    // Show messages has default, 0 = off, 1 = on
int mess_colour = CR_RED;      // the colour of normal messages

///////////////////////////////////////////////////////////////////////
//
// Main Functions
//
// Init, Drawer, Ticker etc.

void HU_Start(void)
{
   HU_MessageClear();
   HU_CenterMessageClear();
}

void HU_Init(void)
{
   shiftxform = english_shiftxform;
   
   // init different modules
   HU_CrossHairInit();
   HU_FragsInit();
   HU_WarningsInit();
   HU_WidgetsInit();
   HU_LoadFont();
}

void HU_Drawer(void)
{
   // draw different modules
   HU_MessageDraw();
   HU_CrossHairDraw();
   HU_FragsDrawer();
   HU_WarningsDrawer();
   HU_WidgetsDraw();
   HU_OverlayDraw();
}

void HU_Ticker(void)
{
   // run tickers for some modules
   HU_CrossHairTick();
   HU_WidgetsTick();
   HU_MessageTick();
}

boolean altdown = false;

boolean HU_Responder(event_t *ev)
{
   if(ev->data1 == KEYD_LALT)
      altdown = ev->type == ev_keydown;
   
   return HU_ChatRespond(ev);
}

//
// HU_NewLevel
//
// Called when loading a new map.
//
void HU_NewLevel(void)
{   

#if 0
   // if commerical mode, OLO loaded and inside the confines of the
   // new level names added, use the olo level name

   if(gamemode == commercial && olo_loaded
      && (gamemap-1 >= olo.levelwarp && gamemap-1 <= olo.lastlevel))
   {
      LevelInfo.levelName = olo.levelname[gamemap-1];
   }  

#endif

   // haleyjd 07/17/04: level name now determined by MapInfo code
   // print the new level name into the console
   
   C_Printf("\n");
   C_Separator();
   C_Printf("%c  %s\n\n", 128+CR_GRAY, LevelInfo.levelName);
   C_InstaPopup();       // put console away
}

        // erase text that can be trashed by small screens
void HU_Erase(void)
{
   if(!viewwindowx || automapactive)
      return;
   
   // run indiv. module erasers
   HU_MessageErase();
   HU_WidgetsErase();
   HU_FragsErase();
}

////////////////////////////////////////////////////////////////////////
//
// Normal Messages
//
// 'picked up a clip' etc.
// seperate from the widgets (below)
//

char hu_messages[MAXHUDMESSAGES][256];
int hud_msg_lines;   // number of message lines in window up to 16
int current_messages;   // the current number of messages
int hud_msg_scrollup;// whether message list scrolls up
int message_timer;   // timer used for normal messages
int scrolltime;         // leveltime when the message list next needs
                        // to scroll up

void HU_PlayerMsg(char *s)
{
   if(current_messages == hud_msg_lines)  // display full
   {
      int i;
      
      // scroll up      
      for(i = 0; i < hud_msg_lines - 1; i++)
         strcpy(hu_messages[i], hu_messages[i+1]);
      
      strcpy(hu_messages[hud_msg_lines-1], s);
   }
   else            // add one to the end
   {
      strcpy(hu_messages[current_messages], s);
      current_messages++;
   }
   
   scrolltime = leveltime + (message_timer * 35) / 1000;
}

// erase the text before drawing

void HU_MessageErase(void)
{
   R_VideoErase(0, 0, SCREENWIDTH, 8 * hud_msg_lines);
}

void HU_MessageDraw(void)
{
   int i;
   int y;
   
   if(!showMessages)
      return;
   
   // go down a bit if chat active
   y = chat_active ? 8 : 0;
   
   for(i = 0; i < current_messages; i++, y += 8)
   {
      int x = 0;

      // haleyjd 12/26/02: center messages in Heretic
      // FIXME/TODO: make this an option in DOOM?
      if(gameModeInfo->type == Game_Heretic)
         x = (SCREENWIDTH - V_StringWidth(hu_messages[i])) >> 1;
      
      V_WriteText(hu_messages[i], x, y);
   }
}

void HU_MessageClear(void)
{
   current_messages = 0;
}

void HU_MessageTick(void)
{
   int i;
   
   if(!hud_msg_scrollup)
      return;   // messages not to scroll
   
   if(leveltime >= scrolltime)
   {
      for(i=0; i<current_messages-1; i++)
         strcpy(hu_messages[i], hu_messages[i+1]);
      current_messages = current_messages ? current_messages-1 : 0;
      scrolltime = leveltime + (message_timer * 35) / 1000;
   }
}

/////////////////////////////////////////////////////////////////////////
//
// Crosshair
//

patch_t *crosshairs[CROSSHAIRS];
patch_t *crosshair=NULL;
char *crosshairpal;
char *targetcolour, *notargetcolour, *friendcolour;
int crosshairnum;       // 0 = none
char *cross_str[]= { "none", "cross", "angle" }; // for console

void HU_CrossHairDraw(void)
{
   int drawx, drawy, top_y;
   
   if(!crosshair) return;
   if(viewcamera || automapactive) return;
  
   // where to draw??
   
   drawx = SCREENWIDTH/2 - crosshair->width/2;
   top_y = SCREENHEIGHT - gameModeInfo->StatusBar->height;
   drawy = scaledviewheight == SCREENHEIGHT ? SCREENHEIGHT/2 : (top_y)/2;
  
   // check for bfglook: make crosshair face forward
      
   if(bfglook == 2 && players[displayplayer].readyweapon == wp_bfg)
      drawy += 
         (players[displayplayer].updownangle * scaledviewheight)/100;

   drawy -= crosshair->height/2;

   if((drawy + crosshair->height) > 
      (scaledwindowy + scaledviewheight)) // ANYRES
      return;
  
   if(crosshairpal == notargetcolour)
      V_DrawPatchTL(drawx, drawy, &vbscreen, crosshair, crosshairpal, FTRANLEVEL);
   else
      V_DrawPatchTranslated(drawx, drawy, &vbscreen, crosshair, crosshairpal, false);
}

void HU_CrossHairInit(void)
{
   crosshairs[0] = W_CacheLumpName("CROSS1", PU_STATIC);
   crosshairs[1] = W_CacheLumpName("CROSS2", PU_STATIC);
   
   notargetcolour = cr_red;
   targetcolour = cr_green;
   friendcolour = cr_blue;
   crosshairpal = notargetcolour;
   crosshair = crosshairnum ? crosshairs[crosshairnum-1] : NULL;
}

void HU_CrossHairTick(void)
{
   // fast as possible: don't bother with this crap if
   // the crosshair isn't going to be displayed anyway
   
   if(!crosshairnum)
      return;

   // default to no target
   crosshairpal = notargetcolour;

   // search for targets
   
   P_AimLineAttack(players[displayplayer].mo,
                   players[displayplayer].mo->angle, 
                   16*64*FRACUNIT, 0);

   if(linetarget)
   {
      // target found
      
      crosshairpal = targetcolour;
      if(linetarget->flags & MF_FRIEND)
         crosshairpal = friendcolour;
   }        
}

///////////////////////////////////////////////////////////////////////
//
// Pop-up Warning Boxes
//
// several different things that appear, quake-style, to warn you of
// problems

//
// Open Socket Warning
//
// Problem with network leads or something like that

//
// VPO Warning indicator
//
// most ports nowadays have removed the visplane overflow problem.
// however, many developers still make wads for plain vanilla doom.
// this should give them a warning for when they have 'a few
// planes too many'

static patch_t *vpo;
static patch_t *socket_pic;

void HU_WarningsInit(void)
{
   vpo = W_CacheLumpName("VPO", PU_STATIC);
   socket_pic = W_CacheLumpName("OPENSOCK", PU_STATIC);
}

extern int num_visplanes;
int show_vpo = 0;

// haleyjd 09/29/04: customizable VPO threshold

int vpo_threshold;

void HU_WarningsDrawer(void)
{
   if(show_vpo && num_visplanes > vpo_threshold)
      V_DrawPatch(250, 10, &vbscreen, vpo);
   
   if(opensocket)
      V_DrawPatch(20, 20, &vbscreen, socket_pic);
}

/////////////////////////////////////////////////////////////////////////
//
// Text Widgets
//
// the main text widgets. does not include the normal messages
// 'picked up a clip' etc

textwidget_t *widgets[MAXWIDGETS];
int num_widgets = 0;

void HU_AddWidget(textwidget_t *widget)
{
#ifdef RANGECHECK
   if(num_widgets >= MAXWIDGETS)
      I_Error("HU_AddWidget: too many widgets!\n");
#endif

   widgets[num_widgets] = widget;
   num_widgets++;
}

// draw widgets

void HU_WidgetsDraw(void)
{
   int i;
   
   // check each widget.
   // draw according to font type, and only if message being displayed

   for(i=0; i<num_widgets; i++)
   {
      if(widgets[i]->message &&
	 (!widgets[i]->cleartic || leveltime < widgets[i]->cleartic))
      {
	(widgets[i]->font ? HU_WriteText : V_WriteText)
	  (widgets[i]->message, widgets[i]->x, widgets[i]->y);
      }
   }
}

void HU_WidgetsTick(void)
{
   int i;
   
   for(i = 0; i < num_widgets; i++)
   {
      if(widgets[i]->handler)
         widgets[i]->handler(widgets[i]); // haleyjd
   }
}

        // erase all the widget text
void HU_WidgetsErase(void)
{
   int i;
   
   for(i = 0; i < num_widgets; ++i)
      R_VideoErase(0, widgets[i]->y, SCREENWIDTH, 8);
}

//////////////////////////////////////
//
// The widgets

void HU_LevelTimeHandler(struct textwidget_s *widget);
void HU_LevelNameHandler(struct textwidget_s *widget);
void HU_ChatHandler(struct textwidget_s *widget);
void HU_CoordHandler(struct textwidget_s *widget); // haleyjd

//////////////////////////////////////////////////
//
// Centre-of-screen, quake-style message
//

textwidget_t hu_centermessage =
{
  0, 0,                      // x,y set by HU_CenterMessage
  0,                         // normal font
  NULL,                      // init to nothing
  NULL,                      // handler
};

void HU_CenterMessageClear(void)
{
   hu_centermessage.message = NULL;
}

//
// HU_CenterMessage
//
// haleyjd 04/27/04: rewritten to use qstring
//
void HU_CenterMessage(const char *s)
{
   static qstring_t qstr;
   static boolean first = true;  
   int st_height = gameModeInfo->StatusBar->height;

   if(first)
   {
      M_QStrCreate(&qstr);
      first = false;
   }
   else
      M_QStrClear(&qstr);
   
   M_QStrCat(&qstr, s);
  
   hu_centermessage.message = M_QStrBuffer(&qstr);
   hu_centermessage.x = (SCREENWIDTH-V_StringWidth(s)) / 2;
   hu_centermessage.y = (SCREENHEIGHT-V_StringHeight(s) -
      ((scaledviewheight==SCREENHEIGHT) ? 0 : st_height-8)) / 2;
   hu_centermessage.cleartic = leveltime + (message_timer * 35) / 1000;
   
   // print to console
   C_Printf("%s\n", s);
}

//
// HU_CenterMessageTimed
//
// haleyjd: timed center message. Originally for FraggleScript,
// now revived for Small.
//
void HU_CenterMessageTimed(const char *s, int tics)
{
   HU_CenterMessage(s);
   hu_centermessage.cleartic = leveltime + tics;
}

/////////////////////////////////////
//
// Elapsed level time (automap)
//

textwidget_t hu_leveltime =
{
  SCREENWIDTH-60, SCREENHEIGHT-ST_HEIGHT-8,      // x, y
  0,                                             // normal font
  NULL,                                          // null msg
  HU_LevelTimeHandler                            // handler
};

void HU_LevelTimeHandler(struct textwidget_s *widget)
{
   static char timestr[32];
   int seconds;
   
   if(!automapactive)
   {
      hu_leveltime.message = NULL;
      return;
   }
   
   seconds = levelTime / 35;
   timestr[0] = '\0';
   
   psnprintf(timestr, sizeof(timestr), "%02i:%02i:%02i", 
             seconds/3600, (seconds%3600)/60, seconds%60);
   
   hu_leveltime.message = timestr;        
}

///////////////////////////////////////////
//
// Automap level name display
//

textwidget_t hu_levelname =
{
  0, SCREENHEIGHT-ST_HEIGHT-8,       // x,y 
  0,                                 // normal font
  NULL,                              // init to nothing
  HU_LevelNameHandler                // handler
};

void HU_LevelNameHandler(struct textwidget_s *widget)
{
   hu_levelname.message = automapactive ? LevelInfo.levelName : NULL;
}

///////////////////////////////////////////////
//
// Chat message display
//

textwidget_t hu_chat = 
{
  0, 0,                 // x,y
  0,                    // use normal font
  NULL,                 // empty message
  HU_ChatHandler        // handler
};
char chatinput[100] = "";

void HU_ChatHandler(struct textwidget_s *widget)
{
   static char tempchatmsg[128];
   
   if(chat_active)
   {
      psnprintf(tempchatmsg, sizeof(tempchatmsg), "%s_", chatinput);
      hu_chat.message = tempchatmsg;
   }
   else
      hu_chat.message = NULL;
}

boolean HU_ChatRespond(event_t *ev)
{
   char ch;
   static boolean shiftdown;
   
   if(ev->data1 == KEYD_RSHIFT) shiftdown = ev->type == ev_keydown;
   
   if(ev->type != ev_keydown) return false;
   
   if(!chat_active)
   {
      if(ev->data1 == key_chat && netgame) 
      {       
         chat_active = true;     // activate chat
         chatinput[0] = 0;       // empty input string
         return true;
      }
      return false;
   }
  
   if(altdown && ev->type == ev_keydown &&
      ev->data1 >= '0' && ev->data1 <= '9')
   {
      // chat macro
      char tempstr[100];
      psnprintf(tempstr, sizeof(tempstr),
                "say \"%s\"", chat_macros[ev->data1-'0']);
      C_RunTextCmd(tempstr);
      chat_active = false;
      return true;
   }
  
   if(ev->data1 == KEYD_ESCAPE)    // kill chat
   {
      chat_active = false;
      return true;
   }
  
   if(ev->data1 == KEYD_BACKSPACE && chatinput[0])
   {
      chatinput[strlen(chatinput)-1] = 0;      // remove last char
      return true;
   }
  
   if(ev->data1 == KEYD_ENTER)
   {
      char tempstr[100];
      psnprintf(tempstr, sizeof(tempstr), "say \"%s\"", chatinput);
      C_RunTextCmd(tempstr);
      chat_active = false;
      return true;
   }

   ch = shiftdown ? shiftxform[ev->data1] : ev->data1; // shifted?
   
   if(ch>31 && ch<127)
   {
      psnprintf(chatinput, sizeof(chatinput), "%s%c", chatinput, ch);
      C_InitTab();
      return true;
   }
   return false;
}

//=================================
//
// Automap coordinate display
//
//   Yet Another Lost MBF Feature
//   Restored by Quasar (tm)
//

textwidget_t hu_coordx =
{
  SCREENWIDTH-64, 8,    // x,y
  0,                    // use normal font
  NULL,                 // empty message
  HU_CoordHandler       // handler
};

textwidget_t hu_coordy =
{
  SCREENWIDTH-64, 17,   // x,y
  0,                    // use normal font
  NULL,                 // empty message
  HU_CoordHandler       // handler
};

textwidget_t hu_coordz =
{
  SCREENWIDTH-64, 25,   // x,y
  0,                    // use normal font
  NULL,                 // empty message
  HU_CoordHandler       // handler
};

void HU_CoordHandler(struct textwidget_s *widget)
{
   player_t *plyr;
   fixed_t x,y,z;
   
   // haleyjd: wow, big bug here -- these buffers were not static
   // and thus corruption was occuring when the function returned
   static char coordxstr[12];
   static char coordystr[12];
   static char coordzstr[12];

   if(!automapactive)
   {
      widget->message = NULL;
      return;
   }
   plyr = &players[displayplayer];

   AM_Coordinates(plyr->mo, &x, &y, &z);

   if(widget == &hu_coordx)
   {
      sprintf(coordxstr, "X: %-5d", x>>FRACBITS);
      widget->message = coordxstr;
   }
   else if(widget == &hu_coordy)
   {
      sprintf(coordystr, "Y: %-5d", y>>FRACBITS);
      widget->message = coordystr;
   }
   else
   {
      sprintf(coordzstr, "Z: %-5d", z>>FRACBITS);
      widget->message = coordzstr;
   }
}
   
// Widgets Init

//
// HU_HticWidgetsInit
//
// haleyjd 12/26/02: called from below to change certain
// widget settings for Heretic
//
static void HU_HticWidgetsInit(void)
{
   // move level name widget
   hu_levelname.x = 20;
   hu_levelname.y = 145;
   
   // move level time widget
   hu_leveltime.x = 240;
   hu_leveltime.y = 10;

   // move coords
   hu_coordx.x = hu_coordy.x = hu_coordz.x = 20;
   hu_coordx.y = 10;
   hu_coordy.y = 19;
   hu_coordz.y = 28;
}

void HU_WidgetsInit(void)
{
   // haleyjd: change stuff for Heretic if necessary
   if(gameModeInfo->type == Game_Heretic)
      HU_HticWidgetsInit();

   HU_AddWidget(&hu_centermessage);
   HU_AddWidget(&hu_levelname);
   HU_AddWidget(&hu_leveltime);
   HU_AddWidget(&hu_chat);
   HU_AddWidget(&hu_coordx); // haleyjd
   HU_AddWidget(&hu_coordy);
   HU_AddWidget(&hu_coordz);
}

////////////////////////////////////////////////////////////////////////
//
// Tables
//

const char* shiftxform;

const char english_shiftxform[] =
{
  0,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31,
  ' ', '!', '"', '#', '$', '%', '&',
  '"', // shift-'
  '(', ')', '*', '+',
  '<', // shift-,
  '_', // shift--
  '>', // shift-.
  '?', // shift-/
  ')', // shift-0
  '!', // shift-1
  '@', // shift-2
  '#', // shift-3
  '$', // shift-4
  '%', // shift-5
  '^', // shift-6
  '&', // shift-7
  '*', // shift-8
  '(', // shift-9
  ':',
  ':', // shift-;
  '<',
  '+', // shift-=
  '>', '?', '@',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '[', // shift-[
  '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
  ']', // shift-]
  '"', '_',
  '\'', // shift-`
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '{', '|', '}', '~', 127
};

/////////////////////////////////////////////////////////////////////////
//
// Console Commands
//

VARIABLE_BOOLEAN(showMessages,  NULL,                   onoff);
VARIABLE_INT(mess_colour,       NULL, 0, CR_LIMIT-1,    textcolours);

VARIABLE_BOOLEAN(obituaries,    NULL,                   onoff);
VARIABLE_INT(obcolour,          NULL, 0, CR_LIMIT-1,    textcolours);

VARIABLE_INT(crosshairnum,      NULL, 0, CROSSHAIRS-1,  cross_str);
VARIABLE_BOOLEAN(show_vpo,      NULL,                   yesno);
VARIABLE_INT(vpo_threshold,     NULL, 1, 128,      NULL);

VARIABLE_INT(hud_msg_lines,     NULL, 0, 14,            NULL);
VARIABLE_INT(message_timer,     NULL, 0, 100000,        NULL);
VARIABLE_BOOLEAN(hud_msg_scrollup,  NULL,               yesno);

CONSOLE_VARIABLE(obituaries, obituaries, 0) {}
CONSOLE_VARIABLE(obcolour, obcolour, 0) {}
CONSOLE_VARIABLE(crosshair, crosshairnum, 0)
{
   int a;
   
   a = atoi(c_argv[0]);
   
   crosshair = a ? crosshairs[a-1] : NULL;
   crosshairnum = a;
}

CONSOLE_VARIABLE(show_vpo, show_vpo, 0) {}
CONSOLE_VARIABLE(vpo_threshold, vpo_threshold, 0) {}
CONSOLE_VARIABLE(messages, showMessages, 0) {}
CONSOLE_VARIABLE(mess_colour, mess_colour, 0) {}
CONSOLE_NETCMD(say, cf_netvar, netcmd_chat)
{
   S_StartSound(NULL, gameModeInfo->c_ChatSound);
   
   doom_printf("%s: %s", players[cmdsrc].name, c_args);
}

CONSOLE_VARIABLE(mess_lines, hud_msg_lines, 0) {}
CONSOLE_VARIABLE(mess_scrollup, hud_msg_scrollup, 0) {}
CONSOLE_VARIABLE(mess_timer, message_timer, 0) {}

extern void HU_FragsAddCommands(void);
extern void HU_OverAddCommands(void);

void HU_AddCommands(void)
{
   C_AddCommand(obituaries);
   C_AddCommand(obcolour);
   C_AddCommand(crosshair);
   C_AddCommand(show_vpo);
   C_AddCommand(messages);
   C_AddCommand(mess_colour);
   C_AddCommand(say);
   
   C_AddCommand(mess_lines);
   C_AddCommand(mess_scrollup);
   C_AddCommand(mess_timer);

   C_AddCommand(vpo_threshold);
   
   HU_FragsAddCommands();
   HU_OverAddCommands();
}

//
// Script functions
//

static cell AMX_NATIVE_CALL sm_centermsgtimed(AMX *amx, cell *params)
{
   int tics, err;
   char *text;

   if((err = A_GetSmallString(amx, &text, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   tics = params[2];

   HU_CenterMessageTimed(text, tics);

   free(text);

   return 0;
}

AMX_NATIVE_INFO hustuff_Natives[] =
{
   { "IO_CenterMsgTimed", sm_centermsgtimed },
   { NULL, NULL }
};

// EOF
