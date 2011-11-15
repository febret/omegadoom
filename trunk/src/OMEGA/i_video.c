/********************************************************************************************************************** 
 * THE OMEGA LIB PROJECT
 *---------------------------------------------------------------------------------------------------------------------
 * Copyright 2010								Electronic Visualization Laboratory, University of Illinois at Chicago
 * Authors:										
 *  Alessandro Febretti							febret@gmail.com
 *---------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2010, Electronic Visualization Laboratory, University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
 * following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
 * disclaimer. Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
 * and the following disclaimer in the documentation and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-----------------------------------------------------------------------------
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *********************************************************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "m_argv.h"
#include "doomstat.h"
#include "doomdef.h"
#include "doomtype.h"
#include "v_video.h"
#include "r_draw.h"
#include "d_main.h"
#include "d_event.h"
#include "i_joy.h"
#include "i_video.h"
#include "z_zone.h"
#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "st_stuff.h"
#include "lprintf.h"
#include "i_pngshot.h"

#include "OMEGA/OMEGA_interface.h"

int gl_colorbuffer_bits=16;
int gl_depthbuffer_bits=16;

extern void M_QuitDOOM(int choice);
#ifdef DISABLE_DOUBLEBUFFER
int use_doublebuffer = 0;
#else
int use_doublebuffer = 1; // Included not to break m_misc, but not relevant to SDL
#endif
int use_fullscreen = 0;

////////////////////////////////////////////////////////////////////////////
// Input code
int             leds_always_off = 0; // Expected by m_misc, not relevant

// Mouse handling
extern int     usemouse;        // config file var
static boolean mouse_enabled; // usemouse, but can be overriden by -nomouse
static boolean mouse_currently_grabbed;

//
// I_StartTic
//

void I_StartTic (void)
{
  // SDL_Event Event;
  // {
    // boolean should_be_grabbed = mouse_enabled &&
      // !(paused || (gamestate != GS_LEVEL) || demoplayback || menuactive);

    // if (mouse_currently_grabbed != should_be_grabbed)
      // SDL_WM_GrabInput((mouse_currently_grabbed = should_be_grabbed)
          // ? SDL_GRAB_ON : SDL_GRAB_OFF);
  // }

  // while ( SDL_PollEvent(&Event) )
    // I_GetEvent(&Event);

  // I_PollJoystick();
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

//
// I_InitInputs
//

static void I_InitInputs(void)
{
  // int nomouse_parm = M_CheckParm("-nomouse");

  // // check if the user wants to use the mouse
  // mouse_enabled = usemouse && !nomouse_parm;

  // // e6y: fix for turn-snapping bug on fullscreen in software mode
  // if (!nomouse_parm)
    // SDL_WarpMouse((unsigned short)(SCREENWIDTH/2), (unsigned short)(SCREENHEIGHT/2));

  // I_InitJoystick();
}

//////////////////////////////////////////////////////////////////////////////
// Graphics API

void I_ShutdownGraphics(void)
{
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
  if (V_GetMode() == VID_MODEGL) 
  {
    gld_Finish();
  }
}

//
// I_ScreenShot
//

int I_ScreenShot (const char *fname)
{
  //if (V_GetMode() == VID_MODEGL)
  //{
  //  int result = -1;
  //  unsigned char *pixel_data = gld_ReadScreen();

  //  if (pixel_data)
  //  {
  //    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
  //        pixel_data, SCREENWIDTH, SCREENHEIGHT, 24, SCREENWIDTH*3,
  //        0x000000ff, 0x0000ff00, 0x00ff0000, 0);

  //    if (surface)
  //    {
  //      result = SAVE_PNG_OR_BMP(surface, fname);
  //      SDL_FreeSurface(surface);
  //    }
  //    free(pixel_data);
  //  }
  //  return result;
  //}
}

//
// I_SetPalette
//
void I_SetPalette (int pal)
{
}

void I_PreInitGraphics(void)
{
  // static const union {
    // const char *c;
    // char *s;
  // } window_pos = {"SDL_VIDEO_WINDOW_POS=center"};

  // unsigned int flags = 0;

  // putenv(window_pos.s);

  // // Initialize SDL
  // // if (!(M_CheckParm("-nodraw") && M_CheckParm("-nosound")))
    // // flags = SDL_INIT_VIDEO;
// // #ifdef _DEBUG
  // // flags |= SDL_INIT_NOPARACHUTE;
// // #endif
  // if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) {
    // I_Error("Could not initialize SDL [%s]", SDL_GetError());
  // }

  // atexit(I_ShutdownSDL);
}

// CPhipps -
// I_CalculateRes
// Calculates the screen resolution, possibly using the supplied guide
void I_CalculateRes(unsigned int width, unsigned int height)
{
	SCREENHEIGHT = OMEGA_canvas_height;
	SCREENWIDTH = OMEGA_canvas_width;
}

// CPhipps -
// I_SetRes
// Sets the screen resolution
void I_SetRes(void)
{
  int i;

  I_CalculateRes(SCREENWIDTH, SCREENHEIGHT);

  // set first three to standard values
  for (i=0; i<3; i++) {
    screens[i].width = SCREENWIDTH;
    screens[i].height = SCREENHEIGHT;
    screens[i].byte_pitch = SCREENPITCH;
    screens[i].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
    screens[i].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);
  }

  // statusbar
  screens[4].width = SCREENWIDTH;
  screens[4].height = (ST_SCALED_HEIGHT+1);
  screens[4].byte_pitch = SCREENPITCH;
  screens[4].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
  screens[4].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);

  lprintf(LO_INFO,"I_SetRes: Using resolution %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
}

void I_InitGraphics(void)
{
  //char titlebuffer[2048];
  static int    firsttime=1;

  if (firsttime)
  {
    firsttime = 0;

    //atexit(I_ShutdownGraphics);
    //lprintf(LO_INFO, "I_InitGraphics: %dx%d\n", SCREENWIDTH, SCREENHEIGHT);

    ///* Set the video mode */
    //I_UpdateVideoMode();

    ///* Setup the window title */
    //strcpy(titlebuffer,PACKAGE);
    //strcat(titlebuffer," ");
    //strcat(titlebuffer,VERSION);
    //SDL_WM_SetCaption(titlebuffer, titlebuffer);

    /* Initialize the input system */
    I_InitInputs();
  }
}

int I_GetModeFromString(const char *modestr)
{
  video_mode_t mode;

  if (!stricmp(modestr,"15")) {
    mode = VID_MODE15;
  } else if (!stricmp(modestr,"15bit")) {
    mode = VID_MODE15;
  } else if (!stricmp(modestr,"16")) {
    mode = VID_MODE16;
  } else if (!stricmp(modestr,"16bit")) {
    mode = VID_MODE16;
  } else if (!stricmp(modestr,"32")) {
    mode = VID_MODE32;
  } else if (!stricmp(modestr,"32bit")) {
    mode = VID_MODE32;
  } else if (!stricmp(modestr,"gl")) {
    mode = VID_MODEGL;
  } else if (!stricmp(modestr,"OpenGL")) {
    mode = VID_MODEGL;
  } else {
    mode = VID_MODE8;
  }
  return mode;
}

void I_UpdateVideoMode(void)
{
  //int init_flags;
  int i;
  video_mode_t mode;

  //lprintf(LO_INFO, "I_UpdateVideoMode: %dx%d (%s)\n", SCREENWIDTH, SCREENHEIGHT, desired_fullscreen ? "fullscreen" : "nofullscreen");

  mode = I_GetModeFromString(default_videomode);
  if ((i=M_CheckParm("-vidmode")) && i<myargc-1) {
    mode = I_GetModeFromString(myargv[i+1]);
  }

  V_InitMode(mode);
  V_DestroyUnusedTrueColorPalettes();
  V_FreeScreens();

  I_SetRes();

// #if 0
  // // Initialize SDL with this graphics mode
  // if (V_GetMode() == VID_MODEGL) {
    // init_flags = SDL_OPENGL;
  // } else {
    // if (use_doublebuffer)
      // init_flags = SDL_DOUBLEBUF;
    // else
      // init_flags = SDL_SWSURFACE;
// #ifndef _DEBUG
    // init_flags |= SDL_HWPALETTE;
// #endif
  // }

  // if ( desired_fullscreen )
    // init_flags |= SDL_FULLSCREEN;

  // if (V_GetMode() == VID_MODEGL) {
    // SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 0 );
    // SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 0 );
    // SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 0 );
    // SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0 );
    // SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0 );
    // SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE, 0 );
    // SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE, 0 );
    // SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE, 0 );
    // SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 0 );
    // SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    // SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, gl_colorbuffer_bits );
    // SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, gl_depthbuffer_bits );
    // screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, gl_colorbuffer_bits, init_flags);
  // } else {
    // screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, V_GetNumPixelBits(), init_flags);
  // }

  // if(screen == NULL) {
    // I_Error("Couldn't set %dx%d video mode [%s]", SCREENWIDTH, SCREENHEIGHT, SDL_GetError());
  // }

  // lprintf(LO_INFO, "I_UpdateVideoMode: 0x%x, %s, %s\n", init_flags, screen->pixels ? "SDL buffer" : "own buffer", SDL_MUSTLOCK(screen) ? "lock-and-copy": "direct access");

  // mouse_currently_grabbed = false;

  // // Get the info needed to render to the display
  // if (!SDL_MUSTLOCK(screen))
  // {
    // screens[0].not_on_heap = true;
    // screens[0].data = (unsigned char *) (screen->pixels);
    // screens[0].byte_pitch = screen->pitch;
    // screens[0].short_pitch = screen->pitch / V_GetModePixelDepth(VID_MODE16);
    // screens[0].int_pitch = screen->pitch / V_GetModePixelDepth(VID_MODE32);
  // }
  // else
  // {
    // screens[0].not_on_heap = false;
  // }
// #endif

	screens[0].not_on_heap = false;

  V_AllocScreens();

  // Hide pointer while over this window
  //SDL_ShowCursor(0);

  R_InitBuffer(SCREENWIDTH, SCREENHEIGHT);

  if (V_GetMode() == VID_MODEGL) {
    // int temp;
    // lprintf(LO_INFO,"SDL OpenGL PixelFormat:\n");
    // SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &temp );
    // lprintf(LO_INFO,"    SDL_GL_RED_SIZE: %i\n",temp);
    // SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &temp );
    // lprintf(LO_INFO,"    SDL_GL_GREEN_SIZE: %i\n",temp);
    // SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &temp );
    // lprintf(LO_INFO,"    SDL_GL_BLUE_SIZE: %i\n",temp);
    // SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &temp );
    // lprintf(LO_INFO,"    SDL_GL_STENCIL_SIZE: %i\n",temp);
    // SDL_GL_GetAttribute( SDL_GL_ACCUM_RED_SIZE, &temp );
    // lprintf(LO_INFO,"    SDL_GL_ACCUM_RED_SIZE: %i\n",temp);
    // SDL_GL_GetAttribute( SDL_GL_ACCUM_GREEN_SIZE, &temp );
    // lprintf(LO_INFO,"    SDL_GL_ACCUM_GREEN_SIZE: %i\n",temp);
    // SDL_GL_GetAttribute( SDL_GL_ACCUM_BLUE_SIZE, &temp );
    // lprintf(LO_INFO,"    SDL_GL_ACCUM_BLUE_SIZE: %i\n",temp);
    // SDL_GL_GetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, &temp );
    // lprintf(LO_INFO,"    SDL_GL_ACCUM_ALPHA_SIZE: %i\n",temp);
    // SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &temp );
    // lprintf(LO_INFO,"    SDL_GL_DOUBLEBUFFER: %i\n",temp);
    // SDL_GL_GetAttribute( SDL_GL_BUFFER_SIZE, &temp );
    // lprintf(LO_INFO,"    SDL_GL_BUFFER_SIZE: %i\n",temp);
    // SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &temp );
    // lprintf(LO_INFO,"    SDL_GL_DEPTH_SIZE: %i\n",temp);
#ifdef GL_DOOM
    gld_Init(SCREENWIDTH, SCREENHEIGHT);
#endif
  }
}
