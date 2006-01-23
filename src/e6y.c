#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <string.h>
#include <math.h>
#include <SDL_opengl.h>

#include <stdio.h>

#include "SDL.h"

#include "hu_lib.h"

#include "doomtype.h"
#include "doomstat.h"
#include "d_main.h"
#include "s_sound.h"
#include "i_main.h"
#include "m_menu.h"
#include "p_spec.h"
#include "lprintf.h"
#include "e6y.h"

#define Pi 3.14159265358979323846f

boolean wasWiped = false;

int secretfound;
int messagecenter_counter;
int demo_skiptics;

int avi_shot_count;
int avi_shot_time;
int avi_shot_num;
const char *avi_shot_fname;
char avi_shot_curr_fname[PATH_MAX];

FILE    *_demofp;
boolean doSkip;
boolean demo_stoponnext;
boolean demo_warp;

int key_speed_up;
int key_speed_down;
int key_speed_default;
int key_demo_jointogame;
int key_demo_nextlevel;
int speed_step;
int key_walkcamera;

int hudadd_gamespeed;
int hudadd_leveltime;
int hudadd_secretarea;
int hudadd_smarttotals;
int movement_mouselook;
int _movement_mouselook;
int movement_mouseinvert;
int movement_strafe50;
int movement_strafe50onturns;
int movement_smooth;
int view_fov;
int _view_fov;
int render_usedetail;
int render_detailwalls;
int render_detailflats;
int render_detailsprites;
int render_detailanims;
int render_usedetailwalls;
int render_usedetailflats;
int render_usedetailsprites;
int demo_smoothturns;
int demo_smoothturnsfactor;
int demo_overwriteexisting;

int palette_ondamage;
int palette_onbonus;
int palette_onpowers;

camera_t walkcamera;
mobj_t *oviewer;

hu_textline_t  w_hudadd;
hu_textline_t  w_centermsg;
char hud_timestr[80];
char hud_centermsg[80];

fixed_t sidemove_normal[2]    = {0x18, 0x28};
fixed_t sidemove_strafe50[2]    = {0x18, 0x32};

fixed_t	r_TicFrac;
int otic;
boolean NewThinkerPresent = FALSE;

fixed_t PrevX;
fixed_t PrevY;
fixed_t PrevZ;

fixed_t oviewx;
fixed_t oviewy;
fixed_t oviewz;
angle_t oviewangle;
angle_t oviewpitch;

int PitchSign;
int mouseSensitivity_mlook;
angle_t viewpitch;
float fovscale;
float tan_pitch;
float skyUpAngle;
float skyUpShift;
float skyXShift;
float skyYShift;

boolean SkyDrawed;

boolean isExtraDDisplay = false;
boolean skipDDisplay = false;
unsigned int DDisplayTime;

int idDetail;
boolean gl_arb_multitexture;
PFNGLACTIVETEXTUREARBPROC        glActiveTextureARB       = NULL;
PFNGLMULTITEXCOORD2FVARBPROC     glMultiTexCoord2fvARB    = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC  glClientActiveTextureARB = NULL;
PFNGLMULTITEXCOORD2FARBPROC      glMultiTexCoord2fARB     = NULL;

static boolean saved_fastdemo;
static boolean saved_nodrawers;
static boolean saved_nosfxparm;
static boolean saved_nomusicparm;

void G_SkipDemoStart(void)
{
  saved_fastdemo = fastdemo;
  saved_nodrawers = nodrawers;
  saved_nosfxparm = nosfxparm;
  saved_nomusicparm = nomusicparm;
  
  doSkip = true;
  fastdemo = true;
  nodrawers = true;
  nosfxparm = true;
  nomusicparm = true;
  I_Init();
}

void G_SkipDemoStop(void)
{
  fastdemo = saved_fastdemo;
  nodrawers = saved_nodrawers;
  nosfxparm = saved_nosfxparm;
  nomusicparm = saved_nomusicparm;

  demo_stoponnext = false;
  demo_warp = false;
  doSkip = false;
  demo_skiptics = 0;
  startmap = 0;
  I_Init();
  S_Init(snd_SfxVolume, snd_MusicVolume);
  S_Start();
}

void M_ChangeSmooth(void)
{
}

void M_ChangeSpeed(void)
{
  extern int sidemove[2];
  extern int sidemove_normal[2];
  extern int sidemove_strafe50[2];

  if(movement_strafe50)
  {
    sidemove[0] = sidemove_strafe50[0];
    sidemove[1] = sidemove_strafe50[1];
  }
  else
  {
    sidemove[0] = sidemove_normal[0];
    sidemove[1] = sidemove_normal[1];
  }
}

void M_ChangeMouseLook(void)
{
#ifdef GL_DOOM
  movement_mouselook = _movement_mouselook;
#else
  movement_mouselook = false;
#endif
  viewpitch = 0;
}

void CheckPitch(signed int *pitch)
{
#define maxAngle (ANG90 - (1<<ANGLETOFINESHIFT))
#define minAngle (-ANG90 + (1<<ANGLETOFINESHIFT))

  if(*pitch > maxAngle)
    *pitch = maxAngle;
  if(*pitch < minAngle)
    *pitch = minAngle;
}

void M_ChangeMouseInvert(void)
{
  if(movement_mouseinvert)
    PitchSign = +1;
  else
    PitchSign = -1;
}

void M_ChangeFOV(void)
{
  float f1, f2;
#ifdef GL_DOOM
  view_fov = _view_fov;
#else
  view_fov = 64;
#endif
  fovscale = 64.0f/(float)view_fov;

  f1 = (float)(320.0f/200.0f/fovscale-0.2f);
  f2 = (float)tan(view_fov/2.0f*Pi/180.0f);
  if (f1-f2<1)
    skyUpAngle = (float)-asin(f1-f2)*180.0f/Pi;
  else
    skyUpAngle = -90.0f;

  skyUpShift = (float)tan((view_fov/2.0f)*Pi/180.0f);
}

void M_ChangeUseDetail(void)
{
#ifdef GL_DOOM
  extern setup_menu_t stat_settings3[];
  
  render_usedetail = render_usedetail;// && gl_arb_multitexture;

  render_usedetailwalls   = render_usedetail && render_detailwalls;
  render_usedetailflats   = render_usedetail && render_detailflats;
  render_usedetailsprites = render_usedetail && render_detailsprites;

  if (render_usedetail)
  {
    stat_settings3[6].m_flags &= ~(S_SKIP|S_SELECT);
    stat_settings3[7].m_flags &= ~(S_SKIP|S_SELECT);
//    stat_settings3[8].m_flags &= ~(S_SKIP|S_SELECT);
  }
  else
  {
    stat_settings3[6].m_flags |= (S_SKIP|S_SELECT);
    stat_settings3[7].m_flags |= (S_SKIP|S_SELECT);
//    stat_settings3[8].m_flags |= (S_SKIP|S_SELECT);
  }
#endif
}

void M_Mouse(int choice, int *sens);
void M_MouseMLook(int choice)
{
  M_Mouse(choice, &mouseSensitivity_mlook);
}

void M_DemosBrowse(void)
{
}

unsigned int TicStart;
unsigned int TicNext;

extern int realtic_clock_rate;
extern int_64_t I_GetTime_Scale;

void Extra_D_Display(void)
{
#ifdef GL_DOOM
  if (movement_smooth)
#else
  if (movement_smooth && gamestate==wipegamestate)
#endif
  {
    isExtraDDisplay = true;
    D_Display();
    isExtraDDisplay = false;
  }
}

fixed_t I_GetTimeFrac (void)
{
  unsigned long now;
  unsigned int step;
  fixed_t frac;


  now = SDL_GetTicks();

  step = TicNext - TicStart;
  if (step == 0)
    return FRACUNIT;
  else
  {
    frac = (fixed_t)((now - TicStart + DDisplayTime) * FRACUNIT / (float)step);
    if (frac < 0)
      frac = 0;
    if (frac > FRACUNIT)
      frac = FRACUNIT;
    return frac;
  }
}

void I_GetTime_SaveMS(void)
{
  float d;

  if (!movement_smooth)
    return;

  TicStart = SDL_GetTicks();
  d = realtic_clock_rate * TICRATE / 100000.0f;
  TicNext = (unsigned int) ((TicStart * d + 1.0f) / d);
}

//------------

int numinterpolations = 0;
int startofdynamicinterpolations = 0;
fixed_t oldipos[MAXINTERPOLATIONS][1];
fixed_t bakipos[MAXINTERPOLATIONS][1];
FActiveInterpolation curipos[MAXINTERPOLATIONS];
boolean NoInterpolateView;
boolean r_NoInterpolate;
static boolean didInterp;

void R_InterpolateView (player_t *player, fixed_t frac)
{
  if (movement_smooth)
  {
    if (NoInterpolateView)
    {
      NoInterpolateView = false;
      oviewx = player->mo->x;
      oviewy = player->mo->y;
      oviewz = player->viewz;

      oviewangle = player->mo->angle + viewangleoffset;
      oviewpitch = player->mo->pitch;

      if(walkcamera.type)
      {
        walkcamera.PrevX = walkcamera.x;
        walkcamera.PrevY = walkcamera.y;
        walkcamera.PrevZ = walkcamera.z;
        walkcamera.PrevAngle = walkcamera.angle;
        walkcamera.PrevPitch = walkcamera.pitch;
      }
    }

    if (walkcamera.type != 2)
    {
      viewx = oviewx + FixedMul (frac, player->mo->x - oviewx);
      viewy = oviewy + FixedMul (frac, player->mo->y - oviewy);
      viewz = oviewz + FixedMul (frac, player->viewz - oviewz);
    }
    else
    {
      viewx = walkcamera.PrevX + FixedMul (frac, walkcamera.x - walkcamera.PrevX);
      viewy = walkcamera.PrevY + FixedMul (frac, walkcamera.y - walkcamera.PrevY);
      viewz = walkcamera.PrevZ + FixedMul (frac, walkcamera.z - walkcamera.PrevZ);
    }

    if (walkcamera.type)
    {
      viewangle = walkcamera.PrevAngle + FixedMul (frac, walkcamera.angle - walkcamera.PrevAngle);
      viewpitch = walkcamera.PrevPitch + FixedMul (frac, walkcamera.pitch - walkcamera.PrevPitch);
    }
    else
    {
      //static FILE* f = NULL;

      viewangle = oviewangle + FixedMul (frac, GetSmoothViewAngel(player->mo->angle) + viewangleoffset - oviewangle);
      viewpitch = oviewpitch + FixedMul (frac, player->mo->pitch /*+ viewangleoffset*/ - oviewpitch);

      //if (!f)
      //  f = fopen("info.txt", "wb");
      //fprintf(f, "%.10d: %.10d-%.10d-%.10d\n", gametic, oviewangle, viewangle, player->mo->angle);
    }
  }
  else
  {
    if (walkcamera.type != 2)
    {
      viewx = player->mo->x;
      viewy = player->mo->y;
      viewz = player->viewz;
    }
    else
    {
      viewx = walkcamera.x;
      viewy = walkcamera.y;
      viewz = walkcamera.z;
    }
    if (walkcamera.type)
    {
      viewangle = walkcamera.angle;
      viewpitch = walkcamera.pitch;
    }
    else
    {
      viewangle = GetSmoothViewAngel(player->mo->angle);
      //viewangle = player->mo->angle + viewangleoffset;
      viewpitch = player->mo->pitch;// + viewangleoffset;
    }
  }
}

void R_ResetViewInterpolation ()
{
  NoInterpolateView = true;
}

void CopyInterpToOld (int i)
{
  switch (curipos[i].Type)
  {
  case INTERP_SectorFloor:
    oldipos[i][0] = ((sector_t*)curipos[i].Address)->floorheight;
    break;
  case INTERP_SectorCeiling:
    oldipos[i][0] = ((sector_t*)curipos[i].Address)->ceilingheight;
    break;
  case INTERP_Vertex:
    oldipos[i][0] = ((vertex_t*)curipos[i].Address)->x;
    oldipos[i][1] = ((vertex_t*)curipos[i].Address)->y;
    break;
  }
}

void CopyBakToInterp (int i)
{
  switch (curipos[i].Type)
  {
  case INTERP_SectorFloor:
    ((sector_t*)curipos[i].Address)->floorheight = bakipos[i][0];
    break;
  case INTERP_SectorCeiling:
    ((sector_t*)curipos[i].Address)->ceilingheight = bakipos[i][0];
    break;
  case INTERP_Vertex:
    ((vertex_t*)curipos[i].Address)->x = bakipos[i][0];
    ((vertex_t*)curipos[i].Address)->y = bakipos[i][1];
    break;
  }
}

void DoAnInterpolation (int i, fixed_t smoothratio)
{
  fixed_t *adr1, pos;

  switch (curipos[i].Type)
  {
  case INTERP_SectorFloor:
    adr1 = &((sector_t*)curipos[i].Address)->floorheight;
    break;
  case INTERP_SectorCeiling:
    adr1 = &((sector_t*)curipos[i].Address)->ceilingheight;
    break;
  case INTERP_Vertex:
    adr1 = &((vertex_t*)curipos[i].Address)->x;
////    adr2 = &((vertex_t*)curipos[i].Address)->y;
    break;
 default:
    return;
  }

  pos = bakipos[i][0] = *adr1;
  *adr1 = oldipos[i][0] + FixedMul (pos - oldipos[i][0], smoothratio);
}

void updateinterpolations()
{
  int i;
  if (!movement_smooth)
    return;
  for (i = numinterpolations-1; i >= 0; --i)
    CopyInterpToOld (i);
}

void setinterpolation(EInterpType type, void *posptr)
{
  int i;
  if (!movement_smooth)
    return;
  if (numinterpolations >= MAXINTERPOLATIONS) return;
  for(i = numinterpolations-1; i >= 0; i--)
    if (curipos[i].Address == posptr && curipos[i].Type == type) return;
  curipos[numinterpolations].Address = posptr;
  curipos[numinterpolations].Type = type;
  CopyInterpToOld (numinterpolations);
  numinterpolations++;
} 

void stopinterpolation(EInterpType type, void *posptr)
{
  int i;

  if (!movement_smooth)
    return;

  for(i=numinterpolations-1; i>= startofdynamicinterpolations; --i)
  {
    if (curipos[i].Address == posptr && curipos[i].Type == type)
    {
      numinterpolations--;
      oldipos[i][0] = oldipos[numinterpolations][0];
      bakipos[i][0] = bakipos[numinterpolations][0];
      curipos[i] = curipos[numinterpolations];
      break;
    }
  }
}

void stopallinterpolation(void)
{
  int i;
  
  if (!movement_smooth)
    return;

  for(i=numinterpolations-1; i>= startofdynamicinterpolations; --i)
  {
    numinterpolations--;
    oldipos[i][0] = oldipos[numinterpolations][0];
    bakipos[i][0] = bakipos[numinterpolations][0];
    curipos[i] = curipos[numinterpolations];
  }
}

void dointerpolations(fixed_t smoothratio)
{
  int i;
  if (!movement_smooth)
    return;
  if (smoothratio == FRACUNIT)
  {
    didInterp = false;
    return;
  }

  didInterp = true;

  for (i = numinterpolations-1; i >= 0; --i)
  {
    DoAnInterpolation (i, smoothratio);
  }
}

void restoreinterpolations()
{
  int i;
  
  if (!movement_smooth)
    return;

  if (didInterp)
  {
    didInterp = false;
    for (i = numinterpolations-1; i >= 0; --i)
    {
      CopyBakToInterp (i);
    }
  }
}

void P_ActivateAllInterpolations()
{
  int i;
  sector_t     *sec;

  if (!movement_smooth)
    return;

  for (i=0, sec = sectors ; i<numsectors ; i++,sec++)
  {
    if (sec->floordata)
    {
      setinterpolation (INTERP_SectorFloor, sec);
    }
    if (sec->ceilingdata)
    {
      setinterpolation (INTERP_SectorCeiling, sec);
    }
  }
}

void SetInterpolationIfNew(thinker_t *th)
{
  void *posptr = NULL;
  EInterpType type;
  int i;

  if (!movement_smooth)
    return;

  if (th->function == T_MoveFloor)
  {
    type = INTERP_SectorFloor;
    posptr = ((floormove_t *)th)->sector;
  }
  else
  if (th->function == T_PlatRaise)
  {
    type = INTERP_SectorFloor;
    posptr = ((plat_t *)th)->sector;
  }
  else
  if (th->function == T_MoveCeiling)
  {
    type = INTERP_SectorCeiling;
    posptr = ((ceiling_t *)th)->sector;
  }
  else
  if (th->function == T_VerticalDoor)
  {
    type = INTERP_SectorCeiling;
    posptr = ((vldoor_t *)th)->sector;
  }

  if(posptr)
  {
    for(i=numinterpolations-1; i>= startofdynamicinterpolations; --i)
      if (curipos[i].Address == posptr)
        return;

    setinterpolation (type, posptr);
  }
}

void StopInterpolationIfNeeded(thinker_t *th)
{
  void *posptr = NULL;
  EInterpType type;

  if (!movement_smooth)
    return;

  if (th->function == T_MoveFloor)
  {
    type = INTERP_SectorFloor;
    posptr = ((floormove_t *)th)->sector;
  }
  else
  if (th->function == T_PlatRaise)
  {
    type = INTERP_SectorFloor;
    posptr = ((plat_t *)th)->sector;
  }
  else
  if (th->function == T_MoveCeiling)
  {
    type = INTERP_SectorCeiling;
    posptr = ((ceiling_t *)th)->sector;
  }
  else
  if (th->function == T_VerticalDoor)
  {
    type = INTERP_SectorCeiling;
    posptr = ((vldoor_t *)th)->sector;
  }

  if(posptr)
  {
    stopinterpolation (type, posptr);
  }
}

#ifdef GL_DOOM

float xCamera,yCamera;
TAnimItemParam *anim_flats = NULL;
TAnimItemParam *anim_textures = NULL;

void e6y_PreprocessLevel(void)
{
  if (gl_arb_multitexture)
  {
    extern void *gld_texcoords;

    glClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2,GL_FLOAT,0,gld_texcoords);
    glClientActiveTextureARB(GL_TEXTURE1_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2,GL_FLOAT,0,gld_texcoords);
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);
    glActiveTextureARB(GL_TEXTURE0_ARB);
  }
}

void e6y_InitExtensions(void)
{
#define isExtensionSupported(ext) strstr(extensions, ext)
  static int imageformats[5] = {0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA};

  extern int gl_tex_filter;
  extern int gl_mipmap_filter;
  extern int gl_texture_filter_anisotropic;
  extern int gl_tex_format;

  char *extensions = (char*)glGetString(GL_EXTENSIONS);

  gl_arb_multitexture = isExtensionSupported("GL_ARB_multitexture") != NULL;

  if (gl_arb_multitexture)
  {
    glActiveTextureARB       = SDL_GL_GetProcAddress("glActiveTextureARB");
    glClientActiveTextureARB = SDL_GL_GetProcAddress("glClientActiveTextureARB");
    glMultiTexCoord2fvARB    = SDL_GL_GetProcAddress("glMultiTexCoord2fvARB");
    glMultiTexCoord2fARB     = SDL_GL_GetProcAddress("glMultiTexCoord2fARB");

    if (!glActiveTextureARB    || !glClientActiveTextureARB ||
        !glMultiTexCoord2fvARB || !glMultiTexCoord2fARB)
      gl_arb_multitexture = false;
  }
  //gl_arb_multitexture = false;

  //if (gl_arb_multitexture)
  {
    HRSRC hDetail;
    void *memDetail;
    SDL_PixelFormat fmt;
    SDL_Surface *surf = NULL;

    hDetail = FindResource(NULL, MAKEINTRESOURCE(115), RT_RCDATA);
    memDetail = LockResource(LoadResource(NULL, hDetail));

    surf = SDL_LoadBMP_RW(SDL_RWFromMem(memDetail, SizeofResource(NULL, hDetail)), 1);
    fmt = *surf->format;
    fmt.BitsPerPixel = 24;
    fmt.BytesPerPixel = 3;
    surf = SDL_ConvertSurface(surf, &fmt, surf->flags);
    if (surf)
    {
      if (gl_arb_multitexture)
        glActiveTextureARB(GL_TEXTURE1_ARB);
      glGenTextures(1, &idDetail);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glBindTexture(GL_TEXTURE_2D, idDetail);
      
      gluBuild2DMipmaps(GL_TEXTURE_2D, 
        surf->format->BytesPerPixel, 
        surf->w, surf->h, 
        imageformats[surf->format->BytesPerPixel], 
        GL_UNSIGNED_BYTE, surf->pixels);
      
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_mipmap_filter);
      //if (gl_texture_filter_anisotropic)
      //  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0);

      if (gl_arb_multitexture)
        glActiveTextureARB(GL_TEXTURE0_ARB);
      
      SDL_FreeSurface(surf);
    }
    else
      render_usedetail = false;
  }
  M_ChangeUseDetail();
  if (gl_arb_multitexture)
    lprintf(LO_INFO,"e6y: using GL_ARB_multitexture\n",glGetString(GL_VENDOR));
}

float distance2piece(float x0, float y0, float x1, float y1, float x2, float y2)
{
  float t, w;
  
  float x01 = x0-x1;
  float x02 = x0-x2;
  float x21 = x2-x1;
  float y01 = y0-y1;
  float y02 = y0-y2;
  float y21 = y2-y1;

  if((x01*x21+y01*y21)*(x02*x21+y02*y21)>0.0001f)
  {
    t = x01*x01 + y01*y01;
    w = x02*x02 + y02*y02;
    if (w < t) t = w;
  }
  else
  {
    float i1 = x01*y21-y01*x21;
    float i2 = x21*x21+y21*y21;
    t = (i1*i1)/i2;
  }
  return t;
}

#endif //GL_DOOM

//int demos_lastturns[MAX_DEMOS_SMOOTHFACTOR*2+1];
int demos_lastturns[MAX_DEMOS_SMOOTHFACTOR];
int_64_t demos_lastturnssum;
int demos_lastturnsindex;
angle_t demos_smoothangle;
const byte *demo_p_end;
int playerscount;

void GetCurrentTurnsSum(void);

void e6y_ProcessDemoHeader(void)
{
  int i;
  playerscount = 0;
  for (i=0; i < MAXPLAYERS; i++)
    if (playeringame[i]) playerscount++;
}

void M_ChangeDemoSmoothTurns(void)
{
  extern setup_menu_t stat_settings2[];

  if (demo_smoothturns)
    stat_settings2[8].m_flags &= ~(S_SKIP|S_SELECT);
  else
    stat_settings2[8].m_flags |= (S_SKIP|S_SELECT);

  ClearSmoothViewAngels();
}

void ClearSmoothViewAngels()
{
  if (demo_smoothturns && demoplayback)
  {
    if (players)
      demos_smoothangle = players[displayplayer].mo->angle;

    memset(demos_lastturns, 0, sizeof(demos_lastturns[0]) * MAX_DEMOS_SMOOTHFACTOR);
    demos_lastturnssum = 0;
    demos_lastturnsindex = 0;
  }
}

void AddSmoothViewAngel(int delta)
{
  if (demo_smoothturns && demoplayback)
  {
    demos_lastturnssum -= demos_lastturns[demos_lastturnsindex];
    demos_lastturns[demos_lastturnsindex] = delta;
//    demos_lastturnsindex = (demos_lastturnsindex + 1)%(demo_smoothturnsfactor*2+1);
    demos_lastturnsindex = (demos_lastturnsindex + 1)%(demo_smoothturnsfactor);
    demos_lastturnssum += delta;

//    demos_smoothangle += (int)(demos_lastturnssum/(demo_smoothturnsfactor*2+1));
    demos_smoothangle += (int)(demos_lastturnssum/(demo_smoothturnsfactor));
  }
}

angle_t GetSmoothViewAngel(angle_t defangle)
{
  if (demo_smoothturns && demoplayback)
    return demos_smoothangle;
  else
    return defangle;
}

void e6y_AfterTeleporting(void)
{
  R_ResetViewInterpolation();
  ClearSmoothViewAngels();
}

float viewPitch;
boolean WasRenderedInTryRunTics;
boolean trasparentpresent;

//int viewMaxY;

//int t_count;
//TRec t_list[128];