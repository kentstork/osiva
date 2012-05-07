/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * iconbar.h is part of Osiva.
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
// File: iconbar.h
//
//  #include <stdlib.h>
//  #include <string.h>
//  #include "ll_image.h"
//  #include "windows.h"
//
///////////////////////////////////////////////////////////////////////////

class IconBar {

public:

  IconBar ();
  ~IconBar ();

  void create (HINSTANCE hInstance);
  // The parameters refer to GIF image custom resources
  void set_images (int IDR_bar, int IDR_barmo, int IDR_barmd, int IDR_barna);
  // The iconbar background color can be changed 
  void set_bar_color (int red, int green, int blue);
  // And changed back to the original color
  void reset_bar_color ();
  // The RECT array is copied
  void set_hotspots (int hotspots, RECT *hotspot);
  // Bind a command code to a spot; the code is returned to the parent
  void set_command (int spot, int command);
  // A repeating spot type behaves like a repeating key
  void set_repeating (int spot);
  // Notify the client when the button is pressed, not released
  void set_ondown (int spot);
  // Drag areas do not generate a command; they're used to move the bar
  void set_isdrag (int spot);
  // Tooltip text
  void set_tooltip (int spot, char *tip);
  // If an icon is selected, the command code is returned in LPARAM
  // WM_MESSAGEs are sent like to a normal Windows Procedure
  void enable_tips ();
  void disable_tips ();
  void enable_spot (int spot);
  void disable_spot (int spot);
  enum {ICON_SELECTED, WM_MESSAGE, DDE_FILENAME}; // in UINT
  enum {LEFT_BUTTON, RIGHT_BUTTON, SHIFT_LEFT, SHIFT_RIGHT}; // in WPARAM
  void set_callback
    (int client, void (*notify)(int client, int type, UINT, WPARAM, LPARAM));
  LLIMG *get_image() {return bar;}
  HWND get_hwnd() {return hw_main;}

  enum {LAMP_TIMER, TIP_TIMER, REPEAT_TIMER, CLIENT_TIMER};

private:

  void handle_mouse_move (int cli_x, int cli_y);
  void handle_left_click (int cli_x, int cli_y);
  void handle_left_unclick (int cli_x, int cli_y, WORD fwKeys = 0);
  void handle_right_click (int cli_x, int cli_y);
  void handle_right_unclick (int cli_x, int cli_y, WORD fwKeys = 0);
  void handle_timer (WPARAM, LPARAM);
  void handle_dde_initiate (WPARAM, LPARAM);
  void handle_dde_execute (WPARAM, LPARAM);
  void handle_dde_terminate (WPARAM, LPARAM);
  void handle_copydata (WPARAM, LPARAM);
  void handle_user_open (WPARAM, LPARAM);
  void handle_drop (WPARAM, LPARAM);
  enum {NORMAL, MOUSE_OVER, MOUSE_DOWN};
  void paint_hotspot (int spot, int state);
  void refresh_bar (HDC hdc);
  void handle_kill_focus ();

  LLIMG *bar;    // Untouched icon bar image
  LLIMG *bar_mo; // Mouse over icon bar image
  LLIMG *bar_md; // Mouse down icon bar image
  LLIMG *bar_na; // Inactive bar image
  int custom_bg; // Custom background color means don't use bar_na

  int bar_bgi;   // Bar background color index
  int bar_mo_bgi;
  int bar_md_bgi;

  int bar_red;   // Original background color 
  int bar_green;
  int bar_blue;

  int _hotspots;  // Number of hotspots in hotspot array
  RECT *_hotspot; // Array of hotspot rectangles
  enum {BE_NONE, BE_REPEATS, BE_ONDOWN, BE_3, BE_DRAG};
  int *behave;    // Array indicating behavior of hotspot
  int *command;   // Array containing command code associated with spot
  int *spot_active;      // Array indicating icon should be active
  class SmlStr *tooltip; // Array containing tooltip text for each spot
  class ToolTip *tipper; // GUI Component for showing tooltips

  POINT click_pt; // The window click point in client coords
  POINT base_pt;  // The last handled point in screen coords
  int in_move;
  int last_mo;    // Last hotspot that was moused over
  int click_spot; // Last hotspot that was clicked on

  void (*_notify)(int, int, UINT, WPARAM, LPARAM);
  int _client;     // Client data for _notify callback;

  HWND hw_main;
  static LRESULT CALLBACK
  main_wproc (HWND, UINT, WPARAM, LPARAM);

};



