/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * ooptions.h is part of Osiva.
 *
 * Osiva is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Osiva is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Osiva.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/


///////////////////////////////////////////////////////////////////////////
//
// File: ooptions.h
//
// Synopsis:
//
// #include <windows.h>
// #include <vector>
// using namespace std
// #include "ooptions.h"
//

class OOptions {

public:

  char *program_path;

  char *reg_option_string; // option string from the registry
  char *reg_gif_command;   // gif command from the registry
  char *reg_jpg_command;   // jpg command from the registry

  int flip_sec;            // fs: flip delay seconds
  int flip_ms;             // fm: flip delay milli seconds
  int flip;                // ff: flag -- to flip or not to flip

  int dissolve;            // df: flag - to dissolve or not 
  int dissolve_bits;       // db: 2^this is the number of dissolve frames
  int dissolve_sec;        // ds: dissolve delay seconds
  int dissolve_ms;         // dm: dissolve delay milli seconds

  int minimize;            // mb: flag -- minimize icon bar on start
  int suppress_zoom;       // sz: flag -- don't zoom on click, just activate
  int stayup;              // su: flag -- stay open with no windows
  int ontop;               // ot: flag -- icon bar topmost style
  int right_click_closes;  // rc: flag -- no context menu

  int erosions;            // em: times to erode the true color mask
  int depth;               // cd: depth of the contour tree to use
  int bg_diff;             // bd: diff allowed to still be background pixel

  int reduction;           // rd: global reduction denominator 2-9

  OSVERSIONINFO osvi;

  int default_options ();
  int read_reg_options ();
  int read_reg_commands ();

  int write_reg_options ();
  int write_reg_commands ();

  OOptions();
  ~OOptions();

  // Subscribing Observers get a (WM_APP+0) message with
  // the wparam parameter in the WPARAM, the appropriate
  // option set enum value in the LPARAM
  enum {TRANSPARENCY, FLIP, FLIP_NOW, FLIP_NOW_R, GENERAL};
  void subscribe (HWND client_hwnd, int wparam);
  
  // It is up to clients of OOptions to envoke the broadcast
  // of change notification to the subscribers (the clients
  // are the dialogs used to set the parameters.)
  void broadcast (int option_set, HWND sender = NULL);

private:

  struct Client {
    HWND hwnd;
    int message;
  };

  vector<struct Client> client;

};

