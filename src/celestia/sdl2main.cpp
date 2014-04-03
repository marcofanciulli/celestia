// glutmain.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// GLUT front-end for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <SDL.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <celengine/celestia.h>
#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celutil/util.h>
#include <celutil/debug.h>
#include <celmath/mathlib.h>
#include <celengine/astro.h>
#include "celestiacore.h"
/* what are you supposed to be?
 #include "popt.h"
 */

using namespace std;

char AppName[] = "Celestia";
#ifndef CONFIG_DATA_DIR
#define CONFIG_DATA_DIR "/home/local/ANT/bradd/git/celestia"
#endif

static CelestiaCore* appCore = NULL;
SDL_Window* sdlWindow = NULL;
SDL_GLContext sdlGlContext = NULL;
//static bool fullscreen = false;
static bool ready = false;
static bool quit = false;

static void Resize(int w, int h) {
  appCore->resize(w, h);
}

void onMouseButton(const SDL_MouseButtonEvent & event) {
  int x = event.x, y = event.y;
  if (SDL_BUTTON_LEFT == event.button) {
    if (SDL_MOUSEBUTTONDOWN == event.type)
      appCore->mouseButtonDown(x, y, CelestiaCore::LeftButton);
    else
      appCore->mouseButtonUp(x, y, CelestiaCore::LeftButton);
  }
  else if (SDL_BUTTON_RIGHT == event.button) {
    if (SDL_MOUSEBUTTONDOWN == event.type)
      appCore->mouseButtonDown(x, y, CelestiaCore::RightButton);
    else
      appCore->mouseButtonUp(x, y, CelestiaCore::RightButton);
  }
  else if (SDL_BUTTON_MIDDLE == event.button) {
    if (SDL_MOUSEBUTTONDOWN == event.type)
      appCore->mouseButtonDown(x, y, CelestiaCore::MiddleButton);
    else
      appCore->mouseButtonUp(x, y, CelestiaCore::MiddleButton);
  }
}

void onMouseWheel(const SDL_MouseWheelEvent & event) {
  // On Linux, mouse wheel up and down are usually translated into
  // mouse button 4 and 5 down events.
  if (event.y > 0) {
    appCore->mouseWheel(-1.0f, 0);
  }
  else if (event.y < 0) {
    appCore->mouseWheel(1.0f, 0);
  }
}

void onMouseMotion(const SDL_MouseMotionEvent & event) {
  int buttons = 0;
  if (event.state & SDL_BUTTON_LMASK)
    buttons |= CelestiaCore::LeftButton;
  if (event.state & SDL_BUTTON_RMASK)
    buttons |= CelestiaCore::RightButton;
  if (event.state & SDL_BUTTON_MMASK)
    buttons |= CelestiaCore::MiddleButton;

  if (buttons) {
    appCore->mouseMove(event.xrel, event.yrel, buttons);
  }
}

void onKeyboardEvent(const SDL_KeyboardEvent & event) {
  const SDL_Keysym & key = event.keysym;
  bool down = event.type == SDL_KEYDOWN;

  // Non-'special' keys
  if (0 == (SDLK_SCANCODE_MASK & key.sym)) {
    if (down) {
      bool ctrl = 0 != (KMOD_CTRL & key.mod);
      if ((SDLK_q == key.sym) && ctrl) {
        quit = true;
      }
      else {
        appCore->charEntered((char) key.sym);
      }
    }
    else {
      appCore->keyUp(key.sym);
    }
    return;
  }

  int k = -1;
  switch (key.sym) {
  case SDLK_UP:
    k = CelestiaCore::Key_Up;
    break;
  case SDLK_DOWN:
    k = CelestiaCore::Key_Down;
    break;
  case SDLK_LEFT:
    k = CelestiaCore::Key_Left;
    break;
  case SDLK_RIGHT:
    k = CelestiaCore::Key_Right;
    break;
  case SDLK_HOME:
    k = CelestiaCore::Key_Home;
    break;
  case SDLK_END:
    k = CelestiaCore::Key_End;
    break;
  case SDLK_F1:
    k = CelestiaCore::Key_F1;
    break;
  case SDLK_F2:
    k = CelestiaCore::Key_F2;
    break;
  case SDLK_F3:
    k = CelestiaCore::Key_F3;
    break;
  case SDLK_F4:
    k = CelestiaCore::Key_F4;
    break;
  case SDLK_F5:
    k = CelestiaCore::Key_F5;
    break;
  case SDLK_F6:
    k = CelestiaCore::Key_F6;
    break;
  case SDLK_F7:
    k = CelestiaCore::Key_F7;
    break;
  case SDLK_F11:
    k = CelestiaCore::Key_F11;
    break;
  case SDLK_F12:
    k = CelestiaCore::Key_F12;
    break;
  }
  if (k >= 0) {
    if (down)
      appCore->keyDown(k);
    else
      appCore->keyUp(k);
  }
}

void onWindowEvent(const SDL_WindowEvent & event) {
  switch (event.event) {
  case SDL_WINDOWEVENT_RESIZED:
    //  glutReshapeFunc(Resize);
    Resize(event.data1, event.data2);
    SDL_Log("Window %d resized to %dx%d", event.windowID, event.data1,
        event.data2);
    break;
  case SDL_WINDOWEVENT_MINIMIZED:
    ready = false;
    SDL_Log("Window %d minimized", event.windowID);
    break;
  case SDL_WINDOWEVENT_MAXIMIZED:
    ready = true;
    SDL_Log("Window %d maximized", event.windowID);
    break;
  case SDL_WINDOWEVENT_RESTORED:
    ready = true;
    SDL_Log("Window %d restored", event.windowID);
    break;
  case SDL_WINDOWEVENT_CLOSE:
    quit = true;
    break;
  }
}

int main(int argc, char* argv[]) {
  setlocale(LC_ALL, "");
  setlocale(LC_NUMERIC, "C");
  fprintf(stdout, "\nDone\n", SDL_GetError());

//  bindtextdomain(PACKAGE, LOCALEDIR);
//  bind_textdomain_codeset(PACKAGE, "UTF-8");
//  textdomain (PACKAGE);

  if (chdir(CONFIG_DATA_DIR) == -1) {
    cerr << "Cannot chdir to '" << CONFIG_DATA_DIR
        << "', probably due to improper installation\n";
  }
  // Not ready to render yet
  ready = false;
  char c;
  int startfile = 0;
  while ((c = getopt(argc, argv, "v::f")) > -1) {
    if (c == '?') {
      cout << "Usage: celestia [-v] [-f <filename>]\n";
      exit(1);
    }
    else if (c == 'v') {
      if (optarg)
        SetDebugVerbosity(atoi(optarg));
      else
        SetDebugVerbosity(0);
    }
    else if (c == 'f') {
      startfile = 1;
    }
  }

  appCore = new CelestiaCore();
  if (appCore == NULL) {
    cerr << "Out of memory.\n";
    return 1;
  }

  if (!appCore->initSimulation()) {
    return 1;
  }

  //  glutInit(&argc, argv);
  if (0 != SDL_Init(SDL_INIT_EVERYTHING)) {
    cerr << endl << "Unable to initialize SDL:  " << SDL_GetError()
        << endl;
    return 1;
  }
  //  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  //  glutInitWindowPosition(0, 0);
  //  glutInitWindowSize(480, 360);
  //  mainWindow = glutCreateWindow("Celestia");
  sdlWindow = SDL_CreateWindow(AppName,
  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 480, 360,
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  //  Resize(480, 360);
  Resize(480, 360);
  sdlGlContext = SDL_GL_CreateContext(sdlWindow);
  SDL_GL_SetSwapInterval(1);

  // GL should be all set up, now initialize the renderer.
  appCore->initRenderer();

  // Set the simulation starting time to the current system time
  time_t curtime = time(NULL);
  appCore->start(
      (double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1));
  localtime(&curtime); // Only doing this to set timezone as a side effect
  appCore->setTimeZoneBias(-timezone);
  appCore->setTimeZoneName(tzname[daylight ? 0 : 1]);
  appCore->splitView(View::VerticalSplit);

  if (startfile == 1) {
    if (argv[argc - 1][0] == '-') {
      cout << "Missing Filename.\n";
      return 1;
    }

    cout << "*** Using CEL File: " << argv[argc - 1] << endl;
    appCore->runScript(argv[argc - 1]);
  }
  ready = true;
  SDL_Event event;
  while (!quit) {
    if (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_WINDOWEVENT:
        onWindowEvent(event.window);
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        onKeyboardEvent(event.key);
        break;
      case SDL_MOUSEMOTION:
        onMouseMotion(event.motion);
        break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        onMouseButton(event.button);
        break;
      case SDL_MOUSEWHEEL:
        onMouseWheel(event.wheel);
        break;
      }
    }
    else {
      appCore->tick();
      if (ready) {
        appCore->draw();
        SDL_GL_SwapWindow(sdlWindow);
      }
    }
  }
  SDL_DestroyWindow(sdlWindow);
  SDL_Quit();
  delete appCore;
  appCore = NULL;

  return 0;
}

