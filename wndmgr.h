/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * wndmgr.h is part of Osiva.
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
// File: wndmgr.h
//
// Synopsis:
//
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <windows.h>
// #include "ll_image.h"
// #include "snapshotw.h"
// #include "iconbar.h"
// #include <vector>
// using namespace std;
// #include "wndmgr.h"
// 


class WndMgr {

public:

  WndMgr();
  ~WndMgr();
  void init (HINSTANCE hInstance);
  HWND new_window (HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow);
  void close_window (SnapShotW *sswindow);
  void tab (SnapShotW *sswindow, int activate = 1, int reverse = 0);
  void all_small (SnapShotW *exclude = NULL);
  void all_big ();
  void all_rotate (SnapShotW *exclude = NULL, int clockwise=1);
  enum {FIT_INSIDE, FIT_EXACTLY, MATCH_AREA};
  void fit_top_window (int how = FIT_INSIDE);
  void center (SnapShotW *exclude = NULL);
  void corner (SnapShotW *exclude = NULL);
  void distribute (SnapShotW *exclude = NULL);
  void cluster (SnapShotW *exclude = NULL);
  void tile_rect (RECT *rect, int lock_aspect=0, SnapShotW *ssw = NULL);
  void tile_screen (SnapShotW *ssw = NULL);
  enum forceType {REPULSE, ATTRACT};
  void reduce_energy (int vKey, forceType force = REPULSE);
  HWND find_top_window ();
  void save_layout (int consolidate = 0);
  enum {FLIP_TOGGLE, FLIP_ON, FLIP_OFF};
  void set_flip (int type = FLIP_TOGGLE);
  void dissolve_init ();
  void dissolve_frame ();
  void dissolve_clear ();
  int is_layout_file (char *path);
  void load_layout_file (char *path);
  void dump_info ();
  void show (int flag = 1, HWND curr_hwnd = NULL);

  int reduction;
  char *exe_path; 

private:

  vector <class SnapShotW *> snapwin;  // The windows being managed
  vector <int> group_icon; // Icon indeci that need more than 1 window
  IconBar iconbar;
  HINSTANCE _hInstance;
  HMENU hmenu;


  // State for frame to frame dissolve
  SnapShotW * prev_ssw;
  SnapShotW * next_ssw;
  LLIMG *frame;          // Contains the current dissolve frame
  LLIMG *screen;         // Contains the screen over the next_ssw
  LLIMG *future;         // Contains the target image of the dissolve
  int frames;            // Total # of frames to use for dissolve
  int frame_num;         // The current dissolve frame showing
  int menu_icon_left;    // Iconbar client coordinate of the menu icon

  // Returns a randomly ordered index
  // The caller must delete [] the returned array
  int *random_index ();

  DWORD PID;
  // Flag to disable flipping while the menu is in use
  int in_menu;  
  // Flag to disable hiding the iconbar if the last window is being closed
  int in_close;

  void do_menu (void);
  void handle_command (WPARAM, LPARAM);
  void handle_timer (WPARAM, LPARAM);

  static void
    iconbar_notify (int client, int type, UINT u, WPARAM w, LPARAM l);
  class DDProxy *ddproxy;

};
