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
//#define OMEGA_NO_GL_HEADERS
#include<omega.h>

extern "C"
{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include "m_fixed.h"
#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "lprintf.h"
#include "m_random.h"
#include "doomstat.h"
#include "g_game.h"
#include "m_misc.h"
#include "i_sound.h"
#include "i_main.h"
#include "r_fps.h"
#include "lprintf.h"
#include "m_menu.h"
#include "s_sound.h"
#include "d_net.h"
#include "p_checksum.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void I_Quit (void);
void D_DoomMainSetup(void);
void I_UpdateVideoMode(void);

int OMEGA_channel_width;
int OMEGA_channel_height;
int OMEGA_canvas_width;
int OMEGA_canvas_height;

}

using namespace omega;

Lock sGlobalLock;

extern const omega::DrawContext* sCurrentDrawContext;

///////////////////////////////////////////////////////////////////////////////////////////////////
static int I_TranslateKey(int key)
{
  int rc = 0;

  switch (key) 
  {
	  case KC_LEFT: rc = KEYD_LEFTARROW;  break;
	  case KC_RIGHT:  rc = KEYD_RIGHTARROW; break;
	  case KC_DOWN: rc = KEYD_DOWNARROW;  break;
	  case KC_UP:   rc = KEYD_UPARROW;  break;
	  case KC_ESCAPE: rc = KEYD_ESCAPE; break;
	  case KC_RETURN: rc = KEYD_ENTER;  break;
	  case KC_TAB:  rc = KEYD_TAB;    break;
	  case KC_F1:   rc = KEYD_F1;   break;
	  case KC_F2:   rc = KEYD_F2;   break;
	  case KC_F3:   rc = KEYD_F3;   break;
	  case KC_F4:   rc = KEYD_F4;   break;
	  case KC_F5:   rc = KEYD_F5;   break;
	  case KC_F6:   rc = KEYD_F6;   break;
	  case KC_F7:   rc = KEYD_F7;   break;
	  case KC_F8:   rc = KEYD_F8;   break;
	  case KC_F9:   rc = KEYD_F9;   break;
	  case KC_F10:  rc = KEYD_F10;    break;
	  case KC_F11:  rc = KEYD_F11;    break;
	  case KC_F12:  rc = KEYD_F12;    break;
	  case KC_BACKSPACE:  rc = KEYD_BACKSPACE;  break;
	  case KC_PAGE_UP: rc = KEYD_PAGEUP; break;
	  case KC_PAGE_DOWN: rc = KEYD_PAGEDOWN; break;
	  case KC_HOME: rc = KEYD_HOME; break;
	  case KC_END:  rc = KEYD_END;  break;
	  case KC_SHIFT_L:
	  case KC_SHIFT_R: rc = KEYD_RSHIFT; break;
	  case KC_CONTROL_L:
	  case KC_CONTROL_R:  rc = KEYD_RCTRL;  break;
	  case KC_ALT_L:
	  case KC_ALT_R: rc = KEYD_RALT;   break;
	  default:    rc = key;    break;
  }
  return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OmegaDoomClient: public ApplicationClient
{
public:
	OmegaDoomClient(ApplicationServer* server): ApplicationClient(server) {}
	virtual void initialize();
	virtual void draw(const DrawContext& context);

private:
	bool myNeedGraphicsInit;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OmegaDoomServer: public ApplicationServer
{
	enum Buttons {Button1, Button2, Button3, Button4, LSButton, RSButton, MaxButtons };
public:
	OmegaDoomServer(Application* app): ApplicationServer(app) {}
	virtual void initialize();
	virtual void handleEvent(const Event& evt);
	virtual void update(const UpdateContext& context);

private:
	bool myButtonState[MaxButtons];
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OmegaDoom: public Application
{
public:
	virtual ApplicationServer* createServer() { return new OmegaDoomServer(this); }
	virtual ApplicationClient* createClient(ApplicationServer* server) { return new OmegaDoomClient(server); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomServer::initialize()
{
	memset(myButtonState, 0, sizeof(bool) * MaxButtons);
	D_DoomMainSetup(); // CPhipps - setup out of main execution stack
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomServer::update(const UpdateContext& context)
{
	sGlobalLock.lock();
    WasRenderedInTryRunTics = false;
    // frame syncronous IO operations
    I_StartFrame ();

    if (ffmap == gamemap) ffmap = 0;

    // process one or more tics
    if (singletics)
    {
        I_StartTic ();
        G_BuildTiccmd (&netcmds[consoleplayer][maketic%BACKUPTICS]);
        if (advancedemo)
        D_DoAdvanceDemo ();
        M_Ticker ();
        G_Ticker ();
        P_Checksum(gametic);
        gametic++;
        maketic++;
    }
    else
	{
		TryRunTics (); // will run at least one tic
	}

    if (players[displayplayer].mo) S_UpdateSounds(players[displayplayer].mo);// move positional sounds
	sGlobalLock.unlock();
}

#define SET_BUTTON_STATE(btn, index) if(evt.getExtraDataFloat(index) > 0) { myButtonState[btn] = true; evt.setProcessed(); } else myButtonState[btn] = false;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomServer::handleEvent(const Event& evt)
{
	event_t event;
	if(evt.getServiceType() == Service::Keyboard)
	{
		int keycode = I_TranslateKey(evt.getSourceId());
		if(evt.getType() == Event::Down)
		{
			event.type = ev_keydown;
			event.data1 = keycode;
			D_PostEvent(&event);
		}
		else if(evt.getType() == Event::Up)
		{
			event.type = ev_keyup;
			event.data1 = keycode;
			D_PostEvent(&event);
		}
	}
	else if(evt.getServiceType() == Service::Controller)
	{
		float n = 1000;

		float x = evt.getExtraDataFloat(1) / n;
		float y = evt.getExtraDataFloat(2) / n;
		float z = evt.getExtraDataFloat(5) / n;
		float yaw = evt.getExtraDataFloat(3) / n;
		float pitch = evt.getExtraDataFloat(4) / n;
		float trigger = evt.getExtraDataFloat(5) / n;
		float tresh = 0.2f;

		if(x < tresh && x > -tresh) x = 0;
		if(y < tresh && y > -tresh) y = 0;
		if(z < tresh && z > -tresh) z = 0;
		if(yaw < tresh && yaw > -tresh) yaw = 0;
		if(pitch < tresh && pitch > -tresh) pitch = 0;
		if(trigger < tresh && trigger > -tresh) trigger = 0;

	    event.type = ev_joystick;
		int xaxis = (x * 32000);
		int yaxis = (y * 32000);

		event.data2 = xaxis;
		event.data3 = yaxis;

		SET_BUTTON_STATE(Button1, 6);
		SET_BUTTON_STATE(Button2, 7);
		SET_BUTTON_STATE(Button3, 8);
		SET_BUTTON_STATE(Button4, 9);
		SET_BUTTON_STATE(RSButton, 10);
		SET_BUTTON_STATE(RSButton, 11);

		// Trigger used as button 1.
		if(trigger < 0) event.data1 = 1;

		event.data1 |= (
			(myButtonState[Button1] << 1) |
			(myButtonState[Button2] << 2) |
			(myButtonState[Button3] << 3));

		D_PostEvent(&event);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomClient::initialize()
{
	myNeedGraphicsInit = true;
	/* Version info */
	//lprintf(LO_INFO,"\n");
	//PrintVer();

	/* cph - Z_Close must be done after I_Quit, so we register it first. */
	atexit(Z_Close);

	Z_Init();                  /* 1/18/98 killough: start up memory stuff first */

	atexit(I_Quit);

#ifndef _DEBUG
	signal(SIGSEGV, I_SignalHandler);
	signal(SIGTERM, I_SignalHandler);
	signal(SIGFPE,  I_SignalHandler);
	signal(SIGILL,  I_SignalHandler);
	signal(SIGINT,  I_SignalHandler);  /* killough 3/6/98: allow CTRL-BRK during init */
	signal(SIGABRT, I_SignalHandler);
#endif

	I_SetAffinityMask();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomClient::draw(const DrawContext& context)
{
	sGlobalLock.lock();

	if(context.task == DrawContext::SceneDrawTask)
	{
		sCurrentDrawContext = &context;

		if(myNeedGraphicsInit)
		{
			myNeedGraphicsInit = false;

			OMEGA_channel_width = context.viewport.width();
			OMEGA_channel_height  = context.viewport.height();
			OMEGA_canvas_width = context.channel->canvasSize->x();
			OMEGA_canvas_height = context.channel->canvasSize->y();

			I_PreInitGraphics();
			I_UpdateVideoMode();
		}

		SCREENWIDTH = context.channel->canvasSize->x();
		SCREENHEIGHT = context.channel->canvasSize->x();

		if (V_GetMode() == VID_MODEGL ? 
		!movement_smooth || !WasRenderedInTryRunTics :
		!movement_smooth || !WasRenderedInTryRunTics || gamestate != wipegamestate)
		{
			glDisable(GL_LIGHTING);
			// Update display, next frame, with current state.
			D_Display();
		}
	}

	sGlobalLock.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	myargc = argc;
	myargv = (const char * const *) argv;

	OmegaDoom app;

	// Read config file name from command line or use default one.
	// NOTE: being a simple application, ohello does not have any application-specific configuration option. 
	// So, we are going to load directly a system configuration file.
	const char* cfgName = "system/desktop.cfg";
	if(argc == 2) cfgName = argv[1];

	omain(app, cfgName, "omegadoom.log", new FilesystemDataSource(OMEGA_DATA_PATH));
}
