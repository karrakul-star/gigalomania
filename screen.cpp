//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>
#include <ctime>

#include "screen.h"
#include "sound.h"
#include "game.h"
#include "utils.h"
#include "gamestate.h"
#include "image.h"
#include "sector.h"
#include "panel.h"
#include "gui.h"
#include "player.h"

//---------------------------------------------------------------------------

Gigalomania::Screen::Screen() : m_pos_x(0), m_pos_y(0), m_down_left(false), m_down_middle(false), m_down_right(false) {
#if SDL_MAJOR_VERSION == 1
	surface = NULL;
#else
	sdlWindow = NULL;
	sdlRenderer = NULL;
	width = 0;
	height = 0;
#endif
}

Gigalomania::Screen::~Screen() {
}

bool Gigalomania::Screen::canOpenFullscreen(int width, int height) {
#if SDL_MAJOR_VERSION == 1
	if( SDL_VideoModeOK(width, height, 32, SDL_HWSURFACE|SDL_HWPALETTE|SDL_FULLSCREEN) != 0 )
		return true;
	return false;
#else
	return true; // assume always true?
#endif
}

bool Gigalomania::Screen::open(int screen_width, int screen_height, bool fullscreen) {
	LOG("Screen::open(%d, %d, %s)\n", screen_width, screen_height, fullscreen?"fullscreen":"windowed");
#if SDL_MAJOR_VERSION == 1
	if( fullscreen )
		surface = SDL_SetVideoMode(screen_width, screen_height, 32, SDL_HWSURFACE|SDL_HWPALETTE|SDL_FULLSCREEN);
	else
		surface = SDL_SetVideoMode(screen_width, screen_height, 32, SDL_HWSURFACE|SDL_HWPALETTE);
	if( surface == NULL ) {
		LOG("failed to open screen at this resolution\n");
		return false;
	}
#else
#if defined(__ANDROID__)
	// Use Android immersive fullscreen.
	fullscreen = true;
#endif
	if( SDL_CreateWindowAndRenderer(screen_width, screen_height, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE, &sdlWindow, &sdlRenderer) != 0 ) {
		LOG("failed to open screen at this resolution\n");
		return false;
	}
	this->width = screen_width;
	this->height = screen_height;
	{
		SDL_BlendMode blendMode = SDL_BLENDMODE_NONE;
		SDL_GetRenderDrawBlendMode(sdlRenderer, &blendMode);
		LOG("render draw blend mode: %d\n", blendMode);
	}
	SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND); // needed for Screen::fillRectWithAlpha, as blending is off by default for drawing functions
#endif
	LOG("screen opened ok\n");

	if( game_g->isMobileUI() ) {
		SDL_ShowCursor(false); // comment out to test with system cursor, for testing purposes when ran on non-touchscreen devices
	}
	else {
		//SDL_ShowCursor(false);
		// Set up a blank cursor
		// We use this hacky way due to a bug in SDL_ShowCursor(false), where when alt-tabbing out of full screen mode in Windows, the mouse pointer is
		// trapped in the top left corner. See 
		unsigned char data[1] = "";
		unsigned char mask[1] = "";
		SDL_Cursor *cursor = SDL_CreateCursor(data, mask, 8, 1, 0, 0);
		SDL_Cursor *old_cursor = SDL_GetCursor();
		SDL_SetCursor(cursor);
		SDL_FreeCursor(old_cursor);
	}

#if SDL_MAJOR_VERSION == 1
	Image::setGraphicsOutput(surface);
#else
	Image::setGraphicsOutput(sdlRenderer);
#endif

	return true;
}

#if SDL_MAJOR_VERSION == 1
#else
void Gigalomania::Screen::setLogicalSize(int width, int height, bool smooth) {
	this->width = width;
	this->height = height;
	LOG("width, height: %d, %d\n", width, height);
	if( smooth ) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
	}
	else {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	}
	#if defined(__ANDROID__)
    // Disable aspect-ratio letterboxing; scale each axis to the full output.
    SDL_RenderSetLogicalSize(sdlRenderer, 0, 0);
    int output_w = 0, output_h = 0;
    if (SDL_GetRendererOutputSize(sdlRenderer, &output_w, &output_h) == 0 && width > 0 && height > 0 && output_w > 0 && output_h > 0) {
        SDL_RenderSetViewport(sdlRenderer, NULL);
        SDL_RenderSetScale(sdlRenderer, (float)output_w / width, (float)output_h / height);
    }
#else
    SDL_RenderSetLogicalSize(sdlRenderer, width, height);
#endif
}
#endif

void Gigalomania::Screen::setTitle(const char *title) {
#if SDL_MAJOR_VERSION == 1
	SDL_WM_SetCaption(title, "");
