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

#include "OMEGA_interface.h"

void I_Quit (void);
void D_DoomMainSetup(void);
void I_UpdateVideoMode(void);

int OMEGA_channel_width;
int OMEGA_channel_height;
int OMEGA_canvas_width;
int OMEGA_canvas_height;
int OMEGA_draw_overlay;

}

using namespace omega;

// This lock is used to serialize rendering on multipipe configurations, since the Doom draw code is potentially not thread-safe.
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
	enum Buttons {Button1, Button2, Button3, Button4, LSButton, RSButton, DPadUp, DPadDown, DPadLeft, DPadRight, MaxButtons };
public:
	OmegaDoomServer(Application* app): ApplicationServer(app) {}
	virtual void initialize();
	virtual void handleEvent(const Event& evt);
	virtual void update(const UpdateContext& context);
	void updateButtonState(Buttons btn, float value);
	void mapButton(Buttons btn, unsigned int key);

private:
	bool myButtonState[MaxButtons];
	bool myButtonStateChanged[MaxButtons];
	float myMaxFps;
	float myLastFrameTime;
	int myLastMouseX;
	int myLastMouseY;
	event_t myMouseEvent;
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
	memset(myButtonStateChanged, 0, sizeof(bool) * MaxButtons);
	
	myMaxFps = 30;
	myLastFrameTime = 10;

	atexit(Z_Close);
	atexit(I_Quit);

	Z_Init();
	I_SetAffinityMask();
	D_DoomMainSetup();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomServer::update(const UpdateContext& context)
{
	movement_smooth = 0;

	float minFrameTime = 1.0f / myMaxFps;
	if(context.time - myLastFrameTime < minFrameTime) return;
	
	myLastFrameTime = context.time;
	
	sGlobalLock.lock();
    WasRenderedInTryRunTics = false;
    // frame syncronous IO operations
    I_StartFrame ();

    if (ffmap == gamemap) ffmap = 0;

    // process one or more tics
	singletics = 0;
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomServer::updateButtonState(Buttons btn, float value)
{
	if(value > 0) 
	{ 
		if(!myButtonState[btn])
		{
			myButtonState[btn] = true; 
			myButtonStateChanged[btn] = true; 
		}
		else
		{
			myButtonStateChanged[btn] = false; 
		}
	} 
	else 
	{
		if(myButtonState[btn])
		{
			myButtonState[btn] = false; 
			myButtonStateChanged[btn] = true; 
		}
		else
		{
			myButtonStateChanged[btn] = false; 
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomServer::mapButton(Buttons btn, unsigned int key)
{
	if(myButtonStateChanged[btn])
	{
		event_t event;
		if(myButtonState[btn]) event.type = ev_keydown;
		else event.type = ev_keyup;
		event.data1 = key;
		D_PostEvent(&event);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomServer::handleEvent(const Event& evt)
{
	sGlobalLock.lock();

	myMouseEvent.data2 = 0;

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
	if(evt.getServiceType() == Service::Pointer)
	{
		int mult = 5;
		myMouseEvent.type = ev_mouse;
		int dx = evt.getPosition().x() - myLastMouseX;
		int dy = evt.getPosition().y() - myLastMouseY;
		int flags = 0;

		if(evt.isFlagSet(Event::Left)) flags |= 1 << 0;
		if(evt.isFlagSet(Event::Right)) flags |= 1 << 1;
		if(evt.isFlagSet(Event::Middle)) flags |= 1 << 2;

		myMouseEvent.data1 = flags;
		myMouseEvent.data2 = dx * mult;
		myMouseEvent.data3 = dy;

		myLastMouseX = evt.getPosition().x();
		myLastMouseY = evt.getPosition().y();

		D_PostEvent(&myMouseEvent);
	}
	//else if(evt.getServiceType() == Service::Controller)
	//{
	//	float n = 1000;

	//	float x = evt.getExtraDataFloat(1) / n;
	//	float y = evt.getExtraDataFloat(2) / n;
	//	float z = evt.getExtraDataFloat(5) / n;
	//	float yaw = evt.getExtraDataFloat(3) / n;
	//	float pitch = evt.getExtraDataFloat(4) / n;
	//	float trigger = evt.getExtraDataFloat(5) / n;
	//	float tresh = 0.2f;

	//	if(x < tresh && x > -tresh) x = 0;
	//	if(y < tresh && y > -tresh) y = 0;
	//	if(z < tresh && z > -tresh) z = 0;
	//	if(yaw < tresh && yaw > -tresh) yaw = 0;
	//	if(pitch < tresh && pitch > -tresh) pitch = 0;
	//	if(trigger < tresh && trigger > -tresh) trigger = 0;

	//	float multiplier = 4000 - trigger * 16000;

	//    event.type = ev_joystick;
	//	int xaxis = ((x + yaw) * multiplier);
	//	int yaxis = (y * multiplier);

	//	event.data1 = 0;
	//	event.data2 = xaxis;
	//	event.data3 = yaxis;

	//	D_PostEvent(&event);

	//	evt.setProcessed(); 

	//	ofmsg("%1%", %evt.getExtraDataFloat(19));

	//	updateButtonState(Button1, evt.getExtraDataFloat(6));
	//	updateButtonState(Button2, evt.getExtraDataFloat(7));
	//	updateButtonState(Button3, evt.getExtraDataFloat(8));
	//	updateButtonState(Button4, evt.getExtraDataFloat(9));
	//	updateButtonState(RSButton, evt.getExtraDataFloat(10));
	//	updateButtonState(LSButton, evt.getExtraDataFloat(11));

	//	mapButton(Button1, KEYD_ENTER);
	//	mapButton(Button1, KEYD_RCTRL);

	//	mapButton(RSButton, KEYD_RALT);
	//	mapButton(RSButton, KEYD_LEFTARROW);

	//	mapButton(LSButton, KEYD_RALT);
	//	mapButton(LSButton, KEYD_RIGHTARROW);

	//	mapButton(Button3, KEYD_SPACEBAR);

	//	mapButton(Button2, KEYD_BACKSPACE);

	//	mapButton(Button4, '0');
	//}

	//D_PostEvent(&myMouseEvent);

	sGlobalLock.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomClient::initialize()
{
	myNeedGraphicsInit = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OmegaDoomClient::draw(const DrawContext& context)
{
	if(myNeedGraphicsInit)
	{
		sGlobalLock.lock();
		myNeedGraphicsInit = false;

		OMEGA_channel_width = context.viewport.width();
		OMEGA_channel_height  = context.viewport.height();
		OMEGA_canvas_width = context.channel->canvasSize->x();
		OMEGA_canvas_height = context.channel->canvasSize->y();

		SCREENHEIGHT = OMEGA_canvas_height;
		SCREENWIDTH = OMEGA_canvas_width;

		// Make sure OpenGL lighting is disabled.
		glDisable(GL_LIGHTING);

		I_UpdateVideoMode();
		sGlobalLock.unlock();
		return;
	}

	//sGlobalLock.lock();
	sCurrentDrawContext = &context;
	if (!movement_smooth || !WasRenderedInTryRunTics)
	{
		if(context.task == DrawContext::OverlayDrawTask) 
		{
			OMEGA_BeginDraw2D();
			SCREENWIDTH = OMEGA_canvas_width;
			SCREENHEIGHT = OMEGA_canvas_height;
			OMEGA_draw_overlay = 1;
		}
		else
		{
			SCREENWIDTH = OMEGA_canvas_width;
			SCREENHEIGHT = OMEGA_canvas_height;
			OMEGA_draw_overlay = 0;
		}
		// Call the main Doom display procedure.
		D_Display();
	}

	//sGlobalLock.unlock();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	myargc = argc;
	myargv = (const char * const *) argv;

	OmegaDoom app;

	// Read config file name from command line or use default one.
	const char* cfgName = "system/desktop.cfg";
	if(argc == 2) cfgName = argv[1];

	omain(app, cfgName, "omegadoom.log", new FilesystemDataSource(OMEGA_DATA_PATH));
}
